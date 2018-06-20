/******************************************************************************
 *
 * Project:  PROJ
 * Purpose:  ISO19111:2018 implementation
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

#ifndef FROM_PROJ_CPP
#define FROM_PROJ_CPP
#endif

#include "proj/coordinatesystem.hpp"
#include "proj/common.hpp"
#include "proj/coordinatesystem_internal.hpp"
#include "proj/internal.hpp"
#include "proj/io.hpp"
#include "proj/io_internal.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace NS_PROJ::common;
using namespace NS_PROJ::cs;
using namespace NS_PROJ::internal;
using namespace NS_PROJ::io;
using namespace NS_PROJ::metadata;
using namespace NS_PROJ::util;

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Meridian::Private {
    Angle longitude_{};

    explicit Private(const Angle &longitude) : longitude_(longitude) {}
};
//! @endcond

// ---------------------------------------------------------------------------

Meridian::Meridian(const Angle &longitudeIn)
    : d(internal::make_unique<Private>(longitudeIn)) {}

// ---------------------------------------------------------------------------

#ifdef notdef
Meridian::Meridian(const Meridian &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

Meridian::~Meridian() = default;

// ---------------------------------------------------------------------------

const Angle &Meridian::longitude() const { return d->longitude_; }

// ---------------------------------------------------------------------------

MeridianNNPtr Meridian::create(const Angle &longitudeIn) {
    return Meridian::nn_make_shared<Meridian>(longitudeIn);
}

// ---------------------------------------------------------------------------

std::string Meridian::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    formatter->startNode(WKTConstants::MERIDIAN, !identifiers().empty());
    formatter->add(longitude().value());
    longitude().unit().exportToWKT(formatter, WKTConstants::ANGLEUNIT);
    if (formatter->outputId()) {
        formatID(formatter);
    }
    formatter->endNode();
    return formatter->toString();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CoordinateSystemAxis::Private {
    std::string axisAbbrev{};
    const AxisDirection *axisDirection = &(AxisDirection::UNSPECIFIED);
    UnitOfMeasure axisUnitID{};
    optional<double> minimumValue{};
    optional<double> maximumValue{};
    MeridianPtr meridian{};
    // TODO rangeMeaning
};
//! @endcond

// ---------------------------------------------------------------------------

CoordinateSystemAxis::CoordinateSystemAxis()
    : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

#ifdef notdef
CoordinateSystemAxis::CoordinateSystemAxis(const CoordinateSystemAxis &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

CoordinateSystemAxis::~CoordinateSystemAxis() = default;

// ---------------------------------------------------------------------------

const std::string &CoordinateSystemAxis::axisAbbrev() const {
    return d->axisAbbrev;
}

// ---------------------------------------------------------------------------

const AxisDirection &CoordinateSystemAxis::axisDirection() const {
    return *(d->axisDirection);
}

// ---------------------------------------------------------------------------

const UnitOfMeasure &CoordinateSystemAxis::axisUnitID() const {
    return d->axisUnitID;
}

// ---------------------------------------------------------------------------

const optional<double> &CoordinateSystemAxis::minimumValue() const {
    return d->minimumValue;
}

// ---------------------------------------------------------------------------

const optional<double> &CoordinateSystemAxis::maximumValue() const {
    return d->maximumValue;
}

// ---------------------------------------------------------------------------

const MeridianPtr &CoordinateSystemAxis::meridian() const {
    return d->meridian;
}

// ---------------------------------------------------------------------------

CoordinateSystemAxisNNPtr CoordinateSystemAxis::create(
    const PropertyMap &properties, const std::string &abbreviation,
    const AxisDirection &direction, const UnitOfMeasure &unit,
    const MeridianPtr &meridian) {
    auto csa(CoordinateSystemAxis::nn_make_shared<CoordinateSystemAxis>());
    csa->setProperties(properties);
    csa->d->axisAbbrev = abbreviation;
    csa->d->axisDirection = &direction;
    csa->d->axisUnitID = unit;
    csa->d->meridian = meridian;
    return csa;
}

// ---------------------------------------------------------------------------

std::string CoordinateSystemAxis::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    return exportToWKT(formatter, 0, false);
}

// ---------------------------------------------------------------------------

std::string CoordinateSystemAxis::normalizeAxisName(const std::string &str) {
    if (str.empty()) {
        return str;
    }
    // on import, transform from WKT2 "longitude" to "Longitude", as in the
    // EPSG database.
    return toupper(str.substr(0, 1)) + str.substr(1);
}

// ---------------------------------------------------------------------------

std::string CoordinateSystemAxis::exportToWKT(WKTFormatterNNPtr formatter,
                                              int order,
                                              bool disableAbbrev) const {
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
    formatter->startNode(WKTConstants::AXIS, !identifiers().empty());
    std::string axisName = *(name()->description());
    std::string abbrev = axisAbbrev();
    std::string parenthesedAbbrev = "(" + abbrev + ")";
    std::string dir = axisDirection().toString();
    std::string axisDesignation;

    // It seems that the convention in WKT2 for axis name is first letter in
    // lower case. Whereas in WKT1 GDAL, it is in upper case (as in the EPSG
    // database)
    if (!axisName.empty()) {
        if (isWKT2) {
            axisDesignation =
                tolower(axisName.substr(0, 1)) + axisName.substr(1);
        } else {
            axisDesignation = axisName;
        }
    }

    if (!disableAbbrev && isWKT2 &&
        // For geodetic CS, export the axis name without abbreviation
        !(axisName == AxisName::Latitude || axisName == AxisName::Longitude)) {
        if (!axisDesignation.empty() && !abbrev.empty()) {
            axisDesignation += " ";
        }
        if (!abbrev.empty()) {
            axisDesignation += parenthesedAbbrev;
        }
    }
    if (!isWKT2) {
        dir = toupper(dir);

        if (axisDirection() == AxisDirection::GEOCENTRIC_Z) {
            dir = AxisDirectionWKT1::NORTH;
        } else if (AxisDirectionWKT1::valueOf(dir) == nullptr) {
            dir = AxisDirectionWKT1::OTHER;
        }
    } else if (!abbrev.empty()) {
        // For geocentric CS, just put the abbreviation
        if (axisDirection() == AxisDirection::GEOCENTRIC_X ||
            axisDirection() == AxisDirection::GEOCENTRIC_Y ||
            axisDirection() == AxisDirection::GEOCENTRIC_Z) {
            axisDesignation = parenthesedAbbrev;
        }
        // For cartesian CS with Easting/Northing, export only the abbreviation
        else if ((order == 1 && axisName == AxisName::Easting &&
                  abbrev == AxisAbbreviation::E) ||
                 (order == 2 && axisName == AxisName::Northing &&
                  abbrev == AxisAbbreviation::N)) {
            axisDesignation = parenthesedAbbrev;
        }
    }
    formatter->addQuotedString(axisDesignation);
    formatter->add(dir);
    if (meridian()) {
        meridian()->exportToWKT(formatter);
    }
    if (formatter->outputAxisOrder() && order > 0) {
        formatter->startNode(WKTConstants::ORDER, false);
        formatter->add(order);
        formatter->endNode();
    }
    if (formatter->outputUnit() &&
        axisUnitID().type() != UnitOfMeasure::Type::NONE) {
        axisUnitID().exportToWKT(formatter);
    }
    if (formatter->outputId()) {
        formatID(formatter);
    }
    formatter->endNode();
    return formatter->toString();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CoordinateSystem::Private {
    std::vector<CoordinateSystemAxisNNPtr> axis{};
};
//! @endcond

// ---------------------------------------------------------------------------

CoordinateSystem::CoordinateSystem() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

CoordinateSystem::CoordinateSystem(
    const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem() {
    d->axis = axisIn;
}

// ---------------------------------------------------------------------------

#ifdef notdef
CoordinateSystem::CoordinateSystem(const CoordinateSystem &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

CoordinateSystem::~CoordinateSystem() = default;

// ---------------------------------------------------------------------------

const std::vector<CoordinateSystemAxisNNPtr> &
CoordinateSystem::axisList() const {
    return d->axis;
}

// ---------------------------------------------------------------------------

std::string CoordinateSystem::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;

    if (isWKT2) {
        formatter->startNode(WKTConstants::CS, !identifiers().empty());
        formatter->add(getWKT2Type(formatter));
        formatter->add(axisList().size());
        formatter->endNode();
        formatter->startNode(std::string(),
                             false); // anonymous indentation level
    }

    UnitOfMeasure unit = UnitOfMeasure::NONE;
    bool bAllSameUnit = true;
    bool bFirstUnit = true;
    for (auto &axis : axisList()) {
        if (bFirstUnit) {
            unit = axis->axisUnitID();
            bFirstUnit = false;
        } else if (unit != axis->axisUnitID()) {
            bAllSameUnit = false;
        }
    }

    formatter->pushOutputUnit(
        isWKT2 && (!bAllSameUnit || !formatter->outputCSUnitOnlyOnceIfSame()));

    int order = 1;
    bool disableAbbrev =
        (axisList().size() == 3 &&
         *(axisList()[0]->name()->description()) == AxisName::Latitude &&
         *(axisList()[1]->name()->description()) == AxisName::Longitude &&
         *(axisList()[2]->name()->description()) ==
             AxisName::Ellipsoidal_height);

    for (auto &axis : axisList()) {
        int axisOrder = (isWKT2 && axisList().size() > 1) ? order : 0;
        axis->exportToWKT(formatter, axisOrder, disableAbbrev);
        order++;
    }
    if (isWKT2 && !axisList().empty() && bAllSameUnit &&
        formatter->outputCSUnitOnlyOnceIfSame()) {
        unit.exportToWKT(formatter);
    }

    formatter->popOutputUnit();

    if (isWKT2) {
        formatter->endNode();
    }
    return formatter->toString();
}

// ---------------------------------------------------------------------------

SphericalCS::SphericalCS() = default;

// ---------------------------------------------------------------------------

SphericalCS::~SphericalCS() = default;

// ---------------------------------------------------------------------------

SphericalCS::SphericalCS(const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
SphericalCS::SphericalCS(const SphericalCS &) = default;
#endif

// ---------------------------------------------------------------------------

SphericalCSNNPtr SphericalCS::create(const PropertyMap &properties,
                                     const CoordinateSystemAxisNNPtr &axis0,
                                     const CoordinateSystemAxisNNPtr &axis1,
                                     const CoordinateSystemAxisNNPtr &axis2) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis0, axis1, axis2};
    auto cs(SphericalCS::nn_make_shared<SphericalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

EllipsoidalCS::EllipsoidalCS() = default;

// ---------------------------------------------------------------------------

EllipsoidalCS::~EllipsoidalCS() = default;

// ---------------------------------------------------------------------------

EllipsoidalCS::EllipsoidalCS(
    const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
EllipsoidalCS::EllipsoidalCS(const EllipsoidalCS &) = default;
#endif

// ---------------------------------------------------------------------------

EllipsoidalCSNNPtr
EllipsoidalCS::create(const PropertyMap &properties,
                      const CoordinateSystemAxisNNPtr &axis0,
                      const CoordinateSystemAxisNNPtr &axis1) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis0, axis1};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

EllipsoidalCSNNPtr
EllipsoidalCS::create(const PropertyMap &properties,
                      const CoordinateSystemAxisNNPtr &axis0,
                      const CoordinateSystemAxisNNPtr &axis1,
                      const CoordinateSystemAxisNNPtr &axis2) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis0, axis1, axis2};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

EllipsoidalCSNNPtr
EllipsoidalCS::createLatitudeLongitude(const UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, AxisName::Latitude),
            AxisAbbreviation::lat, AxisDirection::NORTH, unit),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, AxisName::Longitude),
            AxisAbbreviation::lon, AxisDirection::EAST, unit)};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

EllipsoidalCSNNPtr EllipsoidalCS::createLatitudeLongitudeEllipsoidalHeight(
    const UnitOfMeasure &angularUnit, const UnitOfMeasure &linearUnit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, AxisName::Latitude),
            AxisAbbreviation::lat, AxisDirection::NORTH, angularUnit),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, AxisName::Longitude),
            AxisAbbreviation::lon, AxisDirection::EAST, angularUnit),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY,
                              AxisName::Ellipsoidal_height),
            AxisAbbreviation::h, AxisDirection::UP, linearUnit)};
    auto cs(EllipsoidalCS::nn_make_shared<EllipsoidalCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

VerticalCS::VerticalCS() = default;

// ---------------------------------------------------------------------------

VerticalCS::~VerticalCS() = default;

// ---------------------------------------------------------------------------

VerticalCS::VerticalCS(const CoordinateSystemAxisNNPtr &axisIn)
    : CoordinateSystem(std::vector<CoordinateSystemAxisNNPtr>{axisIn}) {}

// ---------------------------------------------------------------------------

#ifdef notdef
VerticalCS::VerticalCS(const VerticalCS &) = default;
#endif

// ---------------------------------------------------------------------------

VerticalCSNNPtr VerticalCS::create(const PropertyMap &properties,
                                   const CoordinateSystemAxisNNPtr &axis) {
    auto cs(VerticalCS::nn_make_shared<VerticalCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

VerticalCSNNPtr
VerticalCS::createGravityRelatedHeight(const UnitOfMeasure &unit) {
    auto cs(VerticalCS::nn_make_shared<VerticalCS>(CoordinateSystemAxis::create(
        PropertyMap().set(IdentifiedObject::NAME_KEY, "Gravity-related height"),
        "H", AxisDirection::UP, unit)));
    return cs;
}

// ---------------------------------------------------------------------------

CartesianCS::CartesianCS() = default;

// ---------------------------------------------------------------------------

CartesianCS::~CartesianCS() = default;

// ---------------------------------------------------------------------------

CartesianCS::CartesianCS(const std::vector<CoordinateSystemAxisNNPtr> &axisIn)
    : CoordinateSystem(axisIn) {}

// ---------------------------------------------------------------------------

#ifdef notdef
CartesianCS::CartesianCS(const CartesianCS &) = default;
#endif

// ---------------------------------------------------------------------------

CartesianCSNNPtr CartesianCS::create(const PropertyMap &properties,
                                     const CoordinateSystemAxisNNPtr &axis0,
                                     const CoordinateSystemAxisNNPtr &axis1) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis0, axis1};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

CartesianCSNNPtr CartesianCS::create(const PropertyMap &properties,
                                     const CoordinateSystemAxisNNPtr &axis0,
                                     const CoordinateSystemAxisNNPtr &axis1,
                                     const CoordinateSystemAxisNNPtr &axis2) {
    std::vector<CoordinateSystemAxisNNPtr> axis{axis0, axis1, axis2};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

CartesianCSNNPtr CartesianCS::createEastingNorthing(const UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, AxisName::Easting),
            AxisAbbreviation::E, AxisDirection::EAST, unit),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY, AxisName::Northing),
            AxisAbbreviation::N, AxisDirection::NORTH, unit)};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

CartesianCSNNPtr CartesianCS::createGeocentric(const UnitOfMeasure &unit) {
    std::vector<CoordinateSystemAxisNNPtr> axis{
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY,
                              AxisName::Geocentric_X),
            AxisAbbreviation::X, AxisDirection::GEOCENTRIC_X, unit),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY,
                              AxisName::Geocentric_Y),
            AxisAbbreviation::Y, AxisDirection::GEOCENTRIC_Y, unit),
        CoordinateSystemAxis::create(
            PropertyMap().set(IdentifiedObject::NAME_KEY,
                              AxisName::Geocentric_Z),
            AxisAbbreviation::Z, AxisDirection::GEOCENTRIC_Z, unit)};
    auto cs(CartesianCS::nn_make_shared<CartesianCS>(axis));
    return cs;
}

// ---------------------------------------------------------------------------

AxisDirection::AxisDirection(const std::string &nameIn) : CodeList(nameIn) {
    assert(axisDirectionKeys.find(nameIn) == axisDirectionKeys.end());
    axisDirectionRegistry[nameIn] = this;
    axisDirectionKeys.insert(nameIn);
}

// ---------------------------------------------------------------------------

const AxisDirection *AxisDirection::valueOf(const std::string &nameIn) {
    auto iter = axisDirectionRegistry.find(nameIn);
    if (iter == axisDirectionRegistry.end())
        return nullptr;
    return iter->second;
}

// ---------------------------------------------------------------------------

const std::set<std::string> &AxisDirection::getKeys() {
    return axisDirectionKeys;
}

// ---------------------------------------------------------------------------

AxisDirectionWKT1::AxisDirectionWKT1(const std::string &nameIn)
    : CodeList(nameIn) {
    assert(axisDirectionWKT1Keys.find(nameIn) == axisDirectionWKT1Keys.end());
    axisDirectionWKT1Registry[nameIn] = this;
    axisDirectionWKT1Keys.insert(nameIn);
}

// ---------------------------------------------------------------------------

const AxisDirectionWKT1 *AxisDirectionWKT1::valueOf(const std::string &nameIn) {
    auto iter = axisDirectionWKT1Registry.find(nameIn);
    if (iter == axisDirectionWKT1Registry.end())
        return nullptr;
    return iter->second;
}

// ---------------------------------------------------------------------------

const std::set<std::string> &AxisDirectionWKT1::getKeys() {
    return axisDirectionWKT1Keys;
}

// ---------------------------------------------------------------------------

TemporalCS::~TemporalCS() = default;

// ---------------------------------------------------------------------------

TemporalCS::TemporalCS(const CoordinateSystemAxisNNPtr &axisIn)
    : CoordinateSystem(std::vector<CoordinateSystemAxisNNPtr>{axisIn}) {}

// ---------------------------------------------------------------------------

DateTimeTemporalCS::~DateTimeTemporalCS() = default;

// ---------------------------------------------------------------------------

DateTimeTemporalCS::DateTimeTemporalCS(const CoordinateSystemAxisNNPtr &axisIn)
    : TemporalCS(axisIn) {}

// ---------------------------------------------------------------------------

DateTimeTemporalCSNNPtr
DateTimeTemporalCS::create(const PropertyMap &properties,
                           const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(DateTimeTemporalCS::nn_make_shared<DateTimeTemporalCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

std::string DateTimeTemporalCS::getWKT2Type(WKTFormatterNNPtr formatter) const {
    return formatter->use2018Keywords() ? "TemporalDateTime" : "temporal";
}

// ---------------------------------------------------------------------------

TemporalCountCS::~TemporalCountCS() = default;

// ---------------------------------------------------------------------------

TemporalCountCS::TemporalCountCS(const CoordinateSystemAxisNNPtr &axisIn)
    : TemporalCS(axisIn) {}

// ---------------------------------------------------------------------------

TemporalCountCSNNPtr
TemporalCountCS::create(const PropertyMap &properties,
                        const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(TemporalCountCS::nn_make_shared<TemporalCountCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

std::string TemporalCountCS::getWKT2Type(WKTFormatterNNPtr formatter) const {
    return formatter->use2018Keywords() ? "TemporalCount" : "temporal";
}

// ---------------------------------------------------------------------------

TemporalMeasureCS::~TemporalMeasureCS() = default;

// ---------------------------------------------------------------------------

TemporalMeasureCS::TemporalMeasureCS(const CoordinateSystemAxisNNPtr &axisIn)
    : TemporalCS(axisIn) {}

// ---------------------------------------------------------------------------

TemporalMeasureCSNNPtr
TemporalMeasureCS::create(const PropertyMap &properties,
                          const CoordinateSystemAxisNNPtr &axisIn) {
    auto cs(TemporalMeasureCS::nn_make_shared<TemporalMeasureCS>(axisIn));
    cs->setProperties(properties);
    return cs;
}

// ---------------------------------------------------------------------------

std::string TemporalMeasureCS::getWKT2Type(WKTFormatterNNPtr formatter) const {
    return formatter->use2018Keywords() ? "TemporalMeasure" : "temporal";
}
