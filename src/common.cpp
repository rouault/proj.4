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

#include "proj/common.hpp"
#include "proj/internal.hpp"
#include "proj/io.hpp"
#include "proj/io_internal.hpp"
#include "proj/metadata.hpp"
#include "proj/util.hpp"

#include <cmath> // M_PI
#include <memory>
#include <string>
#include <vector>

using namespace NS_PROJ::common;
using namespace NS_PROJ::internal;
using namespace NS_PROJ::io;
using namespace NS_PROJ::metadata;
using namespace NS_PROJ::util;

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct UnitOfMeasure::Private {
    std::string name_{};
    double toSI_ = 1.0;
    UnitOfMeasure::Type type_{UnitOfMeasure::Type::UNKNOWN};
    std::string authority_{};
    std::string code_{};

    Private(const std::string &nameIn, double toSIIn,
            UnitOfMeasure::Type typeIn, const std::string &authorityIn,
            const std::string &codeIn)
        : name_(nameIn), toSI_(toSIIn), type_(typeIn), authority_(authorityIn),
          code_(codeIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

UnitOfMeasure::UnitOfMeasure(const std::string &nameIn, double toSIIn,
                             UnitOfMeasure::Type typeIn,
                             const std::string &authorityIn,
                             const std::string &codeIn)
    : d(internal::make_unique<Private>(nameIn, toSIIn, typeIn, authorityIn,
                                       codeIn)) {}

// ---------------------------------------------------------------------------

UnitOfMeasure::UnitOfMeasure(const UnitOfMeasure &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

UnitOfMeasure::~UnitOfMeasure() = default;

// ---------------------------------------------------------------------------

UnitOfMeasure &UnitOfMeasure::operator=(const UnitOfMeasure &other) {
    if (this != &other) {
        *d = *(other.d);
    }
    return *this;
}

// ---------------------------------------------------------------------------

const std::string &UnitOfMeasure::name() const { return d->name_; }

// ---------------------------------------------------------------------------

double UnitOfMeasure::conversionToSI() const { return d->toSI_; }

// ---------------------------------------------------------------------------

UnitOfMeasure::Type UnitOfMeasure::type() const { return d->type_; }

// ---------------------------------------------------------------------------

const std::string &UnitOfMeasure::authority() const { return d->authority_; }

// ---------------------------------------------------------------------------

const std::string &UnitOfMeasure::code() const { return d->code_; }

// ---------------------------------------------------------------------------

std::string UnitOfMeasure::exportToWKT(
    WKTFormatterNNPtr formatter,
    const std::string &unitType) const // throw(FormattingException)
{
    const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;

    if (formatter->forceUNITKeyword()) {
        formatter->startNode(WKTConstants::UNIT);
    } else if (!unitType.empty()) {
        formatter->startNode(unitType);
    } else {
        if (isWKT2 && type() == Type::LINEAR) {
            formatter->startNode(WKTConstants::LENGTHUNIT);
        } else if (isWKT2 && type() == Type::ANGULAR) {
            formatter->startNode(WKTConstants::ANGLEUNIT);
        } else if (isWKT2 && type() == Type::SCALE) {
            formatter->startNode(WKTConstants::SCALEUNIT);
        } else {
            formatter->startNode(WKTConstants::UNIT);
        }
    }

    {
        formatter->addQuotedString(name());
        formatter->add(conversionToSI());
        if (!authority().empty() && formatter->outputId()) {
            formatter->startNode(isWKT2 ? WKTConstants::ID
                                        : WKTConstants::AUTHORITY);
            formatter->addQuotedString(authority());
            formatter->add(code());
            formatter->endNode();
        }
    }
    formatter->endNode();
    return formatter->toString();
}

// ---------------------------------------------------------------------------

bool UnitOfMeasure::operator==(const UnitOfMeasure &other) const {
    return name() == other.name();
}

// ---------------------------------------------------------------------------

bool UnitOfMeasure::operator!=(const UnitOfMeasure &other) const {
    return name() != other.name();
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct Measure::Private {
    double value_ = 0.0;
    UnitOfMeasure unit_{};

    Private(double valueIn, const UnitOfMeasure &unitIn)
        : value_(valueIn), unit_(unitIn) {}
};
//! @endcond

// ---------------------------------------------------------------------------

Measure::Measure(double valueIn, const UnitOfMeasure &unitIn)
    : d(internal::make_unique<Private>(valueIn, unitIn)) {}

// ---------------------------------------------------------------------------

Measure::Measure(const Measure &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

Measure::~Measure() = default;

// ---------------------------------------------------------------------------

Measure &Measure::operator=(const Measure &other) {
    if (this != &other) {
        *d = *(other.d);
    }
    return *this;
}

// ---------------------------------------------------------------------------

const UnitOfMeasure &Measure::unit() const { return d->unit_; }

// ---------------------------------------------------------------------------

double Measure::getSIValue() const {
    return d->value_ * d->unit_.conversionToSI();
}

// ---------------------------------------------------------------------------

double Measure::value() const { return d->value_; }

// ---------------------------------------------------------------------------

Measure Measure::convertToUnit(const UnitOfMeasure &otherUnit) const {
    return Measure(getSIValue() / otherUnit.conversionToSI(), otherUnit);
}

// ---------------------------------------------------------------------------

Scale::Scale(double valueIn) : Measure(valueIn, UnitOfMeasure::SCALE_UNITY) {}

// ---------------------------------------------------------------------------

Scale::Scale(const Scale &other) : Measure(other) {}

// ---------------------------------------------------------------------------

Scale::~Scale() = default;

// ---------------------------------------------------------------------------

Scale &Scale::operator=(const Scale &other) {
    if (this != &other) {
        Measure::operator=(other);
    }
    return *this;
}

// ---------------------------------------------------------------------------

Angle::Angle(double valueIn, const UnitOfMeasure &unitIn)
    : Measure(valueIn, unitIn) {}

// ---------------------------------------------------------------------------

Angle::Angle(const Angle &other) : Measure(other) {}

// ---------------------------------------------------------------------------

Angle Angle::convertToUnit(const UnitOfMeasure &otherUnit) const {
    return Angle(Measure::convertToUnit(otherUnit).value(), otherUnit);
}

// ---------------------------------------------------------------------------

Angle::~Angle() = default;

// ---------------------------------------------------------------------------

Angle &Angle::operator=(const Angle &other) {
    if (this != &other) {
        Measure::operator=(other);
    }
    return *this;
}

// ---------------------------------------------------------------------------

Length::Length(double valueIn, const UnitOfMeasure &unitIn)
    : Measure(valueIn, unitIn) {}

// ---------------------------------------------------------------------------

Length::Length(const Length &other) : Measure(other) {}

// ---------------------------------------------------------------------------

Length Length::convertToUnit(const UnitOfMeasure &otherUnit) const {
    return Length(Measure::convertToUnit(otherUnit).value(), otherUnit);
}

// ---------------------------------------------------------------------------

Length::~Length() = default;

// ---------------------------------------------------------------------------

Length &Length::operator=(const Length &other) {
    if (this != &other) {
        Measure::operator=(other);
    }
    return *this;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
// cppcheck-suppress copyCtorAndEqOperator
struct IdentifiedObject::Private {
    IdentifierNNPtr name{Identifier::create()};
    std::vector<IdentifierNNPtr> identifiers{};
    std::vector<GenericNameNNPtr> aliases{};
    std::string remarks{};
    bool isDeprecated{};

    void setIdentifiers(const PropertyMap &properties);
    void setName(const PropertyMap &properties);
    void setAliases(const PropertyMap &properties);
};
//! @endcond

// ---------------------------------------------------------------------------

IdentifiedObject::IdentifiedObject() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

IdentifiedObject::IdentifiedObject(const IdentifiedObject &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

IdentifiedObject &IdentifiedObject::operator=(const IdentifiedObject &other) {
    if (this != &other) {
        *d = *(other.d);
    }
    return *this;
}

// ---------------------------------------------------------------------------

IdentifiedObject::~IdentifiedObject() = default;

// ---------------------------------------------------------------------------

IdentifiedObjectNNPtr IdentifiedObject::create(
    const PropertyMap &properties) // throw(InvalidValueTypeException)
{
    auto id(IdentifiedObject::nn_make_shared<IdentifiedObject>());
    id->setProperties(properties);
    return id;
}

// ---------------------------------------------------------------------------

const IdentifierNNPtr &IdentifiedObject::name() const { return d->name; }

// ---------------------------------------------------------------------------

const std::vector<IdentifierNNPtr> &IdentifiedObject::identifiers() const {
    return d->identifiers;
}

// ---------------------------------------------------------------------------

const std::vector<GenericNameNNPtr> &IdentifiedObject::aliases() const {
    return d->aliases;
}

// ---------------------------------------------------------------------------

std::string IdentifiedObject::alias() const {
    if (d->aliases.empty())
        return std::string();
    return d->aliases[0]->toFullyQualifiedName()->toString();
}

// ---------------------------------------------------------------------------

const std::string &IdentifiedObject::remarks() const { return d->remarks; }

// ---------------------------------------------------------------------------

bool IdentifiedObject::isDeprecated() const { return d->isDeprecated; }

// ---------------------------------------------------------------------------

void IdentifiedObject::Private::setName(
    const PropertyMap &properties) // throw(InvalidValueTypeException)
{
    auto oIter = properties.find(NAME_KEY);
    if (oIter == properties.end()) {
        return;
    }
    if (auto genVal =
            util::nn_dynamic_pointer_cast<BoxedValue>(oIter->second)) {
        if (genVal->type() == BoxedValue::Type::STRING) {
            name = Identifier::create(genVal->stringValue());
        } else {
            throw InvalidValueTypeException("Invalid value type for " +
                                            NAME_KEY);
        }
    } else {
        if (auto identifier =
                util::nn_dynamic_pointer_cast<Identifier>(oIter->second)) {
            name = NN_CHECK_ASSERT(identifier);
        } else {
            throw InvalidValueTypeException("Invalid value type for " +
                                            NAME_KEY);
        }
    }
}

// ---------------------------------------------------------------------------

void IdentifiedObject::Private::setIdentifiers(
    const PropertyMap &properties) // throw(InvalidValueTypeException)
{
    auto oIter = properties.find(IDENTIFIER_KEY);
    if (oIter == properties.end()) {
        return;
    }
    if (auto identifier =
            util::nn_dynamic_pointer_cast<Identifier>(oIter->second)) {
        identifiers.clear();
        identifiers.push_back(NN_CHECK_ASSERT(identifier));
    } else {
        if (auto array = util::nn_dynamic_pointer_cast<ArrayOfBaseObject>(
                oIter->second)) {
            identifiers.clear();
            for (const auto &val : array->values) {
                identifier = util::nn_dynamic_pointer_cast<Identifier>(val);
                if (identifier) {
                    identifiers.push_back(NN_CHECK_ASSERT(identifier));
                } else {
                    throw InvalidValueTypeException("Invalid value type for " +
                                                    IDENTIFIER_KEY);
                }
            }
        } else {
            throw InvalidValueTypeException("Invalid value type for " +
                                            IDENTIFIER_KEY);
        }
    }
}

// ---------------------------------------------------------------------------

void IdentifiedObject::Private::setAliases(
    const PropertyMap &properties) // throw(InvalidValueTypeException)
{
    auto oIter = properties.find(ALIAS_KEY);
    if (oIter == properties.end()) {
        return;
    }
    if (auto l_name =
            util::nn_dynamic_pointer_cast<GenericName>(oIter->second)) {
        aliases.clear();
        aliases.push_back(NN_CHECK_ASSERT(l_name));
    } else {
        if (auto array = util::nn_dynamic_pointer_cast<ArrayOfBaseObject>(
                oIter->second)) {
            aliases.clear();
            for (const auto &val : array->values) {
                l_name = util::nn_dynamic_pointer_cast<GenericName>(val);
                if (l_name) {
                    aliases.push_back(NN_CHECK_ASSERT(l_name));
                } else {
                    if (auto genVal =
                            util::nn_dynamic_pointer_cast<BoxedValue>(val)) {
                        if (genVal->type() == BoxedValue::Type::STRING) {
                            aliases.push_back(NameFactory::createLocalName(
                                nullptr, genVal->stringValue()));
                        } else {
                            throw InvalidValueTypeException(
                                "Invalid value type for " + ALIAS_KEY);
                        }
                    } else {
                        throw InvalidValueTypeException(
                            "Invalid value type for " + ALIAS_KEY);
                    }
                }
            }
        } else {
            std::string temp;
            if (properties.getStringValue(ALIAS_KEY, temp)) {
                aliases.clear();
                aliases.push_back(NameFactory::createLocalName(nullptr, temp));
            } else {
                throw InvalidValueTypeException("Invalid value type for " +
                                                ALIAS_KEY);
            }
        }
    }
}

// ---------------------------------------------------------------------------

void IdentifiedObject::setProperties(
    const PropertyMap &properties) // throw(InvalidValueTypeException)
{
    d->name->setProperties(properties);

    d->setName(properties);
    d->setIdentifiers(properties);
    d->setAliases(properties);

    properties.getStringValue(REMARKS_KEY, d->remarks);

    {
        auto oIter = properties.find(DEPRECATED_KEY);
        if (oIter != properties.end()) {
            if (auto genVal =
                    util::nn_dynamic_pointer_cast<BoxedValue>(oIter->second)) {
                if (genVal->type() == BoxedValue::Type::BOOLEAN) {
                    d->isDeprecated = genVal->booleanValue();
                } else {
                    throw InvalidValueTypeException("Invalid value type for " +
                                                    DEPRECATED_KEY);
                }
            } else {
                throw InvalidValueTypeException("Invalid value type for " +
                                                DEPRECATED_KEY);
            }
        }
    }
}

// ---------------------------------------------------------------------------

void IdentifiedObject::formatID(WKTFormatterNNPtr formatter) const {
    if (name()->authority() && name()->authority()->title() &&
        !name()->authority()->title()->empty() && !name()->code().empty()) {
        const bool isWKT2 = formatter->version() == WKTFormatter::Version::WKT2;
        const std::string &code = name()->code();
        if (isWKT2) {
            // TODO citation + uri
            if (code[0] >= '0' && code[0] <= '9') {
                formatter->startNode(WKTConstants::ID);
                formatter->addQuotedString(*(name()->authority()->title()));
                formatter->add(code);
                formatter->endNode();
            } else {
                formatter->startNode(WKTConstants::ID);
                formatter->addQuotedString(*(name()->authority()->title()));
                formatter->addQuotedString(code);
                formatter->endNode();
            }
        } else {
            formatter->startNode(WKTConstants::AUTHORITY);
            formatter->addQuotedString(*(name()->authority()->title()));
            formatter->addQuotedString(code);
            formatter->endNode();
        }
    }
}

// ---------------------------------------------------------------------------

void IdentifiedObject::formatRemarks(WKTFormatterNNPtr formatter) const {
    if (!remarks().empty()) {
        formatter->startNode(WKTConstants::REMARK);
        formatter->addQuotedString(remarks());
        formatter->endNode();
    }
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress"
struct ObjectDomain::Private {
    std::string scope{};
    Extent domainOfValidity{};
};
//! @endcond

// ---------------------------------------------------------------------------

ObjectDomain::ObjectDomain() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

ObjectDomain::ObjectDomain(const ObjectDomain &other)
    : d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

ObjectDomain::~ObjectDomain() = default;

// ---------------------------------------------------------------------------

const std::string &ObjectDomain::scope() const { return d->scope; }

// ---------------------------------------------------------------------------

const Extent &ObjectDomain::domainOfValidity() const {
    return d->domainOfValidity;
}

// ---------------------------------------------------------------------------

//! @cond Doxygen_Suppress
struct ObjectUsage::Private {
    ObjectDomainPtr domain{};
};
//! @endcond

// ---------------------------------------------------------------------------

ObjectUsage::ObjectUsage() : d(internal::make_unique<Private>()) {}

// ---------------------------------------------------------------------------

ObjectUsage::ObjectUsage(const ObjectUsage &other)
    : IdentifiedObject(other), d(internal::make_unique<Private>(*(other.d))) {}

// ---------------------------------------------------------------------------

ObjectUsage::~ObjectUsage() = default;

// ---------------------------------------------------------------------------

const ObjectDomainPtr &ObjectUsage::domain() const { return d->domain; }
