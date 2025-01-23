// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLAttributes.h"
using namespace naja::SNL;

class SNLAttributesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLLibrary* library_;
};

TEST_F(SNLAttributesTest, testCreationOnDesign) {
  SNLAttribute empty;
  EXPECT_EQ(SNLName(), empty.getName());
  EXPECT_EQ(SNLAttributeValue(), empty.getValue());
  EXPECT_FALSE(empty.hasValue());

  auto design = SNLDesign::create(library_, SNLName("DESIGN"));
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
  SNLAttributes::addAttribute(design,
    SNLAttribute(
      SNLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(design,
    SNLAttribute(
      SNLName("PRAGMA2"),
      SNLAttributeValue("value2")));
  SNLAttributes::addAttribute(design, SNLAttribute(SNLName("PRAGMA2")));
  EXPECT_FALSE(SNLAttributes::getAttributes(design).empty());
  EXPECT_EQ(3, SNLAttributes::getAttributes(design).size());

  EXPECT_THAT(
    std::vector(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end()),
    ElementsAre(
      SNLAttribute(
        SNLName("PRAGMA1"),
        SNLAttributeValue("value1")),
      SNLAttribute(
        SNLName("PRAGMA2"),
        SNLAttributeValue("value2")),
      SNLAttribute(SNLName("PRAGMA2")))
    );

  SNLAttributes::clearAttributes(design);
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
}

TEST_F(SNLAttributesTest, testCreationOnDesignObject) {
  auto design = SNLDesign::create(library_, SNLName("DESIGN"));
  auto term = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("term"));
  auto net = SNLScalarNet::create(design, SNLName("net"));
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
  SNLAttributes::addAttribute(term,
    SNLAttribute(
      SNLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(term,
    SNLAttribute(
      SNLName("PRAGMA2"),
      SNLAttributeValue("value2")));
  SNLAttributes::addAttribute(term, SNLAttribute(SNLName("PRAGMA2")));
  EXPECT_FALSE(SNLAttributes::getAttributes(term).empty());
  EXPECT_FALSE(term->getAttributes().empty());
  EXPECT_EQ(3, SNLAttributes::getAttributes(term).size());
  EXPECT_EQ(3, term->getAttributes().size());
  EXPECT_THAT(
    std::vector(
      SNLAttributes::getAttributes(term).begin(),
      SNLAttributes::getAttributes(term).end()),
    ElementsAre(
      SNLAttribute(
        SNLName("PRAGMA1"),
        SNLAttributeValue("value1")),
      SNLAttribute(
        SNLName("PRAGMA2"),
        SNLAttributeValue("value2")),
      SNLAttribute(SNLName("PRAGMA2")))
    );

  EXPECT_THAT(
    std::vector(
      term->getAttributes().begin(),
      term->getAttributes().end()),
    ElementsAre(
      SNLAttribute(
        SNLName("PRAGMA1"),
        SNLAttributeValue("value1")),
      SNLAttribute(
        SNLName("PRAGMA2"),
        SNLAttributeValue("value2")),
      SNLAttribute(SNLName("PRAGMA2")))
    );

  SNLAttributes::clearAttributes(term);
  EXPECT_TRUE(SNLAttributes::getAttributes(term).empty());
}

TEST_F(SNLAttributesTest, testCompare) {
  auto design1 = SNLDesign::create(library_, SNLName("DESIGN1"));
  auto design2 = SNLDesign::create(library_, SNLName("DESIGN2"));
  std::string reason;
  EXPECT_TRUE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_TRUE(reason.empty());

  SNLAttributes::addAttribute(design1,
    SNLAttribute(
      SNLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  EXPECT_FALSE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_FALSE(reason.empty());

  SNLAttributes::addAttribute(design2,
    SNLAttribute(
      SNLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  reason = std::string();
  EXPECT_TRUE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_TRUE(reason.empty());

  SNLAttributes::addAttribute(design1,
    SNLAttribute(
      SNLName("PRAGMA2"),
      SNLAttributeValue("value2")));
  reason = std::string();
  EXPECT_FALSE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_FALSE(reason.empty());
    SNLAttributes::addAttribute(design2,
    SNLAttribute(
      SNLName("PRAGMA2"),
      SNLAttributeValue("value3")));
  reason = std::string();
  EXPECT_FALSE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_FALSE(reason.empty());
}
