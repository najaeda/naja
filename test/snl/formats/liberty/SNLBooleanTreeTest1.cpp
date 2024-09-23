// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBooleanTree.h"
#include "SNLLibertyConstructorException.h"
using namespace naja::SNL;

class SNLBooleanTreeTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLBooleanTreeTest1, testError0) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, SNLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, SNLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  auto aNode = tree->getOrCreateInputNode(and2->getScalarTerm(SNLName("A")));
  auto bNode = tree->getOrCreateInputNode(and2->getScalarTerm(SNLName("B")));
  auto invNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
  tree->setRoot(invNode);
  invNode->addInput(aNode);
  invNode->addInput(bNode);
  EXPECT_THROW(invNode->getValue(), SNLLibertyConstructorException);
}

TEST_F(SNLBooleanTreeTest1, testError1) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, SNLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, SNLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  auto aNode = tree->getOrCreateInputNode(and2->getScalarTerm(SNLName("A")));
  auto bNode = tree->getOrCreateInputNode(and2->getScalarTerm(SNLName("B")));
  auto bufNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::BUFFER);
  tree->setRoot(bufNode);
  bufNode->addInput(aNode);
  bufNode->addInput(bNode);
  EXPECT_THROW(bufNode->getValue(), SNLLibertyConstructorException);
}