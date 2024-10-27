// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDesign.h"
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
  auto design = SNLDesign::create(library_, SNLName("DESIGN"));
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
  SNLAttributes::addAttribute(design, SNLAttributes::SNLAttribute(SNLName("PRAGMA1"), "value1"));
  SNLAttributes::addAttribute(design, SNLAttributes::SNLAttribute(SNLName("PRAGMA2"), "value2"));
  SNLAttributes::addAttribute(design, SNLAttributes::SNLAttribute(SNLName("PRAGMA2")));
  EXPECT_FALSE(SNLAttributes::getAttributes(design).empty());
  EXPECT_EQ(3, SNLAttributes::getAttributes(design).size());

  EXPECT_THAT(
    std::vector(
      SNLAttributes::getAttributes(design).begin(),
      SNLAttributes::getAttributes(design).end()),
    ElementsAre(
      SNLAttributes::SNLAttribute(SNLName("PRAGMA1"), "value1"),
      SNLAttributes::SNLAttribute(SNLName("PRAGMA2"), "value2"),
      SNLAttributes::SNLAttribute(SNLName("PRAGMA2")))
    );

  SNLAttributes::clearAttributes(design);
  EXPECT_TRUE(SNLAttributes::getAttributes(design).empty());
}