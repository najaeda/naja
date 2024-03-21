// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
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
      b0->getBit(0)->setType(SNLNet::Type::Assign0);
      b0->getBit(1)->setType(SNLNet::Type::Assign1);
      b0->getBit(2)->setType(SNLNet::Type::Supply0);
      b0->getBit(3)->setType(SNLNet::Type::Supply1);
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
  //assign0, assign1, n0, n1, b0
  ASSERT_EQ(5, top->getNets().size());
  auto n0 = top->getScalarNet(SNLName("n0"));
  ASSERT_NE(nullptr, n0);
  EXPECT_EQ(SNLNet::Type::Standard, n0->getType());
  EXPECT_EQ(1, n0->getInstTerms().size());
  auto assign_ins0_output = *(n0->getInstTerms().begin());
  EXPECT_EQ(SNLDB0::getAssignOutput(), assign_ins0_output->getBitTerm());
  auto assign_ins0 = assign_ins0_output->getInstance();
  EXPECT_EQ(SNLDB0::getAssign(), assign_ins0->getModel());
  auto assign_ins0_input = assign_ins0->getInstTerm(SNLDB0::getAssignInput());
  auto assign_ins0_input_net = assign_ins0_input->getNet();
  EXPECT_NE(assign_ins0_input_net, nullptr);
  EXPECT_EQ(SNLNet::Type::Assign0, assign_ins0_input_net->getType());
  EXPECT_TRUE(assign_ins0_input_net->isConstant0());
  EXPECT_EQ(SNLNet::Type::Assign0, assign_ins0_input_net->getType());

  auto n1 = top->getScalarNet(SNLName("n1"));
  ASSERT_NE(nullptr, n1);
  EXPECT_EQ(SNLNet::Type::Standard, n1->getType());
  EXPECT_EQ(1, n1->getInstTerms().size());
  auto assign_ins1_output = *(n1->getInstTerms().begin());
  EXPECT_EQ(SNLDB0::getAssignOutput(), assign_ins1_output->getBitTerm());
  auto assign_ins1 = assign_ins1_output->getInstance();
  EXPECT_EQ(SNLDB0::getAssign(), assign_ins1->getModel());
  auto assign_ins1_input = assign_ins1->getInstTerm(SNLDB0::getAssignInput());
  auto assign_ins1_input_net = assign_ins1_input->getNet();
  EXPECT_NE(assign_ins1_input_net, nullptr);
  EXPECT_TRUE(assign_ins1_input_net->isConstant1());
  EXPECT_EQ(SNLNet::Type::Assign1, assign_ins1_input_net->getType());

  auto b0 = top->getBusNet(SNLName("b0"));
  ASSERT_NE(nullptr, b0);
  EXPECT_EQ(4, b0->getBits().size());
  EXPECT_EQ(SNLNet::Type::Assign0, b0->getBit(0)->getType());
  EXPECT_EQ(SNLNet::Type::Assign1, b0->getBit(1)->getType());
  EXPECT_EQ(SNLNet::Type::Supply0, b0->getBit(2)->getType());
  EXPECT_EQ(SNLNet::Type::Supply1, b0->getBit(3)->getType());
}