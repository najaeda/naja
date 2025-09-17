// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"

#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"
using namespace naja::NL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapNpTest2: public ::testing::Test {
  //Test assign to constants
  protected:
    void SetUp() override {
      //
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* designs = NLLibrary::create(db_, NLName("designs"));
      SNLDesign* top = SNLDesign::create(designs, NLName("top"));
      universe->setTopDesign(top);
      auto assign0 = SNLScalarNet::create(top);
      assign0->setType(SNLNet::Type::Assign0);
      auto assign1 = SNLScalarNet::create(top);
      assign1->setType(SNLNet::Type::Assign1);
      auto n0 = SNLScalarNet::create(top, NLName("n0"));
      auto assign_ins0 = SNLInstance::create(top, NLDB0::getAssign());
      assign_ins0->getInstTerm(NLDB0::getAssignInput())->setNet(assign0);
      assign_ins0->getInstTerm(NLDB0::getAssignOutput())->setNet(n0);
      auto n1 = SNLScalarNet::create(top, NLName("n1"));
      auto assign_ins1 = SNLInstance::create(top, NLDB0::getAssign());
      assign_ins1->getInstTerm(NLDB0::getAssignInput())->setNet(assign1);
      assign_ins1->getInstTerm(NLDB0::getAssignOutput())->setNet(n1);
      auto b0 = SNLBusNet::create(top, 3, 0, NLName("b0"));
      b0->getBit(0)->setType(SNLNet::Type::Assign0);
      b0->getBit(1)->setType(SNLNet::Type::Assign1);
      b0->getBit(2)->setType(SNLNet::Type::Supply0);
      b0->getBit(3)->setType(SNLNet::Type::Supply1);

      auto b1 = SNLBusNet::create(top, 3, 0, NLName("b1"));
      b1->getBit(3)->destroy();
      b1->getBit(2)->destroy();
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
  protected:
    NLDB*      db_;
};

TEST_F(SNLCapNpTest2, test0) {
  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapNpTest2_test0.snl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(NLID::DBID(1), db_->getID());
  auto top = db_->getTopDesign();
  EXPECT_NE(nullptr, top);
  //assign0, assign1, n0, n1, b0, b1
  ASSERT_EQ(6, top->getNets().size());
  auto n0 = top->getScalarNet(NLName("n0"));
  ASSERT_NE(nullptr, n0);
  EXPECT_EQ(SNLNet::Type::Standard, n0->getType());
  EXPECT_EQ(1, n0->getInstTerms().size());
  auto assign_ins0_output = *(n0->getInstTerms().begin());
  EXPECT_EQ(NLDB0::getAssignOutput(), assign_ins0_output->getBitTerm());
  auto assign_ins0 = assign_ins0_output->getInstance();
  EXPECT_EQ(NLDB0::getAssign(), assign_ins0->getModel());
  auto assign_ins0_input = assign_ins0->getInstTerm(NLDB0::getAssignInput());
  auto assign_ins0_input_net = assign_ins0_input->getNet();
  EXPECT_NE(assign_ins0_input_net, nullptr);
  EXPECT_EQ(SNLNet::Type::Assign0, assign_ins0_input_net->getType());
  EXPECT_TRUE(assign_ins0_input_net->isConstant0());
  EXPECT_EQ(SNLNet::Type::Assign0, assign_ins0_input_net->getType());

  auto n1 = top->getScalarNet(NLName("n1"));
  ASSERT_NE(nullptr, n1);
  EXPECT_EQ(SNLNet::Type::Standard, n1->getType());
  EXPECT_EQ(1, n1->getInstTerms().size());
  auto assign_ins1_output = *(n1->getInstTerms().begin());
  EXPECT_EQ(NLDB0::getAssignOutput(), assign_ins1_output->getBitTerm());
  auto assign_ins1 = assign_ins1_output->getInstance();
  EXPECT_EQ(NLDB0::getAssign(), assign_ins1->getModel());
  auto assign_ins1_input = assign_ins1->getInstTerm(NLDB0::getAssignInput());
  auto assign_ins1_input_net = assign_ins1_input->getNet();
  EXPECT_NE(assign_ins1_input_net, nullptr);
  EXPECT_TRUE(assign_ins1_input_net->isConstant1());
  EXPECT_EQ(SNLNet::Type::Assign1, assign_ins1_input_net->getType());

  auto b0 = top->getBusNet(NLName("b0"));
  ASSERT_NE(nullptr, b0);
  EXPECT_EQ(4, b0->getBits().size());
  EXPECT_EQ(SNLNet::Type::Assign0, b0->getBit(0)->getType());
  EXPECT_EQ(SNLNet::Type::Assign1, b0->getBit(1)->getType());
  EXPECT_EQ(SNLNet::Type::Supply0, b0->getBit(2)->getType());
  EXPECT_EQ(SNLNet::Type::Supply1, b0->getBit(3)->getType());

  auto b1 = top->getBusNet(NLName("b1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(2, b1->getBits().size());
}
