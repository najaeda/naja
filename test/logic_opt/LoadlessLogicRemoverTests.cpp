// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"
#include "RemoveLoadlessLogic.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLException.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLUniverse.h"
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
    SNLUniverse::get()->destroy();
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
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
  auto outTerm1 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out1"));
  auto outTerm2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out2"));

  SNLDesign* bb = SNLDesign::create(library, SNLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, SNLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, SNLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, SNLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, SNLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, SNLName("inNet1"));
  auto outNet1 = SNLScalarNet::create(mod, SNLName("outNet1"));
  auto outNet2 = SNLScalarNet::create(mod, SNLName("outNet2"));
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
  std::set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 3);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  //std::set<SNLBitNet*> loadlessNets = remover.getLoadlessNets(*dnl, tracedIsos);
  //EXPECT_EQ(loadlessNets.size(), 0);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> loadlessInstances =
      remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 0);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 2);
  destroy();
}

TEST_F(LoadlessRemoveLogicTests, simple_1_loadless) {
  // Create a simple logic with a single input and output
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));

  SNLDesign* bb = SNLDesign::create(library, SNLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, SNLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, SNLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, SNLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, SNLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, SNLName("inNet1"));
  auto outNet1 = SNLScalarNet::create(mod, SNLName("outNet1"));
  // auto inNet2 = SNLScalarNet::create(mod, SNLName("inNet2"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst1->getInstTerm(outTermBB)->setNet(outNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inTerm->setNet(inNet1);
  outTerm->setNet(outNet1);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  std::set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 2);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 0);
  //std::set<SNLBitNet*> loadlessNets = remover.getLoadlessNets(*dnl, tracedIsos);
  //EXPECT_EQ(loadlessNets.size(), 0);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> loadlessInstances =
      remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 1);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 1);
  destroy();
}

TEST_F(LoadlessRemoveLogicTests, simple_2_loadless) {
  // Create a simple logic with a single input and output
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));

  SNLDesign* bb = SNLDesign::create(library, SNLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, SNLName("in"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, SNLName("out"));

  SNLInstance* inst1 = SNLInstance::create(mod, bb, SNLName("bb1"));
  SNLInstance* inst2 = SNLInstance::create(mod, bb, SNLName("bb2"));
  auto inNet1 = SNLScalarNet::create(mod, SNLName("inNet1"));
  inst1->getInstTerm(inTermBB)->setNet(inNet1);
  inst2->getInstTerm(inTermBB)->setNet(inNet1);
  inTerm->setNet(inNet1);
  DNLFull* dnl = get();
  LoadlessLogicRemover remover;
  // Verify each function of remover
  std::set<DNLID> tracedIsos = remover.getTracedIsos(*dnl);
  EXPECT_EQ(tracedIsos.size(), 0);
  std::vector<DNLID> untracedIsos = remover.getUntracedIsos(*dnl, tracedIsos);
  EXPECT_EQ(untracedIsos.size(), 1);
  //std::set<SNLBitNet*> loadlessNets = lnr.getLoadlessNets(*dnl, tracedIsos);
  //EXPECT_EQ(loadlessNets.size(), 1);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> loadlessInstances =
      remover.getLoadlessInstances(*dnl, tracedIsos);
  EXPECT_EQ(loadlessInstances.size(), 2);
  destroy();
  remover.process();
  // Check that the loadless logic is removed
  EXPECT_EQ(mod->getInstances().size(), 0);
  destroy();
}