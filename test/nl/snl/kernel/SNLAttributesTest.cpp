// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLAttributes.h"
using namespace naja::NL;

class SNLAttributesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLLibrary* library_;
};

TEST_F(SNLAttributesTest, testCreationOnDesign) {
  SNLAttribute empty;
  EXPECT_EQ(NLName(), empty.getName());
  EXPECT_EQ(SNLAttributeValue(), empty.getValue());
  EXPECT_FALSE(empty.hasValue());

  auto design = SNLDesign::create(library_, NLName("DESIGN"));
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
  SNLAttributes::addAttribute(design,
    SNLAttribute(
      NLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(design,
    SNLAttribute(
      NLName("PRAGMA2"),
      SNLAttributeValue("value2")));
  SNLAttributes::addAttribute(design, SNLAttribute(NLName("PRAGMA2")));
  EXPECT_FALSE(SNLAttributes::getAttributes(design).empty());
  EXPECT_EQ(3, SNLAttributes::getAttributes(design).size());

  EXPECT_THAT(
    std::vector(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end()),
    ElementsAre(
      SNLAttribute(
        NLName("PRAGMA1"),
        SNLAttributeValue("value1")),
      SNLAttribute(
        NLName("PRAGMA2"),
        SNLAttributeValue("value2")),
      SNLAttribute(NLName("PRAGMA2")))
    );

  SNLAttributes::clearAttributes(design);
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
}

TEST_F(SNLAttributesTest, testCreationOnDesignObject) {
  auto design = SNLDesign::create(library_, NLName("DESIGN"));
  auto term = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term"));
  auto net = SNLScalarNet::create(design, NLName("net"));
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
  SNLAttributes::addAttribute(term,
    SNLAttribute(
      NLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  SNLAttributes::addAttribute(term,
    SNLAttribute(
      NLName("PRAGMA2"),
      SNLAttributeValue("value2")));
  SNLAttributes::addAttribute(term, SNLAttribute(NLName("PRAGMA2")));
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
        NLName("PRAGMA1"),
        SNLAttributeValue("value1")),
      SNLAttribute(
        NLName("PRAGMA2"),
        SNLAttributeValue("value2")),
      SNLAttribute(NLName("PRAGMA2")))
    );

  EXPECT_THAT(
    std::vector(
      term->getAttributes().begin(),
      term->getAttributes().end()),
    ElementsAre(
      SNLAttribute(
        NLName("PRAGMA1"),
        SNLAttributeValue("value1")),
      SNLAttribute(
        NLName("PRAGMA2"),
        SNLAttributeValue("value2")),
      SNLAttribute(NLName("PRAGMA2")))
    );

  SNLAttributes::clearAttributes(term);
  EXPECT_TRUE(SNLAttributes::getAttributes(term).empty());
}

TEST_F(SNLAttributesTest, testAttributeCompare) {
  SNLAttribute attribute1(NLName("PRAGMA1"), SNLAttributeValue("value1"));
  SNLAttribute attribute2(NLName("PRAGMA1"), SNLAttributeValue("value1"));
  EXPECT_EQ(attribute1, attribute2);
  EXPECT_FALSE(attribute1 < attribute2);
  EXPECT_FALSE(attribute1 > attribute2);
  EXPECT_TRUE(attribute1 <= attribute2);
  EXPECT_TRUE(attribute1 >= attribute2);

  SNLAttribute attribute3(NLName("PRAGMA1"), SNLAttributeValue("value2"));
  EXPECT_NE(attribute1, attribute3);
  EXPECT_TRUE(attribute1 < attribute3);
  EXPECT_FALSE(attribute1 > attribute3);
  EXPECT_TRUE(attribute1 <= attribute3);
  EXPECT_FALSE(attribute1 >= attribute3);

  SNLAttribute attribute4(NLName("PRAGMA2"), SNLAttributeValue("value1"));
  EXPECT_NE(attribute1, attribute4);
  EXPECT_TRUE(attribute1 < attribute4);
  EXPECT_FALSE(attribute1 > attribute4);
  EXPECT_TRUE(attribute1 <= attribute4);
  EXPECT_FALSE(attribute1 >= attribute4);

  SNLAttribute attribute5(NLName("PRAGMA2"), SNLAttributeValue("value2"));
  EXPECT_NE(attribute1, attribute5);
  EXPECT_TRUE(attribute1 < attribute5);
  EXPECT_FALSE(attribute1 > attribute5);
  EXPECT_TRUE(attribute1 <= attribute5);
  EXPECT_FALSE(attribute1 >= attribute5);

  //compare attribute5 with attribute4
  EXPECT_NE(attribute5, attribute4);
  EXPECT_GT(attribute5, attribute4);
  EXPECT_LT(attribute4, attribute5);
  EXPECT_LE(attribute4, attribute5);
  EXPECT_GE(attribute5, attribute4);
}

TEST_F(SNLAttributesTest, testCompare) {
  auto design1 = SNLDesign::create(library_, NLName("DESIGN1"));
  auto design2 = SNLDesign::create(library_, NLName("DESIGN2"));
  std::string reason;
  EXPECT_TRUE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_TRUE(reason.empty());

  SNLAttributes::addAttribute(design1,
    SNLAttribute(
      NLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  EXPECT_FALSE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_FALSE(reason.empty());

  SNLAttributes::addAttribute(design2,
    SNLAttribute(
      NLName("PRAGMA1"),
      SNLAttributeValue("value1")));
  reason = std::string();
  EXPECT_TRUE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_TRUE(reason.empty());

  SNLAttributes::addAttribute(design1,
    SNLAttribute(
      NLName("PRAGMA2"),
      SNLAttributeValue("value2")));
  reason = std::string();
  EXPECT_FALSE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_FALSE(reason.empty());
    SNLAttributes::addAttribute(design2,
    SNLAttribute(
      NLName("PRAGMA2"),
      SNLAttributeValue("value3")));
  reason = std::string();
  EXPECT_FALSE(SNLAttributes::compareAttributes(design1, design2, reason));
  EXPECT_FALSE(reason.empty());
}
