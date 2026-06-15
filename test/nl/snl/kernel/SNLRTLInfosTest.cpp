// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstdint>
#include <limits>
#include <type_traits>

#include "NLException.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLRTLInfos.h"
#include "SNLScalarTerm.h"

using namespace naja::NL;

static_assert(std::is_trivially_copyable_v<SNLSourceLoc>);

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

TEST_F(SNLRTLInfosTest, getInfoMissingKeyReturnsEmptyValue) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);

  auto* infos = SNLRTLInfos::create(design);
  ASSERT_NE(nullptr, infos);
  EXPECT_FALSE(infos->hasInfo(NLName("missing")));
  EXPECT_TRUE(infos->getInfo(NLName("missing")).empty());

  infos->setInfo(NLName("known"), "value");

  const auto missing = infos->getInfo(NLName("missing"));
  EXPECT_TRUE(missing.empty());
  EXPECT_FALSE(infos->hasInfo(NLName("missing")));
  EXPECT_TRUE(infos->hasInfo(NLName("known")));
  EXPECT_EQ("value", infos->getInfo(NLName("known")));

  infos->destroy();
}

TEST_F(SNLRTLInfosTest, sourceLocUsesTypedStorage) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);

  auto* infos = SNLRTLInfos::create(design);
  ASSERT_NE(nullptr, infos);
  EXPECT_FALSE(infos->hasSourceLoc());

  infos->setSourceLoc({NLName("top.sv"), 12, 14, 3, 19});
  ASSERT_TRUE(infos->hasSourceLoc());
  ASSERT_TRUE(infos->getSourceLoc());
  EXPECT_EQ("top.sv", infos->getSourceLoc()->file.getString());
  EXPECT_EQ(12, infos->getSourceLoc()->line);
  EXPECT_EQ(14, infos->getSourceLoc()->endLine);
  EXPECT_EQ(3, infos->getSourceLoc()->column);
  EXPECT_EQ(19, infos->getSourceLoc()->endColumn);

  EXPECT_TRUE(infos->hasInfo(NLName("sv_src_file")));
  EXPECT_EQ("top.sv", infos->getInfo(NLName("sv_src_file")));
  EXPECT_EQ("12", infos->getInfo(NLName("sv_src_line")));
  EXPECT_EQ("3", infos->getInfo(NLName("sv_src_column")));
  EXPECT_EQ("14", infos->getInfo(NLName("sv_src_end_line")));
  EXPECT_EQ("19", infos->getInfo(NLName("sv_src_end_column")));
  EXPECT_TRUE(infos->getInfos().empty());

  const auto attributes = infos->getDumpAttributes();
  ASSERT_EQ(5, attributes.size());
  EXPECT_EQ("sv_src_file", attributes[0].first);
  EXPECT_EQ("top.sv", attributes[0].second);
  EXPECT_EQ("sv_src_line", attributes[1].first);
  EXPECT_EQ("12", attributes[1].second);
  EXPECT_EQ("sv_src_column", attributes[2].first);
  EXPECT_EQ("3", attributes[2].second);
  EXPECT_EQ("sv_src_end_line", attributes[3].first);
  EXPECT_EQ("14", attributes[3].second);
  EXPECT_EQ("sv_src_end_column", attributes[4].first);
  EXPECT_EQ("19", attributes[4].second);

  infos->destroy();
}

TEST_F(SNLRTLInfosTest, legacySourceInfoSettersUseTypedStorage) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);

  auto* infos = SNLRTLInfos::create(design);
  ASSERT_NE(nullptr, infos);
  infos->setInfo(NLName("sv_src_file"), "legacy.sv");
  infos->setInfo(NLName("sv_src_line"), "21");
  infos->setInfo(NLName("sv_src_column"), "4");
  infos->setInfo(NLName("sv_src_end_line"), "23");
  infos->setInfo(NLName("sv_src_end_column"), "8");

  ASSERT_TRUE(infos->hasSourceLoc());
  EXPECT_TRUE(infos->getInfos().empty());
  EXPECT_EQ("legacy.sv", infos->getInfo(NLName("sv_src_file")));
  EXPECT_EQ("21", infos->getInfo(NLName("sv_src_line")));
  EXPECT_EQ("4", infos->getInfo(NLName("sv_src_column")));
  EXPECT_EQ("23", infos->getInfo(NLName("sv_src_end_line")));
  EXPECT_EQ("8", infos->getInfo(NLName("sv_src_end_column")));

  infos->destroy();
}

TEST_F(SNLRTLInfosTest, legacySourceInfoIndividualFirstSettersCreateSourceLoc) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);
  auto* lineTerm =
    SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("line"));
  auto* columnTerm =
    SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("column"));
  auto* endLineTerm =
    SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("end_line"));
  auto* endColumnTerm =
    SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("end_column"));
  ASSERT_NE(nullptr, lineTerm);
  ASSERT_NE(nullptr, columnTerm);
  ASSERT_NE(nullptr, endLineTerm);
  ASSERT_NE(nullptr, endColumnTerm);

  auto* lineInfos = SNLRTLInfos::create(lineTerm);
  auto* columnInfos = SNLRTLInfos::create(columnTerm);
  auto* endLineInfos = SNLRTLInfos::create(endLineTerm);
  auto* endColumnInfos = SNLRTLInfos::create(endColumnTerm);
  ASSERT_NE(nullptr, lineInfos);
  ASSERT_NE(nullptr, columnInfos);
  ASSERT_NE(nullptr, endLineInfos);
  ASSERT_NE(nullptr, endColumnInfos);

  lineInfos->setInfo(NLName("sv_src_line"), "2");
  columnInfos->setInfo(NLName("sv_src_column"), "3");
  endLineInfos->setInfo(NLName("sv_src_end_line"), "5");
  endColumnInfos->setInfo(NLName("sv_src_end_column"), "7");

  ASSERT_TRUE(lineInfos->hasSourceLoc());
  ASSERT_TRUE(columnInfos->hasSourceLoc());
  ASSERT_TRUE(endLineInfos->hasSourceLoc());
  ASSERT_TRUE(endColumnInfos->hasSourceLoc());
  EXPECT_EQ("2", lineInfos->getInfo(NLName("sv_src_line")));
  EXPECT_EQ("3", columnInfos->getInfo(NLName("sv_src_column")));
  EXPECT_EQ("5", endLineInfos->getInfo(NLName("sv_src_end_line")));
  EXPECT_EQ("7", endColumnInfos->getInfo(NLName("sv_src_end_column")));

  lineInfos->destroy();
  columnInfos->destroy();
  endLineInfos->destroy();
  endColumnInfos->destroy();
}

TEST_F(SNLRTLInfosTest, legacySourceInfoNumericParsingHandlesInvalidAndOverflow) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);

  auto* infos = SNLRTLInfos::create(design);
  ASSERT_NE(nullptr, infos);
  infos->setInfo(NLName("sv_src_line"), "not-a-number");
  EXPECT_EQ("0", infos->getInfo(NLName("sv_src_line")));

  const auto tooLargeColumn =
    std::to_string(static_cast<unsigned>(std::numeric_limits<std::uint16_t>::max()) + 1U);
  infos->setInfo(NLName("sv_src_column"), tooLargeColumn);
  EXPECT_EQ(
    std::to_string(std::numeric_limits<std::uint16_t>::max()),
    infos->getInfo(NLName("sv_src_column")));

  infos->destroy();
}

TEST_F(SNLRTLInfosTest, cloneCopiesSourceLocAndDeepCopiesExtraInfos) {
  auto* design = SNLDesign::create(library_, NLName("TOP"));
  ASSERT_NE(nullptr, design);
  auto* term = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("a"));
  ASSERT_NE(nullptr, term);

  auto* infos = SNLRTLInfos::create(design);
  ASSERT_NE(nullptr, infos);
  infos->setSourceLoc({NLName("top.sv"), 7, 9, 2, 11});
  infos->setInfo(NLName("custom"), "value");

  auto* clonedInfos = SNLRTLInfos::create(term);
  ASSERT_NE(nullptr, clonedInfos);
  clonedInfos->cloneInfos(*infos);

  ASSERT_TRUE(clonedInfos->hasSourceLoc());
  EXPECT_EQ("top.sv", clonedInfos->getInfo(NLName("sv_src_file")));
  EXPECT_EQ("7", clonedInfos->getInfo(NLName("sv_src_line")));
  EXPECT_EQ("value", clonedInfos->getInfo(NLName("custom")));
  ASSERT_EQ(1, clonedInfos->getInfos().size());

  infos->setInfo(NLName("custom"), "updated");
  EXPECT_EQ("value", clonedInfos->getInfo(NLName("custom")));

  const auto attributes = clonedInfos->getDumpAttributes();
  ASSERT_EQ(6, attributes.size());
  EXPECT_EQ("sv_src_file", attributes[0].first);
  EXPECT_EQ("custom", attributes[5].first);
  EXPECT_EQ("value", attributes[5].second);

  clonedInfos->destroy();
  infos->destroy();
}
