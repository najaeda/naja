// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLBooleanTree.h"
#include "SNLLibertyConstructorException.h"
using namespace naja::SNL;

TEST(SNLBooleanTreeTest1, testError0) {
  auto tree = std::make_unique<SNLBooleanTree>();
  auto invNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::NOT);
  invNode->addInput(new SNLBooleanTreeInputNode(SNLBooleanTreeInputNode::Type::INPUT));
  invNode->addInput(new SNLBooleanTreeInputNode(SNLBooleanTreeInputNode::Type::INPUT));
  EXPECT_THROW(invNode->getValue(), SNLLibertyConstructorException);
}

TEST(SNLBooleanTreeTest1, testError1) {
  auto tree = std::make_unique<SNLBooleanTree>();
  auto invNode = new SNLBooleanTreeFunctionNode(SNLBooleanTreeFunctionNode::Type::BUFFER);
  invNode->addInput(new SNLBooleanTreeInputNode(SNLBooleanTreeInputNode::Type::INPUT));
  invNode->addInput(new SNLBooleanTreeInputNode(SNLBooleanTreeInputNode::Type::INPUT));
  EXPECT_THROW(invNode->getValue(), SNLLibertyConstructorException);
}