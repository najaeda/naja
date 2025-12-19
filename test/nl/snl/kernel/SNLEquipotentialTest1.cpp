// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLEquipotential.h"
#include "SNLPyLoader.h"
using namespace naja::NL;

#ifndef SNL_BENCHS_PATH
#define SNL_BENCHS_PATH "Undefined"
#endif

class SNLEquipotentialTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      db_ = NLDB::create(universe);

      auto equipotentialDesignPath = std::filesystem::path(SNL_BENCHS_PATH);
      equipotentialDesignPath /= "equipotential_design1.py";
      SNLPyLoader::loadDB(db_, equipotentialDesignPath);
      
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB* db_;
};

TEST_F(SNLEquipotentialTest1, testConst0) {
  ASSERT_NE(db_, nullptr);
  auto lib = db_->getLibrary(NLID::LibraryID(1));
  ASSERT_NE(lib, nullptr);
  auto top = lib->getSNLDesign(NLName("TOP"));
  ASSERT_NE(top, nullptr);
  auto topout = top->getScalarTerm(NLName("out"));
  ASSERT_NE(topout, nullptr);
  
  SNLPath::PathStringDescriptor aaStringPath = {"a", "aa"};
  SNLPath aaPath(top, aaStringPath);
  EXPECT_EQ(2, aaPath.size());
  SNLPath::PathStringDescriptor bbStringPath = {"b", "bb"};
  SNLPath bbPath(top, bbStringPath);
  EXPECT_EQ(2, bbPath.size());
  SNLPath cPath(top->getInstance(NLName("c")));
  EXPECT_EQ(1, cPath.size());

  auto bbp = bbPath.getModel()->getInstance(NLName("p"));
  ASSERT_NE(bbp, nullptr);
  auto bbpi = bbp->getInstTerm(bbp->getModel()->getScalarTerm(NLName("i")));
  auto bbpio = SNLOccurrence(bbPath, bbpi);

  auto cp = cPath.getModel()->getInstance(NLName("p"));
  ASSERT_NE(cp, nullptr);
  auto cpi = cp->getInstTerm(cp->getModel()->getScalarTerm(NLName("i")));
  auto cpio = SNLOccurrence(cPath, cpi);
  
  SNLEquipotential equipotentialTopOut(topout);
  using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;
  Terms terms;
  terms.insert(topout);
  std::set<SNLOccurrence> instTermOccurrences;
  instTermOccurrences.insert(bbpio);
  instTermOccurrences.insert(cpio);
  EXPECT_EQ(equipotentialTopOut.getTerms(), naja::NajaCollection(new naja::NajaSTLCollection(&terms)));
  EXPECT_EQ(equipotentialTopOut.getInstTermOccurrences(), naja::NajaCollection(new naja::NajaSTLCollection(&instTermOccurrences)));
  EXPECT_TRUE(equipotentialTopOut.getType() == SNLNet::Type::Assign0);
  EXPECT_TRUE(equipotentialTopOut.isConst0());
}

TEST_F(SNLEquipotentialTest1, testConst1) {
  ASSERT_NE(db_, nullptr);
  auto lib = db_->getLibrary(NLID::LibraryID(1));
  ASSERT_NE(lib, nullptr);
  auto top = lib->getSNLDesign(NLName("TOP"));
  ASSERT_NE(top, nullptr);
  auto topout = top->getScalarTerm(NLName("out"));
  ASSERT_NE(topout, nullptr);

  //get AA
  auto aa = lib->getSNLDesign(NLName("AA"));
  ASSERT_NE(aa, nullptr);
  auto aan = aa->getScalarNet(NLName("n"));
  ASSERT_NE(aan, nullptr);
  EXPECT_TRUE(aan->isAssign0());
  aan->setType(SNLNet::Type::Assign1);

  SNLEquipotential equipotentialTopOut(topout);
  EXPECT_TRUE(equipotentialTopOut.getType() == SNLNet::Type::Assign1);
  EXPECT_TRUE(equipotentialTopOut.isConst1());
}