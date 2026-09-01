// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLDB0.h"
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

class SNLEquipotentialTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      db_ = NLDB::create(universe);

      auto equipotentialDesignPath = std::filesystem::path(SNL_BENCHS_PATH);
      equipotentialDesignPath /= "equipotential_design0.py";
      SNLPyLoader::loadDB(db_, equipotentialDesignPath);
      
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB* db_;
};

TEST_F(SNLEquipotentialTest0, test) {
  ASSERT_NE(db_, nullptr);
  auto lib = db_->getLibrary(NLID::LibraryID(1));
  ASSERT_NE(lib, nullptr);
  auto top = lib->getSNLDesign(NLName("TOP"));
  ASSERT_NE(top, nullptr);
  auto topi0 = top->getScalarTerm(NLName("i0"));
  ASSERT_NE(topi0, nullptr);
  auto topi1 = top->getScalarTerm(NLName("i1"));
  ASSERT_NE(topi1, nullptr);
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

  auto aap = aaPath.getModel()->getInstance(NLName("p"));
  ASSERT_NE(aap, nullptr);
  auto aapi = aap->getInstTerm(aap->getModel()->getScalarTerm(NLName("i")));
  auto aapio = SNLOccurrence(aaPath, aapi);

  auto bbp = bbPath.getModel()->getInstance(NLName("p"));
  ASSERT_NE(bbp, nullptr);
  auto bbpi = bbp->getInstTerm(bbp->getModel()->getScalarTerm(NLName("i")));
  auto bbpio = SNLOccurrence(bbPath, bbpi);

  auto cp = cPath.getModel()->getInstance(NLName("p"));
  ASSERT_NE(cp, nullptr);
  auto cpi = cp->getInstTerm(cp->getModel()->getScalarTerm(NLName("i")));
  auto cpio = SNLOccurrence(cPath, cpi);
  
  SNLEquipotential equipotentialTopI0(topi0);
  using Terms = std::set<SNLBitTerm*, SNLDesignObject::PointerLess>;
  Terms terms;
  terms.insert(topi0);
  terms.insert(topi1);
  terms.insert(topout);
  std::set<SNLOccurrence> instTermOccurrences;
  instTermOccurrences.insert(aapio);
  instTermOccurrences.insert(bbpio);
  instTermOccurrences.insert(cpio);
  EXPECT_EQ(equipotentialTopI0.getTerms(), naja::NajaCollection(new naja::NajaSTLCollection(&terms)));
  EXPECT_EQ(equipotentialTopI0.getInstTermOccurrences(), naja::NajaCollection(new naja::NajaSTLCollection(&instTermOccurrences)));

  SNLEquipotential equipotentialTopI1(topi1);
  EXPECT_EQ(equipotentialTopI1.getTerms(), naja::NajaCollection(new naja::NajaSTLCollection(&terms)));
  EXPECT_EQ(equipotentialTopI1.getInstTermOccurrences(), naja::NajaCollection(new naja::NajaSTLCollection(&instTermOccurrences)));

  SNLEquipotential equipotentialTopOut(topout);
  EXPECT_EQ(equipotentialTopOut.getTerms(), naja::NajaCollection(new naja::NajaSTLCollection(&terms)));
  EXPECT_EQ(equipotentialTopOut.getInstTermOccurrences(), naja::NajaCollection(new naja::NajaSTLCollection(&instTermOccurrences)));

  std::set<SNLOccurrence> instTermOccurrences1;
  instTermOccurrences.insert(aapio);
  instTermOccurrences.insert(bbpio);
  instTermOccurrences.insert(cpio);
  std::set<SNLOccurrence> instTermOccurrences2;
  instTermOccurrences.insert(aapio);
  instTermOccurrences.insert(bbpio);
  instTermOccurrences.insert(bbpio);
  EXPECT_EQ(equipotentialTopOut.getInstTermOccurrences() ==  naja::NajaCollection(new naja::NajaSTLCollection(&instTermOccurrences1)), false);
  EXPECT_EQ(equipotentialTopOut.getInstTermOccurrences() == naja::NajaCollection(new naja::NajaSTLCollection(&instTermOccurrences2)), false);

  //Test comparators
  EXPECT_EQ(equipotentialTopI0, equipotentialTopI1);
  EXPECT_EQ(equipotentialTopI0, equipotentialTopOut);
  EXPECT_EQ(equipotentialTopI1, equipotentialTopOut);
  EXPECT_LE(equipotentialTopI0, equipotentialTopI1);
  EXPECT_LE(equipotentialTopI0, equipotentialTopOut);
  EXPECT_LE(equipotentialTopI1, equipotentialTopOut);
  EXPECT_GE(equipotentialTopI0, equipotentialTopI1);
  EXPECT_GE(equipotentialTopI0, equipotentialTopOut);
  EXPECT_GE(equipotentialTopI1, equipotentialTopOut);
  EXPECT_EQ(equipotentialTopI0 < equipotentialTopI1, false);
  EXPECT_EQ(equipotentialTopI0 < equipotentialTopOut, false);
  EXPECT_EQ(equipotentialTopI1 < equipotentialTopOut, false);
  EXPECT_EQ(equipotentialTopI0 > equipotentialTopI1, false);
  EXPECT_EQ(equipotentialTopI0 > equipotentialTopOut, false);
  EXPECT_EQ(equipotentialTopI1 > equipotentialTopOut, false);
  EXPECT_EQ(equipotentialTopI0 != equipotentialTopI1, false);
}

TEST_F(SNLEquipotentialTest0, testTraverseAssigns) {
  auto lib = db_->getLibrary(NLID::LibraryID(1));
  ASSERT_NE(nullptr, lib);
  auto design = SNLDesign::create(lib, NLName("ASSIGN_TOP"));
  auto input = SNLScalarTerm::create(
    design, SNLTerm::Direction::Input, NLName("input"));
  auto output = SNLScalarTerm::create(
    design, SNLTerm::Direction::Output, NLName("output"));
  auto inputNet = SNLScalarNet::create(design, NLName("input_net"));
  auto middleNet = SNLScalarNet::create(design, NLName("middle_net"));
  auto outputNet = SNLScalarNet::create(design, NLName("output_net"));
  inputNet->setType(SNLNet::Type::Assign1);
  input->setNet(inputNet);
  output->setNet(outputNet);

  auto assign0 = SNLInstance::create(design, NLDB0::getAssign());
  assign0->getInstTerm(NLDB0::getAssignInput())->setNet(inputNet);
  assign0->getInstTerm(NLDB0::getAssignOutput())->setNet(middleNet);
  auto assign1 = SNLInstance::create(design, NLDB0::getAssign());
  assign1->getInstTerm(NLDB0::getAssignInput())->setNet(middleNet);
  assign1->getInstTerm(NLDB0::getAssignOutput())->setNet(outputNet);

  SNLEquipotential standard(input);
  EXPECT_THAT(standard.getTermsSet(), ElementsAre(input));
  EXPECT_THAT(
    standard.getInstTermOccurrencesSet(),
    ElementsAre(SNLOccurrence(assign0->getInstTerm(NLDB0::getAssignInput()))));

  SNLEquipotential traversed(
    input, SNLEquipotential::Mode::TraverseAssigns);
  EXPECT_THAT(traversed.getTermsSet(), ElementsAre(input, output));
  EXPECT_TRUE(traversed.getInstTermOccurrencesSet().empty());
  EXPECT_TRUE(traversed.isConst1());

  SNLEquipotential traversedFromOutput(
    output, SNLEquipotential::Mode::TraverseAssigns);
  EXPECT_EQ(traversed, traversedFromOutput);
  EXPECT_TRUE(traversedFromOutput.isConst1());
}
