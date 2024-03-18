// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLPath.h"
#include "SNLEquipotential.h"
#include "SNLPyLoader.h"
using namespace naja::SNL;

#ifndef SNL_BENCHS_PATH
#define SNL_BENCHS_PATH "Undefined"
#endif

class SNLEquipotentialTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);

      auto equipotentialDesignPath = std::filesystem::path(SNL_BENCHS_PATH);
      equipotentialDesignPath /= "equipotential_design.py";
      SNLPyLoader::loadDB(db_, equipotentialDesignPath);
      
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLEquipotentialTest, test) {
  ASSERT_NE(db_, nullptr);
  auto lib = db_->getLibrary(SNLID::LibraryID(1));
  ASSERT_NE(lib, nullptr);
  auto top = lib->getDesign(SNLName("TOP"));
  ASSERT_NE(top, nullptr);
  auto topi0 = top->getScalarTerm(SNLName("i0"));
  ASSERT_NE(topi0, nullptr);
  auto topi1 = top->getScalarTerm(SNLName("i1"));
  ASSERT_NE(topi1, nullptr);
  auto topout = top->getScalarTerm(SNLName("out"));
  ASSERT_NE(topout, nullptr);
  
  SNLPath::PathStringDescriptor aaStringPath = {"a", "aa"};
  SNLPath aaPath(top, aaStringPath);
  EXPECT_EQ(2, aaPath.size());
  SNLPath::PathStringDescriptor bbStringPath = {"b", "bb"};
  SNLPath bbPath(top, bbStringPath);
  EXPECT_EQ(2, bbPath.size());
  SNLPath cPath(top->getInstance(SNLName("c")));
  EXPECT_EQ(1, cPath.size());

  auto aap = aaPath.getModel()->getInstance(SNLName("p"));
  ASSERT_NE(aap, nullptr);
  auto aapi = aap->getInstTerm(aap->getModel()->getScalarTerm(SNLName("i")));
  auto aapio = SNLInstTermOccurrence(aaPath, aapi);

  auto bbp = bbPath.getModel()->getInstance(SNLName("p"));
  ASSERT_NE(bbp, nullptr);
  auto bbpi = bbp->getInstTerm(bbp->getModel()->getScalarTerm(SNLName("i")));
  auto bbpio = SNLInstTermOccurrence(bbPath, bbpi);

  auto cp = cPath.getModel()->getInstance(SNLName("p"));
  ASSERT_NE(cp, nullptr);
  auto cpi = cp->getInstTerm(cp->getModel()->getScalarTerm(SNLName("i")));
  auto cpio = SNLInstTermOccurrence(cPath, cpi);
  
  SNLEquipotential equipotentialTopI0(topi0);
  EXPECT_THAT(equipotentialTopI0.getTerms(), ElementsAre(topi0, topi1, topout));
  EXPECT_THAT(equipotentialTopI0.getInstTermOccurrences(), ElementsAre(aapio, bbpio, cpio));

  SNLEquipotential equipotentialTopI1(topi1);
  EXPECT_THAT(equipotentialTopI1.getTerms(), ElementsAre(topi0, topi1, topout));
  EXPECT_THAT(equipotentialTopI1.getInstTermOccurrences(), ElementsAre(aapio, bbpio, cpio));

  SNLEquipotential equipotentialTopOut(topout);
  EXPECT_THAT(equipotentialTopOut.getTerms(), ElementsAre(topi0, topi1, topout));
  EXPECT_THAT(equipotentialTopOut.getInstTermOccurrences(), ElementsAre(aapio, bbpio, cpio));
}