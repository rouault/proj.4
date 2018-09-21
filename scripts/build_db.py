#!/usr/bin/env python
###############################################################################
# $Id$
#
#  Project:  PROJ
#  Purpose:  Build SRS and coordinate transform database
#  Author:   Even Rouault <even.rouault at spatialys.com>
#
###############################################################################
#  Copyright (c) 2018, Even Rouault <even.rouault at spatialys.com>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
###############################################################################

import csv
import os
import sqlite3
import sys


def ingest_sqlite_dump(cursor, filename):
    sql = ''
    for line in open(filename, 'rt').readlines():
        sql += line
        if sqlite3.complete_statement(sql):
            sql = sql.strip()
            if sql != 'COMMIT;':
                cursor.execute(sql)
            sql = ''


def ingest_epsg():

    for f in ['PostgreSQL_Data_Script.sql',  'PostgreSQL_Table_Script.sql']:
        if not os.path.exists(f):
            raise Exception('Missing file: ' + f)

    epsg_tmp_db_filename = 'tmp_epsg.db'

    if os.path.exists(epsg_tmp_db_filename):
        os.unlink(epsg_tmp_db_filename)

    conn = sqlite3.connect(epsg_tmp_db_filename)
    cursor = conn.cursor()
    cursor.execute('PRAGMA journal_mode = OFF;')
    ingest_sqlite_dump(cursor, 'PostgreSQL_Table_Script.sql')
    ingest_sqlite_dump(cursor, 'PostgreSQL_Data_Script.sql')
    cursor.close()
    conn.commit()

    return (conn, epsg_tmp_db_filename)


def fill_unit_of_measure(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO unit_of_measure SELECT 'EPSG', uom_code, unit_of_meas_name, unit_of_meas_type, factor_b / factor_c, deprecated FROM epsg.epsg_unitofmeasure")


def fill_ellipsoid(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO ellipsoid SELECT 'EPSG', ellipsoid_code, ellipsoid_name, semi_major_axis, 'EPSG', uom_code, inv_flattening, semi_minor_axis, deprecated FROM epsg.epsg_ellipsoid")


def fill_area(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO area SELECT 'EPSG', area_code, area_name, area_of_use, area_south_bound_lat, area_north_bound_lat, area_west_bound_lon, area_east_bound_lon, deprecated FROM epsg.epsg_area")


def fill_prime_meridian(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO prime_meridian SELECT 'EPSG', prime_meridian_code, prime_meridian_name, greenwich_longitude, 'EPSG', uom_code, deprecated FROM epsg.epsg_primemeridian")


def fill_geodetic_datum(proj_db_cursor):
    proj_db_cursor.execute(
        "SELECT DISTINCT * FROM epsg.epsg_datum WHERE datum_type NOT IN ('geodetic', 'vertical', 'engineering')")
    res = proj_db_cursor.fetchall()
    if res:
        raise Exception('Found unexpected datum_type in epsg_datum: %s' % str(res))

    proj_db_cursor.execute(
        "INSERT INTO geodetic_datum SELECT 'EPSG', datum_code, datum_name, 'EPSG', ellipsoid_code, 'EPSG', prime_meridian_code, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_datum WHERE datum_type = 'geodetic'")


def fill_vertical_datum(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO vertical_datum SELECT 'EPSG', datum_code, datum_name, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_datum WHERE datum_type = 'vertical'")


def fill_coordinate_system(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO coordinate_system SELECT 'EPSG', coord_sys_code, dimension FROM epsg.epsg_coordinatesystem")


def fill_axis(proj_db_cursor):
    proj_db_cursor.execute("INSERT INTO axis SELECT 'EPSG', coord_axis_code, coord_axis_name, coord_axis_abbreviation, coord_axis_orientation, 'EPSG', coord_sys_code, coord_axis_order, 'EPSG', uom_code FROM epsg.epsg_coordinateaxis ca LEFT JOIN epsg.epsg_coordinateaxisname can ON ca.coord_axis_name_code = can.coord_axis_name_code")


def fill_geodetic_crs(proj_db_cursor):
    proj_db_cursor.execute(
        "SELECT DISTINCT * FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind NOT IN ('projected', 'geographic 2D', 'geographic 3D', 'geocentric', 'vertical', 'compound', 'engineering')")
    res = proj_db_cursor.fetchall()
    if res:
        raise Exception('Found unexpected coord_ref_sys_kind in epsg_coordinatereferencesystem: %s' % str(res))

    proj_db_cursor.execute(
        "INSERT INTO crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_kind FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('geographic 2D', 'geographic 3D', 'geocentric') AND datum_code IS NOT NULL")
    proj_db_cursor.execute("INSERT INTO geodetic_crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_name, coord_ref_sys_kind, 'EPSG', coord_sys_code, 'EPSG', datum_code, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('geographic 2D', 'geographic 3D', 'geocentric') AND datum_code IS NOT NULL")


def fill_vertical_crs(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_kind FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('vertical') AND datum_code IS NOT NULL")
    proj_db_cursor.execute("INSERT INTO vertical_crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_name, 'EPSG', coord_sys_code, 'EPSG', datum_code, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('vertical') AND datum_code IS NOT NULL")


def fill_conversion(proj_db_cursor):
    proj_db_cursor.execute("SELECT coord_op_code, coord_op_name, coord_op_method_code, coord_op_method_name FROM epsg.epsg_coordoperation LEFT JOIN epsg.epsg_coordoperationmethod USING (coord_op_method_code) WHERE coord_op_type = 'conversion' AND coord_op_name NOT LIKE '%to DMSH' AND coord_op_method_code")
    for (code, name, method_code, method_name) in proj_db_cursor.fetchall():
        expected_order = 1
        param_auth_name = [None for i in range(7)]
        param_code = [None for i in range(7)]
        param_name = [None for i in range(7)]
        param_value = [None for i in range(7)]
        param_uom_auth_name = [None for i in range(7)]
        param_uom_code = [None for i in range(7)]

        iterator = proj_db_cursor.execute("SELECT sort_order, cop.parameter_code, parameter_name, parameter_value, uom_code from epsg_coordoperationparam cop LEFT JOIN epsg_coordoperationparamvalue copv LEFT JOIN epsg_coordoperationparamusage copu ON cop.parameter_code = copv.parameter_code AND copu.parameter_code = copv.parameter_code WHERE copu.coord_op_method_code = copv.coord_op_method_code AND coord_op_code = ? AND copv.coord_op_method_code = ? ORDER BY sort_order", (code, method_code))
        for (order, parameter_code, parameter_name, parameter_value, uom_code) in iterator:
            # Modified Krovak and Krovak North Oriented
            if order == 8 and method_code in (1042, 1043):
                break
            assert order < 8
            assert order == expected_order
            param_auth_name[order - 1] = 'EPSG'
            param_code[order - 1] = parameter_code
            param_name[order - 1] = parameter_name
            param_value[order - 1] = parameter_value
            param_uom_auth_name[order - 1] = 'EPSG'
            param_uom_code[order - 1] = uom_code
            expected_order += 1

        arg = ('EPSG', code, name, 'EPSG', method_code, method_name,
               param_auth_name[0], param_code[0], param_name[0],
               param_value[0], param_uom_auth_name[0], param_uom_code[0],
               param_auth_name[1], param_code[1], param_name[1], param_value[1],
               param_uom_auth_name[1], param_uom_code[1], param_auth_name[2],
               param_code[2], param_name[2], param_value[2],
               param_uom_auth_name[2], param_uom_code[2],
               param_auth_name[3], param_code[3], param_name[3], param_value[3],
               param_uom_auth_name[3], param_uom_code[3], param_auth_name[4],
               param_code[4], param_name[4], param_value[4],
               param_uom_auth_name[4], param_uom_code[4], param_auth_name[5],
               param_code[5], param_name[5], param_value[5],
               param_uom_auth_name[5], param_uom_code[5], param_auth_name[6],
               param_code[6], param_name[6], param_value[6],
               param_uom_auth_name[6], param_uom_code[6])
        proj_db_cursor.execute('INSERT INTO conversion VALUES (' +
            '?,?,?, ?,?,?, ?,?,?,?,?,?, ?,?,?,?,?,?, ?,?,?,?,?,?, ' +
            '?,?,?,?,?,?, ?,?,?,?,?,?, ?,?,?,?,?,?, ?,?,?,?,?,?)', arg)


def fill_projected_crs(proj_db_cursor):
    #proj_db_cursor.execute(
    #    "INSERT INTO crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_kind FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('projected')")
    #proj_db_cursor.execute("INSERT INTO projected_crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_name, 'EPSG', coord_sys_code, 'EPSG', source_geogcrs_code, 'EPSG', projection_conv_code, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('projected')")
    proj_db_cursor.execute("SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_name, 'EPSG', coord_sys_code, 'EPSG', source_geogcrs_code, 'EPSG', projection_conv_code, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('projected')")
    for row in proj_db_cursor.fetchall():
        (auth_name, code, name, coordinate_system_auth_name, coordinate_system_code, geodetic_crs_auth_name, geodetic_crs_code, conversion_auth_name, conversion_code, area_of_use_auth_name, area_of_use_code, deprecated) = row
        proj_db_cursor.execute("SELECT 1 FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_code = ? AND coord_ref_sys_kind IN ('geographic 2D', 'geographic 3D', 'geocentric')", (geodetic_crs_code,))
        if proj_db_cursor.fetchone():
            proj_db_cursor.execute("INSERT INTO crs VALUES ('EPSG', ?, 'projected')", (code,))
            proj_db_cursor.execute("INSERT INTO projected_crs VALUES (?,?,?,?,?,?,?,?,?,?,?,?)", row)

def fill_compound_crs(proj_db_cursor):
    proj_db_cursor.execute(
        "INSERT INTO crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_kind FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('compound')")
    proj_db_cursor.execute("INSERT INTO compound_crs SELECT 'EPSG', coord_ref_sys_code, coord_ref_sys_name, 'EPSG', cmpd_horizcrs_code, 'EPSG', cmpd_vertcrs_code, 'EPSG', area_of_use_code, deprecated FROM epsg.epsg_coordinatereferencesystem WHERE coord_ref_sys_kind IN ('compound')")

epsg_db_conn, epsg_tmp_db_filename = ingest_epsg()

script_dir_name = os.path.dirname(os.path.realpath(__file__))
sql_dir_name = os.path.join(os.path.dirname(script_dir_name), 'data', 'sql')

proj_db_filename = ':memory:'
if os.path.exists(proj_db_filename):
    os.unlink(proj_db_filename)
proj_db_conn = sqlite3.connect(proj_db_filename)
proj_db_cursor = proj_db_conn.cursor()
#proj_db_cursor.execute('PRAGMA foreign_keys = 1;')

ingest_sqlite_dump(proj_db_cursor, os.path.join(sql_dir_name, 'proj_db_table_defs.sql'))
proj_db_cursor.execute("ATTACH DATABASE '%s' AS epsg;" % epsg_tmp_db_filename)

fill_unit_of_measure(proj_db_cursor)
fill_ellipsoid(proj_db_cursor)
fill_area(proj_db_cursor)
fill_prime_meridian(proj_db_cursor)
fill_geodetic_datum(proj_db_cursor)
fill_vertical_datum(proj_db_cursor)
fill_coordinate_system(proj_db_cursor)
fill_axis(proj_db_cursor)
fill_geodetic_crs(proj_db_cursor)
fill_vertical_crs(proj_db_cursor)
fill_conversion(proj_db_cursor)
fill_projected_crs(proj_db_cursor)
fill_compound_crs(proj_db_cursor)

proj_db_cursor.close()
proj_db_conn.commit()

files = {}

for line in proj_db_conn.iterdump():
    if line.startswith('INSERT INTO "'):
        table_name = line[len('INSERT INTO "'):]
        table_name = table_name[0:table_name.find('"')]
        if table_name in files:
            f = files[table_name]
        else:
            f = open(os.path.join(sql_dir_name, table_name) + '.sql', 'wb')
            f.write("--- This file has been generated by scripts/build_db.py. DO NOT EDIT !\n\n")
            files[table_name] = f
        f.write((line + '\n').encode('UTF-8'))
del files

proj_db_conn = None

epsg_db_conn = None
if os.path.exists(epsg_tmp_db_filename):
    os.unlink(epsg_tmp_db_filename)
