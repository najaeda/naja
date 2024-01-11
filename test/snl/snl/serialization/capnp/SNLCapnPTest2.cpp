// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"

using namespace naja::SNL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapNpTest2: public ::testing::Test {
  //Test assign to constants
  protected:
    void SetUp() override {
      //
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* designs = SNLLibrary::create(db_, SNLName("designs"));
      SNLDesign* top = SNLDesign::create(designs, SNLName("top"));
      universe->setTopDesign(top);
      auto assign0 = SNLScalarNet::create(top);
      assign0->setType(SNLNet::Type::Assign0);
      auto assign1 = SNLScalarNet::create(top);
      assign1->setType(SNLNet::Type::Assign1);
      auto n0 = SNLScalarNet::create(top, SNLName("n0"));
      auto assign_ins0 = SNLInstance::create(top, SNLDB0::getAssign());
      assign_ins0->getInstTerm(SNLDB0::getAssignInput())->setNet(assign0);
      assign_ins0->getInstTerm(SNLDB0::getAssignOutput())->setNet(n0);
      auto n1 = SNLScalarNet::create(top, SNLName("n1"));
      auto assign_ins1 = SNLInstance::create(top, SNLDB0::getAssign());
      assign_ins1->getInstTerm(SNLDB0::getAssignInput())->setNet(assign1);
      assign_ins1->getInstTerm(SNLDB0::getAssignOutput())->setNet(n1);
      auto b0 = SNLBusNet::create(top, 3, 0, SNLName("b0"));
      auto b1 = SNLBusNet::create(top, 3, 0, SNLName("b1"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
  protected:
    SNLDB*      db_;
};

TEST_F(SNLCapNpTest2, test0) {
  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapNpTest2_test0.snl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);
  SNLUniverse::get()->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(SNLID::DBID(1), db_->getID());
  auto top = db_->getTopDesign();
  EXPECT_NE(nullptr, top);
  //assign0, assign1, n0, n1, b0, b1
  ASSERT_EQ(6, top->getNets().size());
  auto n0 = top->getScalarNet(SNLName("n0"));
  ASSERT_NE(nullptr, n0);
  EXPECT_EQ(SNLNet::Type::Standard, n0->getType());
  EXPECT_EQ(1, n0->getInstTerms().size());
  auto assign_ins0_output = *(n0->getInstTerms().begin());
  EXPECT_EQ(SNLDB0::getAssignOutput(), assign_ins0_output->getTerm());
  auto assign_ins0 = assign_ins0_output->getInstance();
  EXPECT_EQ(SNLDB0::getAssign(), assign_ins0->getModel());
  auto assign_ins0_input = assign_ins0->getInstTerm(SNLDB0::getAssignInput());
  auto assign_ins0_input_net = assign_ins0_input->getNet();
  EXPECT_NE(assign_ins0_input_net, nullptr);
  EXPECT_EQ(SNLNet::Type::Assign0, assign_ins0_input_net->getType());
  EXPECT_TRUE(assign_ins0_input_net->isConstant0());
}