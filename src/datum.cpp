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

#include "proj/datum.hpp"
#include "proj/common.hpp"
#include "proj/internal.hpp"
#include "proj/io.hpp"
#include "proj/io_internal.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include <memory>
#include <string>

using namespace NS_PROJ::common;
using namespace NS_PROJ::datum;
using namespace NS_PROJ::internal;
using namespace NS_PROJ::io;
using namespace NS_PROJ::metadata;
using namespace NS_PROJ::util;

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Datum::Private {
    optional<std::string> anchorDefinition{};
    optional<Date> publicationDate{};
    optional<IdentifiedObject> conventionalRS{};
};
//! @endcond

// ---------------------------------------------------------------------------

Datum::Datum() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

#ifdef notdef
Datum::Datum(const Datum &other)
    : ObjectUsage(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

Datum::~Datum() = default;

// ---------------------------------------------------------------------------

const optional<std::string> &Datum::anchorDefinition() const {
    return d->anchorDefinition;
}

// ---------------------------------------------------------------------------

const optional<Date> &Datum::publicationDate() const {
    return d->publicationDate;
}

// ---------------------------------------------------------------------------

const optional<IdentifiedObject> &Datum::conventionalRS() const {
    return d->conventionalRS;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct PrimeMeridian::Private {
    Angle greenwichLongitude{};

    explicit Private(const Angle &longitude) : greenwichLongitude(longitude) {}
};
//! @endcond

// ---------------------------------------------------------------------------

PrimeMeridian::PrimeMeridian(const Angle &longitude)
    : d(internal::make_unique<Private>(longitude)) {}

// ---------------------------------------------------------------------------

#ifdef notdef
PrimeMeridian::PrimeMeridian(const PrimeMeridian &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

PrimeMeridian::~PrimeMeridian() = default;

// ---------------------------------------------------------------------------

const Angle &PrimeMeridian::greenwichLongitude() const {
    return d->greenwichLongitude;
}

// ---------------------------------------------------------------------------

PrimeMeridianNNPtr PrimeMeridian::create(const PropertyMap &properties,
                                         const Angle &longitude) {
    auto pm(PrimeMeridian::nn_make_shared<PrimeMeridian>(longitude));
    pm->setProperties(properties);
    return pm;
}

// ---------------------------------------------------------------------------

const PrimeMeridianNNPtr PrimeMeridian::createGREENWICH() {
    return create(PropertyMap()
                      .set(Identifier::CODESPACE_KEY, "EPSG")
                      .set(Identifier::CODE_KEY, 8901)
                      .set(IdentifiedObject::NAME_KEY, "Greenwich"),
                  Angle(0));
}

// ---------------------------------------------------------------------------

std::string PrimeMeridian::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
    std::string l_name = name()->description().has_value()
                             ? *(name()->description())
                             : "Greenwich";
    if (!(isWKT2 && formatter->primeMeridianOmittedIfGreenwich() &&
          l_name == "Greenwich")) {
        formatter->startNode(WKTConstants::PRIMEM, !identifiers().empty());
        formatter->addQuotedString(l_name);
        if (formatter->primeMeridianInDegree()) {
            formatter->add(greenwichLongitude()
                               .convertToUnit(UnitOfMeasure::DEGREE)
                               .value());
        } else {
            formatter->add(greenwichLongitude().value());
        }
        if (isWKT2) {
            if (!(formatter
                      ->primeMeridianOrParameterUnitOmittedIfSameAsAxis() &&
                  greenwichLongitude().unit() ==
                      *(formatter->axisAngularUnit()))) {
                greenwichLongitude().unit().exportToWKT(
                    formatter, WKTConstants::ANGLEUNIT);
            }
        } else if (!formatter->primeMeridianInDegree()) {
            greenwichLongitude().unit().exportToWKT(formatter);
        }
        if (formatter->outputId()) {
            formatID(formatter);
        }
        formatter->endNode();
    }
    return formatter->toString();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Ellipsoid::Private {
    Length semiMajorAxis_{};
    optional<Scale> inverseFlattening_{};
    optional<Length> semiMinorAxis_{};
    optional<Length> semiMedianAxis_{};

    explicit Private(const Length &radius) : semiMajorAxis_(radius) {}

    Private(const Length &semiMajorAxisIn, const Scale &invFlattening)
        : semiMajorAxis_(semiMajorAxisIn), inverseFlattening_(invFlattening) {}

    Private(const Length &semiMajorAxisIn, const Length &semiMinorAxisIn)
        : semiMajorAxis_(semiMajorAxisIn), semiMinorAxis_(semiMinorAxisIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

Ellipsoid::Ellipsoid(const Length &radius)
    : d(internal::make_unique<Private>(radius)) {}

// ---------------------------------------------------------------------------

Ellipsoid::Ellipsoid(const Length &semiMajorAxisIn, const Scale &invFlattening)
    : d(internal::make_unique<Private>(semiMajorAxisIn, invFlattening)) {}

// ---------------------------------------------------------------------------

Ellipsoid::Ellipsoid(const Length &semiMajorAxisIn,
                     const Length &semiMinorAxisIn)
    : d(internal::make_unique<Private>(semiMajorAxisIn, semiMinorAxisIn)) {}

// ---------------------------------------------------------------------------

#ifdef notdef
Ellipsoid::Ellipsoid(const Ellipsoid &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

Ellipsoid::~Ellipsoid() = default;

// ---------------------------------------------------------------------------

const Length &Ellipsoid::semiMajorAxis() const { return d->semiMajorAxis_; }

// ---------------------------------------------------------------------------

const optional<Scale> &Ellipsoid::inverseFlattening() const {
    return d->inverseFlattening_;
}

// ---------------------------------------------------------------------------

const optional<Length> &Ellipsoid::semiMinorAxis() const {
    return d->semiMinorAxis_;
}

// ---------------------------------------------------------------------------

bool Ellipsoid::isSphere() const {
    if (inverseFlattening().has_value()) {
        return inverseFlattening()->value() == 0;
    }

    if (semiMinorAxis().has_value()) {
        return semiMajorAxis() == *semiMinorAxis();
    }

    return true;
}

// ---------------------------------------------------------------------------

const optional<Length> &Ellipsoid::semiMedianAxis() const {
    return d->semiMedianAxis_;
}

// ---------------------------------------------------------------------------

Scale Ellipsoid::computeInverseFlattening() const {
    if (inverseFlattening().has_value()) {
        return *inverseFlattening();
    }

    if (semiMinorAxis().has_value()) {
        const double a = semiMajorAxis().value();
        const double b = semiMinorAxis()->value();
        return Scale((a == b) ? 0.0 : a / (a - b));
    }

    return Scale(0.0);
}

// ---------------------------------------------------------------------------

Length Ellipsoid::computeSemiMinorAxis() const {
    if (semiMinorAxis().has_value()) {
        return *semiMinorAxis();
    }

    if (inverseFlattening().has_value()) {
        return Length((1.0 - 1.0 / inverseFlattening()->getSIValue()) *
                          semiMajorAxis().value(),
                      semiMajorAxis().unit());
    }

    return semiMajorAxis();
}

// ---------------------------------------------------------------------------

/* static */
EllipsoidNNPtr Ellipsoid::createSphere(const PropertyMap &properties,
                                       const Length &radius) {
    auto ellipsoid(Ellipsoid::nn_make_shared<Ellipsoid>(radius));
    ellipsoid->setProperties(properties);
    return ellipsoid;
}

// ---------------------------------------------------------------------------

/* static */
EllipsoidNNPtr Ellipsoid::createFlattenedSphere(const PropertyMap &properties,
                                                const Length &semiMajorAxisIn,
                                                const Scale &invFlattening) {
    auto ellipsoid(
        Ellipsoid::nn_make_shared<Ellipsoid>(semiMajorAxisIn, invFlattening));
    ellipsoid->setProperties(properties);
    return ellipsoid;
}

// ---------------------------------------------------------------------------

/* static */
EllipsoidNNPtr Ellipsoid::createTwoAxis(const PropertyMap &properties,
                                        const Length &semiMajorAxisIn,
                                        const Length &semiMinorAxisIn) {
    auto ellipsoid(
        Ellipsoid::nn_make_shared<Ellipsoid>(semiMajorAxisIn, semiMinorAxisIn));
    ellipsoid->setProperties(properties);
    return ellipsoid;
}
// ---------------------------------------------------------------------------

const EllipsoidNNPtr Ellipsoid::createEPSG_7030() {
    PropertyMap propertiesEllps;
    propertiesEllps.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 7030)
        .set(IdentifiedObject::NAME_KEY, "WGS 84");
    return createFlattenedSphere(propertiesEllps, Length(6378137),
                                 Scale(298.257223563));
}

// ---------------------------------------------------------------------------

std::string Ellipsoid::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
    formatter->startNode(isWKT2 ? WKTConstants::ELLIPSOID
                                : WKTConstants::SPHEROID,
                         !identifiers().empty());
    {
        formatter->addQuotedString(name()->description().has_value()
                                       ? *(name()->description())
                                       : "unnamed");
        if (isWKT2) {
            formatter->add(semiMajorAxis().value());
        } else {
            formatter->add(
                semiMajorAxis().convertToUnit(UnitOfMeasure::METRE).value());
        }
        formatter->add(computeInverseFlattening().value());
        if (isWKT2 &&
            !(formatter->ellipsoidUnitOmittedIfMetre() &&
              semiMajorAxis().unit() == UnitOfMeasure::METRE)) {
            semiMajorAxis().unit().exportToWKT(formatter,
                                               WKTConstants::LENGTHUNIT);
        }
        if (formatter->outputId()) {
            formatID(formatter);
        }
    }
    formatter->endNode();
    return formatter->toString();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeodeticReferenceFrame::Private {
    PrimeMeridianNNPtr primeMeridian_;
    EllipsoidNNPtr ellipsoid_;

    Private(const EllipsoidNNPtr &ellipsoidIn,
            const PrimeMeridianNNPtr &primeMeridianIn)
        : primeMeridian_(primeMeridianIn), ellipsoid_(ellipsoidIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

GeodeticReferenceFrame::GeodeticReferenceFrame(
    const EllipsoidNNPtr &ellipsoidIn,
    const PrimeMeridianNNPtr &primeMeridianIn)
    : d(internal::make_unique<Private>(ellipsoidIn, primeMeridianIn)) {}

// ---------------------------------------------------------------------------

#ifdef notdef
GeodeticReferenceFrame::GeodeticReferenceFrame(
    const GeodeticReferenceFrame &other)
    : Datum(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

GeodeticReferenceFrame::~GeodeticReferenceFrame() = default;

// ---------------------------------------------------------------------------

const PrimeMeridianNNPtr &GeodeticReferenceFrame::primeMeridian() const {
    return d->primeMeridian_;
}

// ---------------------------------------------------------------------------

const EllipsoidNNPtr &GeodeticReferenceFrame::ellipsoid() const {
    return d->ellipsoid_;
}
// ---------------------------------------------------------------------------

GeodeticReferenceFrameNNPtr
GeodeticReferenceFrame::create(const PropertyMap &properties,
                               const EllipsoidNNPtr &ellipsoid,
                               const optional<std::string> &anchor,
                               const PrimeMeridianNNPtr &primeMeridian) {
    GeodeticReferenceFrameNNPtr grf(
        GeodeticReferenceFrame::nn_make_shared<GeodeticReferenceFrame>(
            ellipsoid, primeMeridian));
    util::nn_static_pointer_cast<Datum>(grf)->d->anchorDefinition = anchor;
    grf->setProperties(properties);
    return grf;
}

// ---------------------------------------------------------------------------

const GeodeticReferenceFrameNNPtr GeodeticReferenceFrame::createEPSG_6326() {
    PropertyMap propertiesDatum;
    propertiesDatum.set(Identifier::CODESPACE_KEY, "EPSG")
        .set(Identifier::CODE_KEY, 6326)
        .set(IdentifiedObject::NAME_KEY, "WGS_1984");

    return create(propertiesDatum, Ellipsoid::EPSG_7030,
                  optional<std::string>(), PrimeMeridian::GREENWICH);
}

// ---------------------------------------------------------------------------

std::string GeodeticReferenceFrame::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
    formatter->startNode(WKTConstants::DATUM, !identifiers().empty());
    formatter->addQuotedString(name()->description().has_value()
                                   ? *(name()->description())
                                   : "unnamed");
    ellipsoid()->exportToWKT(formatter);
    if (isWKT2) {
        if (anchorDefinition()) {
            formatter->startNode(WKTConstants::ANCHOR, false);
            formatter->addQuotedString(*anchorDefinition());
            formatter->endNode();
        }
    } else {
        const auto &TOWGS84Params = formatter->getTOWGS84Parameters();
        if (TOWGS84Params.size() == 7) {
            formatter->startNode(WKTConstants::TOWGS84, false);
            for (const auto &val : TOWGS84Params) {
                formatter->add(val);
            }
            formatter->endNode();
        }
    }
    if (formatter->outputId()) {
        formatID(formatter);
    }
    // the PRIMEM is exported as a child of the CRS
    formatter->endNode();
    return formatter->toString();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct DynamicGeodeticReferenceFrame::Private {
    Measure frameReferenceEpoch{};
};
//! @endcond

// ---------------------------------------------------------------------------

DynamicGeodeticReferenceFrame::DynamicGeodeticReferenceFrame(
    const EllipsoidNNPtr &ellipsoidIn,
    const PrimeMeridianNNPtr &primeMeridianIn)
    : GeodeticReferenceFrame(ellipsoidIn, primeMeridianIn),
      d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

#ifdef notdef
DynamicGeodeticReferenceFrame::DynamicGeodeticReferenceFrame(
    const DynamicGeodeticReferenceFrame &other)
    : GeodeticReferenceFrame(other),
      d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

DynamicGeodeticReferenceFrame::~DynamicGeodeticReferenceFrame() = default;

// ---------------------------------------------------------------------------

const Measure &DynamicGeodeticReferenceFrame::frameReferenceEpoch() const {
    return d->frameReferenceEpoch;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
// cppcheck-suppress copyCtorAndEqOperator
struct DatumEnsemble::Private {
    std::vector<DatumPtr> datums{};
    PositionalAccuracyNNPtr positionalAccuracy;
};
//! @endcond

// ---------------------------------------------------------------------------

#ifdef notdef
DatumEnsemble::DatumEnsemble(const DatumEnsemble &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*other.d)) {}
#endif

// ---------------------------------------------------------------------------

DatumEnsemble::~DatumEnsemble() = default;

// ---------------------------------------------------------------------------

const std::vector<DatumPtr> &DatumEnsemble::datums() const { return d->datums; }

// ---------------------------------------------------------------------------

const PositionalAccuracyNNPtr &DatumEnsemble::positionalAccuracy() const {
    return d->positionalAccuracy;
}

// ---------------------------------------------------------------------------

RealizationMethod::RealizationMethod(const std::string &nameIn)
    : CodeList(nameIn) {}

// ---------------------------------------------------------------------------

RealizationMethod::RealizationMethod(const RealizationMethod &) = default;

// ---------------------------------------------------------------------------

RealizationMethod &RealizationMethod::
operator=(const RealizationMethod &other) {
    CodeList::operator=(other);
    return *this;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
// cppcheck-suppress copyCtorAndEqOperator
struct VerticalReferenceFrame::Private {
    optional<RealizationMethod> realizationMethod_{};
};
//! @endcond

// ---------------------------------------------------------------------------

VerticalReferenceFrame::VerticalReferenceFrame()
    : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

VerticalReferenceFrame::~VerticalReferenceFrame() = default;

// ---------------------------------------------------------------------------

const optional<RealizationMethod> &
VerticalReferenceFrame::realizationMethod() const {
    return d->realizationMethod_;
}

// ---------------------------------------------------------------------------

VerticalReferenceFrameNNPtr VerticalReferenceFrame::create(
    const PropertyMap &properties, const optional<std::string> &anchor,
    const optional<RealizationMethod> &realizationMethodIn) {
    auto rf(VerticalReferenceFrame::nn_make_shared<VerticalReferenceFrame>());
    util::nn_static_pointer_cast<Datum>(rf)->d->anchorDefinition = anchor;
    rf->setProperties(properties);
    if (realizationMethodIn.has_value() &&
        !realizationMethodIn->toString().empty()) {
        rf->d->realizationMethod_ = *realizationMethodIn;
    }
    return rf;
}

// ---------------------------------------------------------------------------

std::string VerticalReferenceFrame::exportToWKT(
    WKTFormatterNNPtr formatter) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
    if (isWKT2) {
        formatter->startNode(WKTConstants::VDATUM, !identifiers().empty());
        formatter->addQuotedString(name()->description().has_value()
                                       ? *(name()->description())
                                       : "unnamed");
        if (anchorDefinition()) {
            formatter->startNode(WKTConstants::ANCHOR, false);
            formatter->addQuotedString(*anchorDefinition());
            formatter->endNode();
        }
        if (formatter->outputId()) {
            formatID(formatter);
        }
        formatter->endNode();
    } else {
        formatter->startNode(WKTConstants::VERT_DATUM, !identifiers().empty());
        formatter->addQuotedString(name()->description().has_value()
                                       ? *(name()->description())
                                       : "unnamed");
        formatter->add(2005); // CS_VD_GeoidModelDerived from OGC 01-009
        if (formatter->outputId()) {
            formatID(formatter);
        }
        formatter->endNode();
    }
    return formatter->toString();
}
