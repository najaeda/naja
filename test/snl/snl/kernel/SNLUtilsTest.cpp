// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLUtils.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLUtilsTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLUtilsTest, testSimpleHierarchy) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  
  auto top = SNLDesign::create(library, SNLName("top"));
  auto level0 = SNLDesign::create(library, SNLName("level0"));
  auto level2 = SNLDesign::create(library, SNLName("level2"));
  auto level1 = SNLDesign::create(library, SNLName("level1"));
  auto level0Ins = SNLInstance::create(level1, level0, SNLName("level0"));
  auto level1Ins = SNLInstance::create(level2, level1, SNLName("level1"));
  auto level2Ins = SNLInstance::create(top, level2, SNLName("level2"));

  SNLUtils::SortedDesigns sortedDesigns;
  SNLUtils::getDesignsSortedByHierarchicalLevel(top, sortedDesigns);
  EXPECT_THAT(sortedDesigns, ElementsAre(
    SNLUtils::DesignLevel(level0, 0),
    SNLUtils::DesignLevel(level1, 1),
    SNLUtils::DesignLevel(level2, 2),
    SNLUtils::DesignLevel(top, 3)
  ));
}

TEST_F(SNLUtilsTest, testDoubleHierarchy) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  
  auto top = SNLDesign::create(library, SNLName("top"));
  auto level21 = SNLDesign::create(library, SNLName("level20"));
  auto level0 = SNLDesign::create(library, SNLName("level0"));
  auto level20 = SNLDesign::create(library, SNLName("level21"));
  auto level1 = SNLDesign::create(library, SNLName("level1"));
  auto level20_1Ins = SNLInstance::create(level20, level1, SNLName("level1"));
  auto level21_1Ins = SNLInstance::create(level21, level1, SNLName("level1"));
  auto level0Ins = SNLInstance::create(level1, level0, SNLName("level0"));
  auto level20Ins = SNLInstance::create(top, level20, SNLName("level20"));
  auto level21Ins = SNLInstance::create(top, level21, SNLName("level21"));

  SNLUtils::SortedDesigns sortedDesigns;
  SNLUtils::getDesignsSortedByHierarchicalLevel(top, sortedDesigns);
  EXPECT_THAT(sortedDesigns, ElementsAre(
    SNLUtils::DesignLevel(level0, 0),
    SNLUtils::DesignLevel(level1, 1),
    SNLUtils::DesignLevel(level21, 2),
    SNLUtils::DesignLevel(level20, 2), //sorted by SNLID
    SNLUtils::DesignLevel(top, 3)
  ));
}