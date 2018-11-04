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

//! @cond Doxygen_Suppress
#define DO_NOT_DEFINE_EXTERN_DERIVED_CRS_TEMPLATE
//! @endcond

#include "proj/crs.hpp"
#include "proj/common.hpp"
#include "proj/coordinateoperation.hpp"
#include "proj/coordinatesystem.hpp"
#include "proj/io.hpp"
#include "proj/util.hpp"

#include "proj/internal/coordinatesystem_internal.hpp"
#include "proj/internal/internal.hpp"
#include "proj/internal/io_internal.hpp"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace NS_PROJ::internal;

#if 0
namespace dropbox{ namespace oxygen {
template<> nn<NS_PROJ::crs::CRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::SingleCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::GeodeticCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::GeographicCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::ProjectedCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::VerticalCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::CompoundCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::TemporalCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::EngineeringCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::ParametricCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::BoundCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedGeodeticCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedGeographicCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedProjectedCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedVerticalCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedTemporalCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedEngineeringCRSPtr>::~nn() = default;
template<> nn<NS_PROJ::crs::DerivedParametricCRSPtr>::~nn() = default;
}}
#endif

NS_PROJ_START

namespace crs {

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CRS::Private {
    BoundCRSPtr canonicalBoundCRS_{};
};
//! @endcond

// ---------------------------------------------------------------------------

CRS::CRS() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

CRS::CRS(const CRS &other)
    : ObjectUsage(other), d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CRS::~CRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the BoundCRS potentially attached to this CRS.
 *
 * In the case this method is called on a object returned by
 * BoundCRS::baseCRSWithCanonicalBoundCRS(), this method will return this
 * BoundCRS
 *
 * @return a BoundCRSPtr, that might be null.
 */
const BoundCRSPtr &CRS::canonicalBoundCRS() PROJ_CONST_DEFN {
    return d->canonicalBoundCRS_;
}

// ---------------------------------------------------------------------------

/** \brief Return the GeodeticCRS of the CRS.
 *
 * Returns the GeodeticCRS contained in a CRS. This works currently with
 * input parameters of type GeodeticCRS or derived, ProjectedCRS,
 * CompoundCRS or BoundCRS.
 *
 * @return a GeodeticCRSPtr, that might be null.
 */
GeodeticCRSPtr CRS::extractGeodeticCRS() const {
    auto raw = extractGeodeticCRSRaw();
    if (raw) {
        return std::dynamic_pointer_cast<GeodeticCRS>(
            raw->shared_from_this().as_nullable());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
const GeodeticCRS *CRS::extractGeodeticCRSRaw() const {
    auto geodCRS = dynamic_cast<const GeodeticCRS *>(this);
    if (geodCRS) {
        return geodCRS;
    }
    auto projCRS = dynamic_cast<const ProjectedCRS *>(this);
    if (projCRS) {
        return projCRS->baseCRS()->extractGeodeticCRSRaw();
    }
    auto compoundCRS = dynamic_cast<const CompoundCRS *>(this);
    if (compoundCRS) {
        for (const auto &subCrs : compoundCRS->componentReferenceSystems()) {
            auto retGeogCRS = subCrs->extractGeodeticCRSRaw();
            if (retGeogCRS) {
                return retGeogCRS;
            }
        }
    }
    auto boundCRS = dynamic_cast<const BoundCRS *>(this);
    if (boundCRS) {
        return boundCRS->baseCRS()->extractGeodeticCRSRaw();
    }
    return nullptr;
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the GeographicCRS of the CRS.
 *
 * Returns the GeographicCRS contained in a CRS. This works currently with
 * input parameters of type GeographicCRS or derived, ProjectedCRS,
 * CompoundCRS or BoundCRS.
 *
 * @return a GeographicCRSPtr, that might be null.
 */
GeographicCRSPtr CRS::extractGeographicCRS() const {
    auto raw = extractGeodeticCRSRaw();
    if (raw) {
        return std::dynamic_pointer_cast<GeographicCRS>(
            raw->shared_from_this().as_nullable());
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Return the VerticalCRS of the CRS.
 *
 * Returns the VerticalCRS contained in a CRS. This works currently with
 * input parameters of type VerticalCRS or derived, CompoundCRS or BoundCRS.
 *
 * @return a VerticalCRSPtr, that might be null.
 */
VerticalCRSPtr CRS::extractVerticalCRS() const {
    auto vertCRS = dynamic_cast<const VerticalCRS *>(this);
    if (vertCRS) {
        return std::dynamic_pointer_cast<VerticalCRS>(
            shared_from_this().as_nullable());
    }
    auto compoundCRS = dynamic_cast<const CompoundCRS *>(this);
    if (compoundCRS) {
        for (const auto &subCrs : compoundCRS->componentReferenceSystems()) {
            auto retVertCRS = subCrs->extractVerticalCRS();
            if (retVertCRS) {
                return retVertCRS;
            }
        }
    }
    auto boundCRS = dynamic_cast<const BoundCRS *>(this);
    if (boundCRS) {
        return boundCRS->baseCRS()->extractVerticalCRS();
    }
    return nullptr;
}

// ---------------------------------------------------------------------------

/** \brief Returns potentially
 * a BoundCRS, with a transformation to EPSG:4326, wrapping this CRS
 *
 * If no such BoundCRS is possible, the object will be returned.
 *
 * The purpose of this method is to be able to format a PROJ.4 string with
 * a +towgs84 parameter or a WKT1:GDAL string with a TOWGS node.
 *
 * This method will fetch the GeographicCRS of this CRS and find a
 * transformation to EPSG:4326 using the domain of the validity of the main CRS.
 *
 * @return a CRS.
 */
CRSNNPtr
CRS::createBoundCRSToWGS84IfPossible(io::DatabaseContextPtr dbContext) const {
    auto thisAsCRS = NN_NO_CHECK(
        std::static_pointer_cast<CRS>(shared_from_this().as_nullable()));
    auto boundCRS = util::nn_dynamic_pointer_cast<BoundCRS>(thisAsCRS);
    if (!boundCRS) {
        boundCRS = canonicalBoundCRS();
    }
    if (boundCRS) {
        if (boundCRS->hubCRS()->isEquivalentTo(
                GeographicCRS::EPSG_4326.get(),
                util::IComparable::Criterion::EQUIVALENT)) {
            return NN_NO_CHECK(boundCRS);
        }
    }

    auto geodCRS = util::nn_dynamic_pointer_cast<GeodeticCRS>(thisAsCRS);
    auto geogCRS = extractGeographicCRS();
    auto hubCRS = util::nn_static_pointer_cast<CRS>(GeographicCRS::EPSG_4326);
    if (geodCRS && !geogCRS) {
        hubCRS = util::nn_static_pointer_cast<CRS>(GeodeticCRS::EPSG_4978);
    } else if (!geogCRS ||
               geogCRS->isEquivalentTo(
                   GeographicCRS::EPSG_4326.get(),
                   util::IComparable::Criterion::EQUIVALENT)) {
        return thisAsCRS;
    } else {
        geodCRS = geogCRS;
    }
    auto l_domains = domains();
    metadata::ExtentPtr extent;
    if (!l_domains.empty()) {
        extent = l_domains[0]->domainOfValidity();
    }

    try {
        auto authFactory = dbContext
                               ? io::AuthorityFactory::create(
                                     NN_NO_CHECK(dbContext), std::string())
                                     .as_nullable()
                               : nullptr;
        auto ctxt = operation::CoordinateOperationContext::create(authFactory,
                                                                  extent, 0.0);
        // ctxt->setSpatialCriterion(
        //    operation::CoordinateOperationContext::SpatialCriterion::PARTIAL_INTERSECTION);
        auto list =
            operation::CoordinateOperationFactory::create()->createOperations(
                NN_NO_CHECK(geodCRS), hubCRS, ctxt);
        for (const auto &op : list) {
            auto transf =
                util::nn_dynamic_pointer_cast<operation::Transformation>(op);
            if (transf) {
                try {
                    transf->getTOWGS84Parameters();
                } catch (const std::exception &) {
                    continue;
                }
                return util::nn_static_pointer_cast<CRS>(
                    BoundCRS::create(thisAsCRS, hubCRS, NN_NO_CHECK(transf)));
            } else {
                auto concatenated =
                    dynamic_cast<const operation::ConcatenatedOperation *>(
                        op.get());
                if (concatenated) {
                    // Case for EPSG:4807 / "NTF (Paris)" that is made of a
                    // longitude rotation followed by a Helmert
                    // The prime meridian shift will be accounted elsewhere
                    const auto &subops = concatenated->operations();
                    if (subops.size() == 2) {
                        auto firstOpIsTransformation =
                            dynamic_cast<const operation::Transformation *>(
                                subops[0].get());
                        auto firstOpIsConversion =
                            dynamic_cast<const operation::Conversion *>(
                                subops[0].get());
                        if ((firstOpIsTransformation &&
                             firstOpIsTransformation->isLongitudeRotation()) ||
                            (dynamic_cast<DerivedCRS *>(thisAsCRS.get()) &&
                             firstOpIsConversion)) {
                            transf = util::nn_dynamic_pointer_cast<
                                operation::Transformation>(subops[1]);
                            if (transf) {
                                try {
                                    transf->getTOWGS84Parameters();
                                } catch (const std::exception &) {
                                    continue;
                                }
                                return util::nn_static_pointer_cast<CRS>(
                                    BoundCRS::create(thisAsCRS, hubCRS,
                                                     NN_NO_CHECK(transf)));
                            }
                        }
                    }
                }
            }
        }
    } catch (const std::exception &) {
    }
    return thisAsCRS;
}

// ---------------------------------------------------------------------------

/** \brief Returns a CRS whose coordinate system does not contain a vertical
 * component
 *
 * @return a CRS.
 */
CRSNNPtr CRS::stripVerticalComponent() const {
    auto self = NN_NO_CHECK(
        std::dynamic_pointer_cast<CRS>(shared_from_this().as_nullable()));

    auto geogCRS = dynamic_cast<const GeographicCRS *>(this);
    if (geogCRS) {
        const auto &axisList = geogCRS->coordinateSystem()->axisList();
        if (axisList.size() == 3) {
            auto cs = cs::EllipsoidalCS::create(util::PropertyMap(),
                                                axisList[0], axisList[1]);
            return util::nn_static_pointer_cast<CRS>(GeographicCRS::create(
                util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                        nameStr()),
                geogCRS->datum(), geogCRS->datumEnsemble(), cs));
        }
    }
    auto projCRS = dynamic_cast<const ProjectedCRS *>(this);
    if (projCRS) {
        const auto &axisList = projCRS->coordinateSystem()->axisList();
        if (axisList.size() == 3) {
            auto cs = cs::CartesianCS::create(util::PropertyMap(), axisList[0],
                                              axisList[1]);
            return util::nn_static_pointer_cast<CRS>(ProjectedCRS::create(
                util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                        nameStr()),
                projCRS->baseCRS(), projCRS->derivingConversion(), cs));
        }
    }
    return self;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct SingleCRS::Private {
    datum::DatumPtr datum{};
    datum::DatumEnsemblePtr datumEnsemble{};
    cs::CoordinateSystemNNPtr coordinateSystem;

    Private(const datum::DatumPtr &datumIn,
            const datum::DatumEnsemblePtr &datumEnsembleIn,
            const cs::CoordinateSystemNNPtr &csIn)
        : datum(datumIn), datumEnsemble(datumEnsembleIn),
          coordinateSystem(csIn) {
        if ((datum ? 1 : 0) + (datumEnsemble ? 1 : 0) != 1) {
            throw util::Exception("datum or datumEnsemble should be set");
        }
    }
};
//! @endcond

// ---------------------------------------------------------------------------

SingleCRS::SingleCRS(const datum::DatumPtr &datumIn,
                     const datum::DatumEnsemblePtr &datumEnsembleIn,
                     const cs::CoordinateSystemNNPtr &csIn)
    : d(internal::make_unique<Private>(datumIn, datumEnsembleIn, csIn)) {}

// ---------------------------------------------------------------------------

SingleCRS::SingleCRS(const SingleCRS &other)
    : CRS(other), d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
SingleCRS::~SingleCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the datum::Datum associated with the CRS.
 *
 * This might be null, in which case datumEnsemble() return will not be null.
 *
 * @return a Datum that might be null.
 */
const datum::DatumPtr &SingleCRS::datum() PROJ_CONST_DEFN { return d->datum; }

// ---------------------------------------------------------------------------

/** \brief Return the datum::DatumEnsemble associated with the CRS.
 *
 * This might be null, in which case datum() return will not be null.
 *
 * @return a DatumEnsemble that might be null.
 */
const datum::DatumEnsemblePtr &SingleCRS::datumEnsemble() PROJ_CONST_DEFN {
    return d->datumEnsemble;
}

// ---------------------------------------------------------------------------

/** \brief Return the cs::CoordinateSystem associated with the CRS.
 *
 * This might be null, in which case datumEnsemble() return will not be null.
 *
 * @return a CoordinateSystem that might be null.
 */
const cs::CoordinateSystemNNPtr &SingleCRS::coordinateSystem() PROJ_CONST_DEFN {
    return d->coordinateSystem;
}

// ---------------------------------------------------------------------------

bool SingleCRS::_isEquivalentTo(const util::IComparable *other,
                                util::IComparable::Criterion criterion) const {
    auto otherSingleCRS = dynamic_cast<const SingleCRS *>(other);
    if (otherSingleCRS == nullptr ||
        !ObjectUsage::isEquivalentTo(other, criterion)) {
        return false;
    }
    if ((datum() != nullptr) ^ (otherSingleCRS->datum() != nullptr)) {
        return false;
    }
    if (datum() &&
        !datum()->isEquivalentTo(otherSingleCRS->datum().get(), criterion))
        return false;

    // TODO test DatumEnsemble
    return coordinateSystem()->isEquivalentTo(
        otherSingleCRS->coordinateSystem().get(), criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void SingleCRS::exportDatumOrDatumEnsembleToWkt(
    io::WKTFormatter *formatter) const // throw(io::FormattingException)
{
    const auto &l_datum = datum();
    if (l_datum) {
        auto exportable =
            dynamic_cast<const io::IWKTExportable *>(l_datum.get());
        if (exportable) {
            exportable->_exportToWKT(formatter);
        }
    } else {
        const auto &l_datumEnsemble = datumEnsemble();
        assert(l_datumEnsemble);
        l_datumEnsemble->_exportToWKT(formatter);
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeodeticCRS::Private {
    std::vector<operation::PointMotionOperationNNPtr> velocityModel{};
    datum::GeodeticReferenceFramePtr datum_;

    explicit Private(const datum::GeodeticReferenceFramePtr &datumIn)
        : datum_(datumIn) {}
};

// ---------------------------------------------------------------------------

static const datum::DatumEnsemblePtr &
checkEnsembleForGeodeticCRS(const datum::GeodeticReferenceFramePtr &datumIn,
                            const datum::DatumEnsemblePtr &ensemble) {
    const char *msg = "One of Datum or DatumEnsemble should be defined";
    if (datumIn) {
        if (!ensemble) {
            return ensemble;
        }
        msg = "Datum and DatumEnsemble should not be defined";
    } else if (ensemble) {
        const auto &datums = ensemble->datums();
        assert(!datums.empty());
        auto grfFirst =
            dynamic_cast<datum::GeodeticReferenceFrame *>(datums[0].get());
        if (grfFirst) {
            return ensemble;
        }
        msg = "Ensemble should contain GeodeticReferenceFrame";
    }
    throw util::Exception(msg);
}

//! @endcond

// ---------------------------------------------------------------------------

GeodeticCRS::GeodeticCRS(const datum::GeodeticReferenceFramePtr &datumIn,
                         const datum::DatumEnsemblePtr &datumEnsembleIn,
                         const cs::EllipsoidalCSNNPtr &csIn)
    : SingleCRS(datumIn, checkEnsembleForGeodeticCRS(datumIn, datumEnsembleIn),
                csIn),
      d(internal::make_unique<Private>(datumIn)) {}

// ---------------------------------------------------------------------------

GeodeticCRS::GeodeticCRS(const datum::GeodeticReferenceFramePtr &datumIn,
                         const datum::DatumEnsemblePtr &datumEnsembleIn,
                         const cs::SphericalCSNNPtr &csIn)
    : SingleCRS(datumIn, checkEnsembleForGeodeticCRS(datumIn, datumEnsembleIn),
                csIn),
      d(internal::make_unique<Private>(datumIn)) {}

// ---------------------------------------------------------------------------

GeodeticCRS::GeodeticCRS(const datum::GeodeticReferenceFramePtr &datumIn,
                         const datum::DatumEnsemblePtr &datumEnsembleIn,
                         const cs::CartesianCSNNPtr &csIn)
    : SingleCRS(datumIn, checkEnsembleForGeodeticCRS(datumIn, datumEnsembleIn),
                csIn),
      d(internal::make_unique<Private>(datumIn)) {}

// ---------------------------------------------------------------------------

GeodeticCRS::GeodeticCRS(const GeodeticCRS &other)
    : SingleCRS(other), d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeodeticCRS::~GeodeticCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

CRSNNPtr GeodeticCRS::shallowClone() const {
    auto crs(GeodeticCRS::nn_make_shared<GeodeticCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the datum::GeodeticReferenceFrame associated with the CRS.
 *
 * @return a GeodeticReferenceFrame or null (in which case datumEnsemble()
 * should return a non-null pointer.)
 */
const datum::GeodeticReferenceFramePtr &GeodeticCRS::datum() PROJ_CONST_DEFN {
    return d->datum_;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static datum::GeodeticReferenceFrame *oneDatum(const GeodeticCRS *crs) {
    const auto &l_datumEnsemble = crs->datumEnsemble();
    assert(l_datumEnsemble);
    const auto &l_datums = l_datumEnsemble->datums();
    return static_cast<datum::GeodeticReferenceFrame *>(l_datums[0].get());
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the PrimeMeridian associated with the GeodeticReferenceFrame
 * or with one of the GeodeticReferenceFrame of the datumEnsemble().
 *
 * @return the PrimeMeridian.
 */
const datum::PrimeMeridianNNPtr &GeodeticCRS::primeMeridian() PROJ_CONST_DEFN {
    if (d->datum_) {
        return d->datum_->primeMeridian();
    }
    return oneDatum(this)->primeMeridian();
}

// ---------------------------------------------------------------------------

/** \brief Return the ellipsoid associated with the GeodeticReferenceFrame
 * or with one of the GeodeticReferenceFrame of the datumEnsemble().
 *
 * @return the PrimeMeridian.
 */
const datum::EllipsoidNNPtr &GeodeticCRS::ellipsoid() PROJ_CONST_DEFN {
    if (d->datum_) {
        return d->datum_->ellipsoid();
    }
    return oneDatum(this)->ellipsoid();
}

// ---------------------------------------------------------------------------

/** \brief Return the velocity model associated with the CRS.
 *
 * @return a velocity model. might be null.
 */
const std::vector<operation::PointMotionOperationNNPtr> &
GeodeticCRS::velocityModel() PROJ_CONST_DEFN {
    return d->velocityModel;
}

// ---------------------------------------------------------------------------

/** \brief Return whether the CRS is a geocentric one.
 *
 * A geocentric CRS is a geodetic CRS that has a Cartesian coordinate system
 * with three axis, whose direction is respectively
 * cs::AxisDirection::GEOCENTRIC_X,
 * cs::AxisDirection::GEOCENTRIC_Y and cs::AxisDirection::GEOCENTRIC_Z.
 *
 * @return true if the CRS is a geocentric CRS.
 */
bool GeodeticCRS::isGeocentric() PROJ_CONST_DEFN {
    const auto &cs = coordinateSystem();
    const auto &axisList = cs->axisList();
    return axisList.size() == 3 &&
           dynamic_cast<cs::CartesianCS *>(cs.get()) != nullptr &&
           &axisList[0]->direction() == &cs::AxisDirection::GEOCENTRIC_X &&
           &axisList[1]->direction() == &cs::AxisDirection::GEOCENTRIC_Y &&
           &axisList[2]->direction() == &cs::AxisDirection::GEOCENTRIC_Z;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeodeticCRS from a datum::GeodeticReferenceFrame and a
 * cs::SphericalCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datum The datum of the CRS.
 * @param cs a SphericalCS.
 * @return new GeodeticCRS.
 */
GeodeticCRSNNPtr
GeodeticCRS::create(const util::PropertyMap &properties,
                    const datum::GeodeticReferenceFrameNNPtr &datum,
                    const cs::SphericalCSNNPtr &cs) {
    return create(properties, datum.as_nullable(), nullptr, cs);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeodeticCRS from a datum::GeodeticReferenceFrame or
 * datum::DatumEnsemble and a cs::SphericalCS.
 *
 * One and only one of datum or datumEnsemble should be set to a non-null value.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datum The datum of the CRS, or nullptr
 * @param datumEnsemble The datum ensemble of the CRS, or nullptr.
 * @param cs a SphericalCS.
 * @return new GeodeticCRS.
 */
GeodeticCRSNNPtr
GeodeticCRS::create(const util::PropertyMap &properties,
                    const datum::GeodeticReferenceFramePtr &datum,
                    const datum::DatumEnsemblePtr &datumEnsemble,
                    const cs::SphericalCSNNPtr &cs) {
    auto crs(
        GeodeticCRS::nn_make_shared<GeodeticCRS>(datum, datumEnsemble, cs));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeodeticCRS from a datum::GeodeticReferenceFrame and a
 * cs::CartesianCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datum The datum of the CRS.
 * @param cs a CartesianCS.
 * @return new GeodeticCRS.
 */
GeodeticCRSNNPtr
GeodeticCRS::create(const util::PropertyMap &properties,
                    const datum::GeodeticReferenceFrameNNPtr &datum,
                    const cs::CartesianCSNNPtr &cs) {
    return create(properties, datum.as_nullable(), nullptr, cs);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeodeticCRS from a datum::GeodeticReferenceFrame or
 * datum::DatumEnsemble and a cs::CartesianCS.
 *
 * One and only one of datum or datumEnsemble should be set to a non-null value.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datum The datum of the CRS, or nullptr
 * @param datumEnsemble The datum ensemble of the CRS, or nullptr.
 * @param cs a CartesianCS
 * @return new GeodeticCRS.
 */
GeodeticCRSNNPtr
GeodeticCRS::create(const util::PropertyMap &properties,
                    const datum::GeodeticReferenceFramePtr &datum,
                    const datum::DatumEnsemblePtr &datumEnsemble,
                    const cs::CartesianCSNNPtr &cs) {
    auto crs(
        GeodeticCRS::nn_make_shared<GeodeticCRS>(datum, datumEnsemble, cs));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void GeodeticCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    formatter->startNode(isWKT2 ? ((formatter->use2018Keywords() &&
                                    dynamic_cast<const GeographicCRS *>(this))
                                       ? io::WKTConstants::GEOGCRS
                                       : io::WKTConstants::GEODCRS)
                                : isGeocentric() ? io::WKTConstants::GEOCCS
                                                 : io::WKTConstants::GEOGCS,
                         !identifiers().empty());
    auto l_name = nameStr();
    const auto &cs = coordinateSystem();
    const auto &axisList = cs->axisList();

    if (formatter->useESRIDialect()) {
        if (axisList.size() != 2) {
            io::FormattingException::Throw(
                "Only export of Geographic 2D CRS is supported in ESRI_WKT1");
        }

        if (l_name == "WGS 84") {
            l_name = "GCS_WGS_1984";
        } else {
            bool aliasFound = false;
            const auto &dbContext = formatter->databaseContext();
            if (dbContext) {
                auto l_alias = dbContext->getAliasFromOfficialName(
                    l_name, "geodetic_crs", "ESRI");
                if (!l_alias.empty()) {
                    l_name = l_alias;
                    aliasFound = true;
                }
            }
            if (!aliasFound) {
                l_name = io::WKTFormatter::morphNameToESRI(l_name);
                if (!starts_with(l_name, "GCS_")) {
                    l_name = "GCS_" + l_name;
                }
            }
        }
    }
    formatter->addQuotedString(l_name);

    const auto &unit = axisList[0]->unit();
    formatter->pushAxisAngularUnit(common::UnitOfMeasure::create(unit));
    exportDatumOrDatumEnsembleToWkt(formatter);
    primeMeridian()->_exportToWKT(formatter);
    formatter->popAxisAngularUnit();
    if (!isWKT2) {
        unit._exportToWKT(formatter);
    }
    cs->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void GeodeticCRS::addGeocentricUnitConversionIntoPROJString(
    io::PROJStringFormatter *formatter) const {

    const auto &axisList = coordinateSystem()->axisList();
    const auto &unit = axisList[0]->unit();
    if (unit != common::UnitOfMeasure::METRE) {
        if (formatter->convention() ==
            io::PROJStringFormatter::Convention::PROJ_4) {
            io::FormattingException::Throw("GeodeticCRS::exportToPROJString(): "
                                           "non-meter unit not supported for "
                                           "PROJ.4");
        }

        formatter->addStep("unitconvert");
        formatter->addParam("xy_in", "m");
        formatter->addParam("z_in", "m");
        {
            auto projUnit = unit.exportToPROJString();
            if (!projUnit.empty()) {
                formatter->addParam("xy_out", projUnit);
                formatter->addParam("z_out", projUnit);
                return;
            }
        }

        const auto &toSI = unit.conversionToSI();
        formatter->addParam("xy_out", toSI);
        formatter->addParam("z_out", toSI);
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void GeodeticCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    if (!isGeocentric()) {
        io::FormattingException::Throw(
            "GeodeticCRS::exportToPROJString() only "
            "supports geocentric coordinate systems");
    }

    if (formatter->convention() ==
        io::PROJStringFormatter::Convention::PROJ_4) {
        formatter->addStep("geocent");
    } else {
        formatter->addStep("cart");
    }
    ellipsoid()->_exportToPROJString(formatter);
    if (formatter->convention() ==
        io::PROJStringFormatter::Convention::PROJ_4) {
        const auto &TOWGS84Params = formatter->getTOWGS84Parameters();
        if (TOWGS84Params.size() == 7) {
            formatter->addParam("towgs84", TOWGS84Params);
        }
    }
    addGeocentricUnitConversionIntoPROJString(formatter);
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void GeodeticCRS::addDatumInfoToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    const auto &TOWGS84Params = formatter->getTOWGS84Parameters();
    bool datumWritten = false;
    const auto &nadgrids = formatter->getHDatumExtension();
    const auto &l_datum = datum();
    if (formatter->convention() ==
            io::PROJStringFormatter::Convention::PROJ_4 &&
        l_datum && TOWGS84Params.empty() && nadgrids.empty()) {
        if (l_datum->isEquivalentTo(
                datum::GeodeticReferenceFrame::EPSG_6326.get(),
                util::IComparable::Criterion::EQUIVALENT)) {
            datumWritten = true;
            formatter->addParam("datum", "WGS84");
        } else if (l_datum->isEquivalentTo(
                       datum::GeodeticReferenceFrame::EPSG_6267.get(),
                       util::IComparable::Criterion::EQUIVALENT)) {
            datumWritten = true;
            formatter->addParam("datum", "NAD27");
        } else if (l_datum->isEquivalentTo(
                       datum::GeodeticReferenceFrame::EPSG_6269.get(),
                       util::IComparable::Criterion::EQUIVALENT)) {
            datumWritten = true;
            formatter->addParam("datum", "NAD83");
        }
    }
    if (!datumWritten) {
        ellipsoid()->_exportToPROJString(formatter);
        primeMeridian()->_exportToPROJString(formatter);
    }
    if (TOWGS84Params.size() == 7) {
        formatter->addParam("towgs84", TOWGS84Params);
    }
    if (!nadgrids.empty()) {
        formatter->addParam("nadgrids", nadgrids);
    }
}
//! @endcond

// ---------------------------------------------------------------------------

bool GeodeticCRS::isEquivalentTo(const util::IComparable *other,
                                 util::IComparable::Criterion criterion) const {
    auto otherGeodCRS = dynamic_cast<const GeodeticCRS *>(other);
    // TODO test velocityModel
    return otherGeodCRS != nullptr &&
           SingleCRS::_isEquivalentTo(other, criterion);
}
// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static util::PropertyMap createMapNameEPSGCode(const char *name, int code) {
    return util::PropertyMap()
        .set(common::IdentifiedObject::NAME_KEY, name)
        .set(metadata::Identifier::CODESPACE_KEY, metadata::Identifier::EPSG)
        .set(metadata::Identifier::CODE_KEY, code);
}
//! @endcond

// ---------------------------------------------------------------------------

GeodeticCRSNNPtr GeodeticCRS::createEPSG_4978() {
    return create(
        createMapNameEPSGCode("WGS 84", 4978),
        datum::GeodeticReferenceFrame::EPSG_6326,
        cs::CartesianCS::createGeocentric(common::UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct GeographicCRS::Private {
    cs::EllipsoidalCSNNPtr coordinateSystem_;
    explicit Private(const cs::EllipsoidalCSNNPtr &csIn)
        : coordinateSystem_(csIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

GeographicCRS::GeographicCRS(const datum::GeodeticReferenceFramePtr &datumIn,
                             const datum::DatumEnsemblePtr &datumEnsembleIn,
                             const cs::EllipsoidalCSNNPtr &csIn)
    : SingleCRS(datumIn, datumEnsembleIn, csIn),
      GeodeticCRS(datumIn,
                  checkEnsembleForGeodeticCRS(datumIn, datumEnsembleIn), csIn),
      d(internal::make_unique<Private>(csIn)) {}

// ---------------------------------------------------------------------------

GeographicCRS::GeographicCRS(const GeographicCRS &other)
    : SingleCRS(other), GeodeticCRS(other),
      d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
GeographicCRS::~GeographicCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

CRSNNPtr GeographicCRS::shallowClone() const {
    auto crs(GeographicCRS::nn_make_shared<GeographicCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the cs::EllipsoidalCS associated with the CRS.
 *
 * @return a EllipsoidalCS.
 */
const cs::EllipsoidalCSNNPtr &
GeographicCRS::coordinateSystem() PROJ_CONST_DEFN {
    return d->coordinateSystem_;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeographicCRS from a datum::GeodeticReferenceFrameNNPtr
 * and a
 * cs::EllipsoidalCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datum The datum of the CRS.
 * @param cs a EllipsoidalCS.
 * @return new GeographicCRS.
 */
GeographicCRSNNPtr
GeographicCRS::create(const util::PropertyMap &properties,
                      const datum::GeodeticReferenceFrameNNPtr &datum,
                      const cs::EllipsoidalCSNNPtr &cs) {
    return create(properties, datum.as_nullable(), nullptr, cs);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a GeographicCRS from a datum::GeodeticReferenceFramePtr
 * or
 * datum::DatumEnsemble and a
 * cs::EllipsoidalCS.
 *
 * One and only one of datum or datumEnsemble should be set to a non-null value.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datum The datum of the CRS, or nullptr
 * @param datumEnsemble The datum ensemble of the CRS, or nullptr.
 * @param cs a EllipsoidalCS.
 * @return new GeographicCRS.
 */
GeographicCRSNNPtr
GeographicCRS::create(const util::PropertyMap &properties,
                      const datum::GeodeticReferenceFramePtr &datum,
                      const datum::DatumEnsemblePtr &datumEnsemble,
                      const cs::EllipsoidalCSNNPtr &cs) {
    GeographicCRSNNPtr crs(
        GeographicCRS::nn_make_shared<GeographicCRS>(datum, datumEnsemble, cs));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

/** \brief Return whether the current GeographicCRS is the 2D part of the
 * other 3D GeographicCRS.
 */
bool GeographicCRS::is2DPartOf3D(util::nn<const GeographicCRS *> other)
    PROJ_CONST_DEFN {
    const auto &axis = d->coordinateSystem_->axisList();
    const auto &otherAxis = other->d->coordinateSystem_->axisList();
    if (!(axis.size() == 2 && otherAxis.size() == 3)) {
        return false;
    }
    const auto &firstAxis = axis[0];
    const auto &secondAxis = axis[1];
    const auto &otherFirstAxis = otherAxis[0];
    const auto &otherSecondAxis = otherAxis[1];
    if (!(firstAxis->isEquivalentTo(otherFirstAxis.get()) &&
          secondAxis->isEquivalentTo(otherSecondAxis.get()))) {
        return false;
    }
    const auto &thisDatum = GeodeticCRS::getPrivate()->datum_;
    const auto &otherDatum = other->GeodeticCRS::getPrivate()->datum_;
    if (thisDatum && otherDatum) {
        return thisDatum->isEquivalentTo(otherDatum.get());
    }
    return false;
}

//! @endcond

// ---------------------------------------------------------------------------

GeographicCRSNNPtr GeographicCRS::createEPSG_4267() {
    return create(createMapNameEPSGCode("NAD27", 4267),
                  datum::GeodeticReferenceFrame::EPSG_6267,
                  cs::EllipsoidalCS::createLatitudeLongitude(
                      common::UnitOfMeasure::DEGREE));
}

// ---------------------------------------------------------------------------

GeographicCRSNNPtr GeographicCRS::createEPSG_4269() {
    return create(createMapNameEPSGCode("NAD83", 4269),
                  datum::GeodeticReferenceFrame::EPSG_6269,
                  cs::EllipsoidalCS::createLatitudeLongitude(
                      common::UnitOfMeasure::DEGREE));
}

// ---------------------------------------------------------------------------

GeographicCRSNNPtr GeographicCRS::createEPSG_4326() {
    return create(createMapNameEPSGCode("WGS 84", 4326),
                  datum::GeodeticReferenceFrame::EPSG_6326,
                  cs::EllipsoidalCS::createLatitudeLongitude(
                      common::UnitOfMeasure::DEGREE));
}

// ---------------------------------------------------------------------------

GeographicCRSNNPtr GeographicCRS::createOGC_CRS84() {
    util::PropertyMap propertiesCRS;
    propertiesCRS
        .set(metadata::Identifier::CODESPACE_KEY, metadata::Identifier::OGC)
        .set(metadata::Identifier::CODE_KEY, "CRS84")
        .set(common::IdentifiedObject::NAME_KEY, "WGS 84");
    return create(propertiesCRS, datum::GeodeticReferenceFrame::EPSG_6326,
                  cs::EllipsoidalCS::createLongitudeLatitude( // Long Lat !
                      common::UnitOfMeasure::DEGREE));
}

// ---------------------------------------------------------------------------

GeographicCRSNNPtr GeographicCRS::createEPSG_4979() {
    return create(
        createMapNameEPSGCode("WGS 84", 4979),
        datum::GeodeticReferenceFrame::EPSG_6326,
        cs::EllipsoidalCS::createLatitudeLongitudeEllipsoidalHeight(
            common::UnitOfMeasure::DEGREE, common::UnitOfMeasure::METRE));
}

// ---------------------------------------------------------------------------

GeographicCRSNNPtr GeographicCRS::createEPSG_4807() {
    auto ellps(datum::Ellipsoid::createFlattenedSphere(
        createMapNameEPSGCode("Clarke 1880 (IGN)", 6807),
        common::Length(6378249.2), common::Scale(293.4660212936269)));

    auto axisLat(cs::CoordinateSystemAxis::create(
        util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                cs::AxisName::Latitude),
        cs::AxisAbbreviation::lat, cs::AxisDirection::NORTH,
        common::UnitOfMeasure::GRAD));

    auto axisLong(cs::CoordinateSystemAxis::create(
        util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                cs::AxisName::Longitude),
        cs::AxisAbbreviation::lon, cs::AxisDirection::EAST,
        common::UnitOfMeasure::GRAD));

    auto cs(cs::EllipsoidalCS::create(util::PropertyMap(), axisLat, axisLong));

    auto datum(datum::GeodeticReferenceFrame::create(
        createMapNameEPSGCode("Nouvelle Triangulation Francaise (Paris)", 6807),
        ellps, util::optional<std::string>(), datum::PrimeMeridian::PARIS));

    auto gcrs(create(createMapNameEPSGCode("NTF (Paris)", 4807), datum, cs));
    return gcrs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void GeographicCRS::addAngularUnitConvertAndAxisSwap(
    io::PROJStringFormatter *formatter) const {
    const auto &axisList = coordinateSystem()->axisList();

    if (formatter->convention() ==
        io::PROJStringFormatter::Convention::PROJ_5) {

        formatter->addStep("unitconvert");
        formatter->addParam("xy_in", "rad");
        if (axisList.size() == 3 && !formatter->omitZUnitConversion()) {
            formatter->addParam("z_in", "m");
        }
        {
            const auto &unitHoriz = axisList[0]->unit();
            const auto projUnit = unitHoriz.exportToPROJString();
            if (projUnit.empty()) {
                formatter->addParam("xy_out", unitHoriz.conversionToSI());
            } else {
                formatter->addParam("xy_out", projUnit);
            }
        }
        if (axisList.size() == 3 && !formatter->omitZUnitConversion()) {
            const auto &unitZ = axisList[2]->unit();
            auto projVUnit = unitZ.exportToPROJString();
            if (projVUnit.empty()) {
                formatter->addParam("z_out", unitZ.conversionToSI());
            } else {
                formatter->addParam("z_out", projVUnit);
            }
        }

        const char *order[2] = {nullptr, nullptr};
        const char *one = "1";
        const char *two = "2";
        for (int i = 0; i < 2; i++) {
            const auto &dir = axisList[i]->direction();
            if (&dir == &cs::AxisDirection::WEST) {
                order[i] = "-1";
            } else if (&dir == &cs::AxisDirection::EAST) {
                order[i] = one;
            } else if (&dir == &cs::AxisDirection::SOUTH) {
                order[i] = "-2";
            } else if (&dir == &cs::AxisDirection::NORTH) {
                order[i] = two;
            }
        }
        if (order[0] && order[1] && (order[0] != one || order[1] != two)) {
            formatter->addStep("axisswap");
            char orderStr[10];
            strcpy(orderStr, order[0]);
            strcat(orderStr, ",");
            strcat(orderStr, order[1]);
            formatter->addParam("order", orderStr);
        }
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void GeographicCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    if (!formatter->omitProjLongLatIfPossible() ||
        primeMeridian()->longitude().getSIValue() != 0.0 ||
        !formatter->getTOWGS84Parameters().empty() ||
        !formatter->getHDatumExtension().empty()) {
        formatter->addStep("longlat");
        addDatumInfoToPROJString(formatter);
    }

    addAngularUnitConvertAndAxisSwap(formatter);
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct VerticalCRS::Private {
    std::vector<operation::TransformationNNPtr> geoidModel{};
    std::vector<operation::PointMotionOperationNNPtr> velocityModel{};
};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const datum::DatumEnsemblePtr &
checkEnsembleForVerticalCRS(const datum::VerticalReferenceFramePtr &datumIn,
                            const datum::DatumEnsemblePtr &ensemble) {
    const char *msg = "One of Datum or DatumEnsemble should be defined";
    if (datumIn) {
        if (!ensemble) {
            return ensemble;
        }
        msg = "Datum and DatumEnsemble should not be defined";
    } else if (ensemble) {
        const auto &datums = ensemble->datums();
        assert(!datums.empty());
        auto grfFirst =
            dynamic_cast<datum::VerticalReferenceFrame *>(datums[0].get());
        if (grfFirst) {
            return ensemble;
        }
        msg = "Ensemble should contain VerticalReferenceFrame";
    }
    throw util::Exception(msg);
}
//! @endcond

// ---------------------------------------------------------------------------

VerticalCRS::VerticalCRS(const datum::VerticalReferenceFramePtr &datumIn,
                         const datum::DatumEnsemblePtr &datumEnsembleIn,
                         const cs::VerticalCSNNPtr &csIn)
    : SingleCRS(datumIn, checkEnsembleForVerticalCRS(datumIn, datumEnsembleIn),
                csIn),
      d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

VerticalCRS::VerticalCRS(const VerticalCRS &other)
    : SingleCRS(other), d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
VerticalCRS::~VerticalCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

CRSNNPtr VerticalCRS::shallowClone() const {
    auto crs(VerticalCRS::nn_make_shared<VerticalCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the datum::VerticalReferenceFrame associated with the CRS.
 *
 * @return a VerticalReferenceFrame.
 */
const datum::VerticalReferenceFramePtr VerticalCRS::datum() const {
    return std::static_pointer_cast<datum::VerticalReferenceFrame>(
        SingleCRS::getPrivate()->datum);
}

// ---------------------------------------------------------------------------

/** \brief Return the geoid model associated with the CRS.
 *
 * Geoid height model or height correction model linked to a geoid-based
 * vertical CRS.
 *
 * @return a geoid model. might be null
 */
const std::vector<operation::TransformationNNPtr> &
VerticalCRS::geoidModel() PROJ_CONST_DEFN {
    return d->geoidModel;
}

// ---------------------------------------------------------------------------

/** \brief Return the velocity model associated with the CRS.
 *
 * @return a velocity model. might be null.
 */
const std::vector<operation::PointMotionOperationNNPtr> &
VerticalCRS::velocityModel() PROJ_CONST_DEFN {
    return d->velocityModel;
}

// ---------------------------------------------------------------------------

/** \brief Return the cs::VerticalCS associated with the CRS.
 *
 * @return a VerticalCS.
 */
const cs::VerticalCSNNPtr VerticalCRS::coordinateSystem() const {
    return util::nn_static_pointer_cast<cs::VerticalCS>(
        SingleCRS::getPrivate()->coordinateSystem);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void VerticalCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    formatter->startNode(isWKT2 ? io::WKTConstants::VERTCRS
                                : io::WKTConstants::VERT_CS,
                         !identifiers().empty());
    formatter->addQuotedString(nameStr());
    exportDatumOrDatumEnsembleToWkt(formatter);
    const auto &cs = SingleCRS::getPrivate()->coordinateSystem;
    const auto &axisList = cs->axisList();
    if (!isWKT2) {
        axisList[0]->unit()._exportToWKT(formatter);
    }
    cs->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

void VerticalCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    auto geoidgrids = formatter->getVDatumExtension();
    if (!geoidgrids.empty()) {
        formatter->addParam("geoidgrids", geoidgrids);
    }

    auto &axisList = coordinateSystem()->axisList();
    if (!axisList.empty()) {
        auto projUnit = axisList[0]->unit().exportToPROJString();
        if (projUnit.empty()) {
            formatter->addParam("vto_meter",
                                axisList[0]->unit().conversionToSI());
        } else {
            formatter->addParam("vunits", projUnit);
        }
    }
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void VerticalCRS::addLinearUnitConvert(
    io::PROJStringFormatter *formatter) const {
    auto &axisList = coordinateSystem()->axisList();

    if (formatter->convention() ==
        io::PROJStringFormatter::Convention::PROJ_5) {
        if (!axisList.empty()) {
            auto projUnit = axisList[0]->unit().exportToPROJString();
            if (axisList[0]->unit().conversionToSI() != 1.0) {
                formatter->addStep("unitconvert");
                formatter->addParam("z_in", "m");
                auto projVUnit = axisList[0]->unit().exportToPROJString();
                if (projVUnit.empty()) {
                    formatter->addParam("z_out",
                                        axisList[0]->unit().conversionToSI());
                } else {
                    formatter->addParam("z_out", projVUnit);
                }
            }
        }
    }
}
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Instanciate a VerticalCRS from a datum::VerticalReferenceFrame and a
 * cs::VerticalCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datumIn The datum of the CRS.
 * @param csIn a VerticalCS.
 * @return new VerticalCRS.
 */
VerticalCRSNNPtr
VerticalCRS::create(const util::PropertyMap &properties,
                    const datum::VerticalReferenceFrameNNPtr &datumIn,
                    const cs::VerticalCSNNPtr &csIn) {
    return create(properties, datumIn.as_nullable(), nullptr, csIn);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a VerticalCRS from a datum::VerticalReferenceFrame or
 * datum::DatumEnsemble and a cs::VerticalCS.
 *
 * One and only one of datum or datumEnsemble should be set to a non-null value.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datumIn The datum of the CRS, or nullptr
 * @param datumEnsembleIn The datum ensemble of the CRS, or nullptr.
 * @param csIn a VerticalCS.
 * @return new VerticalCRS.
 */
VerticalCRSNNPtr
VerticalCRS::create(const util::PropertyMap &properties,
                    const datum::VerticalReferenceFramePtr &datumIn,
                    const datum::DatumEnsemblePtr &datumEnsembleIn,
                    const cs::VerticalCSNNPtr &csIn) {
    auto crs(VerticalCRS::nn_make_shared<VerticalCRS>(datumIn, datumEnsembleIn,
                                                      csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

bool VerticalCRS::isEquivalentTo(const util::IComparable *other,
                                 util::IComparable::Criterion criterion) const {
    auto otherVertCRS = dynamic_cast<const VerticalCRS *>(other);
    // TODO test geoidModel and velocityModel
    return otherVertCRS != nullptr &&
           SingleCRS::_isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct DerivedCRS::Private {
    SingleCRSNNPtr baseCRS_;
    operation::ConversionNNPtr derivingConversion_;

    Private(const SingleCRSNNPtr &baseCRSIn,
            const operation::ConversionNNPtr &derivingConversionIn)
        : baseCRS_(baseCRSIn), derivingConversion_(derivingConversionIn) {}

    // For the conversion make a shallowClone(), so that we can later set
    // its targetCRS to this.
    Private(const Private &other)
        : baseCRS_(other.baseCRS_),
          derivingConversion_(other.derivingConversion_->shallowClone()) {}
};

//! @endcond

// ---------------------------------------------------------------------------

// DerivedCRS is an abstract class, that virtually inherits from SingleCRS
// Consequently the base constructor in SingleCRS will never be called by
// that constructor. clang -Wabstract-vbase-init and VC++ underline this, but
// other
// compilers will complain if we don't call the base constructor.

DerivedCRS::DerivedCRS(const SingleCRSNNPtr &baseCRSIn,
                       const operation::ConversionNNPtr &derivingConversionIn,
                       const cs::CoordinateSystemNNPtr &
#if !defined(COMPILER_WARNS_ABOUT_ABSTRACT_VBASE_INIT)
                           cs
#endif
                       )
    :
#if !defined(COMPILER_WARNS_ABOUT_ABSTRACT_VBASE_INIT)
      SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), cs),
#endif
      d(internal::make_unique<Private>(baseCRSIn, derivingConversionIn)) {
}

// ---------------------------------------------------------------------------

DerivedCRS::DerivedCRS(const DerivedCRS &other)
    :
#if !defined(COMPILER_WARNS_ABOUT_ABSTRACT_VBASE_INIT)
      SingleCRS(other),
#endif
      d(internal::make_unique<Private>(*other.d)) {
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
DerivedCRS::~DerivedCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

/** \brief Return the base CRS of a DerivedCRS.
 *
 * @return the base CRS.
 */
const SingleCRSNNPtr &DerivedCRS::baseCRS() PROJ_CONST_DEFN {
    return d->baseCRS_;
}

// ---------------------------------------------------------------------------

/** \brief Return the deriving conversion from the base CRS to this CRS.
 *
 * @return the deriving conversion.
 */
const operation::ConversionNNPtr DerivedCRS::derivingConversion() const {
    return d->derivingConversion_->shallowClone();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
const operation::ConversionNNPtr &
DerivedCRS::derivingConversionRef() PROJ_CONST_DEFN {
    return d->derivingConversion_;
}
//! @endcond

// ---------------------------------------------------------------------------

bool DerivedCRS::isEquivalentTo(const util::IComparable *other,
                                util::IComparable::Criterion criterion) const {
    auto otherDerivedCRS = dynamic_cast<const DerivedCRS *>(other);
    if (otherDerivedCRS == nullptr ||
        !SingleCRS::_isEquivalentTo(other, criterion)) {
        return false;
    }
    return d->baseCRS_->isEquivalentTo(otherDerivedCRS->d->baseCRS_.get(),
                                       criterion) &&
           d->derivingConversion_->isEquivalentTo(
               otherDerivedCRS->d->derivingConversion_.get(), criterion);
}

// ---------------------------------------------------------------------------

void DerivedCRS::setDerivingConversionCRS() {
    derivingConversionRef()->setWeakSourceTargetCRS(
        baseCRS().as_nullable(),
        std::static_pointer_cast<CRS>(shared_from_this().as_nullable()));
}

// ---------------------------------------------------------------------------

void DerivedCRS::baseExportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    d->derivingConversion_->_exportToPROJString(formatter);
}

// ---------------------------------------------------------------------------

void DerivedCRS::baseExportToWKT(io::WKTFormatter *&formatter,
                                 const std::string &keyword,
                                 const std::string &baseKeyword) const {
    formatter->startNode(keyword, !identifiers().empty());
    formatter->addQuotedString(nameStr());

    const auto &l_baseCRS = d->baseCRS_;
    formatter->startNode(baseKeyword, !l_baseCRS->identifiers().empty());
    formatter->addQuotedString(l_baseCRS->nameStr());
    l_baseCRS->exportDatumOrDatumEnsembleToWkt(formatter);
    formatter->endNode();

    formatter->setUseDerivingConversion(true);
    derivingConversionRef()->_exportToWKT(formatter);
    formatter->setUseDerivingConversion(false);

    coordinateSystem()->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct ProjectedCRS::Private {
    GeodeticCRSNNPtr baseCRS_;
    cs::CartesianCSNNPtr cs_;
    Private(const GeodeticCRSNNPtr &baseCRSIn, const cs::CartesianCSNNPtr &csIn)
        : baseCRS_(baseCRSIn), cs_(csIn) {}

    inline const GeodeticCRSNNPtr &baseCRS() const { return baseCRS_; }

    inline const cs::CartesianCSNNPtr &coordinateSystem() const { return cs_; }
};
//! @endcond

// ---------------------------------------------------------------------------

ProjectedCRS::ProjectedCRS(
    const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::CartesianCSNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn),
      d(internal::make_unique<Private>(baseCRSIn, csIn)) {}

// ---------------------------------------------------------------------------

ProjectedCRS::ProjectedCRS(const ProjectedCRS &other)
    : SingleCRS(other), DerivedCRS(other),
      d(internal::make_unique<Private>(other.baseCRS(),
                                       other.coordinateSystem())) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
ProjectedCRS::~ProjectedCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

CRSNNPtr ProjectedCRS::shallowClone() const {
    auto crs(ProjectedCRS::nn_make_shared<ProjectedCRS>(*this));
    crs->assignSelf(crs);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS (a GeodeticCRS, which is generally a
 * GeographicCRS) of the ProjectedCRS.
 *
 * @return the base CRS.
 */
const GeodeticCRSNNPtr &ProjectedCRS::baseCRS() PROJ_CONST_DEFN {
    return d->baseCRS();
}

// ---------------------------------------------------------------------------

/** \brief Return the cs::CartesianCS associated with the CRS.
 *
 * @return a CartesianCS
 */
const cs::CartesianCSNNPtr &ProjectedCRS::coordinateSystem() PROJ_CONST_DEFN {
    return d->coordinateSystem();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void ProjectedCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;

    if (!isWKT2 && !formatter->useESRIDialect() &&
        starts_with(nameStr(), "Popular Visualisation CRS / Mercator")) {
        formatter->startNode(io::WKTConstants::PROJCS, !identifiers().empty());
        formatter->addQuotedString(nameStr());
        formatter->setTOWGS84Parameters({0, 0, 0, 0, 0, 0, 0});
        baseCRS()->_exportToWKT(formatter);
        formatter->setTOWGS84Parameters({});

        formatter->startNode(io::WKTConstants::PROJECTION, false);
        formatter->addQuotedString("Mercator_1SP");
        formatter->endNode();

        formatter->startNode(io::WKTConstants::PARAMETER, false);
        formatter->addQuotedString("central_meridian");
        formatter->add(0.0);
        formatter->endNode();

        formatter->startNode(io::WKTConstants::PARAMETER, false);
        formatter->addQuotedString("scale_factor");
        formatter->add(1.0);
        formatter->endNode();

        formatter->startNode(io::WKTConstants::PARAMETER, false);
        formatter->addQuotedString("false_easting");
        formatter->add(0.0);
        formatter->endNode();

        formatter->startNode(io::WKTConstants::PARAMETER, false);
        formatter->addQuotedString("false_northing");
        formatter->add(0.0);
        formatter->endNode();

        const auto &axisList = d->coordinateSystem()->axisList();
        axisList[0]->unit()._exportToWKT(formatter);
        d->coordinateSystem()->_exportToWKT(formatter);
        derivingConversionRef()->addWKTExtensionNode(formatter);
        ObjectUsage::baseExportToWKT(formatter);
        formatter->endNode();
        return;
    }

    formatter->startNode(isWKT2 ? io::WKTConstants::PROJCRS
                                : io::WKTConstants::PROJCS,
                         !identifiers().empty());
    auto l_name = nameStr();
    if (formatter->useESRIDialect()) {
        bool aliasFound = false;
        const auto &dbContext = formatter->databaseContext();
        if (dbContext) {
            auto l_alias = dbContext->getAliasFromOfficialName(
                l_name, "projected_crs", "ESRI");
            if (!l_alias.empty()) {
                l_name = l_alias;
                aliasFound = true;
            }
        }
        if (!aliasFound) {
            l_name = io::WKTFormatter::morphNameToESRI(l_name);
        }
    }
    formatter->addQuotedString(l_name);

    const auto &l_baseCRS = d->baseCRS();
    const auto &geodeticCRSAxisList = l_baseCRS->coordinateSystem()->axisList();

    if (isWKT2) {
        formatter->startNode(
            (formatter->use2018Keywords() &&
             dynamic_cast<const GeographicCRS *>(l_baseCRS.get()))
                ? io::WKTConstants::BASEGEOGCRS
                : io::WKTConstants::BASEGEODCRS,
            !l_baseCRS->identifiers().empty());
        formatter->addQuotedString(l_baseCRS->nameStr());
        l_baseCRS->exportDatumOrDatumEnsembleToWkt(formatter);
        // insert ellipsoidal cs unit when the units of the map
        // projection angular parameters are not explicitly given within those
        // parameters. See
        // http://docs.opengeospatial.org/is/12-063r5/12-063r5.html#61
        if (formatter->primeMeridianOrParameterUnitOmittedIfSameAsAxis()) {
            geodeticCRSAxisList[0]->unit()._exportToWKT(formatter);
        }
        l_baseCRS->primeMeridian()->_exportToWKT(formatter);
        formatter->endNode();
    } else {
        l_baseCRS->_exportToWKT(formatter);
    }

    const auto &axisList = d->coordinateSystem()->axisList();
    formatter->pushAxisLinearUnit(
        common::UnitOfMeasure::create(axisList[0]->unit()));

    formatter->pushAxisAngularUnit(
        common::UnitOfMeasure::create(geodeticCRSAxisList[0]->unit()));

    derivingConversionRef()->_exportToWKT(formatter);

    formatter->popAxisAngularUnit();

    formatter->popAxisLinearUnit();

    if (!isWKT2) {
        axisList[0]->unit()._exportToWKT(formatter);
    }

    d->coordinateSystem()->_exportToWKT(formatter);

    if (!isWKT2 && !formatter->useESRIDialect()) {
        derivingConversionRef()->addWKTExtensionNode(formatter);
    }

    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
    return;
}
//! @endcond

// ---------------------------------------------------------------------------

void ProjectedCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    baseExportToPROJString(formatter);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a ProjectedCRS from a base CRS, a deriving
 * operation::Conversion
 * and a coordinate system.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param baseCRSIn The base CRS, a GeodeticCRS that is generally a
 * GeographicCRS.
 * @param derivingConversionIn The deriving operation::Conversion (typically
 * using a map
 * projection method)
 * @param csIn The coordniate system.
 * @return new ProjectedCRS.
 */
ProjectedCRSNNPtr
ProjectedCRS::create(const util::PropertyMap &properties,
                     const GeodeticCRSNNPtr &baseCRSIn,
                     const operation::ConversionNNPtr &derivingConversionIn,
                     const cs::CartesianCSNNPtr &csIn) {
    auto crs = ProjectedCRS::nn_make_shared<ProjectedCRS>(
        baseCRSIn, derivingConversionIn, csIn);
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

bool ProjectedCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherProjCRS = dynamic_cast<const ProjectedCRS *>(other);
    return otherProjCRS != nullptr &&
           DerivedCRS::isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void ProjectedCRS::addUnitConvertAndAxisSwap(io::PROJStringFormatter *formatter,
                                             bool axisSpecFound) const {
    const auto &axisList = d->coordinateSystem()->axisList();
    const auto &unit = axisList[0]->unit();
    if (unit != common::UnitOfMeasure::METRE) {
        auto projUnit = unit.exportToPROJString();
        const double toSI = unit.conversionToSI();
        if (formatter->convention() ==
            io::PROJStringFormatter::Convention::PROJ_5) {
            formatter->addStep("unitconvert");
            formatter->addParam("xy_in", "m");
            formatter->addParam("z_in", "m");
            if (projUnit.empty()) {
                formatter->addParam("xy_out", toSI);
                formatter->addParam("z_out", toSI);
            } else {
                formatter->addParam("xy_out", projUnit);
                formatter->addParam("z_out", projUnit);
            }
        } else {
            if (projUnit.empty()) {
                formatter->addParam("to_meter", toSI);
            } else {
                formatter->addParam("units", projUnit);
            }
        }
    }

    if (formatter->convention() ==
            io::PROJStringFormatter::Convention::PROJ_5 &&
        !axisSpecFound) {
        const auto &dir0 = axisList[0]->direction();
        const auto &dir1 = axisList[1]->direction();
        if (!(&dir0 == &cs::AxisDirection::EAST &&
              &dir1 == &cs::AxisDirection::NORTH) &&
            // For polar projections, that have south+south direction,
            // we don't want to mess with axes.
            dir0 != dir1) {

            const char *order[2] = {nullptr, nullptr};
            for (int i = 0; i < 2; i++) {
                const auto &dir = axisList[i]->direction();
                if (&dir == &cs::AxisDirection::WEST)
                    order[i] = "-1";
                else if (&dir == &cs::AxisDirection::EAST)
                    order[i] = "1";
                else if (&dir == &cs::AxisDirection::SOUTH)
                    order[i] = "-2";
                else if (&dir == &cs::AxisDirection::NORTH)
                    order[i] = "2";
            }

            if (order[0] && order[1]) {
                formatter->addStep("axisswap");
                char orderStr[10];
                strcpy(orderStr, order[0]);
                strcat(orderStr, ",");
                strcat(orderStr, order[1]);
                formatter->addParam("order", orderStr);
            }
        } else {
            const auto &name0 = axisList[0]->nameStr();
            const auto &name1 = axisList[1]->nameStr();
            const bool northingEasting = ci_starts_with(name0, "northing") &&
                                         ci_starts_with(name1, "easting");
            // case of EPSG:32661 ["WGS 84 / UPS North (N,E)]"
            // case of EPSG:32761 ["WGS 84 / UPS South (N,E)]"
            if (((&dir0 == &cs::AxisDirection::SOUTH &&
                  &dir1 == &cs::AxisDirection::SOUTH) ||
                 (&dir0 == &cs::AxisDirection::NORTH &&
                  &dir1 == &cs::AxisDirection::NORTH)) &&
                northingEasting) {
                formatter->addStep("axisswap");
                formatter->addParam("order", "2,1");
            }
        }
    }
}
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct CompoundCRS::Private {
    std::vector<CRSNNPtr> components_{};
};
//! @endcond

// ---------------------------------------------------------------------------

CompoundCRS::CompoundCRS(const std::vector<CRSNNPtr> &components)
    : CRS(), d(internal::make_unique<Private>()) {
    d->components_ = components;
}

// ---------------------------------------------------------------------------

CompoundCRS::CompoundCRS(const CompoundCRS &other)
    : CRS(other), d(internal::make_unique<Private>(*other.d)) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
CompoundCRS::~CompoundCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

CRSNNPtr CompoundCRS::shallowClone() const {
    auto crs(CompoundCRS::nn_make_shared<CompoundCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the components of a CompoundCRS.
 *
 * @return the components.
 */
const std::vector<CRSNNPtr> &
CompoundCRS::componentReferenceSystems() PROJ_CONST_DEFN {
    return d->components_;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a CompoundCRS from a vector of CRS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param components the component CRS of the CompoundCRS.
 * @return new CompoundCRS.
 */
CompoundCRSNNPtr CompoundCRS::create(const util::PropertyMap &properties,
                                     const std::vector<CRSNNPtr> &components) {
    auto compoundCRS(CompoundCRS::nn_make_shared<CompoundCRS>(components));
    compoundCRS->assignSelf(compoundCRS);
    compoundCRS->setProperties(properties);
    if (properties.find(common::IdentifiedObject::NAME_KEY) ==
        properties.end()) {
        std::string name;
        for (const auto &crs : components) {
            if (!name.empty()) {
                name += " + ";
            }
            const auto &l_name = crs->nameStr();
            if (!l_name.empty()) {
                name += l_name;
            } else {
                name += "unnamed";
            }
        }
        util::PropertyMap propertyName;
        propertyName.set(common::IdentifiedObject::NAME_KEY, name);
        compoundCRS->setProperties(propertyName);
    }

    return compoundCRS;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void CompoundCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    formatter->startNode(isWKT2 ? io::WKTConstants::COMPOUNDCRS
                                : io::WKTConstants::COMPD_CS,
                         !identifiers().empty());
    formatter->addQuotedString(nameStr());
    for (const auto &crs : componentReferenceSystems()) {
        crs->_exportToWKT(formatter);
    }
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

void CompoundCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    for (const auto &crs : componentReferenceSystems()) {
        auto crs_exportable =
            dynamic_cast<const IPROJStringExportable *>(crs.get());
        if (crs_exportable) {
            crs_exportable->_exportToPROJString(formatter);
        }
    }
}

// ---------------------------------------------------------------------------

bool CompoundCRS::isEquivalentTo(const util::IComparable *other,
                                 util::IComparable::Criterion criterion) const {
    auto otherCompoundCRS = dynamic_cast<const CompoundCRS *>(other);
    if (otherCompoundCRS == nullptr ||
        !ObjectUsage::isEquivalentTo(other, criterion)) {
        return false;
    }
    const auto &components = componentReferenceSystems();
    const auto &otherComponents = otherCompoundCRS->componentReferenceSystems();
    if (components.size() != otherComponents.size()) {
        return false;
    }
    for (size_t i = 0; i < components.size(); i++) {
        if (!components[i]->isEquivalentTo(otherComponents[i].get(),
                                           criterion)) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct BoundCRS::Private {
    CRSNNPtr baseCRS_;
    CRSNNPtr hubCRS_;
    operation::TransformationNNPtr transformation_;

    Private(const CRSNNPtr &baseCRSIn, const CRSNNPtr &hubCRSIn,
            const operation::TransformationNNPtr &transformationIn);

    inline const CRSNNPtr &baseCRS() const { return baseCRS_; }
    inline const CRSNNPtr &hubCRS() const { return hubCRS_; }
    inline const operation::TransformationNNPtr &transformation() const {
        return transformation_;
    }
};

BoundCRS::Private::Private(
    const CRSNNPtr &baseCRSIn, const CRSNNPtr &hubCRSIn,
    const operation::TransformationNNPtr &transformationIn)
    : baseCRS_(baseCRSIn), hubCRS_(hubCRSIn),
      transformation_(transformationIn) {}

//! @endcond

// ---------------------------------------------------------------------------

BoundCRS::BoundCRS(const CRSNNPtr &baseCRSIn, const CRSNNPtr &hubCRSIn,
                   const operation::TransformationNNPtr &transformationIn)
    : d(internal::make_unique<Private>(baseCRSIn, hubCRSIn, transformationIn)) {
}

// ---------------------------------------------------------------------------

BoundCRS::BoundCRS(const BoundCRS &other)
    : CRS(other),
      d(internal::make_unique<Private>(other.d->baseCRS(), other.d->hubCRS(),
                                       other.d->transformation())) {}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
BoundCRS::~BoundCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

BoundCRSNNPtr BoundCRS::shallowCloneAsBoundCRS() const {
    auto crs(BoundCRS::nn_make_shared<BoundCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

CRSNNPtr BoundCRS::shallowClone() const { return shallowCloneAsBoundCRS(); }

// ---------------------------------------------------------------------------

/** \brief Return the base CRS.
 *
 * This is the CRS into which coordinates of the BoundCRS are expressed.
 *
 * @return the base CRS.
 */
const CRSNNPtr &BoundCRS::baseCRS() PROJ_CONST_DEFN { return d->baseCRS_; }

// ---------------------------------------------------------------------------

// The only legit caller is BoundCRS::baseCRSWithCanonicalBoundCRS()
void CRS::setCanonicalBoundCRS(const BoundCRSNNPtr &boundCRS) {

    d->canonicalBoundCRS_ = boundCRS;
}

// ---------------------------------------------------------------------------

/** \brief Return a shallow clone of the base CRS that points to a
 * shallow clone of this BoundCRS.
 *
 * The base CRS is the CRS into which coordinates of the BoundCRS are expressed.
 *
 * The returned CRS will actually be a shallow clone of the actual base CRS,
 * with the extra property that CRS::canonicalBoundCRS() will point to a
 * shallow clone of this BoundCRS. Use this only if you want to work with
 * the base CRS object rather than the BoundCRS, but wanting to be able to
 * retrieve the BoundCRS later.
 *
 * @return the base CRS.
 */
CRSNNPtr BoundCRS::baseCRSWithCanonicalBoundCRS() const {
    auto baseCRSClone = baseCRS()->shallowClone();
    baseCRSClone->setCanonicalBoundCRS(shallowCloneAsBoundCRS());
    return baseCRSClone;
}

// ---------------------------------------------------------------------------

/** \brief Return the target / hub CRS.
 *
 * @return the hub CRS.
 */
const CRSNNPtr &BoundCRS::hubCRS() PROJ_CONST_DEFN { return d->hubCRS_; }

// ---------------------------------------------------------------------------

/** \brief Return the transformation to the hub RS.
 *
 * @return transformation.
 */
const operation::TransformationNNPtr &
BoundCRS::transformation() PROJ_CONST_DEFN {
    return d->transformation_;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a BoundCRS from a base CRS, a hub CRS and a
 * transformation.
 *
 * @param baseCRSIn base CRS.
 * @param hubCRSIn hub CRS.
 * @param transformationIn transformation from base CRS to hub CRS.
 * @return new BoundCRS.
 */
BoundCRSNNPtr
BoundCRS::create(const CRSNNPtr &baseCRSIn, const CRSNNPtr &hubCRSIn,
                 const operation::TransformationNNPtr &transformationIn) {
    auto crs = BoundCRS::nn_make_shared<BoundCRS>(baseCRSIn, hubCRSIn,
                                                  transformationIn);
    crs->assignSelf(crs);
    const auto &l_name = baseCRSIn->nameStr();
    if (!l_name.empty()) {
        crs->setProperties(util::PropertyMap().set(
            common::IdentifiedObject::NAME_KEY, l_name));
    }
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a BoundCRS from a base CRS and TOWGS84 parameters
 *
 * @param baseCRSIn base CRS.
 * @param TOWGS84Parameters a vector of 3 or 7 double values representing WKT1
 * TOWGS84 parameter.
 * @return new BoundCRS.
 */
BoundCRSNNPtr
BoundCRS::createFromTOWGS84(const CRSNNPtr &baseCRSIn,
                            const std::vector<double> &TOWGS84Parameters) {
    return create(
        baseCRSIn, GeographicCRS::EPSG_4326,
        operation::Transformation::createTOWGS84(baseCRSIn, TOWGS84Parameters));
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a BoundCRS from a base CRS and nadgrids parameters
 *
 * @param baseCRSIn base CRS.
 * @param filename Horizontal grid filename
 * @return new BoundCRS.
 */
BoundCRSNNPtr BoundCRS::createFromNadgrids(const CRSNNPtr &baseCRSIn,
                                           const std::string &filename) {
    const CRSPtr sourceGeographicCRS = baseCRSIn->extractGeographicCRS();
    auto transformationSourceCRS =
        sourceGeographicCRS ? sourceGeographicCRS : baseCRSIn.as_nullable();
    std::string transformationName = transformationSourceCRS->nameStr();
    transformationName += " to WGS84";

    return create(
        baseCRSIn, GeographicCRS::EPSG_4326,
        operation::Transformation::createNTv2(
            util::PropertyMap().set(common::IdentifiedObject::NAME_KEY,
                                    transformationName),
            baseCRSIn, GeographicCRS::EPSG_4326, filename,
            std::vector<metadata::PositionalAccuracyNNPtr>()));
}

// ---------------------------------------------------------------------------

bool BoundCRS::isTOWGS84Compatible() const {
    return dynamic_cast<GeodeticCRS *>(d->hubCRS().get()) != nullptr &&
           ci_equal(d->hubCRS()->nameStr(), "WGS 84");
}

// ---------------------------------------------------------------------------

std::string BoundCRS::getHDatumPROJ4GRIDS() const {
    if (ci_equal(d->hubCRS()->nameStr(), "WGS 84")) {
        return d->transformation()->getNTv2Filename();
    }
    return std::string();
}

// ---------------------------------------------------------------------------

std::string BoundCRS::getVDatumPROJ4GRIDS() const {
    if (dynamic_cast<VerticalCRS *>(d->baseCRS().get()) &&
        ci_equal(d->hubCRS()->nameStr(), "WGS 84")) {
        return d->transformation()->getHeightToGeographic3DFilename();
    }
    return std::string();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void BoundCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (isWKT2) {
        formatter->startNode(io::WKTConstants::BOUNDCRS, false);
        formatter->startNode(io::WKTConstants::SOURCECRS, false);
        d->baseCRS()->_exportToWKT(formatter);
        formatter->endNode();
        formatter->startNode(io::WKTConstants::TARGETCRS, false);
        d->hubCRS()->_exportToWKT(formatter);
        formatter->endNode();
        formatter->setAbridgedTransformation(true);
        d->transformation()->_exportToWKT(formatter);
        formatter->setAbridgedTransformation(false);
        formatter->endNode();
    } else {

        auto vdatumProj4GridName = getVDatumPROJ4GRIDS();
        if (!vdatumProj4GridName.empty()) {
            formatter->setVDatumExtension(vdatumProj4GridName);
            d->baseCRS()->_exportToWKT(formatter);
            formatter->setVDatumExtension(std::string());
            return;
        }

        auto hdatumProj4GridName = getHDatumPROJ4GRIDS();
        if (!hdatumProj4GridName.empty()) {
            formatter->setHDatumExtension(hdatumProj4GridName);
            d->baseCRS()->_exportToWKT(formatter);
            formatter->setHDatumExtension(std::string());
            return;
        }

        if (!isTOWGS84Compatible()) {
            io::FormattingException::Throw(
                "Cannot export BoundCRS with non-WGS 84 hub CRS in WKT1");
        }
        auto params = d->transformation()->getTOWGS84Parameters();
        if (!formatter->useESRIDialect()) {
            formatter->setTOWGS84Parameters(params);
        }
        d->baseCRS()->_exportToWKT(formatter);
        formatter->setTOWGS84Parameters(std::vector<double>());
    }
}
//! @endcond

// ---------------------------------------------------------------------------

void BoundCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    if (formatter->convention() ==
        io::PROJStringFormatter::Convention::PROJ_5) {
        io::FormattingException::Throw(
            "BoundCRS cannot be exported as a PROJ.5 string, but its baseCRS "
            "might");
    }

    auto crs_exportable =
        dynamic_cast<const io::IPROJStringExportable *>(d->baseCRS().get());
    if (!crs_exportable) {
        io::FormattingException::Throw(
            "baseCRS of BoundCRS cannot be exported as a PROJ string");
    }

    auto vdatumProj4GridName = getVDatumPROJ4GRIDS();
    if (!vdatumProj4GridName.empty()) {
        formatter->setVDatumExtension(vdatumProj4GridName);
        crs_exportable->_exportToPROJString(formatter);
        formatter->setVDatumExtension(std::string());
    } else {
        auto hdatumProj4GridName = getHDatumPROJ4GRIDS();
        if (!hdatumProj4GridName.empty()) {
            formatter->setHDatumExtension(hdatumProj4GridName);
            crs_exportable->_exportToPROJString(formatter);
            formatter->setHDatumExtension(std::string());
        } else {
            if (isTOWGS84Compatible()) {
                auto params = transformation()->getTOWGS84Parameters();
                formatter->setTOWGS84Parameters(params);
            }
            crs_exportable->_exportToPROJString(formatter);
            formatter->setTOWGS84Parameters(std::vector<double>());
        }
    }
}

// ---------------------------------------------------------------------------

bool BoundCRS::isEquivalentTo(const util::IComparable *other,
                              util::IComparable::Criterion criterion) const {
    auto otherBoundCRS = dynamic_cast<const BoundCRS *>(other);
    if (otherBoundCRS == nullptr ||
        !ObjectUsage::isEquivalentTo(other, criterion)) {
        return false;
    }
    return d->baseCRS()->isEquivalentTo(otherBoundCRS->d->baseCRS().get(),
                                        criterion) &&
           d->hubCRS()->isEquivalentTo(otherBoundCRS->d->hubCRS().get(),
                                       criterion) &&
           d->transformation()->isEquivalentTo(
               otherBoundCRS->d->transformation().get(), criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct DerivedGeodeticCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
DerivedGeodeticCRS::~DerivedGeodeticCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

DerivedGeodeticCRS::DerivedGeodeticCRS(
    const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::CartesianCSNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      GeodeticCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

DerivedGeodeticCRS::DerivedGeodeticCRS(
    const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::SphericalCSNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      GeodeticCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

DerivedGeodeticCRS::DerivedGeodeticCRS(const DerivedGeodeticCRS &other)
    : SingleCRS(other), GeodeticCRS(other), DerivedCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr DerivedGeodeticCRS::shallowClone() const {
    auto crs(DerivedGeodeticCRS::nn_make_shared<DerivedGeodeticCRS>(*this));
    crs->assignSelf(crs);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS (a GeodeticCRS) of a DerivedGeodeticCRS.
 *
 * @return the base CRS.
 */
const GeodeticCRSNNPtr DerivedGeodeticCRS::baseCRS() const {
    return NN_NO_CHECK(util::nn_dynamic_pointer_cast<GeodeticCRS>(
        DerivedCRS::getPrivate()->baseCRS_));
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a DerivedGeodeticCRS from a base CRS, a deriving
 * conversion and a cs::CartesianCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param baseCRSIn base CRS.
 * @param derivingConversionIn the deriving conversion from the base CRS to this
 * CRS.
 * @param csIn the coordinate system.
 * @return new DerivedGeodeticCRS.
 */
DerivedGeodeticCRSNNPtr DerivedGeodeticCRS::create(
    const util::PropertyMap &properties, const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::CartesianCSNNPtr &csIn) {
    auto crs(DerivedGeodeticCRS::nn_make_shared<DerivedGeodeticCRS>(
        baseCRSIn, derivingConversionIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a DerivedGeodeticCRS from a base CRS, a deriving
 * conversion and a cs::SphericalCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param baseCRSIn base CRS.
 * @param derivingConversionIn the deriving conversion from the base CRS to this
 * CRS.
 * @param csIn the coordinate system.
 * @return new DerivedGeodeticCRS.
 */
DerivedGeodeticCRSNNPtr DerivedGeodeticCRS::create(
    const util::PropertyMap &properties, const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::SphericalCSNNPtr &csIn) {
    auto crs(DerivedGeodeticCRS::nn_make_shared<DerivedGeodeticCRS>(
        baseCRSIn, derivingConversionIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void DerivedGeodeticCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2) {
        io::FormattingException::Throw(
            "DerivedGeodeticCRS can only be exported to WKT2");
    }
    formatter->startNode(io::WKTConstants::GEODCRS, !identifiers().empty());
    formatter->addQuotedString(nameStr());

    auto l_baseCRS = baseCRS();
    formatter->startNode((formatter->use2018Keywords() &&
                          dynamic_cast<const GeographicCRS *>(l_baseCRS.get()))
                             ? io::WKTConstants::BASEGEOGCRS
                             : io::WKTConstants::BASEGEODCRS,
                         !baseCRS()->identifiers().empty());
    formatter->addQuotedString(l_baseCRS->nameStr());
    auto l_datum = l_baseCRS->datum();
    if (l_datum) {
        l_datum->_exportToWKT(formatter);
    } else {
        auto l_datumEnsemble = datumEnsemble();
        assert(l_datumEnsemble);
        l_datumEnsemble->_exportToWKT(formatter);
    }
    l_baseCRS->primeMeridian()->_exportToWKT(formatter);
    formatter->endNode();

    formatter->setUseDerivingConversion(true);
    derivingConversionRef()->_exportToWKT(formatter);
    formatter->setUseDerivingConversion(false);

    coordinateSystem()->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

void DerivedGeodeticCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    baseExportToPROJString(formatter);
}

// ---------------------------------------------------------------------------

bool DerivedGeodeticCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherDerivedCRS = dynamic_cast<const DerivedGeodeticCRS *>(other);
    return otherDerivedCRS != nullptr &&
           DerivedCRS::isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct DerivedGeographicCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
DerivedGeographicCRS::~DerivedGeographicCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

DerivedGeographicCRS::DerivedGeographicCRS(
    const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::EllipsoidalCSNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      GeographicCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

DerivedGeographicCRS::DerivedGeographicCRS(const DerivedGeographicCRS &other)
    : SingleCRS(other), GeographicCRS(other), DerivedCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr DerivedGeographicCRS::shallowClone() const {
    auto crs(DerivedGeographicCRS::nn_make_shared<DerivedGeographicCRS>(*this));
    crs->assignSelf(crs);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS (a GeodeticCRS) of a DerivedGeographicCRS.
 *
 * @return the base CRS.
 */
const GeodeticCRSNNPtr DerivedGeographicCRS::baseCRS() const {
    return NN_NO_CHECK(util::nn_dynamic_pointer_cast<GeodeticCRS>(
        DerivedCRS::getPrivate()->baseCRS_));
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a DerivedGeographicCRS from a base CRS, a deriving
 * conversion and a cs::EllipsoidalCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param baseCRSIn base CRS.
 * @param derivingConversionIn the deriving conversion from the base CRS to this
 * CRS.
 * @param csIn the coordinate system.
 * @return new DerivedGeographicCRS.
 */
DerivedGeographicCRSNNPtr DerivedGeographicCRS::create(
    const util::PropertyMap &properties, const GeodeticCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::EllipsoidalCSNNPtr &csIn) {
    auto crs(DerivedGeographicCRS::nn_make_shared<DerivedGeographicCRS>(
        baseCRSIn, derivingConversionIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void DerivedGeographicCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2) {
        io::FormattingException::Throw(
            "DerivedGeographicCRS can only be exported to WKT2");
    }
    formatter->startNode(formatter->use2018Keywords()
                             ? io::WKTConstants::GEOGCRS
                             : io::WKTConstants::GEODCRS,
                         !identifiers().empty());
    formatter->addQuotedString(nameStr());

    auto l_baseCRS = baseCRS();
    formatter->startNode((formatter->use2018Keywords() &&
                          dynamic_cast<const GeographicCRS *>(l_baseCRS.get()))
                             ? io::WKTConstants::BASEGEOGCRS
                             : io::WKTConstants::BASEGEODCRS,
                         !l_baseCRS->identifiers().empty());
    formatter->addQuotedString(l_baseCRS->nameStr());
    l_baseCRS->exportDatumOrDatumEnsembleToWkt(formatter);
    l_baseCRS->primeMeridian()->_exportToWKT(formatter);
    formatter->endNode();

    formatter->setUseDerivingConversion(true);
    derivingConversionRef()->_exportToWKT(formatter);
    formatter->setUseDerivingConversion(false);

    coordinateSystem()->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

void DerivedGeographicCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    baseExportToPROJString(formatter);
}

// ---------------------------------------------------------------------------

bool DerivedGeographicCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherDerivedCRS = dynamic_cast<const DerivedGeographicCRS *>(other);
    return otherDerivedCRS != nullptr &&
           DerivedCRS::isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct DerivedProjectedCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
DerivedProjectedCRS::~DerivedProjectedCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

DerivedProjectedCRS::DerivedProjectedCRS(
    const ProjectedCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::CoordinateSystemNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

DerivedProjectedCRS::DerivedProjectedCRS(const DerivedProjectedCRS &other)
    : SingleCRS(other), DerivedCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr DerivedProjectedCRS::shallowClone() const {
    auto crs(DerivedProjectedCRS::nn_make_shared<DerivedProjectedCRS>(*this));
    crs->assignSelf(crs);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS (a ProjectedCRS) of a DerivedProjectedCRS.
 *
 * @return the base CRS.
 */
const ProjectedCRSNNPtr DerivedProjectedCRS::baseCRS() const {
    return NN_NO_CHECK(util::nn_dynamic_pointer_cast<ProjectedCRS>(
        DerivedCRS::getPrivate()->baseCRS_));
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a DerivedProjectedCRS from a base CRS, a deriving
 * conversion and a cs::CS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param baseCRSIn base CRS.
 * @param derivingConversionIn the deriving conversion from the base CRS to this
 * CRS.
 * @param csIn the coordinate system.
 * @return new DerivedProjectedCRS.
 */
DerivedProjectedCRSNNPtr DerivedProjectedCRS::create(
    const util::PropertyMap &properties, const ProjectedCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::CoordinateSystemNNPtr &csIn) {
    auto crs(DerivedProjectedCRS::nn_make_shared<DerivedProjectedCRS>(
        baseCRSIn, derivingConversionIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void DerivedProjectedCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2 || !formatter->use2018Keywords()) {
        io::FormattingException::Throw(
            "DerivedProjectedCRS can only be exported to WKT2:2018");
    }
    formatter->startNode(io::WKTConstants::DERIVEDPROJCRS,
                         !identifiers().empty());
    formatter->addQuotedString(nameStr());

    {
        auto l_baseProjCRS = baseCRS();
        formatter->startNode(io::WKTConstants::BASEPROJCRS,
                             !l_baseProjCRS->identifiers().empty());
        formatter->addQuotedString(l_baseProjCRS->nameStr());

        auto l_baseGeodCRS = l_baseProjCRS->baseCRS();
        auto &geodeticCRSAxisList =
            l_baseGeodCRS->coordinateSystem()->axisList();

        formatter->startNode(
            dynamic_cast<const GeographicCRS *>(l_baseGeodCRS.get())
                ? io::WKTConstants::BASEGEOGCRS
                : io::WKTConstants::BASEGEODCRS,
            !l_baseGeodCRS->identifiers().empty());
        formatter->addQuotedString(l_baseGeodCRS->nameStr());
        l_baseGeodCRS->exportDatumOrDatumEnsembleToWkt(formatter);
        // insert ellipsoidal cs unit when the units of the map
        // projection angular parameters are not explicitly given within those
        // parameters. See
        // http://docs.opengeospatial.org/is/12-063r5/12-063r5.html#61
        if (formatter->primeMeridianOrParameterUnitOmittedIfSameAsAxis() &&
            !geodeticCRSAxisList.empty()) {
            geodeticCRSAxisList[0]->unit()._exportToWKT(formatter);
        }
        l_baseGeodCRS->primeMeridian()->_exportToWKT(formatter);
        formatter->endNode();

        l_baseProjCRS->derivingConversionRef()->_exportToWKT(formatter);
        formatter->endNode();
    }

    formatter->setUseDerivingConversion(true);
    derivingConversionRef()->_exportToWKT(formatter);
    formatter->setUseDerivingConversion(false);

    coordinateSystem()->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

void DerivedProjectedCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    baseExportToPROJString(formatter);
}

// ---------------------------------------------------------------------------

bool DerivedProjectedCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherDerivedCRS = dynamic_cast<const DerivedProjectedCRS *>(other);
    return otherDerivedCRS != nullptr &&
           DerivedCRS::isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct TemporalCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
TemporalCRS::~TemporalCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

TemporalCRS::TemporalCRS(const datum::TemporalDatumNNPtr &datumIn,
                         const cs::TemporalCSNNPtr &csIn)
    : SingleCRS(datumIn.as_nullable(), nullptr, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

TemporalCRS::TemporalCRS(const TemporalCRS &other)
    : SingleCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr TemporalCRS::shallowClone() const {
    auto crs(TemporalCRS::nn_make_shared<TemporalCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the datum::TemporalDatum associated with the CRS.
 *
 * @return a TemporalDatum
 */
const datum::TemporalDatumNNPtr TemporalCRS::datum() const {
    return NN_NO_CHECK(std::static_pointer_cast<datum::TemporalDatum>(
        SingleCRS::getPrivate()->datum));
}

// ---------------------------------------------------------------------------

/** \brief Return the cs::TemporalCS associated with the CRS.
 *
 * @return a TemporalCS
 */
const cs::TemporalCSNNPtr TemporalCRS::coordinateSystem() const {
    return util::nn_static_pointer_cast<cs::TemporalCS>(
        SingleCRS::getPrivate()->coordinateSystem);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a TemporalCRS from a datum and a coordinate system.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datumIn the datum.
 * @param csIn the coordinate system.
 * @return new TemporalCRS.
 */
TemporalCRSNNPtr TemporalCRS::create(const util::PropertyMap &properties,
                                     const datum::TemporalDatumNNPtr &datumIn,
                                     const cs::TemporalCSNNPtr &csIn) {
    auto crs(TemporalCRS::nn_make_shared<TemporalCRS>(datumIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void TemporalCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2) {
        io::FormattingException::Throw(
            "TemporalCRS can only be exported to WKT2");
    }
    formatter->startNode(io::WKTConstants::TIMECRS, !identifiers().empty());
    formatter->addQuotedString(nameStr());
    datum()->_exportToWKT(formatter);
    coordinateSystem()->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

bool TemporalCRS::isEquivalentTo(const util::IComparable *other,
                                 util::IComparable::Criterion criterion) const {
    auto otherTemporalCRS = dynamic_cast<const TemporalCRS *>(other);
    return otherTemporalCRS != nullptr &&
           SingleCRS::_isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct EngineeringCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
EngineeringCRS::~EngineeringCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

EngineeringCRS::EngineeringCRS(const datum::EngineeringDatumNNPtr &datumIn,
                               const cs::CoordinateSystemNNPtr &csIn)
    : SingleCRS(datumIn.as_nullable(), nullptr, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

EngineeringCRS::EngineeringCRS(const EngineeringCRS &other)
    : SingleCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr EngineeringCRS::shallowClone() const {
    auto crs(EngineeringCRS::nn_make_shared<EngineeringCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the datum::EngineeringDatum associated with the CRS.
 *
 * @return a EngineeringDatum
 */
const datum::EngineeringDatumNNPtr EngineeringCRS::datum() const {
    return NN_NO_CHECK(std::static_pointer_cast<datum::EngineeringDatum>(
        SingleCRS::getPrivate()->datum));
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a EngineeringCRS from a datum and a coordinate system.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datumIn the datum.
 * @param csIn the coordinate system.
 * @return new EngineeringCRS.
 */
EngineeringCRSNNPtr
EngineeringCRS::create(const util::PropertyMap &properties,
                       const datum::EngineeringDatumNNPtr &datumIn,
                       const cs::CoordinateSystemNNPtr &csIn) {
    auto crs(EngineeringCRS::nn_make_shared<EngineeringCRS>(datumIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void EngineeringCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    formatter->startNode(isWKT2 ? io::WKTConstants::ENGCRS
                                : io::WKTConstants::LOCAL_CS,
                         !identifiers().empty());
    formatter->addQuotedString(nameStr());
    if (isWKT2 || !datum()->nameStr().empty()) {
        datum()->_exportToWKT(formatter);
        coordinateSystem()->_exportToWKT(formatter);
    }
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

bool EngineeringCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherEngineeringCRS = dynamic_cast<const EngineeringCRS *>(other);
    return otherEngineeringCRS != nullptr &&
           SingleCRS::_isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct ParametricCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
ParametricCRS::~ParametricCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

ParametricCRS::ParametricCRS(const datum::ParametricDatumNNPtr &datumIn,
                             const cs::ParametricCSNNPtr &csIn)
    : SingleCRS(datumIn.as_nullable(), nullptr, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

ParametricCRS::ParametricCRS(const ParametricCRS &other)
    : SingleCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr ParametricCRS::shallowClone() const {
    auto crs(ParametricCRS::nn_make_shared<ParametricCRS>(*this));
    crs->assignSelf(crs);
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the datum::ParametricDatum associated with the CRS.
 *
 * @return a ParametricDatum
 */
const datum::ParametricDatumNNPtr ParametricCRS::datum() const {
    return NN_NO_CHECK(std::static_pointer_cast<datum::ParametricDatum>(
        SingleCRS::getPrivate()->datum));
}

// ---------------------------------------------------------------------------

/** \brief Return the cs::TemporalCS associated with the CRS.
 *
 * @return a TemporalCS
 */
const cs::ParametricCSNNPtr ParametricCRS::coordinateSystem() const {
    return util::nn_static_pointer_cast<cs::ParametricCS>(
        SingleCRS::getPrivate()->coordinateSystem);
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a ParametricCRS from a datum and a coordinate system.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param datumIn the datum.
 * @param csIn the coordinate system.
 * @return new ParametricCRS.
 */
ParametricCRSNNPtr
ParametricCRS::create(const util::PropertyMap &properties,
                      const datum::ParametricDatumNNPtr &datumIn,
                      const cs::ParametricCSNNPtr &csIn) {
    auto crs(ParametricCRS::nn_make_shared<ParametricCRS>(datumIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void ParametricCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2) {
        io::FormattingException::Throw(
            "ParametricCRS can only be exported to WKT2");
    }
    formatter->startNode(io::WKTConstants::PARAMETRICCRS,
                         !identifiers().empty());
    formatter->addQuotedString(nameStr());
    datum()->_exportToWKT(formatter);
    coordinateSystem()->_exportToWKT(formatter);
    ObjectUsage::baseExportToWKT(formatter);
    formatter->endNode();
}
//! @endcond

// ---------------------------------------------------------------------------

bool ParametricCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherParametricCRS = dynamic_cast<const ParametricCRS *>(other);
    return otherParametricCRS != nullptr &&
           SingleCRS::_isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct DerivedVerticalCRS::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
DerivedVerticalCRS::~DerivedVerticalCRS() = default;
//! @endcond

// ---------------------------------------------------------------------------

DerivedVerticalCRS::DerivedVerticalCRS(
    const VerticalCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::VerticalCSNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      VerticalCRS(baseCRSIn->datum(), baseCRSIn->datumEnsemble(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

DerivedVerticalCRS::DerivedVerticalCRS(const DerivedVerticalCRS &other)
    : SingleCRS(other), VerticalCRS(other), DerivedCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

CRSNNPtr DerivedVerticalCRS::shallowClone() const {
    auto crs(DerivedVerticalCRS::nn_make_shared<DerivedVerticalCRS>(*this));
    crs->assignSelf(crs);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

/** \brief Return the base CRS (a VerticalCRS) of a DerivedVerticalCRS.
 *
 * @return the base CRS.
 */
const VerticalCRSNNPtr DerivedVerticalCRS::baseCRS() const {
    return NN_NO_CHECK(util::nn_dynamic_pointer_cast<VerticalCRS>(
        DerivedCRS::getPrivate()->baseCRS_));
}

// ---------------------------------------------------------------------------

/** \brief Instanciate a DerivedVerticalCRS from a base CRS, a deriving
 * conversion and a cs::VerticalCS.
 *
 * @param properties See \ref general_properties.
 * At minimum the name should be defined.
 * @param baseCRSIn base CRS.
 * @param derivingConversionIn the deriving conversion from the base CRS to this
 * CRS.
 * @param csIn the coordinate system.
 * @return new DerivedVerticalCRS.
 */
DerivedVerticalCRSNNPtr DerivedVerticalCRS::create(
    const util::PropertyMap &properties, const VerticalCRSNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const cs::VerticalCSNNPtr &csIn) {
    auto crs(DerivedVerticalCRS::nn_make_shared<DerivedVerticalCRS>(
        baseCRSIn, derivingConversionIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
void DerivedVerticalCRS::_exportToWKT(io::WKTFormatter *formatter) const {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2) {
        io::FormattingException::Throw(
            "DerivedVerticalCRS can only be exported to WKT2");
    }
    baseExportToWKT(formatter, io::WKTConstants::VERTCRS,
                    io::WKTConstants::BASEVERTCRS);
}
//! @endcond

// ---------------------------------------------------------------------------

void DerivedVerticalCRS::_exportToPROJString(
    io::PROJStringFormatter *formatter) const // throw(io::FormattingException)
{
    baseExportToPROJString(formatter);
}

// ---------------------------------------------------------------------------

bool DerivedVerticalCRS::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherDerivedCRS = dynamic_cast<const DerivedVerticalCRS *>(other);
    return otherDerivedCRS != nullptr &&
           DerivedCRS::isEquivalentTo(other, criterion);
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
template <class DerivedCRSTraits>
struct DerivedCRSTemplate<DerivedCRSTraits>::Private {};
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
template <class DerivedCRSTraits>
DerivedCRSTemplate<DerivedCRSTraits>::~DerivedCRSTemplate() = default;
//! @endcond

// ---------------------------------------------------------------------------

template <class DerivedCRSTraits>
DerivedCRSTemplate<DerivedCRSTraits>::DerivedCRSTemplate(
    const BaseNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn, const CSNNPtr &csIn)
    : SingleCRS(baseCRSIn->datum().as_nullable(), nullptr, csIn),
      BaseType(baseCRSIn->datum(), csIn),
      DerivedCRS(baseCRSIn, derivingConversionIn, csIn), d(nullptr) {}

// ---------------------------------------------------------------------------

template <class DerivedCRSTraits>
DerivedCRSTemplate<DerivedCRSTraits>::DerivedCRSTemplate(
    const DerivedCRSTemplate &other)
    : SingleCRS(other), BaseType(other), DerivedCRS(other), d(nullptr) {}

// ---------------------------------------------------------------------------

template <class DerivedCRSTraits>
const typename DerivedCRSTemplate<DerivedCRSTraits>::BaseNNPtr
DerivedCRSTemplate<DerivedCRSTraits>::baseCRS() const {
    auto l_baseCRS = DerivedCRS::getPrivate()->baseCRS_;
    return NN_NO_CHECK(util::nn_dynamic_pointer_cast<BaseType>(l_baseCRS));
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress

template <class DerivedCRSTraits>
CRSNNPtr DerivedCRSTemplate<DerivedCRSTraits>::shallowClone() const {
    auto crs(DerivedCRSTemplate::nn_make_shared<DerivedCRSTemplate>(*this));
    crs->assignSelf(crs);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

template <class DerivedCRSTraits>
typename DerivedCRSTemplate<DerivedCRSTraits>::NNPtr
DerivedCRSTemplate<DerivedCRSTraits>::create(
    const util::PropertyMap &properties, const BaseNNPtr &baseCRSIn,
    const operation::ConversionNNPtr &derivingConversionIn,
    const CSNNPtr &csIn) {
    auto crs(DerivedCRSTemplate::nn_make_shared<DerivedCRSTemplate>(
        baseCRSIn, derivingConversionIn, csIn));
    crs->assignSelf(crs);
    crs->setProperties(properties);
    crs->setDerivingConversionCRS();
    return crs;
}

// ---------------------------------------------------------------------------

static void DerivedCRSTemplateCheckExportToWKT(io::WKTFormatter *&formatter,
                                               const std::string &crsName,
                                               bool wkt2_2018_only) {
    const bool isWKT2 = formatter->version() == io::WKTFormatter::Version::WKT2;
    if (!isWKT2 || (wkt2_2018_only && !formatter->use2018Keywords())) {
        io::FormattingException::Throw(crsName +
                                       " can only be exported to WKT2" +
                                       (wkt2_2018_only ? ":2018" : ""));
    }
}

// ---------------------------------------------------------------------------

template <class DerivedCRSTraits>
void DerivedCRSTemplate<DerivedCRSTraits>::_exportToWKT(
    io::WKTFormatter *formatter) const {
    DerivedCRSTemplateCheckExportToWKT(formatter, DerivedCRSTraits::CRSName(),
                                       DerivedCRSTraits::wkt2_2018_only);
    baseExportToWKT(formatter, DerivedCRSTraits::WKTKeyword(),
                    DerivedCRSTraits::WKTBaseKeyword());
}

// ---------------------------------------------------------------------------

template <class DerivedCRSTraits>
bool DerivedCRSTemplate<DerivedCRSTraits>::isEquivalentTo(
    const util::IComparable *other,
    util::IComparable::Criterion criterion) const {
    auto otherDerivedCRS = dynamic_cast<const DerivedCRSTemplate *>(other);
    return otherDerivedCRS != nullptr &&
           DerivedCRS::isEquivalentTo(other, criterion);
}

//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string STRING_DerivedEngineeringCRS("DerivedEngineeringCRS");
const std::string &DerivedEngineeringCRSTraits::CRSName() {
    return STRING_DerivedEngineeringCRS;
}
const std::string &DerivedEngineeringCRSTraits::WKTKeyword() {
    return io::WKTConstants::ENGCRS;
}
const std::string &DerivedEngineeringCRSTraits::WKTBaseKeyword() {
    return io::WKTConstants::BASEENGCRS;
}

template class DerivedCRSTemplate<DerivedEngineeringCRSTraits>;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string STRING_DerivedParametricCRS("DerivedParametricCRS");
const std::string &DerivedParametricCRSTraits::CRSName() {
    return STRING_DerivedParametricCRS;
}
const std::string &DerivedParametricCRSTraits::WKTKeyword() {
    return io::WKTConstants::PARAMETRICCRS;
}
const std::string &DerivedParametricCRSTraits::WKTBaseKeyword() {
    return io::WKTConstants::BASEPARAMCRS;
}

template class DerivedCRSTemplate<DerivedParametricCRSTraits>;
//! @endcond

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
static const std::string STRING_DerivedTemporalCRS("DerivedTemporalCRS");
const std::string &DerivedTemporalCRSTraits::CRSName() {
    return STRING_DerivedTemporalCRS;
}
const std::string &DerivedTemporalCRSTraits::WKTKeyword() {
    return io::WKTConstants::TIMECRS;
}
const std::string &DerivedTemporalCRSTraits::WKTBaseKeyword() {
    return io::WKTConstants::BASETIMECRS;
}

template class DerivedCRSTemplate<DerivedTemporalCRSTraits>;
//! @endcond

// ---------------------------------------------------------------------------

} // namespace crs
NS_PROJ_END
