// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLException.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLRTLInfos.h"
#include "SNLScalarTerm.h"

using namespace naja::NL;

class SNLRTLInfosTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLRTLInfosTest, createWithNullDesignObjectThrows) {
  EXPECT_THROW(
    SNLRTLInfos::create(static_cast<SNLDesignObject*>(nullptr)),
    NLException);
}

TEST_F(SNLRTLInfosTest, createWithExistingDesignObjectRTLInfosThrows) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);
  auto* term = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("a"));
  ASSERT_NE(nullptr, term);

  auto* infos = SNLRTLInfos::create(term);
  ASSERT_NE(nullptr, infos);
  EXPECT_THROW(SNLRTLInfos::create(term), NLException);
  infos->destroy();
  EXPECT_FALSE(term->hasRTLInfos());
}

TEST_F(SNLRTLInfosTest, createWithNullDesignThrows) {
  EXPECT_THROW(
    SNLRTLInfos::create(static_cast<SNLDesign*>(nullptr)),
    NLException);
}

TEST_F(SNLRTLInfosTest, createWithExistingDesignRTLInfosThrows) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);

  auto* infos = SNLRTLInfos::create(design);
  ASSERT_NE(nullptr, infos);
  EXPECT_THROW(SNLRTLInfos::create(design), NLException);
  infos->destroy();
  EXPECT_FALSE(design->hasRTLInfos());
}

