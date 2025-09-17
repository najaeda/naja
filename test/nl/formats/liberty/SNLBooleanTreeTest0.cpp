// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;
using ::testing::Key;

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLBooleanTree.h"
#include "SNLLibertyConstructorException.h"

using namespace naja::NL;

class SNLBooleanTreeTest0: public ::testing::Test {
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
    NLLibrary*  library_;
};

TEST_F(SNLBooleanTreeTest0, test00) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  size_t pos = 0;
  auto tree = std::make_unique<SNLBooleanTree>();
  auto a = tree->parseInput(and2, "A & B", pos);
  EXPECT_NE(nullptr, a);
  EXPECT_EQ(a->getTerm(), and2->getScalarTerm(NLName("A")));
  EXPECT_EQ(pos, 1);
  pos = 4;
  auto b = tree->parseInput(and2, "A & B", pos);
  EXPECT_NE(nullptr, b);
  EXPECT_EQ(b->getTerm(), and2->getScalarTerm(NLName("B")));
  EXPECT_EQ(pos, 5);
}

TEST_F(SNLBooleanTreeTest0, test01) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND2"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A")));
  inputs.push_back(SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("B")));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(and2, "A & B");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::AND, root->getType());
  ASSERT_EQ(2, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(and2->getScalarTerm(NLName("A"))),
    Key(and2->getScalarTerm(NLName("B")))
  ));

  EXPECT_FALSE(root->getValue());
  tree->getInput(and2->getScalarTerm(NLName("A")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  tree->getInput(and2->getScalarTerm(NLName("B")))->setValue(true);
  EXPECT_TRUE(root->getValue());

  auto truthTable = tree->getTruthTable(inputs);
  EXPECT_EQ(2, truthTable.size());
  uint64_t result = 0b1000;
  EXPECT_TRUE(NLBitVecDynamic(result, 4) == truthTable.bits());
}

TEST_F(SNLBooleanTreeTest0, test10) {
  auto and3 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AND3"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(and3, SNLTerm::Direction::Input, NLName("A")));
  inputs.push_back(SNLScalarTerm::create(and3, SNLTerm::Direction::Input, NLName("B")));
  inputs.push_back(SNLScalarTerm::create(and3, SNLTerm::Direction::Input, NLName("C")));
  SNLScalarTerm::create(and3, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(and3, "A & B & C");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::AND, root->getType());
  ASSERT_EQ(3, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(and3->getScalarTerm(NLName("A"))),
    Key(and3->getScalarTerm(NLName("B"))),
    Key(and3->getScalarTerm(NLName("C")))
  ));
  EXPECT_FALSE(root->getValue());
  tree->getInput(and3->getScalarTerm(NLName("A")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  tree->getInput(and3->getScalarTerm(NLName("B")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  tree->getInput(and3->getScalarTerm(NLName("C")))->setValue(true);
  EXPECT_TRUE(root->getValue());

  auto truthTable = tree->getTruthTable(inputs);
  EXPECT_EQ(3, truthTable.size());
  uint64_t result = 0b10000000;
  EXPECT_TRUE(NLBitVecDynamic(result, 8) == truthTable.bits());
}

TEST_F(SNLBooleanTreeTest0, test20) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("ANDN2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A1"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  size_t pos = 0;
  auto a1 = tree->parseInput(and2, "A1 & !A2", pos);
  EXPECT_NE(nullptr, a1);
  EXPECT_EQ(a1->getTerm(), and2->getScalarTerm(NLName("A1")));
  EXPECT_EQ(pos, 2);
  pos = 6;
  auto a2 = tree->parseInput(and2, "A1 & !A2", pos);
  EXPECT_NE(nullptr, a2);
  EXPECT_EQ(a2->getTerm(), and2->getScalarTerm(NLName("A2")));
  EXPECT_EQ(pos, 8);
}

TEST_F(SNLBooleanTreeTest0, test21) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("ANDN2"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A1")));
  inputs.push_back(SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("A2")));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(and2, "A1 & !A2");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::AND, root->getType());
  ASSERT_EQ(2, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(and2->getScalarTerm(NLName("A1"))),
    Key(and2->getScalarTerm(NLName("A2")))
  ));
  EXPECT_FALSE(root->getValue());
  tree->getInput(and2->getScalarTerm(NLName("A1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(and2->getScalarTerm(NLName("A2")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  auto tt = tree->getTruthTable(inputs);
  EXPECT_EQ(2, tt.size());
  uint64_t result = 0b0100;
  EXPECT_TRUE(NLBitVecDynamic(result, 4) == tt.bits());
}

TEST_F(SNLBooleanTreeTest0, test30) {
  auto or2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("ANDN2"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(or2, SNLTerm::Direction::Input, NLName("A1")));
  inputs.push_back(SNLScalarTerm::create(or2, SNLTerm::Direction::Input, NLName("A2")));
  SNLScalarTerm::create(or2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(or2, "A1 | A2");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::OR, root->getType());
  ASSERT_EQ(2, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(or2->getScalarTerm(NLName("A1"))),
    Key(or2->getScalarTerm(NLName("A2")))
  ));
  EXPECT_FALSE(root->getValue());
  tree->getInput(or2->getScalarTerm(NLName("A1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(or2->getScalarTerm(NLName("A2")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  auto tt = tree->getTruthTable(inputs);
  EXPECT_EQ(2, tt.size());
  uint64_t result = 0b1110;
  EXPECT_TRUE(NLBitVecDynamic(result, 4) == tt.bits());
}

TEST_F(SNLBooleanTreeTest0, test40) {
  auto xor2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("XOR2"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(xor2, SNLTerm::Direction::Input, NLName("A1")));
  inputs.push_back(SNLScalarTerm::create(xor2, SNLTerm::Direction::Input, NLName("A2")));
  SNLScalarTerm::create(xor2, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(xor2, "A1 ^ A2");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::XOR, root->getType());
  ASSERT_EQ(2, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(xor2->getScalarTerm(NLName("A1"))),
    Key(xor2->getScalarTerm(NLName("A2")))
  ));
  EXPECT_FALSE(root->getValue());
  tree->getInput(xor2->getScalarTerm(NLName("A1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(xor2->getScalarTerm(NLName("A2")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  tree->getInput(xor2->getScalarTerm(NLName("A1")))->setValue(false);
  EXPECT_TRUE(root->getValue());
  auto tt = tree->getTruthTable(inputs);
  EXPECT_EQ(2, tt.size());
  uint64_t result = 0b0110;
  EXPECT_TRUE(NLBitVecDynamic(result, 4) == tt.bits());
}

TEST_F(SNLBooleanTreeTest0, test50) {
  auto aor3 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("AO12"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(aor3, SNLTerm::Direction::Input, NLName("A1")));
  inputs.push_back(SNLScalarTerm::create(aor3, SNLTerm::Direction::Input, NLName("A2")));
  inputs.push_back(SNLScalarTerm::create(aor3, SNLTerm::Direction::Input, NLName("A3")));
  SNLScalarTerm::create(aor3, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(aor3, "A1 & (A2 | A3)");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::AND, root->getType());
  ASSERT_EQ(3, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(aor3->getScalarTerm(NLName("A1"))),
    Key(aor3->getScalarTerm(NLName("A2"))),
    Key(aor3->getScalarTerm(NLName("A3")))
  ));
  EXPECT_FALSE(root->getValue());
  tree->getInput(aor3->getScalarTerm(NLName("A1")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  tree->getInput(aor3->getScalarTerm(NLName("A2")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(aor3->getScalarTerm(NLName("A1")))->setValue(false);
  EXPECT_FALSE(root->getValue());
  auto tt = tree->getTruthTable(inputs);
  EXPECT_EQ(3, tt.size());
  uint64_t result = 0xE0;
  EXPECT_TRUE(NLBitVecDynamic(result, 8) == tt.bits());
}

TEST_F(SNLBooleanTreeTest0, test60) {
  auto oai222 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("OAI222"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("A1")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("A2")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("B1")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("B2")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("C1")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("C2")));
  SNLScalarTerm::create(oai222, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(oai222, "!(((A1 | A2) & (B1 | B2)) & (C1 | C2))");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::NOT, root->getType());
  ASSERT_EQ(6, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(oai222->getScalarTerm(NLName("A1"))),
    Key(oai222->getScalarTerm(NLName("A2"))),
    Key(oai222->getScalarTerm(NLName("B1"))),
    Key(oai222->getScalarTerm(NLName("B2"))),
    Key(oai222->getScalarTerm(NLName("C1"))),
    Key(oai222->getScalarTerm(NLName("C2")))
  ));
  //all 0
  EXPECT_TRUE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("A1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("B1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("C1")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  auto tt = tree->getTruthTable(inputs);
  EXPECT_EQ(6, tt.size());
  uint64_t result = 0x111F111F111FFFFF;
  EXPECT_TRUE(NLBitVecDynamic(result, 64) == tt.bits());
}

TEST_F(SNLBooleanTreeTest0, testFunctionError) {
  auto test = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("TEST"));
  auto tree = std::make_unique<SNLBooleanTree>();
  size_t pos = 0;
  EXPECT_THROW(tree->parseInput(test, "&", pos), SNLLibertyConstructorException);
  EXPECT_THROW(tree->parseInput(test, "A", pos), SNLLibertyConstructorException);
}

TEST_F(SNLBooleanTreeTest0, testEmptyTreeError) {
  auto test = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("TEST"));
  auto tree = std::make_unique<SNLBooleanTree>();
  EXPECT_THROW(tree->getTruthTable(SNLBooleanTree::Terms()), SNLLibertyConstructorException);
}

TEST_F(SNLBooleanTreeTest0, test7Inputs) {
  auto oai222 = SNLDesign::create(library_, SNLDesign::Type::Primitive, NLName("OAI222"));
  SNLBooleanTree::Terms inputs;
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("A1")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("A2")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("B1")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("B2")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("C1")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("C2")));
  inputs.push_back(SNLScalarTerm::create(oai222, SNLTerm::Direction::Input, NLName("D1")));
  SNLScalarTerm::create(oai222, SNLTerm::Direction::Output, NLName("Y"));
  auto tree = std::make_unique<SNLBooleanTree>();
  tree->parse(oai222, "(!(((A1 | A2) & (B1 | B2)) & (C1 | C2))) | D1");
  ASSERT_NE(nullptr, tree);
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  //EXPECT_EQ(SNLBooleanTreeFunctionNode::Type::NOT, root->getType());
  ASSERT_EQ(7, tree->getInputs().size());
  EXPECT_THAT(tree->getInputs(), ElementsAre(
    Key(oai222->getScalarTerm(NLName("A1"))),
    Key(oai222->getScalarTerm(NLName("A2"))),
    Key(oai222->getScalarTerm(NLName("B1"))),
    Key(oai222->getScalarTerm(NLName("B2"))),
    Key(oai222->getScalarTerm(NLName("C1"))),
    Key(oai222->getScalarTerm(NLName("C2"))),
    Key(oai222->getScalarTerm(NLName("D1")))
  ));
  //all 0
  EXPECT_TRUE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("A1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("B1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("C1")))->setValue(true);
  EXPECT_FALSE(root->getValue());
  tree->getInput(oai222->getScalarTerm(NLName("D1")))->setValue(true);
  EXPECT_TRUE(root->getValue());
  auto tt = tree->getTruthTable(inputs);
  EXPECT_EQ(7, tt.size());
  // uint64_t result = 0x111F111F111FFFFF;
  // EXPECT_TRUE(NLBitVecDynamic(result, 128) == tt.bits());
}