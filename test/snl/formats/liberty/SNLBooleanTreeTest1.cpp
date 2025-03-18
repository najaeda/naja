// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLBooleanTree.h"
#include "SNLLibertyConstructorException.h"
using namespace naja::SNL;

class SNLBooleanTreeTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*      library_;
};

TEST_F(SNLBooleanTreeTest1, testError0) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  auto aNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("A")));
  auto bNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("B")));
  auto invNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
  tree->setRoot(invNode);
  invNode->addInput(aNode);
  invNode->addInput(bNode);
  EXPECT_THROW(invNode->getValue(), SNLLibertyConstructorException);
}

TEST_F(SNLBooleanTreeTest1, testError1) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  auto aNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("A")));
  auto bNode = tree->getOrCreateInputNode(and2->getScalarTerm(NLName("B")));
  auto bufNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::BUFFER);
  tree->setRoot(bufNode);
  bufNode->addInput(aNode);
  bufNode->addInput(bNode);
  EXPECT_THROW(bufNode->getValue(), SNLLibertyConstructorException);
}
