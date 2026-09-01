
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "NajaPrivateProperty.h"
#include "NLUniverse.h"

#include "SNLAttributes.h"
#include "SNLScalarTerm.h"
#include "SNLDesignModeling.h"
using namespace naja::NL;

namespace {

class WrongTruthTableProperty: public naja::NajaPrivateProperty {
  public:
    static WrongTruthTableProperty* create(naja::NajaObject* object) {
      preCreate(object, Name);
      auto property = new WrongTruthTableProperty();
      property->postCreate(object);
      return property;
    }

    std::string getName() const override { return Name; }
    std::string getString() const override { return Name; }

  private:
    static const inline std::string Name = "SNLDesignTruthTableProperty";
};

}  // namespace

class SNLDesignTruthTableTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignTruthTableTest0, testTruthTablesConflictError) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  //set truth table
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, SNLTruthTable::fullDependencies(2)));
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1, SNLTruthTable::fullDependencies(2))), NLException);
}

TEST_F(SNLDesignTruthTableTest0, testDesignAttributeWithoutTruthTable) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("attributed"));
  SNLScalarTerm::create(
      design, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName("O"));
  const SNLAttribute attribute(
      NLName("blackbox"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "1"));
  SNLAttributes::addAttribute(design, attribute);

  EXPECT_EQ(0, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(design).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(design, output->getOrderID())
          .isInitialized());

  const std::vector<SNLAttribute> attributes(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end());
  ASSERT_EQ(1, attributes.size());
  EXPECT_EQ(attribute, attributes[0]);
}

TEST_F(SNLDesignTruthTableTest0, testDesignAttributeAndTruthTableCoexist) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("attributed_with_tt"));
  SNLScalarTerm::create(
      design, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName("O"));
  const SNLAttribute attribute(
      NLName("blackbox"),
      SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "1"));
  SNLAttributes::addAttribute(design, attribute);
  const SNLTruthTable truthTable(
      1, 0b01, SNLTruthTable::fullDependencies(1));
  SNLDesignModeling::setTruthTable(design, truthTable);

  EXPECT_EQ(1, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_EQ(truthTable, SNLDesignModeling::getTruthTable(design));
  EXPECT_EQ(
      truthTable,
      SNLDesignModeling::getTruthTable(design, output->getOrderID()));

  const std::vector<SNLAttribute> attributes(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end());
  ASSERT_EQ(1, attributes.size());
  EXPECT_EQ(attribute, attributes[0]);
}

TEST_F(SNLDesignTruthTableTest0, testWrongTruthTablePropertyTypeIsIgnored) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(
      prims, SNLDesign::Type::Primitive, NLName("wrong_tt_property"));
  auto output = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName("O"));
  WrongTruthTableProperty::create(design);

  EXPECT_EQ(0, SNLDesignModeling::getTruthTableCount(design));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(design).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(design, output->getOrderID())
          .isInitialized());
}
