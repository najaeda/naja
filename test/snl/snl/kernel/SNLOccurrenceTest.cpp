// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLPath.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLInstTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLOccurrenceTest: public ::testing::Test {
  protected:
    void SetUp() override {
      // top: i(i)
      //  |-> h0: i(i)
      //       |-> h1: i(i)
      //            |-> h2: i(i)
      //                 |-> prim: i(i)
      //                 |-> prim: i(o) (dangling)
      // simple test design with i term and net at each level
      // connected to upper and lower level.
      // At bottom level prim:o is unconnected.
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto primitivesLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
      auto designsLib = SNLLibrary::create(db);
      auto prim = SNLDesign::create(primitivesLib, SNLDesign::Type::Primitive, SNLName("PRIM"));
      auto primi = SNLScalarTerm::create(prim, SNLTerm::Direction::Input, SNLName("i"));
      auto primo = SNLScalarTerm::create(prim, SNLTerm::Direction::Input, SNLName("o"));
      auto primiNet = SNLScalarNet::create(prim, SNLName("i"));
      primi->setNet(primiNet);
      auto top = SNLDesign::create(designsLib, SNLName("TOP"));
      auto topi = SNLScalarTerm::create(top, SNLTerm::Direction::Input, SNLName("i"));
      auto topiNet = SNLScalarNet::create(top, SNLName("i"));
      topi->setNet(topiNet);
      auto h0 = SNLDesign::create(designsLib, SNLName("H0"));
      auto h0i = SNLScalarTerm::create(h0, SNLTerm::Direction::Input, SNLName("i"));
      auto h0iNet = SNLScalarNet::create(h0, SNLName("i"));
      h0i->setNet(h0iNet);
      auto h1 = SNLDesign::create(designsLib, SNLName("H1"));
      auto h1i = SNLScalarTerm::create(h1, SNLTerm::Direction::Input, SNLName("i"));
      auto h1iNet = SNLScalarNet::create(h1, SNLName("i"));
      h1i->setNet(h1iNet);
      auto h2 = SNLDesign::create(designsLib, SNLName("H2"));
      auto h2i = SNLScalarTerm::create(h2, SNLTerm::Direction::Input, SNLName("i"));
      auto h2iNet = SNLScalarNet::create(h2, SNLName("i"));
      h2i->setNet(h2iNet);
      primInstance_ = SNLInstance::create(h2, prim, SNLName("prim"));
      h2Instance_ = SNLInstance::create(h1, h2, SNLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, SNLName("h1"));
      h0Instance_ = SNLInstance::create(top, h0, SNLName("h0"));
      primInstance_->getInstTerm(prim->getScalarTerm(SNLName("i")))->setNet(h2->getScalarNet(SNLName("i")));
      h2Instance_->getInstTerm(h2->getScalarTerm(SNLName("i")))->setNet(h1->getScalarNet(SNLName("i")));
      h1Instance_->getInstTerm(h1->getScalarTerm(SNLName("i")))->setNet(h0->getScalarNet(SNLName("i")));
      h0Instance_->getInstTerm(h0->getScalarTerm(SNLName("i")))->setNet(top->getScalarNet(SNLName("i")));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }

    SNLInstance* primInstance_  {nullptr};
    SNLInstance* h2Instance_    {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h0Instance_    {nullptr};
};

TEST_F(SNLOccurrenceTest, testEmptyOccurrences0) {
  ASSERT_NE(SNLUniverse::get(), nullptr);
  auto emptyPath = SNLPath();
  auto emptyNetOccurrence = SNLBitNetOccurrence();
  auto emptyInstTermOccurrence = SNLInstTermOccurrence();
  auto emptyTermOccurrence = SNLBitTermOccurrence();
  EXPECT_FALSE(emptyNetOccurrence.isValid());
  EXPECT_FALSE(emptyInstTermOccurrence.isValid());
  EXPECT_FALSE(emptyTermOccurrence.isValid());
  EXPECT_EQ(emptyNetOccurrence, emptyInstTermOccurrence);
  EXPECT_EQ(emptyNetOccurrence, SNLInstTermOccurrence());
  EXPECT_FALSE(emptyNetOccurrence < emptyInstTermOccurrence);
  EXPECT_FALSE(emptyInstTermOccurrence < emptyNetOccurrence);
}

TEST_F(SNLOccurrenceTest, testEmptyOccurrences1) {
  ASSERT_NE(SNLUniverse::get(), nullptr);
  auto emptyPath = SNLPath();
  auto emptyNetOccurrence = SNLBitNetOccurrence(emptyPath, nullptr);
  auto emptyInstTermOccurrence = SNLInstTermOccurrence(emptyPath, nullptr);
  auto emptyTermOccurrence = SNLBitTermOccurrence(emptyPath, nullptr);
  EXPECT_FALSE(emptyNetOccurrence.isValid());
  EXPECT_EQ(emptyNetOccurrence, emptyInstTermOccurrence);
  EXPECT_EQ(emptyNetOccurrence, SNLInstTermOccurrence());
  EXPECT_FALSE(emptyNetOccurrence < emptyInstTermOccurrence);
  EXPECT_FALSE(emptyInstTermOccurrence < emptyNetOccurrence);
  EXPECT_TRUE(emptyInstTermOccurrence <= emptyNetOccurrence);
  EXPECT_TRUE(emptyInstTermOccurrence >= emptyNetOccurrence);
  EXPECT_FALSE(emptyInstTermOccurrence > emptyNetOccurrence);
  std::string emptyPathString = emptyTermOccurrence.getString();
}

TEST_F(SNLOccurrenceTest, testh0Level) {
  auto topITerm = h0Instance_->getDesign()->getScalarTerm(SNLName("i"));
  ASSERT_NE(topITerm, nullptr);
  auto topITermOccurrence = SNLBitTermOccurrence(topITerm);
  EXPECT_TRUE(topITermOccurrence.getPath().empty());
  EXPECT_EQ(topITermOccurrence.getTerm(), topITerm);
  EXPECT_EQ(topITermOccurrence, SNLBitTermOccurrence(topITerm));

  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_, SNLPath());
  auto iTerm = h0Instance_->getModel()->getScalarTerm(SNLName("i"));
  ASSERT_NE(nullptr, iTerm);
  auto h0iTermOccurrence = SNLBitTermOccurrence(h0Path, iTerm);
  EXPECT_EQ(h0Path, h0iTermOccurrence.getPath());
  EXPECT_EQ(iTerm, h0iTermOccurrence.getTerm());
  EXPECT_EQ(iTerm->getNet(), h0iTermOccurrence.getNet());
  EXPECT_EQ(SNLBitNetOccurrence(h0Path, iTerm->getNet()), h0iTermOccurrence.getNetOccurrence());
  EXPECT_TRUE(SNLBitNetOccurrence(iTerm->getNet()).isValid());
  EXPECT_LT(SNLBitTermOccurrence(), h0iTermOccurrence);
  EXPECT_LT(h0iTermOccurrence, SNLBitNetOccurrence(h0Path, iTerm->getNet()));

  auto h0IInstTerm = h0Instance_->getInstTerm(iTerm);
  ASSERT_NE(h0IInstTerm, nullptr);
  auto h0IInstTermOccurrence = SNLInstTermOccurrence(h0IInstTerm);
  std::string h0IInstTermString = h0IInstTermOccurrence.getString();
  EXPECT_EQ(h0IInstTermOccurrence, SNLInstTermOccurrence(SNLPath(), h0IInstTerm));
  EXPECT_TRUE(h0IInstTermOccurrence.getPath().empty());
  EXPECT_EQ(h0IInstTermOccurrence.getInstTerm(), h0IInstTerm);
}

TEST_F(SNLOccurrenceTest, testh1Level) {
  ASSERT_NE(h1Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_);
  auto h1Path = SNLPath(h0Path, h1Instance_);
  auto iTerm = h1Instance_->getModel()->getScalarTerm(SNLName("i"));
  ASSERT_NE(nullptr, iTerm);
  auto h1iTermOccurrence = SNLBitTermOccurrence(h1Path, iTerm);
  EXPECT_EQ(h1Path, h1iTermOccurrence.getPath());
  EXPECT_EQ(iTerm, h1iTermOccurrence.getTerm());
  EXPECT_EQ(iTerm->getNet(), h1iTermOccurrence.getNet());
  EXPECT_EQ(SNLBitNetOccurrence(h1Path, iTerm->getNet()), h1iTermOccurrence.getNetOccurrence());
  EXPECT_LT(SNLBitTermOccurrence(), h1iTermOccurrence);
  EXPECT_LT(h1iTermOccurrence, SNLBitNetOccurrence(h1Path, iTerm->getNet()));
}

TEST_F(SNLOccurrenceTest, testh2Level) {
  auto h0Path = SNLPath(h0Instance_);
  auto h1Path = SNLPath(h0Path, h1Instance_);
  auto h2Path = SNLPath(h1Path, h2Instance_);
  ASSERT_EQ(3, h2Path.size());
  auto primI = primInstance_->getInstTerm(primInstance_->getModel()->getScalarTerm(SNLName("i")));
  auto primO = primInstance_->getInstTerm(primInstance_->getModel()->getScalarTerm(SNLName("o")));
  ASSERT_NE(nullptr, primI);
  ASSERT_NE(nullptr, primO);
  auto primITermOccurrence = SNLInstTermOccurrence(h2Path, primI);
  auto primOTermOccurrence = SNLInstTermOccurrence(h2Path, primO);
  ASSERT_TRUE(primITermOccurrence.isValid());
  ASSERT_TRUE(primOTermOccurrence.isValid());
  EXPECT_FALSE(primOTermOccurrence.getNetOccurrence().isValid());
  EXPECT_EQ(nullptr, primOTermOccurrence.getNet());
}

TEST_F(SNLOccurrenceTest, testEquipotential0) {
  ASSERT_NE(h0Instance_, nullptr);
  auto top = h0Instance_->getDesign();
  auto topi = top->getScalarTerm(SNLName("i"));
  ASSERT_NE(topi, nullptr);
  SNLEquipotential equipotential(topi);
  EXPECT_EQ(equipotential.getTerms().size(), 1);
  EXPECT_EQ(*equipotential.getTerms().begin(), topi);
  SNLPath::PathStringDescriptor h2StringPath = {"h0", "h1", "h2"};
  SNLPath h2Path(top, h2StringPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  auto h2 = h2Path.getModel();
  auto primi = primInstance_->getInstTerm(primInstance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(nullptr, primi);
  auto primiOccurrence = SNLInstTermOccurrence(h2Path, primi);
  ASSERT_TRUE(primiOccurrence.isValid());
  EXPECT_EQ(equipotential.getInstTermOccurrences().size(), 1);
  EXPECT_EQ(*equipotential.getInstTermOccurrences().begin(), primiOccurrence);
}

TEST_F(SNLOccurrenceTest, testEquipotential1) {
  ASSERT_NE(h0Instance_, nullptr);
  auto top = h0Instance_->getDesign();
  auto topi = top->getScalarTerm(SNLName("i"));
  ASSERT_NE(topi, nullptr);
  SNLPath::PathStringDescriptor h1StringPath = {"h0", "h1"};
  SNLPath h1Path(top, h1StringPath);
  EXPECT_EQ(2, h1Path.size());
  auto h2i = h2Instance_->getInstTerm(h2Instance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(nullptr, h2i);
  auto h2iOccurrence = SNLInstTermOccurrence(h1Path, h2i);
  SNLEquipotential equipotential(h2iOccurrence);
  EXPECT_EQ(equipotential.getTerms().size(), 1);
  EXPECT_EQ(*equipotential.getTerms().begin(), topi);
  SNLPath::PathStringDescriptor h2StringPath = {"h0", "h1", "h2"};
  SNLPath h2Path(top, h2StringPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  auto h2 = h2Path.getModel();
  auto primi = primInstance_->getInstTerm(primInstance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(nullptr, primi);
  auto primiOccurrence = SNLInstTermOccurrence(h2Path, primi);
  ASSERT_TRUE(primiOccurrence.isValid());
  EXPECT_EQ(equipotential.getInstTermOccurrences().size(), 1);
  EXPECT_EQ(*equipotential.getInstTermOccurrences().begin(), primiOccurrence);
}

TEST_F(SNLOccurrenceTest, testErrors) {
  auto top = h0Instance_->getDesign();
  SNLPath::PathStringDescriptor h2StringPath = {"h0", "h1", "h2"};
  SNLPath h2Path(top, h2StringPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  auto h1i = h1Instance_->getInstTerm(h1Instance_->getModel()->getScalarTerm(SNLName("i")));
  ASSERT_NE(h1i, nullptr);
  EXPECT_THROW(SNLInstTermOccurrence(h2Path, h1i), SNLException);
}