--- Table structures

CREATE TABLE metadata(
    record TEXT NOT NULL
);

CREATE TABLE unit_of_measure(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    conv_factor FLOAT,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_unit_of_measure PRIMARY KEY (auth_name, code)
);

CREATE TABLE ellipsoid (
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    semi_major_axis FLOAT NOT NULL,
    uom_auth_name TEXT NOT NULL,
    uom_code TEXT NOT NULL,
    inv_flattening FLOAT,
    semi_minor_axis FLOAT,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_ellipsoid PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_ellipsoid_unit_of_measure FOREIGN KEY (uom_auth_name, uom_code) REFERENCES unit_of_measure(auth_name, code)
);

CREATE TABLE area(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    description TEXT NOT NULL,
    south_lat FLOAT,
    north_lat FLOAT,
    west_lon FLOAT,
    east_lon FLOAT,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_area PRIMARY KEY (auth_name, code)
);

CREATE TABLE prime_meridian(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    longitude FLOAT NOT NULL,
    uom_auth_name TEXT NOT NULL,
    uom_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_prime_meridian PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_prime_meridian_unit_of_measure FOREIGN KEY (uom_auth_name, uom_code) REFERENCES unit_of_measure(auth_name, code)
);

CREATE TABLE geodetic_datum (
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    ellipsoid_auth_name TEXT NOT NULL,
    ellipsoid_code TEXT NOT NULL,
    prime_meridian_auth_name TEXT NOT NULL,
    prime_meridian_code TEXT NOT NULL,
    area_of_use_auth_name TEXT NOT NULL,
    area_of_use_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_geodetic_datum PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_geodetic_datum_ellipsoid FOREIGN KEY (ellipsoid_auth_name, ellipsoid_code) REFERENCES ellipsoid(auth_name, code),
    CONSTRAINT fk_geodetic_datum_prime_meridian FOREIGN KEY (prime_meridian_auth_name, prime_meridian_code) REFERENCES prime_meridian(auth_name, code),
    CONSTRAINT fk_geodetic_datum_area FOREIGN KEY (area_of_use_auth_name, area_of_use_code) REFERENCES area(auth_name, code)
);

CREATE TABLE vertical_datum (
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    area_of_use_auth_name TEXT NOT NULL,
    area_of_use_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_vertical_datum PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_vertical_datum_area FOREIGN KEY (area_of_use_auth_name, area_of_use_code) REFERENCES area(auth_name, code)
);

CREATE TABLE coordinate_system(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    dimension SMALLINT NOT NULL,
    CONSTRAINT pk_coordinate_system PRIMARY KEY (auth_name, code)
);

CREATE TABLE axis(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    abbrev TEXT NOT NULL,
    orientation TEXT NOT NULL,
    coordinate_system_auth_name TEXT NOT NULL,
    coordinate_system_code TEXT NOT NULL,
    coordinate_system_order SMALLINT NOT NULL,
    uom_auth_name TEXT NOT NULL,
    uom_code TEXT NOT NULL,
    CONSTRAINT pk_axis PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_axis_coordinate_system FOREIGN KEY (coordinate_system_auth_name, coordinate_system_code) REFERENCES coordinate_system(auth_name, code),
    CONSTRAINT fk_axis_unit_of_measure FOREIGN KEY (uom_auth_name, uom_code) REFERENCES unit_of_measure(auth_name, code)
);

CREATE TABLE crs(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    type TEXT NOT NULL,
    CONSTRAINT pk_geodetic_crs PRIMARY KEY (auth_name, code)
);

CREATE TABLE geodetic_crs(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    coordinate_system_auth_name TEXT NOT NULL,
    coordinate_system_code TEXT NOT NULL,
    datum_auth_name TEXT NOT NULL,
    datum_code TEXT NOT NULL,
    area_of_use_auth_name TEXT NOT NULL,
    area_of_use_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_geodetic_crs PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_geodetic_crs_crs FOREIGN KEY (auth_name, code) REFERENCES crs(auth_name, code),
    CONSTRAINT fk_geodetic_crs_coordinate_system FOREIGN KEY (coordinate_system_auth_name, coordinate_system_code) REFERENCES coordinate_system(auth_name, code),
    CONSTRAINT fk_geodetic_crs_datum FOREIGN KEY (datum_auth_name, datum_code) REFERENCES geodetic_datum(auth_name, code),
    CONSTRAINT fk_geodetic_crs_area FOREIGN KEY (area_of_use_auth_name, area_of_use_code) REFERENCES area(auth_name, code)
);

CREATE TABLE vertical_crs(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    coordinate_system_auth_name TEXT NOT NULL,
    coordinate_system_code TEXT NOT NULL,
    datum_auth_name TEXT NOT NULL,
    datum_code TEXT NOT NULL,
    area_of_use_auth_name TEXT NOT NULL,
    area_of_use_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_vertical_crs PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_vertical_crs_coordinate_system FOREIGN KEY (coordinate_system_auth_name, coordinate_system_code) REFERENCES coordinate_system(auth_name, code),
    CONSTRAINT fk_vertical_crs_datum FOREIGN KEY (datum_auth_name, datum_code) REFERENCES vertical_datum(auth_name, code),
    CONSTRAINT fk_vertical_crs_area FOREIGN KEY (area_of_use_auth_name, area_of_use_code) REFERENCES area(auth_name, code)
);

CREATE TABLE conversion(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,

    method_auth_name TEXT NOT NULL,
    method_code TEXT NOT NULL,
    method_name NOT NULL,

    param1_auth_name TEXT,
    param1_code TEXT,
    param1_name TEXT,
    param1_value FLOAT,
    param1_uom_auth_name TEXT,
    param1_uom_code TEXT,

    param2_auth_name TEXT,
    param2_code TEXT,
    param2_name TEXT,
    param2_value FLOAT,
    param2_uom_auth_name TEXT,
    param2_uom_code TEXT,

    param3_auth_name TEXT,
    param3_code TEXT,
    param3_name TEXT,
    param3_value FLOAT,
    param3_uom_auth_name TEXT,
    param3_uom_code TEXT,

    param4_auth_name TEXT,
    param4_code TEXT,
    param4_name TEXT,
    param4_value FLOAT,
    param4_uom_auth_name TEXT,
    param4_uom_code TEXT,

    param5_auth_name TEXT,
    param5_code TEXT,
    param5_name TEXT,
    param5_value FLOAT,
    param5_uom_auth_name TEXT,
    param5_uom_code TEXT,

    param6_auth_name TEXT,
    param6_code TEXT,
    param6_name TEXT,
    param6_value FLOAT,
    param6_uom_auth_name TEXT,
    param6_uom_code TEXT,

    param7_auth_name TEXT,
    param7_code TEXT,
    param7_name TEXT,
    param7_value FLOAT,
    param7_uom_auth_name TEXT,
    param7_uom_code TEXT,

    CONSTRAINT pk_conversion PRIMARY KEY (auth_name, code)
);

CREATE TABLE projected_crs(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    coordinate_system_auth_name TEXT NOT NULL,
    coordinate_system_code TEXT NOT NULL,
    geodetic_crs_auth_name TEXT NOT NULL,
    geodetic_crs_code TEXT NOT NULL,
    conversion_auth_name TEXT NOT NULL,
    conversion_code TEXT NOT NULL,
    area_of_use_auth_name TEXT NOT NULL,
    area_of_use_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_projected_crs PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_projected_crs_crs FOREIGN KEY (auth_name, code) REFERENCES crs(auth_name, code),
    CONSTRAINT fk_projected_crs_coordinate_system FOREIGN KEY (coordinate_system_auth_name, coordinate_system_code) REFERENCES coordinate_system(auth_name, code),
    CONSTRAINT fk_projected_crs_geodetic_crs FOREIGN KEY (geodetic_crs_auth_name, geodetic_crs_code) REFERENCES geodetic_crs(auth_name, code),
    CONSTRAINT fk_projected_crs_conversion FOREIGN KEY (conversion_auth_name, conversion_code) REFERENCES conversion(auth_name, code),
    CONSTRAINT fk_projected_crs_area FOREIGN KEY (area_of_use_auth_name, area_of_use_code) REFERENCES area(auth_name, code)
);

CREATE TABLE compound_crs(
    auth_name TEXT NOT NULL,
    code TEXT NOT NULL,
    name TEXT NOT NULL,
    horiz_crs_auth_name TEXT NOT NULL,
    horiz_crs_code TEXT NOT NULL,
    vertical_crs_auth_name TEXT NOT NULL,
    vertical_crs_code TEXT NOT NULL,
    area_of_use_auth_name TEXT NOT NULL,
    area_of_use_code TEXT NOT NULL,
    deprecated BOOLEAN NOT NULL,
    CONSTRAINT pk_compound_crs PRIMARY KEY (auth_name, code),
    CONSTRAINT fk_compound_crs_crs FOREIGN KEY (auth_name, code) REFERENCES crs(auth_name, code),
    CONSTRAINT fk_compound_crs_horiz_crs FOREIGN KEY (horiz_crs_auth_name, horiz_crs_code) REFERENCES crs(auth_name, code),
    CONSTRAINT fk_compound_crs_vertical_crs FOREIGN KEY (vertical_crs_auth_name, vertical_crs_code) REFERENCES vertical_crs(auth_name, code),
    CONSTRAINT fk_compoundcrs_area FOREIGN KEY (area_of_use_auth_name, area_of_use_code) REFERENCES area(auth_name, code)
);
