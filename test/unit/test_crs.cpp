/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  Test ISO19111:2018 implementation
 * Author:   Even Rouault <even dot rouault at spatialys dot com>
 *
 ******************************************************************************
 * Copyright (c) 2018, Even Rouault <even dot rouault at spatialys dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "gtest_include.h"

// to be able to use internal::replaceAll
#define FROM_PROJ_CPP

#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/coordinatesystem.hpp"
#include "proj/crs.hpp"
#include "proj/datum.hpp"
#include "proj/io.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include "proj/internal/internal.hpp"
#include "proj/internal/io_internal.hpp"

using namespace osgeo::proj::common;
using namespace osgeo::proj::crs;
using namespace osgeo::proj::cs;
using namespace osgeo::proj::datum;
using namespace osgeo::proj::io;
using namespace osgeo::proj::internal;
using namespace osgeo::proj::metadata;
using namespace osgeo::proj::operation;
using namespace osgeo::proj::util;

namespace {
struct UnrelatedObject : public IComparable {
    UnrelatedObject() = default;

    bool _isEquivalentTo(const IComparable *, Criterion) const override {
        assert(false);
        return false;
    }
};

static nn<std::shared_ptr<UnrelatedObject>> createUnrelatedObject() {
    return nn_make_shared<UnrelatedObject>();
}
} // namespace

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_get_components) {
    auto crs = GeographicCRS::EPSG_4326;
    ASSERT_EQ(crs->identifiers().size(), 1);
    EXPECT_EQ(crs->identifiers()[0]->code(), "4326");
    EXPECT_EQ(*(crs->identifiers()[0]->codeSpace()), "EPSG");
    EXPECT_EQ(crs->nameStr(), "WGS 84");

    auto datum = crs->datum();
    ASSERT_EQ(datum->identifiers().size(), 1);
    EXPECT_EQ(datum->identifiers()[0]->code(), "6326");
    EXPECT_EQ(*(datum->identifiers()[0]->codeSpace()), "EPSG");
    EXPECT_EQ(datum->nameStr(), "World Geodetic System 1984");

    auto ellipsoid = datum->ellipsoid();
    EXPECT_EQ(ellipsoid->semiMajorAxis().value(), 6378137.0);
    EXPECT_EQ(ellipsoid->semiMajorAxis().unit(), UnitOfMeasure::METRE);
    EXPECT_EQ(ellipsoid->inverseFlattening()->value(), 298.257223563);
    ASSERT_EQ(ellipsoid->identifiers().size(), 1);
    EXPECT_EQ(ellipsoid->identifiers()[0]->code(), "7030");
    EXPECT_EQ(*(ellipsoid->identifiers()[0]->codeSpace()), "EPSG");
    EXPECT_EQ(ellipsoid->nameStr(), "WGS 84");
}

// ---------------------------------------------------------------------------

TEST(crs, GeographicCRS_isEquivalentTo) {
    auto crs = GeographicCRS::EPSG_4326;
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));

    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_FALSE(crs->isEquivalentTo(GeographicCRS::EPSG_4979.get()));
    EXPECT_FALSE(crs->isEquivalentTo(GeographicCRS::EPSG_4979.get(),
                                     IComparable::Criterion::EQUIVALENT));

    EXPECT_FALSE(
        GeographicCRS::create(
            PropertyMap(), GeodeticReferenceFrame::EPSG_6326,
            EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE))
            ->isEquivalentTo(crs.get()));

    EXPECT_FALSE(
        GeographicCRS::create(
            PropertyMap(), GeodeticReferenceFrame::EPSG_6326,
            EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE))
            ->isEquivalentTo(
                GeographicCRS::create(PropertyMap(),
                                      GeodeticReferenceFrame::create(
                                          PropertyMap(), Ellipsoid::WGS84,
                                          optional<std::string>(),
                                          PrimeMeridian::GREENWICH),
                                      EllipsoidalCS::createLatitudeLongitude(
                                          UnitOfMeasure::DEGREE))
                    .get()));
}

// ---------------------------------------------------------------------------

TEST(crs, GeographicCRS_datum_ensemble) {
    auto ensemble_vdatum = DatumEnsemble::create(
        PropertyMap(),
        std::vector<DatumNNPtr>{GeodeticReferenceFrame::EPSG_6326,
                                GeodeticReferenceFrame::EPSG_6326},
        PositionalAccuracy::create("100"));
    auto crs = GeographicCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"), nullptr,
        ensemble_vdatum,
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018));
    f->simulCurNodeHasId();
    crs->exportToWKT(f.get());
    auto expected = "GEOGCRS[\"unnamed\",\n"
                    "    ENSEMBLE[\"unnamed\",\n"
                    "        MEMBER[\"World Geodetic System 1984\"],\n"
                    "        MEMBER[\"World Geodetic System 1984\"],\n"
                    "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "        ENSEMBLEACCURACY[100]],\n"
                    "    PRIMEM[\"Greenwich\",0,\n"
                    "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
                    "    CS[ellipsoidal,2],\n"
                    "        AXIS[\"latitude\",north,\n"
                    "            ORDER[1],\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
                    "        AXIS[\"longitude\",east,\n"
                    "            ORDER[2],\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]]]";

    EXPECT_EQ(f->toString(), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, GeographicCRS_ensemble_exception_in_create) {
    EXPECT_THROW(GeographicCRS::create(PropertyMap(), nullptr, nullptr,
                                       EllipsoidalCS::createLatitudeLongitude(
                                           UnitOfMeasure::DEGREE)),
                 Exception);

    auto ensemble_vdatum = DatumEnsemble::create(
        PropertyMap(),
        std::vector<DatumNNPtr>{
            VerticalReferenceFrame::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "vdatum1")),
            VerticalReferenceFrame::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "vdatum2"))},
        PositionalAccuracy::create("100"));
    EXPECT_THROW(GeographicCRS::create(PropertyMap(), nullptr, ensemble_vdatum,
                                       EllipsoidalCS::createLatitudeLongitude(
                                           UnitOfMeasure::DEGREE)),
                 Exception);
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(WKTFormatter::create());
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"WGS 84\",\n"
              "    DATUM[\"World Geodetic System 1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
              "            LENGTHUNIT[\"metre\",1]]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north,\n"
              "            ORDER[1],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            ORDER[2],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_2018) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEOGCRS[\"WGS 84\",\n"
              "    DATUM[\"World Geodetic System 1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
              "            LENGTHUNIT[\"metre\",1]]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north,\n"
              "            ORDER[1],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            ORDER[2],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"WGS 84\",\n"
              "    DATUM[\"World Geodetic System 1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north],\n"
              "        AXIS[\"longitude\",east],\n"
              "        UNIT[\"degree\",0.0174532925199433],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_SIMPLIFIED_single_line) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    f->setMultiLine(false);
    crs->exportToWKT(f.get());
    EXPECT_EQ(
        f->toString(),
        "GEODCRS[\"WGS 84\",DATUM[\"World Geodetic System "
        "1984\",ELLIPSOID[\"WGS "
        "84\",6378137,298.257223563]],"
        "CS[ellipsoidal,2],AXIS[\"latitude\",north],AXIS[\"longitude\",east],"
        "UNIT[\"degree\",0.0174532925199433],ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT2_2018_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018_SIMPLIFIED));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEOGCRS[\"WGS 84\",\n"
              "    DATUM[\"World Geodetic System 1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north],\n"
              "        AXIS[\"longitude\",east],\n"
              "        UNIT[\"degree\",0.0174532925199433],\n"
              "    ID[\"EPSG\",4326]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT1_GDAL) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEOGCS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
              "            AUTHORITY[\"EPSG\",\"7030\"]],\n"
              "        AUTHORITY[\"EPSG\",\"6326\"]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "    UNIT[\"degree\",0.0174532925199433,\n"
              "        AUTHORITY[\"EPSG\",\"9122\"]],\n"
              "    AXIS[\"Latitude\",NORTH],\n"
              "    AXIS[\"Longitude\",EAST],\n"
              "    AUTHORITY[\"EPSG\",\"4326\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT1_ESRI_with_database) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(WKTFormatter::create(
        WKTFormatter::Convention::WKT1_ESRI, DatabaseContext::create()));
    EXPECT_EQ(crs->exportToWKT(f.get()),
              "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_"
              "1984\",6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
              "UNIT[\"Degree\",0.0174532925199433]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_WKT1_ESRI_without_database) {
    auto crs = GeographicCRS::EPSG_4326;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI));
    EXPECT_EQ(crs->exportToWKT(f.get()),
              "GEOGCS[\"GCS_WGS_1984\",DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_"
              "1984\",6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
              "UNIT[\"Degree\",0.0174532925199433]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4326_as_PROJ_string) {
    auto crs = GeographicCRS::EPSG_4326;
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=longlat +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=rad +xy_out=deg +step +proj=axisswap "
              "+order=2,1");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=longlat +datum=WGS84");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT2_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"WGS 84\",\n"
              "    DATUM[\"World Geodetic System 1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,3],\n"
              "        AXIS[\"latitude\",north,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"ellipsoidal height\",up,\n"
              "            UNIT[\"metre\",1]],\n"
              "    ID[\"EPSG\",4979]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT2_2018_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018_SIMPLIFIED));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEOGCRS[\"WGS 84\",\n"
              "    DATUM[\"World Geodetic System 1984\",\n"
              "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
              "    CS[ellipsoidal,3],\n"
              "        AXIS[\"latitude\",north,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            UNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"ellipsoidal height\",up,\n"
              "            UNIT[\"metre\",1]],\n"
              "    ID[\"EPSG\",4979]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT1_GDAL) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f.get());
    // FIXME? WKT1 only supports 2 axis for GEOGCS. So this is an extension of
    // WKT1 as it
    // and GDAL doesn't really export such as beast, although it can import it
    EXPECT_EQ(f->toString(),
              "GEOGCS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
              "            AUTHORITY[\"EPSG\",\"7030\"]],\n"
              "        AUTHORITY[\"EPSG\",\"6326\"]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "    UNIT[\"degree\",0.0174532925199433,\n"
              "        AUTHORITY[\"EPSG\",\"9122\"]],\n"
              "    AXIS[\"Latitude\",NORTH],\n"
              "    AXIS[\"Longitude\",EAST],\n"
              "    AXIS[\"Ellipsoidal height\",UP],\n"
              "    AUTHORITY[\"EPSG\",\"4979\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4979_as_WKT1_ESRI) {
    auto crs = GeographicCRS::EPSG_4979;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI));
    EXPECT_THROW(crs->exportToWKT(f.get()), FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, GeographicCRS_is2DPartOf3D) {
    EXPECT_TRUE(GeographicCRS::EPSG_4326->is2DPartOf3D(
        NN_NO_CHECK(GeographicCRS::EPSG_4979.get())));
    EXPECT_FALSE(GeographicCRS::EPSG_4326->is2DPartOf3D(
        NN_NO_CHECK(GeographicCRS::EPSG_4326.get())));
    EXPECT_FALSE(GeographicCRS::EPSG_4979->is2DPartOf3D(
        NN_NO_CHECK(GeographicCRS::EPSG_4326.get())));
    EXPECT_FALSE(GeographicCRS::EPSG_4979->is2DPartOf3D(
        NN_NO_CHECK(GeographicCRS::EPSG_4979.get())));
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT2) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(WKTFormatter::create(WKTFormatter::Convention::WKT2));
    crs->exportToWKT(f.get());
    EXPECT_EQ(
        f->toString(),
        "GEODCRS[\"NTF (Paris)\",\n"
        "    DATUM[\"Nouvelle Triangulation Francaise (Paris)\",\n"
        "        ELLIPSOID[\"Clarke 1880 (IGN)\",6378249.2,293.466021293627,\n"
        "            LENGTHUNIT[\"metre\",1]]],\n"
        "    PRIMEM[\"Paris\",2.5969213,\n"
        "        ANGLEUNIT[\"grad\",0.015707963267949]],\n"
        "    CS[ellipsoidal,2],\n"
        "        AXIS[\"latitude\",north,\n"
        "            ORDER[1],\n"
        "            ANGLEUNIT[\"grad\",0.015707963267949]],\n"
        "        AXIS[\"longitude\",east,\n"
        "            ORDER[2],\n"
        "            ANGLEUNIT[\"grad\",0.015707963267949]],\n"
        "    ID[\"EPSG\",4807]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT2_SIMPLIFIED) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEODCRS[\"NTF (Paris)\",\n"
              "    DATUM[\"Nouvelle Triangulation Francaise (Paris)\",\n"
              "        ELLIPSOID[\"Clarke 1880 "
              "(IGN)\",6378249.2,293.466021293627]],\n"
              "    PRIMEM[\"Paris\",2.5969213],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north],\n"
              "        AXIS[\"longitude\",east],\n"
              "        UNIT[\"grad\",0.015707963267949],\n"
              "    ID[\"EPSG\",4807]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT1_GDAL) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f.get());
    EXPECT_EQ(
        f->toString(),
        "GEOGCS[\"NTF (Paris)\",\n"
        "    DATUM[\"Nouvelle_Triangulation_Francaise_Paris\",\n"
        "        SPHEROID[\"Clarke 1880 (IGN)\",6378249.2,293.466021293627,\n"
        "            AUTHORITY[\"EPSG\",\"6807\"]],\n"
        "        AUTHORITY[\"EPSG\",\"6807\"]],\n"
        "    PRIMEM[\"Paris\",2.33722917,\n" /* WKT1_GDAL weirdness: PRIMEM is
                                                converted to degree */
        "        AUTHORITY[\"EPSG\",\"8903\"]],\n"
        "    UNIT[\"grad\",0.015707963267949,\n"
        "        AUTHORITY[\"EPSG\",\"9105\"]],\n"
        "    AXIS[\"Latitude\",NORTH],\n"
        "    AXIS[\"Longitude\",EAST],\n"
        "    AUTHORITY[\"EPSG\",\"4807\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT1_ESRI_with_database) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(WKTFormatter::create(
        WKTFormatter::Convention::WKT1_ESRI, DatabaseContext::create()));
    EXPECT_EQ(crs->exportToWKT(f.get()),
              "GEOGCS[\"GCS_NTF_Paris\",DATUM[\"D_NTF\",SPHEROID[\"Clarke_1880_"
              "IGN\",6378249.2,293.466021293627]],PRIMEM[\"Paris\",2.33722917],"
              "UNIT[\"Grad\",0.015707963267949]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_WKT1_ESRI_without_database) {
    auto crs = GeographicCRS::EPSG_4807;
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI));
    EXPECT_EQ(crs->exportToWKT(f.get()),
              "GEOGCS[\"GCS_NTF_Paris\",DATUM[\"D_Nouvelle_Triangulation_"
              "Francaise_Paris\",SPHEROID[\"Clarke_1880_IGN\",6378249.2,293."
              "466021293627]],PRIMEM[\"Paris\",2.33722917],UNIT[\"Grad\",0."
              "015707963267949]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4807_as_PROJ_string) {
    auto crs = GeographicCRS::EPSG_4807;
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=longlat +ellps=clrk80ign "
              "+pm=paris +step +proj=unitconvert +xy_in=rad +xy_out=grad +step "
              "+proj=axisswap +order=2,1");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=longlat +ellps=clrk80ign +pm=paris");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4267) {
    auto crs = GeographicCRS::EPSG_4267;
    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create().get()),
              "GEODCRS[\"NAD27\",\n"
              "    DATUM[\"North American Datum 1927\",\n"
              "        ELLIPSOID[\"Clarke 1866\",6378206.4,294.978698213898,\n"
              "            LENGTHUNIT[\"metre\",1]]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north,\n"
              "            ORDER[1],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            ORDER[2],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    ID[\"EPSG\",4267]]");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=longlat +datum=NAD27");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4267_as_WKT1_ESRI_with_database) {
    auto crs = GeographicCRS::EPSG_4267;
    WKTFormatterNNPtr f(WKTFormatter::create(
        WKTFormatter::Convention::WKT1_ESRI, DatabaseContext::create()));
    EXPECT_EQ(crs->exportToWKT(f.get()),
              "GEOGCS[\"GCS_North_American_1927\","
              "DATUM[\"D_North_American_1927\",SPHEROID[\"Clarke_1866\","
              "6378206.4,294.978698213898]],PRIMEM[\"Greenwich\",0.0],"
              "UNIT[\"Degree\",0.0174532925199433]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4269) {
    auto crs = GeographicCRS::EPSG_4269;
    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create().get()),
              "GEODCRS[\"NAD83\",\n"
              "    DATUM[\"North American Datum 1983\",\n"
              "        ELLIPSOID[\"GRS 1980\",6378137,298.257222101,\n"
              "            LENGTHUNIT[\"metre\",1]]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    CS[ellipsoidal,2],\n"
              "        AXIS[\"latitude\",north,\n"
              "            ORDER[1],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "        AXIS[\"longitude\",east,\n"
              "            ORDER[2],\n"
              "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
              "    ID[\"EPSG\",4269]]");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=longlat +datum=NAD83");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_4268_geogcrs_deprecated_as_WKT1_GDAL) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    auto crs = factory->createCoordinateReferenceSystem("4268");
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    auto wkt = crs->exportToWKT(f.get());
    EXPECT_TRUE(wkt.find("GEOGCS[\"NAD27 Michigan (deprecated)\"") == 0) << wkt;
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_2008_projcrs_deprecated_as_WKT1_GDAL) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    auto crs = factory->createCoordinateReferenceSystem("2008");
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    auto wkt = crs->exportToWKT(f.get());
    EXPECT_TRUE(
        wkt.find("PROJCS[\"NAD27(CGQ77) / SCoPQ zone 2 (deprecated)\"") == 0)
        << wkt;
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_27561_projected_with_geodetic_in_grad_as_PROJ_string_and_WKT1) {
    auto obj = WKTParser().createFromWKT(
        "PROJCRS[\"NTF (Paris) / Lambert Nord France\",\n"
        "  BASEGEODCRS[\"NTF (Paris)\",\n"
        "    DATUM[\"Nouvelle Triangulation Francaise (Paris)\",\n"
        "      ELLIPSOID[\"Clarke 1880 "
        "(IGN)\",6378249.2,293.4660213,LENGTHUNIT[\"metre\",1.0]]],\n"
        "    PRIMEM[\"Paris\",2.5969213,ANGLEUNIT[\"grad\",0.015707963268]]],\n"
        "  CONVERSION[\"Lambert Nord France\",\n"
        "    METHOD[\"Lambert Conic Conformal (1SP)\",ID[\"EPSG\",9801]],\n"
        "    PARAMETER[\"Latitude of natural "
        "origin\",55,ANGLEUNIT[\"grad\",0.015707963268]],\n"
        "    PARAMETER[\"Longitude of natural "
        "origin\",0,ANGLEUNIT[\"grad\",0.015707963268]],\n"
        "    PARAMETER[\"Scale factor at natural "
        "origin\",0.999877341,SCALEUNIT[\"unity\",1.0]],\n"
        "    PARAMETER[\"False easting\",600000,LENGTHUNIT[\"metre\",1.0]],\n"
        "    PARAMETER[\"False northing\",200000,LENGTHUNIT[\"metre\",1.0]]],\n"
        "  CS[cartesian,2],\n"
        "    AXIS[\"easting (X)\",east,ORDER[1]],\n"
        "    AXIS[\"northing (Y)\",north,ORDER[2]],\n"
        "    LENGTHUNIT[\"metre\",1.0],\n"
        "  ID[\"EPSG\",27561]]");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(
        crs->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=grad +xy_out=rad +step +inv +proj=longlat "
        "+ellps=clrk80ign +pm=paris +step +proj=lcc +lat_1=49.5 "
        "+lat_0=49.5 +lon_0=0 +k_0=0.999877341 +x_0=600000 +y_0=200000 "
        "+ellps=clrk80ign +pm=paris");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=lcc +lat_1=49.5 +lat_0=49.5 +lon_0=0 +k_0=0.999877341 "
        "+x_0=600000 +y_0=200000 +ellps=clrk80ign +pm=paris");

    auto nn_crs = NN_CHECK_ASSERT(crs);
    EXPECT_TRUE(nn_crs->isEquivalentTo(nn_crs.get()));
    EXPECT_FALSE(nn_crs->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_FALSE(
        nn_crs->DerivedCRS::isEquivalentTo(createUnrelatedObject().get()));

    auto wkt1 = crs->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get());
    EXPECT_EQ(
        wkt1,
        "PROJCS[\"NTF (Paris) / Lambert Nord France\",\n"
        "    GEOGCS[\"NTF (Paris)\",\n"
        "        DATUM[\"Nouvelle_Triangulation_Francaise_Paris\",\n"
        "            SPHEROID[\"Clarke 1880 (IGN)\",6378249.2,293.4660213]],\n"
        "        PRIMEM[\"Paris\",2.33722917000759],\n"
        "        UNIT[\"grad\",0.015707963268],\n"
        "        AXIS[\"Latitude\",NORTH],\n"
        "        AXIS[\"Longitude\",EAST]],\n"
        "    PROJECTION[\"Lambert_Conformal_Conic_1SP\"],\n"
        "    PARAMETER[\"latitude_of_origin\",55],\n"
        "    PARAMETER[\"central_meridian\",0],\n"
        "    PARAMETER[\"scale_factor\",0.999877341],\n"
        "    PARAMETER[\"false_easting\",600000],\n"
        "    PARAMETER[\"false_northing\",200000],\n"
        "    UNIT[\"metre\",1],\n"
        "    AXIS[\"Easting\",EAST],\n"
        "    AXIS[\"Northing\",NORTH],\n"
        "    AUTHORITY[\"EPSG\",\"27561\"]]");

    auto wkt1_esri = crs->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                             DatabaseContext::create())
            .get());
    EXPECT_EQ(
        wkt1_esri,
        "PROJCS[\"NTF_Paris_Lambert_Nord_France\",GEOGCS[\"GCS_NTF_Paris\","
        "DATUM[\"D_NTF\",SPHEROID[\"Clarke_1880_IGN\",6378249.2,"
        "293.4660213]],PRIMEM[\"Paris\",2.33722917000759],"
        "UNIT[\"Grad\",0.015707963268]],"
        "PROJECTION[\"Lambert_Conformal_Conic\"],"
        "PARAMETER[\"False_Easting\",600000.0],"
        "PARAMETER[\"False_Northing\",200000.0],"
        "PARAMETER[\"Central_Meridian\",0.0],"
        "PARAMETER[\"Standard_Parallel_1\",55.0],"
        "PARAMETER[\"Scale_Factor\",0.999877341],"
        "PARAMETER[\"Latitude_Of_Origin\",55.0],"
        "UNIT[\"Meter\",1.0]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_3040_projected_northing_easting_as_PROJ_string) {
    auto obj = WKTParser().createFromWKT(
        "PROJCRS[\"ETRS89 / UTM zone 28N (N-E)\",\n"
        "  BASEGEODCRS[\"ETRS89\",\n"
        "    DATUM[\"European Terrestrial Reference System 1989\",\n"
        "      ELLIPSOID[\"GRS "
        "1980\",6378137,298.257222101,LENGTHUNIT[\"metre\",1.0]]]],\n"
        "  CONVERSION[\"UTM zone 28N\",\n"
        "    METHOD[\"Transverse Mercator\",ID[\"EPSG\",9807]],\n"
        "    PARAMETER[\"Latitude of natural "
        "origin\",0,ANGLEUNIT[\"degree\",0.01745329252]],\n"
        "    PARAMETER[\"Longitude of natural "
        "origin\",-15,ANGLEUNIT[\"degree\",0.01745329252]],\n"
        "    PARAMETER[\"Scale factor at natural "
        "origin\",0.9996,SCALEUNIT[\"unity\",1.0]],\n"
        "    PARAMETER[\"False easting\",500000,LENGTHUNIT[\"metre\",1.0]],\n"
        "    PARAMETER[\"False northing\",0,LENGTHUNIT[\"metre\",1.0]]],\n"
        "  CS[cartesian,2],\n"
        "    AXIS[\"northing (N)\",north,ORDER[1]],\n"
        "    AXIS[\"easting (E)\",east,ORDER[2]],\n"
        "    LENGTHUNIT[\"metre\",1.0],\n"
        "  ID[\"EPSG\",3040]]");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=utm "
              "+zone=28 +ellps=GRS80 +step +proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_2222_projected_unit_foot_as_PROJ_string_and_WKT1) {
    auto obj = WKTParser().createFromWKT(
        "PROJCRS[\"NAD83 / Arizona East (ft)\",\n"
        "  BASEGEODCRS[\"NAD83\",\n"
        "    DATUM[\"North American Datum 1983\",\n"
        "      ELLIPSOID[\"GRS "
        "1980\",6378137,298.257222101,LENGTHUNIT[\"metre\",1.0]]]],\n"
        "  CONVERSION[\"SPCS83 Arizona East zone (International feet)\",\n"
        "    METHOD[\"Transverse Mercator\",ID[\"EPSG\",9807]],\n"
        "    PARAMETER[\"Latitude of natural "
        "origin\",31,ANGLEUNIT[\"degree\",0.01745329252]],\n"
        "    PARAMETER[\"Longitude of natural "
        "origin\",-110.166666666667,ANGLEUNIT[\"degree\",0.01745329252]],\n"
        "    PARAMETER[\"Scale factor at natural "
        "origin\",0.9999,SCALEUNIT[\"unity\",1.0]],\n"
        "    PARAMETER[\"False easting\",700000,LENGTHUNIT[\"foot\",0.3048]],\n"
        "    PARAMETER[\"False northing\",0,LENGTHUNIT[\"foot\",0.3048]]],\n"
        "  CS[cartesian,2],\n"
        "    AXIS[\"easting (X)\",east,ORDER[1]],\n"
        "    AXIS[\"northing (Y)\",north,ORDER[2]],\n"
        "    LENGTHUNIT[\"foot\",0.3048],\n"
        "  ID[\"EPSG\",2222]]");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=tmerc "
              "+lat_0=31 +lon_0=-110.166666666667 +k_0=0.9999 +x_0=213360 "
              "+y_0=0 +ellps=GRS80 +step +proj=unitconvert +xy_in=m +z_in=m "
              "+xy_out=ft +z_out=ft");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=tmerc +lat_0=31 +lon_0=-110.166666666667 +k_0=0.9999 "
        "+x_0=213360 +y_0=0 +datum=NAD83 +units=ft");

    auto wkt1 = crs->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get());
    EXPECT_EQ(wkt1,
              "PROJCS[\"NAD83 / Arizona East (ft)\",\n"
              "    GEOGCS[\"NAD83\",\n"
              "        DATUM[\"North_American_Datum_1983\",\n"
              "            SPHEROID[\"GRS 1980\",6378137,298.257222101]],\n"
              "        PRIMEM[\"Greenwich\",0,\n"
              "            AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "        UNIT[\"degree\",0.0174532925199433,\n"
              "            AUTHORITY[\"EPSG\",\"9122\"]],\n"
              "        AXIS[\"Latitude\",NORTH],\n"
              "        AXIS[\"Longitude\",EAST]],\n"
              "    PROJECTION[\"Transverse_Mercator\"],\n"
              "    PARAMETER[\"latitude_of_origin\",31],\n"
              "    PARAMETER[\"central_meridian\",-110.166666666667],\n"
              "    PARAMETER[\"scale_factor\",0.9999],\n"
              "    PARAMETER[\"false_easting\",700000],\n"
              "    PARAMETER[\"false_northing\",0],\n"
              "    UNIT[\"foot\",0.3048],\n"
              "    AXIS[\"Easting\",EAST],\n"
              "    AXIS[\"Northing\",NORTH],\n"
              "    AUTHORITY[\"EPSG\",\"2222\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, projected_with_parameter_unit_different_than_cs_unit_as_WKT1) {
    auto obj = WKTParser().createFromWKT(
        "PROJCRS[\"unknown\","
        "    BASEGEODCRS[\"unknown\","
        "        DATUM[\"Unknown based on GRS80 ellipsoid\","
        "            ELLIPSOID[\"GRS 1980\",6378137,298.257222101,"
        "                LENGTHUNIT[\"metre\",1]]],"
        "        PRIMEM[\"Greenwich\",0]],"
        "    CONVERSION[\"UTM zone 32N\","
        "        METHOD[\"Transverse Mercator\"],"
        "        PARAMETER[\"Latitude of natural origin\",0],"
        "        PARAMETER[\"Longitude of natural origin\",9],"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996],"
        "        PARAMETER[\"False easting\",500000,LENGTHUNIT[\"metre\",1]],"
        "        PARAMETER[\"False northing\",0,LENGTHUNIT[\"metre\",1]]],"
        "    CS[Cartesian,2],"
        "        AXIS[\"(E)\",east],"
        "        AXIS[\"(N)\",north],"
        "    LENGTHUNIT[\"US survey foot\",0.304800609601219]]");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto wkt1 = crs->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get());
    EXPECT_EQ(wkt1,
              "PROJCS[\"unknown\",\n"
              "    GEOGCS[\"unknown\",\n"
              "        DATUM[\"Unknown_based_on_GRS80_ellipsoid\",\n"
              "            SPHEROID[\"GRS 1980\",6378137,298.257222101]],\n"
              "        PRIMEM[\"Greenwich\",0],\n"
              "        UNIT[\"degree\",0.0174532925199433,\n"
              "            AUTHORITY[\"EPSG\",\"9122\"]],\n"
              "        AXIS[\"Latitude\",NORTH],\n"
              "        AXIS[\"Longitude\",EAST]],\n"
              "    PROJECTION[\"Transverse_Mercator\"],\n"
              "    PARAMETER[\"latitude_of_origin\",0],\n"
              "    PARAMETER[\"central_meridian\",9],\n"
              "    PARAMETER[\"scale_factor\",0.9996],\n"
              "    PARAMETER[\"false_easting\",1640416.66666667],\n"
              "    PARAMETER[\"false_northing\",0],\n"
              "    UNIT[\"US survey foot\",0.304800609601219],\n"
              "    AXIS[\"Easting\",EAST],\n"
              "    AXIS[\"Northing\",NORTH]]");
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_32661_projected_north_pole_north_east) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    auto crs = factory->createCoordinateReferenceSystem("32661");
    auto proj_crs = nn_dynamic_pointer_cast<ProjectedCRS>(crs);
    ASSERT_TRUE(proj_crs != nullptr);
    auto proj_string =
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=stere "
        "+lat_0=90 +lon_0=0 +k=0.994 +x_0=2000000 +y_0=2000000 "
        "+ellps=WGS84 +step +proj=axisswap +order=2,1";
    EXPECT_EQ(proj_crs->exportToPROJString(PROJStringFormatter::create().get()),
              proj_string);

    auto obj_from_proj = PROJStringParser().createFromPROJString(proj_string);
    auto crs_from_proj = nn_dynamic_pointer_cast<ProjectedCRS>(obj_from_proj);
    ASSERT_TRUE(crs_from_proj != nullptr);
    EXPECT_EQ(
        crs_from_proj->exportToPROJString(PROJStringFormatter::create().get()),
        proj_string);
    EXPECT_TRUE(crs_from_proj->coordinateSystem()->isEquivalentTo(
        proj_crs->coordinateSystem().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_5041_projected_north_pole_east_north) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    auto crs = factory->createCoordinateReferenceSystem("5041");
    auto proj_crs = nn_dynamic_pointer_cast<ProjectedCRS>(crs);
    ASSERT_TRUE(proj_crs != nullptr);
    auto proj_string =
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=stere "
        "+lat_0=90 +lon_0=0 +k=0.994 +x_0=2000000 +y_0=2000000 "
        "+ellps=WGS84";
    EXPECT_EQ(proj_crs->exportToPROJString(PROJStringFormatter::create().get()),
              proj_string);

    auto obj_from_proj = PROJStringParser().createFromPROJString(proj_string);
    auto crs_from_proj = nn_dynamic_pointer_cast<ProjectedCRS>(obj_from_proj);
    ASSERT_TRUE(crs_from_proj != nullptr);
    EXPECT_EQ(
        crs_from_proj->exportToPROJString(PROJStringFormatter::create().get()),
        proj_string);
    EXPECT_TRUE(crs_from_proj->coordinateSystem()->isEquivalentTo(
        proj_crs->coordinateSystem().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_32761_projected_south_pole_north_east) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    auto crs = factory->createCoordinateReferenceSystem("32761");
    auto proj_crs = nn_dynamic_pointer_cast<ProjectedCRS>(crs);
    ASSERT_TRUE(proj_crs != nullptr);
    auto proj_string =
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=stere "
        "+lat_0=-90 +lon_0=0 +k=0.994 +x_0=2000000 +y_0=2000000 "
        "+ellps=WGS84 +step +proj=axisswap +order=2,1";
    EXPECT_EQ(proj_crs->exportToPROJString(PROJStringFormatter::create().get()),
              proj_string);

    auto obj_from_proj = PROJStringParser().createFromPROJString(proj_string);
    auto crs_from_proj = nn_dynamic_pointer_cast<ProjectedCRS>(obj_from_proj);
    ASSERT_TRUE(crs_from_proj != nullptr);
    EXPECT_EQ(
        crs_from_proj->exportToPROJString(PROJStringFormatter::create().get()),
        proj_string);
    EXPECT_TRUE(crs_from_proj->coordinateSystem()->isEquivalentTo(
        proj_crs->coordinateSystem().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, EPSG_5042_projected_south_pole_east_north) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    auto crs = factory->createCoordinateReferenceSystem("5042");
    auto proj_crs = nn_dynamic_pointer_cast<ProjectedCRS>(crs);
    ASSERT_TRUE(proj_crs != nullptr);
    EXPECT_EQ(proj_crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=stere "
              "+lat_0=-90 +lon_0=0 +k=0.994 +x_0=2000000 +y_0=2000000 "
              "+ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(crs, geodetic_crs_both_datum_datum_ensemble_null) {
    EXPECT_THROW(GeodeticCRS::create(
                     PropertyMap(), nullptr, nullptr,
                     CartesianCS::createGeocentric(UnitOfMeasure::METRE)),
                 Exception);
}

// ---------------------------------------------------------------------------

TEST(crs, geodetic_crs_both_datum_datum_ensemble_non_null) {
    auto ensemble = DatumEnsemble::create(
        PropertyMap(),
        std::vector<DatumNNPtr>{GeodeticReferenceFrame::EPSG_6326,
                                GeodeticReferenceFrame::EPSG_6326},
        PositionalAccuracy::create("100"));
    EXPECT_THROW(GeodeticCRS::create(
                     PropertyMap(), GeodeticReferenceFrame::EPSG_6326, ensemble,
                     CartesianCS::createGeocentric(UnitOfMeasure::METRE)),
                 Exception);
}

// ---------------------------------------------------------------------------

static GeodeticCRSNNPtr createGeocentric() {
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 4328)
        .set(IdentifiedObject::NAME_KEY, "WGS 84");
    return GeodeticCRS::create(
        propertiesCRS, GeodeticReferenceFrame::EPSG_6326,
        CartesianCS::createGeocentric(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_WKT2) {
    auto crs = createGeocentric();

    auto expected = "GEODCRS[\"WGS 84\",\n"
                    "    DATUM[\"World Geodetic System 1984\",\n"
                    "        ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "            LENGTHUNIT[\"metre\",1]]],\n"
                    "    PRIMEM[\"Greenwich\",0,\n"
                    "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "        AXIS[\"(Y)\",geocentricY,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "        AXIS[\"(Z)\",geocentricZ,\n"
                    "            ORDER[3],\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "    ID[\"EPSG\",4328]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create().get()), expected);
    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_WKT2_simplified) {
    auto crs = createGeocentric();

    auto expected = "GEODCRS[\"WGS 84\",\n"
                    "    DATUM[\"World Geodetic System 1984\",\n"
                    "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX],\n"
                    "        AXIS[\"(Y)\",geocentricY],\n"
                    "        AXIS[\"(Z)\",geocentricZ],\n"
                    "        UNIT[\"metre\",1],\n"
                    "    ID[\"EPSG\",4328]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create(
                                   WKTFormatter::Convention::WKT2_SIMPLIFIED)
                                   .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_WKT1_GDAL) {
    auto crs = createGeocentric();
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL));
    crs->exportToWKT(f.get());
    EXPECT_EQ(f->toString(),
              "GEOCCS[\"WGS 84\",\n"
              "    DATUM[\"WGS_1984\",\n"
              "        SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
              "            AUTHORITY[\"EPSG\",\"7030\"]],\n"
              "        AUTHORITY[\"EPSG\",\"6326\"]],\n"
              "    PRIMEM[\"Greenwich\",0,\n"
              "        AUTHORITY[\"EPSG\",\"8901\"]],\n"
              "    UNIT[\"metre\",1,\n"
              "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
              "    AXIS[\"Geocentric X\",OTHER],\n"
              "    AXIS[\"Geocentric Y\",OTHER],\n"
              "    AXIS[\"Geocentric Z\",NORTH],\n"
              "    AUTHORITY[\"EPSG\",\"4328\"]]");
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_as_PROJ_string) {
    auto crs = createGeocentric();
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=cart +ellps=WGS84");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=geocent +ellps=WGS84");
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_non_meter_unit_as_PROJ_string) {
    auto crs = GeodeticCRS::create(
        PropertyMap(), GeodeticReferenceFrame::EPSG_6326,
        CartesianCS::createGeocentric(
            UnitOfMeasure("kilometre", 1000.0, UnitOfMeasure::Type::LINEAR)));

    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=cart +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=m +z_in=m +xy_out=km +z_out=km");
    EXPECT_THROW(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, geocentricCRS_unsupported_unit_as_PROJ_string) {
    auto crs = GeodeticCRS::create(
        PropertyMap(), GeodeticReferenceFrame::EPSG_6326,
        CartesianCS::createGeocentric(
            UnitOfMeasure("my unit", 500.0, UnitOfMeasure::Type::LINEAR)));

    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=cart +ellps=WGS84 +step "
              "+proj=unitconvert +xy_in=m +z_in=m +xy_out=500 +z_out=500");
}

// ---------------------------------------------------------------------------

static ProjectedCRSNNPtr createProjected() {
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 32631)
        .set(IdentifiedObject::NAME_KEY, "WGS 84 / UTM zone 31N");
    return ProjectedCRS::create(
        propertiesCRS, GeographicCRS::EPSG_4326,
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_derivingConversion) {
    auto crs = createProjected();
    auto conv = crs->derivingConversion();
    EXPECT_TRUE(conv->sourceCRS() != nullptr);
    ASSERT_TRUE(conv->targetCRS() != nullptr);
    EXPECT_EQ(conv->targetCRS().get(), crs.get());

    // derivingConversion() returns a copy of the internal conversion
    auto targetCRSAsProjCRS =
        std::dynamic_pointer_cast<ProjectedCRS>(conv->targetCRS());
    ASSERT_TRUE(targetCRSAsProjCRS != nullptr);
    EXPECT_NE(targetCRSAsProjCRS->derivingConversion(), conv);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_shallowClone) {
    {
        auto crs = createProjected();
        EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
        EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));
        auto clone = nn_dynamic_pointer_cast<ProjectedCRS>(crs->shallowClone());
        EXPECT_TRUE(clone->isEquivalentTo(crs.get()));
        EXPECT_EQ(clone->derivingConversion()->targetCRS().get(), clone.get());
    }

    {
        ProjectedCRSPtr clone;
        {
            auto crs = ProjectedCRS::create(
                PropertyMap(), createGeocentric(),
                Conversion::createUTM(PropertyMap(), 31, true),
                CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
            clone = nn_dynamic_pointer_cast<ProjectedCRS>(crs->shallowClone());
        }
        EXPECT_EQ(clone->baseCRS()->exportToPROJString(
                      PROJStringFormatter::create().get()),
                  "+proj=cart +ellps=WGS84");
    }
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT2) {
    auto crs = createProjected();

    auto expected =
        "PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "    BASEGEODCRS[\"WGS 84\",\n"
        "        DATUM[\"World Geodetic System 1984\",\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                LENGTHUNIT[\"metre\",1]]],\n"
        "        PRIMEM[\"Greenwich\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
        "    CONVERSION[\"UTM zone 31N\",\n"
        "        METHOD[\"Transverse Mercator\",\n"
        "            ID[\"EPSG\",9807]],\n"
        "        PARAMETER[\"Latitude of natural origin\",0,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "            ID[\"EPSG\",8801]],\n"
        "        PARAMETER[\"Longitude of natural origin\",3,\n"
        "            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "            ID[\"EPSG\",8802]],\n"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996,\n"
        "            SCALEUNIT[\"unity\",1],\n"
        "            ID[\"EPSG\",8805]],\n"
        "        PARAMETER[\"False easting\",500000,\n"
        "            LENGTHUNIT[\"metre\",1],\n"
        "            ID[\"EPSG\",8806]],\n"
        "        PARAMETER[\"False northing\",0,\n"
        "            LENGTHUNIT[\"metre\",1],\n"
        "            ID[\"EPSG\",8807]]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east,\n"
        "            ORDER[1],\n"
        "            LENGTHUNIT[\"metre\",1]],\n"
        "        AXIS[\"(N)\",north,\n"
        "            ORDER[2],\n"
        "            LENGTHUNIT[\"metre\",1]],\n"
        "    ID[\"EPSG\",32631]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create().get()), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT2_simplified) {
    auto crs = createProjected();

    auto expected =
        "PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "    BASEGEODCRS[\"WGS 84\",\n"
        "        DATUM[\"World Geodetic System 1984\",\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
        "        UNIT[\"degree\",0.0174532925199433]],\n"
        "    CONVERSION[\"UTM zone 31N\",\n"
        "        METHOD[\"Transverse Mercator\"],\n"
        "        PARAMETER[\"Latitude of natural origin\",0],\n"
        "        PARAMETER[\"Longitude of natural origin\",3],\n"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996],\n"
        "        PARAMETER[\"False easting\",500000],\n"
        "        PARAMETER[\"False northing\",0]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east],\n"
        "        AXIS[\"(N)\",north],\n"
        "        UNIT[\"metre\",1],\n"
        "    ID[\"EPSG\",32631]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create(
                                   WKTFormatter::Convention::WKT2_SIMPLIFIED)
                                   .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT2_2018_simplified) {
    auto crs = createProjected();

    auto expected =
        "PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "    BASEGEOGCRS[\"WGS 84\",\n"
        "        DATUM[\"World Geodetic System 1984\",\n"
        "            ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
        "        UNIT[\"degree\",0.0174532925199433]],\n"
        "    CONVERSION[\"UTM zone 31N\",\n"
        "        METHOD[\"Transverse Mercator\"],\n"
        "        PARAMETER[\"Latitude of natural origin\",0],\n"
        "        PARAMETER[\"Longitude of natural origin\",3],\n"
        "        PARAMETER[\"Scale factor at natural origin\",0.9996],\n"
        "        PARAMETER[\"False easting\",500000],\n"
        "        PARAMETER[\"False northing\",0]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east],\n"
        "        AXIS[\"(N)\",north],\n"
        "        UNIT[\"metre\",1],\n"
        "    ID[\"EPSG\",32631]]";

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018_SIMPLIFIED)
                .get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT1_GDAL) {
    auto crs = createProjected();

    auto expected = "PROJCS[\"WGS 84 / UTM zone 31N\",\n"
                    "    GEOGCS[\"WGS 84\",\n"
                    "        DATUM[\"WGS_1984\",\n"
                    "            SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
                    "                AUTHORITY[\"EPSG\",\"7030\"]],\n"
                    "            AUTHORITY[\"EPSG\",\"6326\"]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            AUTHORITY[\"EPSG\",\"8901\"]],\n"
                    "        UNIT[\"degree\",0.0174532925199433,\n"
                    "            AUTHORITY[\"EPSG\",\"9122\"]],\n"
                    "        AXIS[\"Latitude\",NORTH],\n"
                    "        AXIS[\"Longitude\",EAST],\n"
                    "        AUTHORITY[\"EPSG\",\"4326\"]],\n"
                    "    PROJECTION[\"Transverse_Mercator\"],\n"
                    "    PARAMETER[\"latitude_of_origin\",0],\n"
                    "    PARAMETER[\"central_meridian\",3],\n"
                    "    PARAMETER[\"scale_factor\",0.9996],\n"
                    "    PARAMETER[\"false_easting\",500000],\n"
                    "    PARAMETER[\"false_northing\",0],\n"
                    "    UNIT[\"metre\",1,\n"
                    "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
                    "    AXIS[\"Easting\",EAST],\n"
                    "    AXIS[\"Northing\",NORTH],\n"
                    "    AUTHORITY[\"EPSG\",\"32631\"]]";

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_WKT1_ESRI) {
    auto crs = createProjected();

    auto expected = "PROJCS[\"WGS_1984_UTM_Zone_31N\",GEOGCS[\"GCS_WGS_1984\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.0,"
                    "298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Transverse_Mercator\"],"
                    "PARAMETER[\"False_Easting\",500000.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",3.0],"
                    "PARAMETER[\"Scale_Factor\",0.9996],"
                    "PARAMETER[\"Latitude_Of_Origin\",0.0],"
                    "UNIT[\"Meter\",1.0]]";

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, projectedCRS_as_PROJ_string) {
    auto crs = createProjected();
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
              "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=utm "
              "+zone=31 +ellps=WGS84");
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=utm +zone=31 +datum=WGS84");
}

// ---------------------------------------------------------------------------

TEST(crs, mercator_1SP_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=merc +lon_0=110 +k=0.997 +x_0=3900000 +y_0=900000 "
        "+ellps=bessel");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_Unknown_based_on_Bessel_1841_ellipsoid\","
                    "SPHEROID[\"Bessel_1841\",6377397.155,299.1528128]],"
                    "PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Mercator\"],"
                    "PARAMETER[\"False_Easting\",3900000.0],"
                    "PARAMETER[\"False_Northing\",900000.0],"
                    "PARAMETER[\"Central_Meridian\",110.0],"
                    "PARAMETER[\"Standard_Parallel_1\",4.45405154589748],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Plate_Carree_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+title=my Plate carree +proj=eqc");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"my_Plate_carree\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Plate_Carree\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Equidistant_Cylindrical_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString("+proj=eqc");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Equidistant_Cylindrical\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "PARAMETER[\"Standard_Parallel_1\",0.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Hotine_Oblique_Mercator_Azimuth_Natural_Origin_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=omerc +no_uoff +gamma=295 +alpha=295");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION["
                    "\"Hotine_Oblique_Mercator_Azimuth_Natural_Origin\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    // we renormalize angles to [-180,180]
                    "PARAMETER[\"Azimuth\",-65.0],"
                    "PARAMETER[\"Longitude_Of_Center\",0.0],"
                    "PARAMETER[\"Latitude_Of_Center\",0.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Rectified_Skew_Orthomorphic_Natural_Origin_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=omerc +no_uoff +gamma=3 +alpha=2");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION["
                    "\"Rectified_Skew_Orthomorphic_Natural_Origin\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    "PARAMETER[\"Azimuth\",2.0],"
                    "PARAMETER[\"Longitude_Of_Center\",0.0],"
                    "PARAMETER[\"Latitude_Of_Center\",0.0],"
                    "PARAMETER[\"XY_Plane_Rotation\",3.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Hotine_Oblique_Mercator_Azimuth_Center_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=omerc +gamma=2 +alpha=2");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION["
                    "\"Hotine_Oblique_Mercator_Azimuth_Center\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    "PARAMETER[\"Azimuth\",2.0],"
                    "PARAMETER[\"Longitude_Of_Center\",0.0],"
                    "PARAMETER[\"Latitude_Of_Center\",0.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Rectified_Skew_Orthomorphic_Center_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=omerc +gamma=3 +alpha=2");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION["
                    "\"Rectified_Skew_Orthomorphic_Center\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    "PARAMETER[\"Azimuth\",2.0],"
                    "PARAMETER[\"Longitude_Of_Center\",0.0],"
                    "PARAMETER[\"Latitude_Of_Center\",0.0],"
                    "PARAMETER[\"XY_Plane_Rotation\",3.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Gauss_Kruger_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+title=my Gauss Kruger +proj=tmerc");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"my_Gauss_Kruger\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.0,"
                    "298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Gauss_Kruger\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    "PARAMETER[\"Latitude_Of_Origin\",0.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Stereographic_North_Pole_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=stere +lat_0=90 +lat_ts=70");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Stereographic_North_Pole\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "PARAMETER[\"Standard_Parallel_1\",70.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Stereographic_South_Pole_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=stere +lat_0=-90 +lat_ts=-70");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Stereographic_South_Pole\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "PARAMETER[\"Standard_Parallel_1\",-70.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Krovak_North_Orientated_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString("+proj=krovak");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Krovak\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Pseudo_Standard_Parallel_1\",78.5],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    "PARAMETER[\"Azimuth\",30.2881397222222],"
                    "PARAMETER[\"Longitude_Of_Center\",0.0],"
                    "PARAMETER[\"Latitude_Of_Center\",0.0],"
                    "PARAMETER[\"X_Scale\",-1.0],"
                    "PARAMETER[\"Y_Scale\",1.0],"
                    "PARAMETER[\"XY_Plane_Rotation\",90.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, Krovak_as_WKT1_ESRI) {

    auto obj =
        PROJStringParser().createFromPROJString("+proj=krovak +axis=swu");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Krovak\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Pseudo_Standard_Parallel_1\",78.5],"
                    "PARAMETER[\"Scale_Factor\",1.0],"
                    "PARAMETER[\"Azimuth\",30.2881397222222],"
                    "PARAMETER[\"Longitude_Of_Center\",0.0],"
                    "PARAMETER[\"Latitude_Of_Center\",0.0],"
                    "PARAMETER[\"X_Scale\",1.0],"
                    "PARAMETER[\"Y_Scale\",1.0],"
                    "PARAMETER[\"XY_Plane_Rotation\",0.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, LCC_1SP_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=lcc +lat_1=1 +lat_0=1 +k=0.9");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Lambert_Conformal_Conic\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "PARAMETER[\"Standard_Parallel_1\",1.0],"
                    "PARAMETER[\"Scale_Factor\",0.9],"
                    "PARAMETER[\"Latitude_Of_Origin\",1.0],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, LCC_2SP_as_WKT1_ESRI) {

    auto obj = PROJStringParser().createFromPROJString(
        "+proj=lcc +lat_0=1.5 +lat_1=1 +lat_2=2");
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected = "PROJCS[\"unknown\",GEOGCS[\"GCS_unknown\","
                    "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\","
                    "6378137.0,298.257223563]],PRIMEM[\"Greenwich\",0.0],"
                    "UNIT[\"Degree\",0.0174532925199433]],"
                    "PROJECTION[\"Lambert_Conformal_Conic\"],"
                    "PARAMETER[\"False_Easting\",0.0],"
                    "PARAMETER[\"False_Northing\",0.0],"
                    "PARAMETER[\"Central_Meridian\",0.0],"
                    "PARAMETER[\"Standard_Parallel_1\",1.0],"
                    "PARAMETER[\"Standard_Parallel_2\",2.0],"
                    "PARAMETER[\"Latitude_Of_Origin\",1.5],"
                    "UNIT[\"Meter\",1.0]]";
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, ESRI_WKT1_to_ESRI_WKT1) {

    auto in_wkt =
        "PROJCS[\"NAD_1983_CORS96_StatePlane_North_Carolina_FIPS_3200_Ft_US\","
        "GEOGCS[\"GCS_NAD_1983_CORS96\",DATUM[\"D_NAD_1983_CORS96\","
        "SPHEROID[\"GRS_1980\",6378137.0,298.257222101]],"
        "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],"
        "PROJECTION[\"Lambert_Conformal_Conic\"],"
        "PARAMETER[\"False_Easting\",2000000.0],"
        "PARAMETER[\"False_Northing\",0.0],"
        "PARAMETER[\"Central_Meridian\",-79.0],"
        "PARAMETER[\"Standard_Parallel_1\",34.33333333333334],"
        "PARAMETER[\"Standard_Parallel_2\",36.16666666666666],"
        "PARAMETER[\"Latitude_Of_Origin\",33.75],"
        "UNIT[\"Foot_US\",0.3048006096012192]]";

    auto obj = WKTParser().createFromWKT(in_wkt);
    auto crs = nn_dynamic_pointer_cast<ProjectedCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto expected =
        "PROJCS[\"NAD_1983_CORS96_StatePlane_North_Carolina_FIPS_3200_Ft_US\","
        "GEOGCS[\"GCS_NAD_1983_CORS96\",DATUM[\"D_NAD_1983_CORS96\","
        "SPHEROID[\"GRS_1980\",6378137.0,298.257222101]],"
        "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],"
        "PROJECTION[\"Lambert_Conformal_Conic\"],"
        "PARAMETER[\"False_Easting\",2000000.0],"
        "PARAMETER[\"False_Northing\",0.0],"
        "PARAMETER[\"Central_Meridian\",-79.0],"
        "PARAMETER[\"Standard_Parallel_1\",34.3333333333333],"
        "PARAMETER[\"Standard_Parallel_2\",36.1666666666667],"
        "PARAMETER[\"Latitude_Of_Origin\",33.75],"
        "UNIT[\"Foot_US\",0.304800609601219]]";

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT1_ESRI,
                                       DatabaseContext::create())
                      .get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(datum, cs_with_MERIDIAN) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "Easting"), "X",
            AxisDirection::SOUTH, UnitOfMeasure::METRE,
            Meridian::create(Angle(90.0))),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "Northing"), "Y",
            AxisDirection::SOUTH, UnitOfMeasure::METRE,
            Meridian::create(Angle(180.0)))};
    auto cs(CartesianCS::create(PropertyMap(), axis[0], axis[1]));

    auto expected = "CS[Cartesian,2],\n"
                    "    AXIS[\"easting (X)\",south,\n"
                    "        MERIDIAN[90,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
                    "        ORDER[1],\n"
                    "        LENGTHUNIT[\"metre\",1]],\n"
                    "    AXIS[\"northing (Y)\",south,\n"
                    "        MERIDIAN[180,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
                    "        ORDER[2],\n"
                    "        LENGTHUNIT[\"metre\",1]]";

    auto formatter = WKTFormatter::create();
    formatter->setOutputId(false);
    EXPECT_EQ(cs->exportToWKT(formatter.get()), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, scope_area_bbox_remark) {
    auto in_wkt = "GEODETICCRS[\"JGD2000\","
                  "DATUM[\"Japanese Geodetic Datum 2000\","
                  "  ELLIPSOID[\"GRS 1980\",6378137,298.257222101]],"
                  "CS[Cartesian,3],"
                  "  AXIS[\"(X)\",geocentricX],"
                  "  AXIS[\"(Y)\",geocentricY],"
                  "  AXIS[\"(Z)\",geocentricZ],"
                  "  LENGTHUNIT[\"metre\",1.0],"
                  "SCOPE[\"Geodesy, topographic mapping and cadastre\"],"
                  "AREA[\"Japan\"],"
                  "BBOX[17.09,122.38,46.05,157.64],"
                  "VERTICALEXTENT[-10000,10000],"
                  "TIMEEXTENT[2002-04-01,2011-10-21],"
                  "ID[\"EPSG\",4946],\n"
                  "REMARK[\"some_remark\"]]";
    auto crs =
        nn_dynamic_pointer_cast<GeodeticCRS>(WKTParser().createFromWKT(in_wkt));
    ASSERT_TRUE(crs != nullptr);

    ASSERT_EQ(crs->domains().size(), 1);
    auto domain = crs->domains()[0];
    EXPECT_TRUE(domain->scope().has_value());
    EXPECT_EQ(*(domain->scope()), "Geodesy, topographic mapping and cadastre");
    ASSERT_TRUE(domain->domainOfValidity() != nullptr);
    EXPECT_TRUE(domain->domainOfValidity()->description().has_value());
    EXPECT_EQ(*(domain->domainOfValidity()->description()), "Japan");
    ASSERT_EQ(domain->domainOfValidity()->geographicElements().size(), 1);
    auto geogElement = domain->domainOfValidity()->geographicElements()[0];
    auto bbox = nn_dynamic_pointer_cast<GeographicBoundingBox>(geogElement);
    ASSERT_TRUE(bbox != nullptr);
    EXPECT_EQ(bbox->southBoundLatitude(), 17.09);
    EXPECT_EQ(bbox->westBoundLongitude(), 122.38);
    EXPECT_EQ(bbox->northBoundLatitude(), 46.05);
    EXPECT_EQ(bbox->eastBoundLongitude(), 157.64);

    ASSERT_EQ(domain->domainOfValidity()->verticalElements().size(), 1);
    auto verticalElement = domain->domainOfValidity()->verticalElements()[0];
    EXPECT_EQ(verticalElement->minimumValue(), -10000);
    EXPECT_EQ(verticalElement->maximumValue(), 10000);
    EXPECT_EQ(*(verticalElement->unit()), UnitOfMeasure::METRE);

    ASSERT_EQ(domain->domainOfValidity()->temporalElements().size(), 1);
    auto temporalElement = domain->domainOfValidity()->temporalElements()[0];
    EXPECT_EQ(temporalElement->start(), "2002-04-01");
    EXPECT_EQ(temporalElement->stop(), "2011-10-21");

    auto got_wkt = crs->exportToWKT(WKTFormatter::create().get());
    auto expected =
        "GEODCRS[\"JGD2000\",\n"
        "    DATUM[\"Japanese Geodetic Datum 2000\",\n"
        "        ELLIPSOID[\"GRS 1980\",6378137,298.257222101,\n"
        "            LENGTHUNIT[\"metre\",1]]],\n"
        "    PRIMEM[\"Greenwich\",0,\n"
        "        ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "    CS[Cartesian,3],\n"
        "        AXIS[\"(X)\",geocentricX,\n"
        "            ORDER[1],\n"
        "            LENGTHUNIT[\"metre\",1]],\n"
        "        AXIS[\"(Y)\",geocentricY,\n"
        "            ORDER[2],\n"
        "            LENGTHUNIT[\"metre\",1]],\n"
        "        AXIS[\"(Z)\",geocentricZ,\n"
        "            ORDER[3],\n"
        "            LENGTHUNIT[\"metre\",1]],\n"
        "    SCOPE[\"Geodesy, topographic mapping and cadastre\"],\n"
        "    AREA[\"Japan\"],\n"
        "    BBOX[17.09,122.38,46.05,157.64],\n"
        "    VERTICALEXTENT[-10000,10000,\n"
        "        LENGTHUNIT[\"metre\",1]],\n"
        "    TIMEEXTENT[2002-04-01,2011-10-21],\n"
        "    ID[\"EPSG\",4946],\n"
        "    REMARK[\"some_remark\"]]";

    EXPECT_EQ(got_wkt, expected);
}

// ---------------------------------------------------------------------------

TEST(crs, usage) {
    auto in_wkt = "GEODETICCRS[\"JGD2000\","
                  "DATUM[\"Japanese Geodetic Datum 2000\","
                  "  ELLIPSOID[\"GRS 1980\",6378137,298.257222101]],"
                  "CS[Cartesian,3],"
                  "  AXIS[\"(X)\",geocentricX],"
                  "  AXIS[\"(Y)\",geocentricY],"
                  "  AXIS[\"(Z)\",geocentricZ],"
                  "  LENGTHUNIT[\"metre\",1.0],"
                  "USAGE[SCOPE[\"scope\"],AREA[\"area.\"]]]";
    auto crs =
        nn_dynamic_pointer_cast<GeodeticCRS>(WKTParser().createFromWKT(in_wkt));
    ASSERT_TRUE(crs != nullptr);

    auto got_wkt = crs->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get());
    auto expected = "GEODCRS[\"JGD2000\",\n"
                    "    DATUM[\"Japanese Geodetic Datum 2000\",\n"
                    "        ELLIPSOID[\"GRS 1980\",6378137,298.257222101,\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]]],\n"
                    "    PRIMEM[\"Greenwich\",0,\n"
                    "        ANGLEUNIT[\"degree\",0.0174532925199433],\n"
                    "        ID[\"EPSG\",8901]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "        AXIS[\"(Y)\",geocentricY,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "        AXIS[\"(Z)\",geocentricZ,\n"
                    "            ORDER[3],\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "    USAGE[\n"
                    "        SCOPE[\"scope\"],\n"
                    "        AREA[\"area.\"]]]";
    EXPECT_EQ(got_wkt, expected);
}

// ---------------------------------------------------------------------------

TEST(crs, multiple_ID) {

    PropertyMap propertiesCRS;
    propertiesCRS.set(IdentifiedObject::NAME_KEY, "WGS 84");
    auto identifiers = ArrayOfBaseObject::create();
    identifiers->add(Identifier::create(
        "codeA", PropertyMap().set(Identifier::CODESPACE_KEY, "authorityA")));
    identifiers->add(Identifier::create(
        "codeB", PropertyMap().set(Identifier::CODESPACE_KEY, "authorityB")));
    propertiesCRS.set(IdentifiedObject::IDENTIFIERS_KEY, identifiers);
    auto crs = GeodeticCRS::create(
        propertiesCRS, GeodeticReferenceFrame::EPSG_6326,
        CartesianCS::createGeocentric(UnitOfMeasure::METRE));

    auto got_wkt = crs->exportToWKT(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_SIMPLIFIED).get());
    auto expected = "GEODCRS[\"WGS 84\",\n"
                    "    DATUM[\"World Geodetic System 1984\",\n"
                    "        ELLIPSOID[\"WGS 84\",6378137,298.257223563]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX],\n"
                    "        AXIS[\"(Y)\",geocentricY],\n"
                    "        AXIS[\"(Z)\",geocentricZ],\n"
                    "        UNIT[\"metre\",1],\n"
                    "    ID[\"authorityA\",\"codeA\"],\n"
                    "    ID[\"authorityB\",\"codeB\"]]";

    EXPECT_EQ(got_wkt, expected);
}

// ---------------------------------------------------------------------------

static VerticalCRSNNPtr createVerticalCRS() {
    PropertyMap propertiesVDatum;
    propertiesVDatum.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 5101)
        .set(IdentifiedObject::NAME_KEY, "Ordnance Datum Newlyn");
    auto vdatum = VerticalReferenceFrame::create(propertiesVDatum);
    PropertyMap propertiesCRS;
    propertiesCRS.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 5701)
        .set(IdentifiedObject::NAME_KEY, "ODN height");
    return VerticalCRS::create(
        propertiesCRS, vdatum,
        VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, verticalCRS_as_WKT2) {
    auto crs = createVerticalCRS();
    auto expected = "VERTCRS[\"ODN height\",\n"
                    "    VDATUM[\"Ordnance Datum Newlyn\"],\n"
                    "    CS[vertical,1],\n"
                    "        AXIS[\"gravity-related height (H)\",up,\n"
                    "            LENGTHUNIT[\"metre\",1]],\n"
                    "    ID[\"EPSG\",5701]]";

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create().get()), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, verticalCRS_as_WKT1_GDAL) {
    auto crs = createVerticalCRS();
    auto expected = "VERT_CS[\"ODN height\",\n"
                    "    VERT_DATUM[\"Ordnance Datum Newlyn\",2005,\n"
                    "        AUTHORITY[\"EPSG\",\"5101\"]],\n"
                    "    UNIT[\"metre\",1,\n"
                    "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
                    "    AXIS[\"Gravity-related height\",UP],\n"
                    "    AUTHORITY[\"EPSG\",\"5701\"]]";

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, verticalCRS_datum_ensemble) {
    auto ensemble = DatumEnsemble::create(
        PropertyMap(),
        std::vector<DatumNNPtr>{
            VerticalReferenceFrame::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "vdatum1")),
            VerticalReferenceFrame::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "vdatum2"))},
        PositionalAccuracy::create("100"));
    auto crs = VerticalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"), nullptr,
        ensemble, VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));
    WKTFormatterNNPtr f(
        WKTFormatter::create(WKTFormatter::Convention::WKT2_2018));
    f->simulCurNodeHasId();
    crs->exportToWKT(f.get());
    auto expected = "VERTCRS[\"unnamed\",\n"
                    "    ENSEMBLE[\"unnamed\",\n"
                    "        MEMBER[\"vdatum1\"],\n"
                    "        MEMBER[\"vdatum2\"],\n"
                    "        ENSEMBLEACCURACY[100]],\n"
                    "    CS[vertical,1],\n"
                    "        AXIS[\"gravity-related height (H)\",up,\n"
                    "            LENGTHUNIT[\"metre\",1]]]";
    EXPECT_EQ(f->toString(), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, VerticalCRS_ensemble_exception_in_create) {
    EXPECT_THROW(VerticalCRS::create(PropertyMap(), nullptr, nullptr,
                                     VerticalCS::createGravityRelatedHeight(
                                         UnitOfMeasure::METRE)),
                 Exception);

    auto ensemble_hdatum = DatumEnsemble::create(
        PropertyMap(),
        std::vector<DatumNNPtr>{GeodeticReferenceFrame::EPSG_6326,
                                GeodeticReferenceFrame::EPSG_6326},
        PositionalAccuracy::create("100"));
    EXPECT_THROW(VerticalCRS::create(PropertyMap(), nullptr, ensemble_hdatum,
                                     VerticalCS::createGravityRelatedHeight(
                                         UnitOfMeasure::METRE)),
                 Exception);
}

// ---------------------------------------------------------------------------

TEST(datum, vdatum_with_anchor) {
    PropertyMap propertiesVDatum;
    propertiesVDatum.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 5101)
        .set(IdentifiedObject::NAME_KEY, "Ordnance Datum Newlyn");
    auto vdatum = VerticalReferenceFrame::create(
        propertiesVDatum, optional<std::string>("my anchor"),
        optional<RealizationMethod>(RealizationMethod::LEVELLING));
    EXPECT_TRUE(vdatum->realizationMethod().has_value());
    EXPECT_EQ(*(vdatum->realizationMethod()), RealizationMethod::LEVELLING);

    auto expected = "VDATUM[\"Ordnance Datum Newlyn\",\n"
                    "    ANCHOR[\"my anchor\"],\n"
                    "    ID[\"EPSG\",5101]]";

    EXPECT_EQ(vdatum->exportToWKT(WKTFormatter::create().get()), expected);

    EXPECT_TRUE(vdatum->isEquivalentTo(vdatum.get()));
    EXPECT_FALSE(vdatum->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

static CompoundCRSNNPtr createCompoundCRS() {
    PropertyMap properties;
    properties.set(Identifier::CODESPACE_KEY, "codespace")
        .set(Identifier::CODE_KEY, "code")
        .set(IdentifiedObject::NAME_KEY, "horizontal + vertical");
    return CompoundCRS::create(
        properties,
        std::vector<CRSNNPtr>{createProjected(), createVerticalCRS()});
}

// ---------------------------------------------------------------------------

TEST(crs, compoundCRS_as_WKT2) {
    auto crs = createCompoundCRS();
    auto expected =
        "COMPOUNDCRS[\"horizontal + vertical\",\n"
        "    PROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "        BASEGEODCRS[\"WGS 84\",\n"
        "            DATUM[\"World Geodetic System 1984\",\n"
        "                ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                    LENGTHUNIT[\"metre\",1]]],\n"
        "            PRIMEM[\"Greenwich\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
        "        CONVERSION[\"UTM zone 31N\",\n"
        "            METHOD[\"Transverse Mercator\",\n"
        "                ID[\"EPSG\",9807]],\n"
        "            PARAMETER[\"Latitude of natural origin\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "                ID[\"EPSG\",8801]],\n"
        "            PARAMETER[\"Longitude of natural origin\",3,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "                ID[\"EPSG\",8802]],\n"
        "            PARAMETER[\"Scale factor at natural origin\",0.9996,\n"
        "                SCALEUNIT[\"unity\",1],\n"
        "                ID[\"EPSG\",8805]],\n"
        "            PARAMETER[\"False easting\",500000,\n"
        "                LENGTHUNIT[\"metre\",1],\n"
        "                ID[\"EPSG\",8806]],\n"
        "            PARAMETER[\"False northing\",0,\n"
        "                LENGTHUNIT[\"metre\",1],\n"
        "                ID[\"EPSG\",8807]]],\n"
        "        CS[Cartesian,2],\n"
        "            AXIS[\"(E)\",east,\n"
        "                ORDER[1],\n"
        "                LENGTHUNIT[\"metre\",1]],\n"
        "            AXIS[\"(N)\",north,\n"
        "                ORDER[2],\n"
        "                LENGTHUNIT[\"metre\",1]]],\n"
        "    VERTCRS[\"ODN height\",\n"
        "        VDATUM[\"Ordnance Datum Newlyn\"],\n"
        "        CS[vertical,1],\n"
        "            AXIS[\"gravity-related height (H)\",up,\n"
        "                LENGTHUNIT[\"metre\",1]]],\n"
        "    ID[\"codespace\",\"code\"]]";

    EXPECT_EQ(crs->exportToWKT(WKTFormatter::create().get()), expected);
}

// ---------------------------------------------------------------------------

TEST(crs, compoundCRS_isEquivalentTo) {

    auto crs = createCompoundCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
    auto compoundCRSOfProjCRS =
        CompoundCRS::create(PropertyMap().set(IdentifiedObject::NAME_KEY, ""),
                            std::vector<CRSNNPtr>{createProjected()});
    auto emptyCompoundCRS =
        CompoundCRS::create(PropertyMap().set(IdentifiedObject::NAME_KEY, ""),
                            std::vector<CRSNNPtr>{});
    EXPECT_FALSE(compoundCRSOfProjCRS->isEquivalentTo(emptyCompoundCRS.get()));
    auto compoundCRSOfGeogCRS =
        CompoundCRS::create(PropertyMap().set(IdentifiedObject::NAME_KEY, ""),
                            std::vector<CRSNNPtr>{GeographicCRS::EPSG_4326});
    EXPECT_FALSE(
        compoundCRSOfProjCRS->isEquivalentTo(compoundCRSOfGeogCRS.get()));
}

// ---------------------------------------------------------------------------

TEST(crs, compoundCRS_as_WKT1_GDAL) {
    auto crs = createCompoundCRS();
    auto expected =
        "COMPD_CS[\"horizontal + vertical\",\n"
        "    PROJCS[\"WGS 84 / UTM zone 31N\",\n"
        "        GEOGCS[\"WGS 84\",\n"
        "            DATUM[\"WGS_1984\",\n"
        "                SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
        "                    AUTHORITY[\"EPSG\",\"7030\"]],\n"
        "                AUTHORITY[\"EPSG\",\"6326\"]],\n"
        "            PRIMEM[\"Greenwich\",0,\n"
        "                AUTHORITY[\"EPSG\",\"8901\"]],\n"
        "            UNIT[\"degree\",0.0174532925199433,\n"
        "                AUTHORITY[\"EPSG\",\"9122\"]],\n"
        "            AXIS[\"Latitude\",NORTH],\n"
        "            AXIS[\"Longitude\",EAST],\n"
        "            AUTHORITY[\"EPSG\",\"4326\"]],\n"
        "        PROJECTION[\"Transverse_Mercator\"],\n"
        "        PARAMETER[\"latitude_of_origin\",0],\n"
        "        PARAMETER[\"central_meridian\",3],\n"
        "        PARAMETER[\"scale_factor\",0.9996],\n"
        "        PARAMETER[\"false_easting\",500000],\n"
        "        PARAMETER[\"false_northing\",0],\n"
        "        UNIT[\"metre\",1,\n"
        "            AUTHORITY[\"EPSG\",\"9001\"]],\n"
        "        AXIS[\"Easting\",EAST],\n"
        "        AXIS[\"Northing\",NORTH],\n"
        "        AUTHORITY[\"EPSG\",\"32631\"]],\n"
        "    VERT_CS[\"ODN height\",\n"
        "        VERT_DATUM[\"Ordnance Datum Newlyn\",2005,\n"
        "            AUTHORITY[\"EPSG\",\"5101\"]],\n"
        "        UNIT[\"metre\",1,\n"
        "            AUTHORITY[\"EPSG\",\"9001\"]],\n"
        "        AXIS[\"Gravity-related height\",UP],\n"
        "        AUTHORITY[\"EPSG\",\"5701\"]],\n"
        "    AUTHORITY[\"codespace\",\"code\"]]";

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, compoundCRS_as_PROJ_string) {
    auto crs = createCompoundCRS();
    auto expected = "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
                    "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=utm "
                    "+zone=31 +ellps=WGS84 +vunits=m";

    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              expected);
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=utm +zone=31 +datum=WGS84 +vunits=m");
}

// ---------------------------------------------------------------------------

TEST(crs, compoundCRS_no_name_provided) {
    auto crs = CompoundCRS::create(
        PropertyMap(),
        std::vector<CRSNNPtr>{createProjected(), createVerticalCRS()});
    EXPECT_EQ(crs->nameStr(), "WGS 84 / UTM zone 31N + ODN height");
}

// ---------------------------------------------------------------------------

TEST(crs, boundCRS_to_WKT2) {

    auto projcrs = ProjectedCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "my PROJCRS"),
        GeographicCRS::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "my GEOGCRS"),
            GeodeticReferenceFrame::EPSG_6326,
            EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE)),
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto crs = BoundCRS::createFromTOWGS84(
        projcrs, std::vector<double>{1, 2, 3, 4, 5, 6, 7});

    EXPECT_EQ(crs->baseCRS()->nameStr(), projcrs->nameStr());

    EXPECT_EQ(crs->hubCRS()->nameStr(), GeographicCRS::EPSG_4326->nameStr());

    ASSERT_TRUE(crs->transformation()->sourceCRS() != nullptr);
    EXPECT_EQ(crs->transformation()->sourceCRS()->nameStr(),
              projcrs->baseCRS()->nameStr());

    ASSERT_TRUE(crs->transformation()->targetCRS() != nullptr);
    EXPECT_EQ(crs->transformation()->targetCRS()->nameStr(),
              GeographicCRS::EPSG_4326->nameStr());

    auto values = crs->transformation()->parameterValues();
    ASSERT_EQ(values.size(), 7);
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[0]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8605));
        EXPECT_EQ(paramName, "X-axis translation");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::METRE);
        EXPECT_EQ(measure.value(), 1.0);
    }
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[1]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8606));
        EXPECT_EQ(paramName, "Y-axis translation");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::METRE);
        EXPECT_EQ(measure.value(), 2.0);
    }
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[2]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8607));
        EXPECT_EQ(paramName, "Z-axis translation");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::METRE);
        EXPECT_EQ(measure.value(), 3.0);
    }
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[3]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8608));
        EXPECT_EQ(paramName, "X-axis rotation");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::ARC_SECOND);
        EXPECT_EQ(measure.value(), 4.0);
    }
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[4]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8609));
        EXPECT_EQ(paramName, "Y-axis rotation");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::ARC_SECOND);
        EXPECT_EQ(measure.value(), 5.0);
    }
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[5]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8610));
        EXPECT_EQ(paramName, "Z-axis rotation");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::ARC_SECOND);
        EXPECT_EQ(measure.value(), 6.0);
    }
    {
        const auto &opParamvalue =
            nn_dynamic_pointer_cast<OperationParameterValue>(values[6]);
        ASSERT_TRUE(opParamvalue);
        const auto &paramName = opParamvalue->parameter()->nameStr();
        const auto &parameterValue = opParamvalue->parameterValue();
        EXPECT_TRUE(opParamvalue->parameter()->isEPSG(8611));
        EXPECT_EQ(paramName, "Scale difference");
        EXPECT_EQ(parameterValue->type(), ParameterValue::Type::MEASURE);
        auto measure = parameterValue->value();
        EXPECT_EQ(measure.unit(), UnitOfMeasure::PARTS_PER_MILLION);
        EXPECT_EQ(measure.value(), 7.0);
    }

    auto expected =
        "BOUNDCRS[SOURCECRS[" +
        projcrs->exportToWKT(WKTFormatter::create().get()) + "],\n" +
        "TARGETCRS[" +
        GeographicCRS::EPSG_4326->exportToWKT(WKTFormatter::create().get()) +
        "],\n"
        "    ABRIDGEDTRANSFORMATION[\"Transformation from myGEOGCRS to "
        "WGS84\",\n"
        "        METHOD[\"Position Vector transformation (geog2D "
        "domain)\",\n"
        "            ID[\"EPSG\",9606]],\n"
        "        PARAMETER[\"X-axis translation\",1,\n"
        "            ID[\"EPSG\",8605]],\n"
        "        PARAMETER[\"Y-axis translation\",2,\n"
        "            ID[\"EPSG\",8606]],\n"
        "        PARAMETER[\"Z-axis translation\",3,\n"
        "            ID[\"EPSG\",8607]],\n"
        "        PARAMETER[\"X-axis rotation\",4,\n"
        "            ID[\"EPSG\",8608]],\n"
        "        PARAMETER[\"Y-axis rotation\",5,\n"
        "            ID[\"EPSG\",8609]],\n"
        "        PARAMETER[\"Z-axis rotation\",6,\n"
        "            ID[\"EPSG\",8610]],\n"
        "        PARAMETER[\"Scale difference\",1.000007,\n"
        "            ID[\"EPSG\",8611]]]]";

    EXPECT_EQ(
        replaceAll(
            replaceAll(crs->exportToWKT(WKTFormatter::create().get()), " ", ""),
            "\n", ""),
        replaceAll(replaceAll(expected, " ", ""), "\n", ""));

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, boundCRS_crs_link) {

    {
        std::weak_ptr<CRS> oriBaseCRS;
        {
            auto baseCRSIn = GeographicCRS::EPSG_4267->shallowClone();
            oriBaseCRS = baseCRSIn.as_nullable();
            EXPECT_EQ(oriBaseCRS.use_count(), 1);
            {
                auto boundCRS = BoundCRS::createFromTOWGS84(
                    baseCRSIn, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
                EXPECT_EQ(oriBaseCRS.use_count(), 3);
            }
            EXPECT_EQ(oriBaseCRS.use_count(), 1);
        }
        EXPECT_TRUE(oriBaseCRS.expired());
    }

    {
        CRSPtr baseCRS;
        {
            auto baseCRSIn = GeographicCRS::EPSG_4267->shallowClone();
            CRS *baseCRSPtr = baseCRSIn.get();
            auto boundCRS = BoundCRS::createFromTOWGS84(
                baseCRSIn, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
            baseCRS = boundCRS->baseCRS().as_nullable();
            EXPECT_TRUE(baseCRS.get() == baseCRSPtr);
        }
        EXPECT_TRUE(baseCRS->isEquivalentTo(GeographicCRS::EPSG_4267.get()));
        EXPECT_TRUE(baseCRS->canonicalBoundCRS() == nullptr);
    }

    {
        CRSPtr baseCRS;
        {
            auto boundCRS = BoundCRS::createFromTOWGS84(
                GeographicCRS::EPSG_4267->shallowClone(),
                std::vector<double>{1, 2, 3, 4, 5, 6, 7});
            baseCRS = boundCRS->baseCRSWithCanonicalBoundCRS().as_nullable();
        }
        EXPECT_TRUE(baseCRS->isEquivalentTo(GeographicCRS::EPSG_4267.get()));
        EXPECT_TRUE(baseCRS->canonicalBoundCRS() != nullptr);

        EXPECT_TRUE(
            baseCRS->createBoundCRSToWGS84IfPossible(nullptr)->isEquivalentTo(
                baseCRS->canonicalBoundCRS().get()));
    }

    {
        std::weak_ptr<CRS> oriBaseCRS;
        {
            BoundCRSPtr boundCRSExterior;
            {
                auto baseCRS = GeographicCRS::EPSG_4267->shallowClone();
                oriBaseCRS = baseCRS.as_nullable();
                EXPECT_EQ(oriBaseCRS.use_count(), 1);
                auto boundCRS = BoundCRS::createFromTOWGS84(
                    baseCRS, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
                EXPECT_EQ(oriBaseCRS.use_count(), 3);
                boundCRSExterior = boundCRS->baseCRSWithCanonicalBoundCRS()
                                       ->canonicalBoundCRS();
                EXPECT_EQ(oriBaseCRS.use_count(), 4);
            }
            EXPECT_EQ(oriBaseCRS.use_count(), 2);
            EXPECT_TRUE(!oriBaseCRS.expired());
            EXPECT_TRUE(boundCRSExterior->baseCRS()->isEquivalentTo(
                GeographicCRS::EPSG_4267.get()));
        }
        EXPECT_EQ(oriBaseCRS.use_count(), 0);
        EXPECT_TRUE(oriBaseCRS.expired());
    }

    {
        std::weak_ptr<CRS> oriBaseCRS;
        {
            BoundCRSPtr boundCRSExterior;
            {
                auto baseCRS = createProjected();
                oriBaseCRS = baseCRS.as_nullable();
                EXPECT_EQ(oriBaseCRS.use_count(), 1);
                auto boundCRS = BoundCRS::createFromTOWGS84(
                    baseCRS, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
                EXPECT_EQ(oriBaseCRS.use_count(), 2);
                boundCRSExterior = boundCRS->baseCRSWithCanonicalBoundCRS()
                                       ->canonicalBoundCRS();
                EXPECT_EQ(oriBaseCRS.use_count(), 3);
            }
            EXPECT_EQ(oriBaseCRS.use_count(), 1);
            EXPECT_TRUE(!oriBaseCRS.expired());
            EXPECT_TRUE(boundCRSExterior->baseCRS()->isEquivalentTo(
                createProjected().get()));
        }
        EXPECT_EQ(oriBaseCRS.use_count(), 0);
        EXPECT_TRUE(oriBaseCRS.expired());
    }
}

// ---------------------------------------------------------------------------

TEST(crs, boundCRS_to_WKT1) {

    auto projcrs = ProjectedCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "my PROJCRS"),
        GeographicCRS::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "my GEOGCRS"),
            GeodeticReferenceFrame::EPSG_6326,
            EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE)),
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto crs = BoundCRS::createFromTOWGS84(
        projcrs, std::vector<double>{1, 2, 3, 4, 5, 6, 7});
    auto expected = "PROJCS[\"my PROJCRS\",\n"
                    "    GEOGCS[\"my GEOGCRS\",\n"
                    "        DATUM[\"WGS_1984\",\n"
                    "            SPHEROID[\"WGS 84\",6378137,298.257223563,\n"
                    "                AUTHORITY[\"EPSG\",\"7030\"]],\n"
                    "            TOWGS84[1,2,3,4,5,6,7],\n"
                    "            AUTHORITY[\"EPSG\",\"6326\"]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            AUTHORITY[\"EPSG\",\"8901\"]],\n"
                    "        UNIT[\"degree\",0.0174532925199433,\n"
                    "            AUTHORITY[\"EPSG\",\"9122\"]],\n"
                    "        AXIS[\"Latitude\",NORTH],\n"
                    "        AXIS[\"Longitude\",EAST]],\n"
                    "    PROJECTION[\"Transverse_Mercator\"],\n"
                    "    PARAMETER[\"latitude_of_origin\",0],\n"
                    "    PARAMETER[\"central_meridian\",3],\n"
                    "    PARAMETER[\"scale_factor\",0.9996],\n"
                    "    PARAMETER[\"false_easting\",500000],\n"
                    "    PARAMETER[\"false_northing\",0],\n"
                    "    UNIT[\"metre\",1,\n"
                    "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
                    "    AXIS[\"Easting\",EAST],\n"
                    "    AXIS[\"Northing\",NORTH]]";

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, boundCRS_geographicCRS_to_PROJ_string) {

    auto basecrs = GeographicCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "my GEOGCRS"),
        GeodeticReferenceFrame::EPSG_6326,
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));

    auto crs = BoundCRS::createFromTOWGS84(
        basecrs, std::vector<double>{1, 2, 3, 4, 5, 6, 7});

    EXPECT_THROW(crs->exportToPROJString(PROJStringFormatter::create().get()),
                 FormattingException);
    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=longlat +ellps=WGS84 +towgs84=1,2,3,4,5,6,7");
}

// ---------------------------------------------------------------------------

TEST(crs, boundCRS_projectedCRS_to_PROJ_string) {

    auto projcrs = ProjectedCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "my PROJCRS"),
        GeographicCRS::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "my GEOGCRS"),
            GeodeticReferenceFrame::EPSG_6326,
            EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE)),
        Conversion::createUTM(PropertyMap(), 31, true),
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));

    auto crs = BoundCRS::createFromTOWGS84(
        projcrs, std::vector<double>{1, 2, 3, 4, 5, 6, 7});

    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=utm +zone=31 +ellps=WGS84 +towgs84=1,2,3,4,5,6,7");
}

// ---------------------------------------------------------------------------

TEST(crs, incompatible_boundCRS_hubCRS_to_WKT1) {

    auto crs = BoundCRS::create(
        GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4807,
        Transformation::createGeocentricTranslations(
            PropertyMap(), GeographicCRS::EPSG_4326, GeographicCRS::EPSG_4807,
            1.0, 2.0, 3.0, std::vector<PositionalAccuracyNNPtr>()));

    EXPECT_THROW(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, incompatible_boundCRS_transformation_to_WKT1) {

    auto crs = BoundCRS::create(
        GeographicCRS::EPSG_4807, GeographicCRS::EPSG_4326,
        Transformation::create(PropertyMap(), GeographicCRS::EPSG_4807,
                               GeographicCRS::EPSG_4326, nullptr, PropertyMap(),
                               std::vector<OperationParameterNNPtr>(),
                               std::vector<ParameterValueNNPtr>(),
                               std::vector<PositionalAccuracyNNPtr>()));

    EXPECT_THROW(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, WKT1_DATUM_EXTENSION_to_WKT1_and_PROJ_string) {
    auto wkt =
        "PROJCS[\"unnamed\",\n"
        "    GEOGCS[\"International 1909 (Hayford)\",\n"
        "        DATUM[\"unknown\",\n"
        "            SPHEROID[\"intl\",6378388,297],\n"
        "            EXTENSION[\"PROJ4_GRIDS\",\"nzgd2kgrid0005.gsb\"]],\n"
        "        PRIMEM[\"Greenwich\",0],\n"
        "        UNIT[\"degree\",0.0174532925199433],\n"
        "        AXIS[\"Longitude\",EAST],\n"
        "        AXIS[\"Latitude\",NORTH]],\n"
        "    PROJECTION[\"New_Zealand_Map_Grid\"],\n"
        "    PARAMETER[\"latitude_of_origin\",-41],\n"
        "    PARAMETER[\"central_meridian\",173],\n"
        "    PARAMETER[\"false_easting\",2510000],\n"
        "    PARAMETER[\"false_northing\",6023150],\n"
        "    UNIT[\"Meter\",1],\n"
        "    AXIS[\"Easting\",EAST],\n"
        "    AXIS[\"Northing\",NORTH]]";

    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<BoundCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        wkt);

    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+proj=nzmg +lat_0=-41 +lon_0=173 +x_0=2510000 +y_0=6023150 "
        "+ellps=intl +nadgrids=nzgd2kgrid0005.gsb +units=m");
}

// ---------------------------------------------------------------------------

TEST(crs, WKT1_VERT_DATUM_EXTENSION_to_WKT1) {
    auto wkt = "VERT_CS[\"EGM2008 geoid height\",\n"
               "    VERT_DATUM[\"EGM2008 geoid\",2005,\n"
               "        EXTENSION[\"PROJ4_GRIDS\",\"egm08_25.gtx\"],\n"
               "        AUTHORITY[\"EPSG\",\"1027\"]],\n"
               "    UNIT[\"metre\",1,\n"
               "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
               "    AXIS[\"Up\",UP],\n"
               "    AUTHORITY[\"EPSG\",\"3855\"]]";

    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<BoundCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        wkt);
}

// ---------------------------------------------------------------------------

TEST(crs, WKT1_VERT_DATUM_EXTENSION_to_WKT2) {
    auto wkt = "VERT_CS[\"EGM2008 geoid height\",\n"
               "    VERT_DATUM[\"EGM2008 geoid\",2005,\n"
               "        EXTENSION[\"PROJ4_GRIDS\",\"egm08_25.gtx\"],\n"
               "        AUTHORITY[\"EPSG\",\"1027\"]],\n"
               "    UNIT[\"metre\",1,\n"
               "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
               "    AXIS[\"Up\",UP],\n"
               "    AUTHORITY[\"EPSG\",\"3855\"]]";

    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<BoundCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    auto wkt2 =
        "BOUNDCRS[\n"
        "    SOURCECRS[\n"
        "        VERTCRS[\"EGM2008 geoid height\",\n"
        "            VDATUM[\"EGM2008 geoid\"],\n"
        "            CS[vertical,1],\n"
        "                AXIS[\"up\",up,\n"
        "                    LENGTHUNIT[\"metre\",1]],\n"
        "            ID[\"EPSG\",3855]]],\n"
        "    TARGETCRS[\n"
        "        GEODCRS[\"WGS 84\",\n"
        "            DATUM[\"World Geodetic System 1984\",\n"
        "                ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                    LENGTHUNIT[\"metre\",1]]],\n"
        "            PRIMEM[\"Greenwich\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "            CS[ellipsoidal,3],\n"
        "                AXIS[\"latitude\",north,\n"
        "                    ORDER[1],\n"
        "                    ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "                AXIS[\"longitude\",east,\n"
        "                    ORDER[2],\n"
        "                    ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
        "                AXIS[\"ellipsoidal height\",up,\n"
        "                    ORDER[3],\n"
        "                    LENGTHUNIT[\"metre\",1]],\n"
        "            ID[\"EPSG\",4979]]],\n"
        "    ABRIDGEDTRANSFORMATION[\"EGM2008 geoid height to WGS84 "
        "ellipsoidal height\",\n"
        "        METHOD[\"GravityRelatedHeight to Geographic3D\"],\n"
        "        PARAMETERFILE[\"Geoid (height correction) model "
        "file\",\"egm08_25.gtx\",\n"
        "            ID[\"EPSG\",8666]]]]";

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              wkt2);
}

// ---------------------------------------------------------------------------

TEST(crs, WKT1_VERT_DATUM_EXTENSION_to_PROJ_string) {
    auto wkt = "VERT_CS[\"EGM2008 geoid height\",\n"
               "    VERT_DATUM[\"EGM2008 geoid\",2005,\n"
               "        EXTENSION[\"PROJ4_GRIDS\",\"egm08_25.gtx\"],\n"
               "        AUTHORITY[\"EPSG\",\"1027\"]],\n"
               "    UNIT[\"metre\",1,\n"
               "        AUTHORITY[\"EPSG\",\"9001\"]],\n"
               "    AXIS[\"Up\",UP],\n"
               "    AUTHORITY[\"EPSG\",\"3855\"]]";

    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<BoundCRS>(obj);
    ASSERT_TRUE(crs != nullptr);

    EXPECT_EQ(
        crs->exportToPROJString(
            PROJStringFormatter::create(PROJStringFormatter::Convention::PROJ_4)
                .get()),
        "+geoidgrids=egm08_25.gtx +vunits=m");
}

// ---------------------------------------------------------------------------

TEST(crs, extractGeographicCRS) {
    EXPECT_EQ(GeographicCRS::EPSG_4326->extractGeographicCRS(),
              GeographicCRS::EPSG_4326);
    EXPECT_EQ(createProjected()->extractGeographicCRS(),
              GeographicCRS::EPSG_4326);
    EXPECT_EQ(
        CompoundCRS::create(PropertyMap(),
                            std::vector<CRSNNPtr>{GeographicCRS::EPSG_4326})
            ->extractGeographicCRS(),
        GeographicCRS::EPSG_4326);
}

// ---------------------------------------------------------------------------

TEST(crs, extractVerticalCRS) {
    EXPECT_EQ(GeographicCRS::EPSG_4326->extractVerticalCRS(), nullptr);
    {
        auto vertcrs = createCompoundCRS()->extractVerticalCRS();
        ASSERT_TRUE(vertcrs != nullptr);
        EXPECT_TRUE(vertcrs->isEquivalentTo(createVerticalCRS().get()));
    }
}

// ---------------------------------------------------------------------------

static DerivedGeographicCRSNNPtr createDerivedGeographicCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Atlantic pole"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Pole rotation"),
        std::vector<OperationParameterNNPtr>{
            OperationParameter::create(PropertyMap().set(
                IdentifiedObject::NAME_KEY, "Latitude of rotated pole")),
            OperationParameter::create(PropertyMap().set(
                IdentifiedObject::NAME_KEY, "Longitude of rotated pole")),
            OperationParameter::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "Axis rotation")),
        },
        std::vector<ParameterValueNNPtr>{
            ParameterValue::create(Angle(52.0)),
            ParameterValue::create(Angle(-30.0)),
            ParameterValue::create(Angle(-25.0)),
        });

    return DerivedGeographicCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "WMO Atlantic Pole"),
        GeographicCRS::EPSG_4326, derivingConversion,
        EllipsoidalCS::createLatitudeLongitude(UnitOfMeasure::DEGREE));
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeographicCRS_WKT2) {

    auto expected = "GEODCRS[\"WMO Atlantic Pole\",\n"
                    "    BASEGEODCRS[\"WGS 84\",\n"
                    "        DATUM[\"World Geodetic System 1984\",\n"
                    "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "                LENGTHUNIT[\"metre\",1]]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
                    "    DERIVINGCONVERSION[\"Atlantic pole\",\n"
                    "        METHOD[\"Pole rotation\"],\n"
                    "        PARAMETER[\"Latitude of rotated pole\",52,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        PARAMETER[\"Longitude of rotated pole\",-30,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        PARAMETER[\"Axis rotation\",-25,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]]],\n"
                    "    CS[ellipsoidal,2],\n"
                    "        AXIS[\"latitude\",north,\n"
                    "            ORDER[1],\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        AXIS[\"longitude\",east,\n"
                    "            ORDER[2],\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]]]";

    auto crs = createDerivedGeographicCRS();
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              expected);

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeographicCRS_WKT2_2018) {

    auto expected = "GEOGCRS[\"WMO Atlantic Pole\",\n"
                    "    BASEGEOGCRS[\"WGS 84\",\n"
                    "        DATUM[\"World Geodetic System 1984\",\n"
                    "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "                LENGTHUNIT[\"metre\",1]]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
                    "    DERIVINGCONVERSION[\"Atlantic pole\",\n"
                    "        METHOD[\"Pole rotation\"],\n"
                    "        PARAMETER[\"Latitude of rotated pole\",52,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        PARAMETER[\"Longitude of rotated pole\",-30,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        PARAMETER[\"Axis rotation\",-25,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]]],\n"
                    "    CS[ellipsoidal,2],\n"
                    "        AXIS[\"latitude\",north,\n"
                    "            ORDER[1],\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]],\n"
                    "        AXIS[\"longitude\",east,\n"
                    "            ORDER[2],\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433,\n"
                    "                ID[\"EPSG\",9122]]]]";

    EXPECT_EQ(
        createDerivedGeographicCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeographicCRS_WKT1) {

    EXPECT_THROW(
        createDerivedGeographicCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeographicCRS_to_PROJ) {

    auto wkt = "GEODCRS[\"WMO Atlantic Pole\",\n"
               "    BASEGEODCRS[\"WGS 84\",\n"
               "        DATUM[\"World Geodetic System 1984\",\n"
               "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
               "                LENGTHUNIT[\"metre\",1]]],\n"
               "        PRIMEM[\"Greenwich\",0,\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
               "    DERIVINGCONVERSION[\"unnamed\",\n"
               "        METHOD[\"PROJ ob_tran o_proj=longlat\"],\n"
               "        PARAMETER[\"o_lat_p\",52],\n"
               "        PARAMETER[\"o_lon_p\",-30],\n"
               "        PARAMETER[\"lon_0\",-25]],\n"
               "    CS[ellipsoidal,2],\n"
               "        AXIS[\"latitude\",north,\n"
               "            ORDER[1],\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433]],\n"
               "        AXIS[\"longitude\",east,\n"
               "            ORDER[2],\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433]]]";

    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<DerivedGeographicCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(
        crs->exportToPROJString(PROJStringFormatter::create().get()),
        "+proj=pipeline +step +proj=axisswap +order=2,1 +step "
        "+proj=unitconvert +xy_in=deg +xy_out=rad +step +proj=ob_tran "
        "+o_proj=longlat +o_lat_p=52 +o_lon_p=-30 +lon_0=-25 +ellps=WGS84 "
        "+step +proj=unitconvert +xy_in=rad +xy_out=deg +step "
        "+proj=axisswap +order=2,1");
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeographicCRS_with_affine_transform_to_PROJ) {

    auto wkt = "GEODCRS[\"WGS 84 Translated\",\n"
               "    BASEGEODCRS[\"WGS 84\",\n"
               "        DATUM[\"World Geodetic System 1984\",\n"
               "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
               "                LENGTHUNIT[\"metre\",1]]],\n"
               "        PRIMEM[\"Greenwich\",0]],\n"
               "    DERIVINGCONVERSION[\"Translation\",\n"
               "        METHOD[\"Affine parametric transformation\",\n"
               "            ID[\"EPSG\",9624]],\n"
               "        PARAMETER[\"A0\",0.5,\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
               "            ID[\"EPSG\",8623]],\n"
               "        PARAMETER[\"A1\",1,\n"
               "            SCALEUNIT[\"coefficient\",1],\n"
               "            ID[\"EPSG\",8624]],\n"
               "        PARAMETER[\"A2\",0,\n"
               "            SCALEUNIT[\"coefficient\",1],\n"
               "            ID[\"EPSG\",8625]],\n"
               "        PARAMETER[\"B0\",2.5,\n"
               "            ANGLEUNIT[\"degree\",0.0174532925199433],\n"
               "            ID[\"EPSG\",8639]],\n"
               "        PARAMETER[\"B1\",0,\n"
               "            SCALEUNIT[\"coefficient\",1],\n"
               "            ID[\"EPSG\",8640]],\n"
               "        PARAMETER[\"B2\",1,\n"
               "            SCALEUNIT[\"coefficient\",1],\n"
               "            ID[\"EPSG\",8641]]],\n"
               "    CS[ellipsoidal,2],\n"
               "        AXIS[\"latitude\",north],\n"
               "        AXIS[\"longitude\",east],\n"
               "        ANGLEUNIT[\"degree\",0.0174532925199433]]";

    auto obj = WKTParser().createFromWKT(wkt);
    auto crs = nn_dynamic_pointer_cast<DerivedGeographicCRS>(obj);
    ASSERT_TRUE(crs != nullptr);
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=affine +xoff=0.5 +s11=1 +s12=0 +yoff=2.5 +s21=0 +s22=1");
}

// ---------------------------------------------------------------------------

static DerivedGeodeticCRSNNPtr createDerivedGeodeticCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Some conversion"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Some method"),
        std::vector<OperationParameterNNPtr>{},
        std::vector<ParameterValueNNPtr>{});

    return DerivedGeodeticCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Derived geodetic CRS"),
        GeographicCRS::EPSG_4326, derivingConversion,
        CartesianCS::createGeocentric(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeodeticCRS_WKT2) {

    auto expected = "GEODCRS[\"Derived geodetic CRS\",\n"
                    "    BASEGEODCRS[\"WGS 84\",\n"
                    "        DATUM[\"World Geodetic System 1984\",\n"
                    "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "                LENGTHUNIT[\"metre\",1]]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
                    "    DERIVINGCONVERSION[\"Some conversion\",\n"
                    "        METHOD[\"Some method\"]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(Y)\",geocentricY,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(Z)\",geocentricZ,\n"
                    "            ORDER[3],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]]]";

    auto crs = createDerivedGeodeticCRS();
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              expected);

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeodeticCRS_WKT2_2018) {

    auto expected = "GEODCRS[\"Derived geodetic CRS\",\n"
                    "    BASEGEOGCRS[\"WGS 84\",\n"
                    "        DATUM[\"World Geodetic System 1984\",\n"
                    "            ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
                    "                LENGTHUNIT[\"metre\",1]]],\n"
                    "        PRIMEM[\"Greenwich\",0,\n"
                    "            ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
                    "    DERIVINGCONVERSION[\"Some conversion\",\n"
                    "        METHOD[\"Some method\"]],\n"
                    "    CS[Cartesian,3],\n"
                    "        AXIS[\"(X)\",geocentricX,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(Y)\",geocentricY,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(Z)\",geocentricZ,\n"
                    "            ORDER[3],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]]]";

    EXPECT_EQ(
        createDerivedGeodeticCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, derivedGeodeticCRS_WKT1) {

    EXPECT_THROW(
        createDerivedGeodeticCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

static DerivedProjectedCRSNNPtr createDerivedProjectedCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "PROJ unimplemented"),
        std::vector<OperationParameterNNPtr>{},
        std::vector<ParameterValueNNPtr>{});

    return DerivedProjectedCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "derived projectedCRS"),
        createProjected(), derivingConversion,
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, derivedProjectedCRS_WKT2_2018) {

    auto expected =
        "DERIVEDPROJCRS[\"derived projectedCRS\",\n"
        "    BASEPROJCRS[\"WGS 84 / UTM zone 31N\",\n"
        "        BASEGEOGCRS[\"WGS 84\",\n"
        "            DATUM[\"World Geodetic System 1984\",\n"
        "                ELLIPSOID[\"WGS 84\",6378137,298.257223563,\n"
        "                    LENGTHUNIT[\"metre\",1]]],\n"
        "            PRIMEM[\"Greenwich\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433]]],\n"
        "        CONVERSION[\"UTM zone 31N\",\n"
        "            METHOD[\"Transverse Mercator\",\n"
        "                ID[\"EPSG\",9807]],\n"
        "            PARAMETER[\"Latitude of natural origin\",0,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "                ID[\"EPSG\",8801]],\n"
        "            PARAMETER[\"Longitude of natural origin\",3,\n"
        "                ANGLEUNIT[\"degree\",0.0174532925199433],\n"
        "                ID[\"EPSG\",8802]],\n"
        "            PARAMETER[\"Scale factor at natural origin\",0.9996,\n"
        "                SCALEUNIT[\"unity\",1],\n"
        "                ID[\"EPSG\",8805]],\n"
        "            PARAMETER[\"False easting\",500000,\n"
        "                LENGTHUNIT[\"metre\",1],\n"
        "                ID[\"EPSG\",8806]],\n"
        "            PARAMETER[\"False northing\",0,\n"
        "                LENGTHUNIT[\"metre\",1],\n"
        "                ID[\"EPSG\",8807]]]],\n"
        "    DERIVINGCONVERSION[\"unnamed\",\n"
        "        METHOD[\"PROJ unimplemented\"]],\n"
        "    CS[Cartesian,2],\n"
        "        AXIS[\"(E)\",east,\n"
        "            ORDER[1],\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]]],\n"
        "        AXIS[\"(N)\",north,\n"
        "            ORDER[2],\n"
        "            LENGTHUNIT[\"metre\",1,\n"
        "                ID[\"EPSG\",9001]]]]";

    auto crs = createDerivedProjectedCRS();
    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_FALSE(crs->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, derivedProjectedCRS_WKT2_2015) {

    auto crs = createDerivedProjectedCRS();
    EXPECT_THROW(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2015).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, derivedProjectedCRS_to_PROJ) {

    auto crs = createDerivedProjectedCRS();
    EXPECT_EQ(crs->exportToPROJString(PROJStringFormatter::create().get()),
              "+proj=unimplemented");
}

// ---------------------------------------------------------------------------

static DateTimeTemporalCSNNPtr createDateTimeTemporalCS() {
    return DateTimeTemporalCS::create(
        PropertyMap(),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "Time"), "T",
            AxisDirection::FUTURE, UnitOfMeasure::NONE));
    ;
}

// ---------------------------------------------------------------------------

static TemporalCRSNNPtr createDateTimeTemporalCRS() {

    auto datum = TemporalDatum::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Gregorian calendar"),
        DateTime::create("0000-01-01"),
        TemporalDatum::CALENDAR_PROLEPTIC_GREGORIAN);

    return TemporalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Temporal CRS"), datum,
        createDateTimeTemporalCS());
}

// ---------------------------------------------------------------------------

TEST(crs, dateTimeTemporalCRS_WKT2) {

    auto expected = "TIMECRS[\"Temporal CRS\",\n"
                    "    TDATUM[\"Gregorian calendar\",\n"
                    "        TIMEORIGIN[0000-01-01]],\n"
                    "    CS[temporal,1],\n"
                    "        AXIS[\"time (T)\",future]]";

    auto crs = createDateTimeTemporalCRS();
    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              expected);

    EXPECT_THROW(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);

    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));
}

// ---------------------------------------------------------------------------

TEST(crs, dateTimeTemporalCRS_WKT2_2018) {

    auto expected = "TIMECRS[\"Temporal CRS\",\n"
                    "    TDATUM[\"Gregorian calendar\",\n"
                    "        CALENDAR[\"proleptic Gregorian\"],\n"
                    "        TIMEORIGIN[0000-01-01]],\n"
                    "    CS[TemporalDateTime,1],\n"
                    "        AXIS[\"time (T)\",future]]";

    EXPECT_EQ(
        createDateTimeTemporalCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

static TemporalCRSNNPtr createTemporalCountCRSWithConvFactor() {

    auto datum = TemporalDatum::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "GPS time origin"),
        DateTime::create("1980-01-01T00:00:00.0Z"),
        TemporalDatum::CALENDAR_PROLEPTIC_GREGORIAN);

    auto cs = TemporalCountCS::create(
        PropertyMap(),
        CoordinateSystemAxis::create(PropertyMap(), "T", AxisDirection::FUTURE,
                                     UnitOfMeasure("milliseconds (ms)", 0.001,
                                                   UnitOfMeasure::Type::TIME)));

    return TemporalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "GPS milliseconds"),
        datum, cs);
}

// ---------------------------------------------------------------------------

TEST(crs, temporalCountCRSWithConvFactor_WKT2_2018) {

    auto expected = "TIMECRS[\"GPS milliseconds\",\n"
                    "    TDATUM[\"GPS time origin\",\n"
                    "        CALENDAR[\"proleptic Gregorian\"],\n"
                    "        TIMEORIGIN[1980-01-01T00:00:00.0Z]],\n"
                    "    CS[TemporalCount,1],\n"
                    "        AXIS[\"(T)\",future,\n"
                    "            TIMEUNIT[\"milliseconds (ms)\",0.001]]]";

    EXPECT_EQ(
        createTemporalCountCRSWithConvFactor()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

static TemporalCRSNNPtr createTemporalCountCRSWithoutConvFactor() {

    auto datum = TemporalDatum::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "29 December 1979"),
        DateTime::create("1979-12-29T00"),
        TemporalDatum::CALENDAR_PROLEPTIC_GREGORIAN);

    auto cs = TemporalCountCS::create(
        PropertyMap(),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "Time"), "",
            AxisDirection::FUTURE,
            UnitOfMeasure("hour", 0, UnitOfMeasure::Type::TIME)));

    return TemporalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY,
                          "Calendar hours from 1979-12-29"),
        datum, cs);
}

// ---------------------------------------------------------------------------

TEST(crs, temporalCountCRSWithoutConvFactor_WKT2_2018) {

    auto expected = "TIMECRS[\"Calendar hours from 1979-12-29\",\n"
                    "    TDATUM[\"29 December 1979\",\n"
                    "        CALENDAR[\"proleptic Gregorian\"],\n"
                    "        TIMEORIGIN[1979-12-29T00]],\n"
                    "    CS[TemporalCount,1],\n"
                    "        AXIS[\"time\",future,\n"
                    "            TIMEUNIT[\"hour\"]]]";

    EXPECT_EQ(
        createTemporalCountCRSWithoutConvFactor()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

static TemporalCRSNNPtr createTemporalMeasureCRSWithoutConvFactor() {

    auto datum = TemporalDatum::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Common Era"),
        DateTime::create("0000"), TemporalDatum::CALENDAR_PROLEPTIC_GREGORIAN);

    auto cs = TemporalMeasureCS::create(
        PropertyMap(),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "Decimal years"), "a",
            AxisDirection::FUTURE,
            UnitOfMeasure("year", 0, UnitOfMeasure::Type::TIME)));

    return TemporalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Decimal Years CE"),
        datum, cs);
}

// ---------------------------------------------------------------------------

TEST(crs, temporalMeasureCRSWithoutConvFactor_WKT2_2018) {

    auto expected = "TIMECRS[\"Decimal Years CE\",\n"
                    "    TDATUM[\"Common Era\",\n"
                    "        CALENDAR[\"proleptic Gregorian\"],\n"
                    "        TIMEORIGIN[0000]],\n"
                    "    CS[TemporalMeasure,1],\n"
                    "        AXIS[\"decimal years (a)\",future,\n"
                    "            TIMEUNIT[\"year\"]]]";

    EXPECT_EQ(
        createTemporalMeasureCRSWithoutConvFactor()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

static EngineeringCRSNNPtr createEngineeringCRS() {

    auto datum = EngineeringDatum::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Engineering datum"));

    return EngineeringCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Engineering CRS"), datum,
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, engineeringCRS_WKT2) {

    auto expected = "ENGCRS[\"Engineering CRS\",\n"
                    "    EDATUM[\"Engineering datum\"],\n"
                    "    CS[Cartesian,2],\n"
                    "        AXIS[\"(E)\",east,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(N)\",north,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]]]";

    auto crs = createEngineeringCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, engineeringCRS_WKT1) {

    auto expected = "LOCAL_CS[\"Engineering CRS\",\n"
                    "    LOCAL_DATUM[\"Engineering datum\",32767],\n"
                    "    AXIS[\"Easting\",EAST],\n"
                    "    AXIS[\"Northing\",NORTH]]";
    EXPECT_EQ(
        createEngineeringCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        expected);
}

// ---------------------------------------------------------------------------

static ParametricCSNNPtr createParametricCS() {

    return ParametricCS::create(
        PropertyMap(),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, "pressure"), "hPa",
            AxisDirection::UP, UnitOfMeasure("HectoPascal", 100,
                                             UnitOfMeasure::Type::PARAMETRIC)));
}

// ---------------------------------------------------------------------------

static ParametricCRSNNPtr createParametricCRS() {

    auto datum = ParametricDatum::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Parametric datum"));

    return ParametricCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Parametric CRS"), datum,
        createParametricCS());
}

// ---------------------------------------------------------------------------

TEST(crs, parametricCRS_WKT2) {

    auto expected = "PARAMETRICCRS[\"Parametric CRS\",\n"
                    "    PDATUM[\"Parametric datum\"],\n"
                    "    CS[parametric,1],\n"
                    "        AXIS[\"pressure (hPa)\",up,\n"
                    "            PARAMETRICUNIT[\"HectoPascal\",100]]]";
    auto crs = createParametricCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, parametricCRS_WKT1) {

    EXPECT_THROW(
        createParametricCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

static DerivedVerticalCRSNNPtr createDerivedVerticalCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "PROJ unimplemented"),
        std::vector<OperationParameterNNPtr>{},
        std::vector<ParameterValueNNPtr>{});

    return DerivedVerticalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Derived vertCRS"),
        createVerticalCRS(), derivingConversion,
        VerticalCS::createGravityRelatedHeight(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, DerivedVerticalCRS_WKT2) {

    auto expected = "VERTCRS[\"Derived vertCRS\",\n"
                    "    BASEVERTCRS[\"ODN height\",\n"
                    "        VDATUM[\"Ordnance Datum Newlyn\"]],\n"
                    "    DERIVINGCONVERSION[\"unnamed\",\n"
                    "        METHOD[\"PROJ unimplemented\"]],\n"
                    "    CS[vertical,1],\n"
                    "        AXIS[\"gravity-related height (H)\",up,\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]]]";

    auto crs = createDerivedVerticalCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));

    EXPECT_EQ(crs->exportToWKT(
                  WKTFormatter::create(WKTFormatter::Convention::WKT2).get()),
              expected);
}

// ---------------------------------------------------------------------------

TEST(crs, DerivedVerticalCRS_WKT1) {

    EXPECT_THROW(
        createDerivedVerticalCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

static DerivedEngineeringCRSNNPtr createDerivedEngineeringCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "PROJ unimplemented"),
        std::vector<OperationParameterNNPtr>{},
        std::vector<ParameterValueNNPtr>{});

    return DerivedEngineeringCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Derived EngineeringCRS"),
        createEngineeringCRS(), derivingConversion,
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

TEST(crs, DerivedEngineeringCRS_WKT2) {

    auto expected = "ENGCRS[\"Derived EngineeringCRS\",\n"
                    "    BASEENGCRS[\"Engineering CRS\",\n"
                    "        EDATUM[\"Engineering datum\"]],\n"
                    "    DERIVINGCONVERSION[\"unnamed\",\n"
                    "        METHOD[\"PROJ unimplemented\"]],\n"
                    "    CS[Cartesian,2],\n"
                    "        AXIS[\"(E)\",east,\n"
                    "            ORDER[1],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]],\n"
                    "        AXIS[\"(N)\",north,\n"
                    "            ORDER[2],\n"
                    "            LENGTHUNIT[\"metre\",1,\n"
                    "                ID[\"EPSG\",9001]]]]";

    auto crs = createDerivedEngineeringCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_TRUE(crs->coordinateSystem()->isEquivalentTo(
        CartesianCS::createEastingNorthing(UnitOfMeasure::METRE).get()));
    EXPECT_TRUE(
        crs->datum()->isEquivalentTo(createEngineeringCRS()->datum().get()));

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
    EXPECT_THROW(
        createDerivedEngineeringCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2015).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, DerivedEngineeringCRS_WKT1) {

    EXPECT_THROW(
        createDerivedEngineeringCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

static DerivedParametricCRSNNPtr createDerivedParametricCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "PROJ unimplemented"),
        std::vector<OperationParameterNNPtr>{},
        std::vector<ParameterValueNNPtr>{});

    return DerivedParametricCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Derived ParametricCRS"),
        createParametricCRS(), derivingConversion, createParametricCS());
}

// ---------------------------------------------------------------------------

TEST(crs, DerivedParametricCRS_WKT2) {

    auto expected = "PARAMETRICCRS[\"Derived ParametricCRS\",\n"
                    "    BASEPARAMCRS[\"Parametric CRS\",\n"
                    "        PDATUM[\"Parametric datum\"]],\n"
                    "    DERIVINGCONVERSION[\"unnamed\",\n"
                    "        METHOD[\"PROJ unimplemented\"]],\n"
                    "    CS[parametric,1],\n"
                    "        AXIS[\"pressure (hPa)\",up,\n"
                    "            PARAMETRICUNIT[\"HectoPascal\",100]]]";

    auto crs = createDerivedParametricCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_TRUE(
        crs->coordinateSystem()->isEquivalentTo(createParametricCS().get()));
    EXPECT_TRUE(
        crs->datum()->isEquivalentTo(createParametricCRS()->datum().get()));

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, DerivedParametricCRS_WKT1) {

    EXPECT_THROW(
        createDerivedParametricCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

static DerivedTemporalCRSNNPtr createDerivedTemporalCRS() {

    auto derivingConversion = Conversion::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "unnamed"),
        PropertyMap().set(IdentifiedObject::NAME_KEY, "PROJ unimplemented"),
        std::vector<OperationParameterNNPtr>{},
        std::vector<ParameterValueNNPtr>{});

    return DerivedTemporalCRS::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Derived TemporalCRS"),
        createDateTimeTemporalCRS(), derivingConversion,
        createDateTimeTemporalCS());
}

// ---------------------------------------------------------------------------

TEST(crs, DeriveTemporalCRS_WKT2) {

    auto expected = "TIMECRS[\"Derived TemporalCRS\",\n"
                    "    BASETIMECRS[\"Temporal CRS\",\n"
                    "        TDATUM[\"Gregorian calendar\",\n"
                    "            CALENDAR[\"proleptic Gregorian\"],\n"
                    "            TIMEORIGIN[0000-01-01]]],\n"
                    "    DERIVINGCONVERSION[\"unnamed\",\n"
                    "        METHOD[\"PROJ unimplemented\"]],\n"
                    "    CS[TemporalDateTime,1],\n"
                    "        AXIS[\"time (T)\",future]]";

    auto crs = createDerivedTemporalCRS();
    EXPECT_TRUE(crs->isEquivalentTo(crs.get()));
    EXPECT_TRUE(crs->shallowClone()->isEquivalentTo(crs.get()));
    EXPECT_TRUE(!crs->isEquivalentTo(createUnrelatedObject().get()));
    EXPECT_TRUE(crs->coordinateSystem()->isEquivalentTo(
        createDateTimeTemporalCS().get()));
    EXPECT_TRUE(crs->datum()->isEquivalentTo(
        createDateTimeTemporalCRS()->datum().get()));

    EXPECT_EQ(
        crs->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT2_2018).get()),
        expected);
}

// ---------------------------------------------------------------------------

TEST(crs, DeriveTemporalCRS_WKT1) {

    EXPECT_THROW(
        createDerivedTemporalCRS()->exportToWKT(
            WKTFormatter::create(WKTFormatter::Convention::WKT1_GDAL).get()),
        FormattingException);
}

// ---------------------------------------------------------------------------

TEST(crs, crs_createBoundCRSToWGS84IfPossible) {
    auto dbContext = DatabaseContext::create();
    auto factory = AuthorityFactory::create(dbContext, "EPSG");
    {
        auto crs_4326 = factory->createCoordinateReferenceSystem("4326");
        EXPECT_EQ(crs_4326->createBoundCRSToWGS84IfPossible(dbContext),
                  crs_4326);
    }
    {
        auto crs_32631 = factory->createCoordinateReferenceSystem("32631");
        EXPECT_EQ(crs_32631->createBoundCRSToWGS84IfPossible(dbContext),
                  crs_32631);
    }
    {
        // Pulkovo 42 East Germany
        auto crs_5670 = factory->createCoordinateReferenceSystem("5670");
        EXPECT_EQ(crs_5670->createBoundCRSToWGS84IfPossible(dbContext),
                  crs_5670);
    }
    {
        // Pulkovo 42 Romania
        auto crs_3844 = factory->createCoordinateReferenceSystem("3844");
        auto bound = crs_3844->createBoundCRSToWGS84IfPossible(dbContext);
        EXPECT_NE(bound, crs_3844);
        EXPECT_EQ(bound->createBoundCRSToWGS84IfPossible(dbContext), bound);
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(bound);
        ASSERT_TRUE(boundCRS != nullptr);
        EXPECT_EQ(boundCRS->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_4)
                          .get()),
                  "+proj=sterea +lat_0=46 +lon_0=25 +k=0.99975 +x_0=500000 "
                  "+y_0=500000 +ellps=krass "
                  "+towgs84=2.329,-147.042,-92.08,-0.309,0.325,0.497,5.69");
    }
    {
        // Pulkovo 42 Poland
        auto crs_2171 = factory->createCoordinateReferenceSystem("2171");
        auto bound = crs_2171->createBoundCRSToWGS84IfPossible(dbContext);
        EXPECT_NE(bound, crs_2171);
        EXPECT_EQ(bound->createBoundCRSToWGS84IfPossible(dbContext), bound);
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(bound);
        ASSERT_TRUE(boundCRS != nullptr);
        EXPECT_EQ(boundCRS->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_4)
                          .get()),
                  "+proj=sterea +lat_0=50.625 +lon_0=21.0833333333333 "
                  "+k=0.9998 +x_0=4637000 +y_0=5647000 +ellps=krass "
                  "+towgs84=33.4,-146.6,-76.3,-0.359,-0.053,0.844,-0.84");
    }
    {
        // NTF (Paris)
        auto crs_4807 = factory->createCoordinateReferenceSystem("4807");
        auto bound = crs_4807->createBoundCRSToWGS84IfPossible(dbContext);
        EXPECT_NE(bound, crs_4807);
        EXPECT_EQ(bound->createBoundCRSToWGS84IfPossible(dbContext), bound);
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(bound);
        ASSERT_TRUE(boundCRS != nullptr);
        EXPECT_EQ(boundCRS->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_4)
                          .get()),
                  "+proj=longlat +ellps=clrk80ign +pm=paris "
                  "+towgs84=-168,-60,320,0,0,0,0");
    }
    {
        // NTF (Paris) / Lambert zone II + NGF-IGN69 height
        auto crs_7421 = factory->createCoordinateReferenceSystem("7421");
        auto bound = crs_7421->createBoundCRSToWGS84IfPossible(dbContext);
        EXPECT_NE(bound, crs_7421);
        EXPECT_EQ(bound->createBoundCRSToWGS84IfPossible(dbContext), bound);
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(bound);
        ASSERT_TRUE(boundCRS != nullptr);
        EXPECT_EQ(boundCRS->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_4)
                          .get()),
                  "+proj=lcc +lat_1=46.8 +lat_0=46.8 +lon_0=0 +k_0=0.99987742 "
                  "+x_0=600000 +y_0=2200000 +ellps=clrk80ign +pm=paris "
                  "+towgs84=-168,-60,320,0,0,0,0 +vunits=m");
    }
    {
        auto crs = createVerticalCRS();
        EXPECT_EQ(crs->createBoundCRSToWGS84IfPossible(dbContext), crs);
    }
    {
        auto factoryIGNF =
            AuthorityFactory::create(DatabaseContext::create(), "IGNF");
        auto crs = factoryIGNF->createCoordinateReferenceSystem("TERA50STEREO");
        auto bound = crs->createBoundCRSToWGS84IfPossible(dbContext);
        EXPECT_NE(bound, crs);
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(bound);
        ASSERT_TRUE(boundCRS != nullptr);
        EXPECT_EQ(boundCRS->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_4)
                          .get()),
                  "+proj=stere +lat_0=-90 +lat_ts=-67 +lon_0=140 +x_0=300000 "
                  "+y_0=-2299363.482 +ellps=intl "
                  "+towgs84=324.912,153.282,172.026,0,0,0,0");
    }
    {
        auto factoryIGNF =
            AuthorityFactory::create(DatabaseContext::create(), "IGNF");
        auto crs = factoryIGNF->createCoordinateReferenceSystem("AMST63");
        auto bound = crs->createBoundCRSToWGS84IfPossible(dbContext);
        EXPECT_NE(bound, crs);
        auto boundCRS = nn_dynamic_pointer_cast<BoundCRS>(bound);
        ASSERT_TRUE(boundCRS != nullptr);
        EXPECT_EQ(boundCRS->exportToPROJString(
                      PROJStringFormatter::create(
                          PROJStringFormatter::Convention::PROJ_4)
                          .get()),
                  "+proj=geocent +ellps=intl "
                  "+towgs84=109.753,-528.133,-362.244,0,0,0,0");
    }
}

// ---------------------------------------------------------------------------

TEST(crs, crs_stripVerticalComponent) {

    {
        auto crs = GeographicCRS::EPSG_4979->stripVerticalComponent();
        auto geogCRS = nn_dynamic_pointer_cast<GeographicCRS>(crs);
        ASSERT_TRUE(geogCRS != nullptr);
        EXPECT_EQ(geogCRS->coordinateSystem()->axisList().size(), 2);
    }

    {
        auto crs = GeographicCRS::EPSG_4326->stripVerticalComponent();
        EXPECT_TRUE(crs->isEquivalentTo(GeographicCRS::EPSG_4326.get()));
    }

    {
        std::vector<CoordinateSystemAxisNNPtr> axis{
            CoordinateSystemAxis::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "Easting"), "E",
                AxisDirection::EAST, UnitOfMeasure::METRE),
            CoordinateSystemAxis::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "Northing"), "N",
                AxisDirection::NORTH, UnitOfMeasure::METRE),
            CoordinateSystemAxis::create(
                PropertyMap().set(IdentifiedObject::NAME_KEY, "Height"), "z",
                AxisDirection::UP, UnitOfMeasure::METRE)};
        auto cs(CartesianCS::create(PropertyMap(), axis[0], axis[1], axis[2]));
        auto projected3DCrs = ProjectedCRS::create(
            PropertyMap(), GeographicCRS::EPSG_4326,
            Conversion::createUTM(PropertyMap(), 31, true), cs);
        auto projCRS = nn_dynamic_pointer_cast<ProjectedCRS>(
            projected3DCrs->stripVerticalComponent());
        ASSERT_TRUE(projCRS != nullptr);
        EXPECT_EQ(projCRS->coordinateSystem()->axisList().size(), 2);
    }
}
