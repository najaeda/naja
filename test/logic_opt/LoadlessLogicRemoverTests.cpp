// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"
#include "RemoveLoadlessLogic.h"
#include "NLException.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "NLUniverse.h"
#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::NAJA_OPT;

class LoadlessRemoveLogicTests : public ::testing::Test {
 protected:
  LoadlessRemoveLogicTests() {
    // You can do set-up work for each test here
  }
  ~LoadlessRemoveLogicTests() override {
    // You can do clean-up work that doesn't throw exceptions here
  }
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }
  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    // Destroy the SNL
    NLUniverse::get()->destroy();
  }
};

// Create a test to check the LoadlessLogicRemover
// 1. Create a simple blackbox design with a single input and output
// 2. Create a design with the 2 instaces of the simple blackbox, one drives the
// top output, one is not.
// 3. Create a DNL with the design
// 4. Create a LoadlessLogicRemover with the DNL
// 5. Check RemoveLoadlessLogic is removing all loadless logic from SNL

TEST_F(LoadlessRemoveLogicTests, simple_0_loadless) {
  // Create a simple logic with a single input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm1 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out1"));
  auto outTerm2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, NLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  auto outNet1 = SNLScalarNet::create(mod, NLName("outNet1"));
  auto outNet2 = SNLScalarNet::create(mod, NLName("outNet2"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst1->getInstTerm(outTermBB)->setNet(outNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inst2->getInstTerm(outTermBB)->setNet(outNet2);
  inTerm->setNet(inNet1);
  outTerm1->setNet(outNet1);
  outTerm2->setNet(outNet2);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 3);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = remover.getLoadlessNets(*dnl,
  // tracedIsos); EXPECT_EQ(loadlessNets.size(), 0);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 0);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 2);
  destroy();
}

TEST_F(LoadlessRemoveLogicTests, simple_0_loadlessNonMT) {
  setenv("NON_MT", "", 1);
  // Create a simple logic with a single input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm1 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out1"));
  auto outTerm2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, NLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  auto outNet1 = SNLScalarNet::create(mod, NLName("outNet1"));
  auto outNet2 = SNLScalarNet::create(mod, NLName("outNet2"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst1->getInstTerm(outTermBB)->setNet(outNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inst2->getInstTerm(outTermBB)->setNet(outNet2);
  inTerm->setNet(inNet1);
  outTerm1->setNet(outNet1);
  outTerm2->setNet(outNet2);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 3);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = remover.getLoadlessNets(*dnl,
  // tracedIsos); EXPECT_EQ(loadlessNets.size(), 0);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 0);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 2);
  destroy();
}

TEST_F(LoadlessRemoveLogicTests, simple_1_loadless) {
  // Create a simple logic with a single input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, NLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  auto outNet1 = SNLScalarNet::create(mod, NLName("outNet1"));
  // auto inNet2 = SNLScalarNet::create(mod, NLName("inNet2"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst1->getInstTerm(outTermBB)->setNet(outNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inTerm->setNet(inNet1);
  outTerm->setNet(outNet1);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 2);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = remover.getLoadlessNets(*dnl,
  // tracedIsos); EXPECT_EQ(loadlessNets.size(), 0);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 1);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 1);
  destroy();
}

TEST_F(LoadlessRemoveLogicTests, simple_2_loadless) {
  // Create a simple logic with a single input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, NLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inTerm->setNet(inNet1);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 0);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 1);
  // std::set<SNLBitNet*> loadlessNets = lnr.getLoadlessNets(*dnl, tracedIsos);
  // EXPECT_EQ(loadlessNets.size(), 1);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 2);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 0);
  destroy();
}

TEST_F(LoadlessRemoveLogicTests, simple_2_loadless_nonMT) {
  setenv("NON_MT", "", 1);
  // Create a simple logic with a single input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bbNoOutput = SNLDesign::create(library, NLName("bbNoOutput"));
  auto inTermBBno = SNLScalarTerm::create(bbNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  SNLDesign* bbNoOutputNoInput =
      SNLDesign::create(library, NLName("bbNoOutputNoInput"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, NLName("bb2"));
  SNLInstance* inst3 = SNLInstance::create(mod, bbNoOutput, NLName("bbno"));
  SNLInstance* inst4 =
      SNLInstance::create(mod, bbNoOutputNoInput, NLName("bbnoni"));
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inst3->getInstTerm(inTermBBno)->setNet(inNet1);
  inTerm->setNet(inNet1);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 1);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = lnr.getLoadlessNets(*dnl, tracedIsos);
  // EXPECT_EQ(loadlessNets.size(), 1);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 2);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 2);
  destroy();
}

// Building a test like the prvious only with 2 levels of hierarchy
TEST_F(LoadlessRemoveLogicTests, simple_2_loadless_2_levels) {
  // Create a simple logic with a single
  // input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bbNoOutput = SNLDesign::create(library, NLName("bbNoOutput"));
  auto inTermBBno = SNLScalarTerm::create(bbNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  SNLDesign* bbNoOutputNoInput =
      SNLDesign::create(library, NLName("bbNoOutputNoInput"));

  SNLInstance* bb1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* bb2 = SNLInstance::create(mod, bbNoOutput, NLName("bb2"));
  SNLInstance* bbnoni =
      SNLInstance::create(mod, bbNoOutputNoInput, NLName("bbnoni"));

  SNLDesign* hierNoOutput = SNLDesign::create(library, NLName("hierNoOutput"));
  auto inTermHIno = SNLScalarTerm::create(hierNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  
  SNLInstance* bb3 =
      SNLInstance::create(hierNoOutput, bb, NLName("bb1"));
  SNLInstance* bb4 =
      SNLInstance::create(hierNoOutput, bb, NLName("bb2"));
  
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  bb1->getInstTerm(inTermBB)->setNet(inNet1);
  bb2->getInstTerm(inTermBBno)->setNet(inNet1);
  inTerm->setNet(inNet1);
  auto inNetHier = SNLScalarNet::create(hierNoOutput, NLName("inNetHier"));
  bb3->getInstTerm(inTermBB)->setNet(inNetHier);
  bb4->getInstTerm(inTermBB)->setNet(inNetHier);
  inTermHIno->setNet(inNetHier);
  SNLInstance* hi1 = SNLInstance::create(mod, hierNoOutput, NLName("hi1"));
  SNLInstance* hi2 = SNLInstance::create(mod, hierNoOutput, NLName("hi2"));
  hi1->getInstTerm(inTermHIno)->setNet(inNet1);
  hi2->getInstTerm(inTermHIno)->setNet(inNet1);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 1);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = lnr.getLoadlessNets(*dnl, tracedIsos);
  // EXPECT_EQ(loadlessNets.size(), 1);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 5);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 4);
  destroy();
}

// Building a test like the prvious only with 3 levels of hierarchy
TEST_F(LoadlessRemoveLogicTests, simple_2_loadless_3_levels) {
  // Create a simple logic with a single
  // input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* top = SNLDesign::create(library, NLName("top"));
  univ->setTopDesign(top);
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
 
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bbNoOutput = SNLDesign::create(library, NLName("bbNoOutput"));
  auto inTermBBno = SNLScalarTerm::create(bbNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  SNLDesign* bbNoOutputNoInput =
      SNLDesign::create(library, NLName("bbNoOutputNoInput"));

  SNLInstance* bb1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* bb2 = SNLInstance::create(mod, bbNoOutput, NLName("bb2"));
  SNLInstance* bbnoni =
      SNLInstance::create(mod, bbNoOutputNoInput, NLName("bbnoni"));

  SNLDesign* hierNoOutput = SNLDesign::create(library, NLName("hierNoOutput"));
  auto inTermHIno = SNLScalarTerm::create(hierNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  
  SNLInstance* bb3 =
      SNLInstance::create(hierNoOutput, bb, NLName("bb1"));
  SNLInstance* bb4 =
      SNLInstance::create(hierNoOutput, bb, NLName("bb2"));
  
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  bb1->getInstTerm(inTermBB)->setNet(inNet1);
  bb2->getInstTerm(inTermBBno)->setNet(inNet1);
  inTerm->setNet(inNet1);
  auto inNetHier = SNLScalarNet::create(hierNoOutput, NLName("inNetHier"));
  bb3->getInstTerm(inTermBB)->setNet(inNetHier);
  bb4->getInstTerm(inTermBB)->setNet(inNetHier);
  inTermHIno->setNet(inNetHier);
  SNLInstance* hi1 = SNLInstance::create(mod, hierNoOutput, NLName("hi1"));
  SNLInstance* hi2 = SNLInstance::create(mod, hierNoOutput, NLName("hi2"));
  hi1->getInstTerm(inTermHIno)->setNet(inNet1);
  hi2->getInstTerm(inTermHIno)->setNet(inNet1);
  SNLInstance* modInst1 = SNLInstance::create(top, mod, NLName("modInst1"));
  SNLInstance* modInst2 = SNLInstance::create(top, mod, NLName("modInst2"));
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  remover.setNormalizedUniquification(false);
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 1);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = lnr.getLoadlessNets(*dnl, tracedIsos);
  // EXPECT_EQ(loadlessNets.size(), 1);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 10);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  destroy();
}


// Building a test like the prvious only with 3 levels of hierarchy
TEST_F(LoadlessRemoveLogicTests, simple_2_loadless_3_levels_bne) {
  // Create a simple logic with a single
  // input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* top = SNLDesign::create(library, NLName("top"));
  univ->setTopDesign(top);
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
 
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb = SNLDesign::create(library, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bbNoOutput = SNLDesign::create(library, NLName("bbNoOutput"));
  auto inTermBBno = SNLScalarTerm::create(bbNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  SNLDesign* bbNoOutputNoInput =
      SNLDesign::create(library, NLName("bbNoOutputNoInput"));

  SNLInstance* bb1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* bb2 = SNLInstance::create(mod, bbNoOutput, NLName("bb2"));
  SNLInstance* bbnoni =
      SNLInstance::create(mod, bbNoOutputNoInput, NLName("bbnoni"));

  SNLDesign* hierNoOutput = SNLDesign::create(library, NLName("hierNoOutput"));
  auto inTermHIno = SNLScalarTerm::create(hierNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  
  SNLInstance* bb3 =
      SNLInstance::create(hierNoOutput, bb, NLName("bb1"));
  SNLInstance* bb4 =
      SNLInstance::create(hierNoOutput, bb, NLName("bb2"));
  
  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  bb1->getInstTerm(inTermBB)->setNet(inNet1);
  bb2->getInstTerm(inTermBBno)->setNet(inNet1);
  inTerm->setNet(inNet1);
  auto inNetHier = SNLScalarNet::create(hierNoOutput, NLName("inNetHier"));
  bb3->getInstTerm(inTermBB)->setNet(inNetHier);
  bb4->getInstTerm(inTermBB)->setNet(inNetHier);
  inTermHIno->setNet(inNetHier);
  SNLInstance* hi1 = SNLInstance::create(mod, hierNoOutput, NLName("hi1"));
  SNLInstance* hi2 = SNLInstance::create(mod, hierNoOutput, NLName("hi2"));
  hi1->getInstTerm(inTermHIno)->setNet(inNet1);
  hi2->getInstTerm(inTermHIno)->setNet(inNet1);
  SNLInstance* modInst1 = SNLInstance::create(top, mod, NLName("modInst1"));
  SNLInstance* modInst2 = SNLInstance::create(top, mod, NLName("modInst2"));
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  remover.setNormalizedUniquification(true);
  // Verify each function of remover
  tbb::concurrent_unordered_set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 1);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  // std::set<SNLBitNet*> loadlessNets = lnr.getLoadlessNets(*dnl, tracedIsos);
  // EXPECT_EQ(loadlessNets.size(), 1);
  auto loadlessInstances = remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 10);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  destroy();
}

