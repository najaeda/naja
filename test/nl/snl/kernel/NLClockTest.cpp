// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <algorithm>

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLClock.h"
#include "NLException.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"
using namespace naja::NL;

class NLClockTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLName("MYLIB"));
      top_ = SNLDesign::create(library_, NLName("TOP"));
      clk_ = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("clk"));
      clk2_ = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("clk2"));
      model_ = SNLDesign::create(library_, NLName("MODEL"));
      modelClk_ = SNLScalarTerm::create(model_, SNLTerm::Direction::Input, NLName("clk"));
      instance_ = SNLInstance::create(top_, model_, NLName("inst"));
      other_ = SNLDesign::create(library_, NLName("OTHER"));
      otherClk_ = SNLScalarTerm::create(other_, SNLTerm::Direction::Input, NLName("clk"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    NLDB*           db_        {nullptr};
    NLLibrary*      library_   {nullptr};
    SNLDesign*      top_       {nullptr};
    SNLScalarTerm*  clk_       {nullptr};
    SNLScalarTerm*  clk2_      {nullptr};
    SNLDesign*      model_     {nullptr};
    SNLScalarTerm*  modelClk_  {nullptr};
    SNLInstance*    instance_  {nullptr};
    SNLDesign*      other_     {nullptr};
    SNLScalarTerm*  otherClk_  {nullptr};
};

TEST_F(NLClockTest, testPrimaryClock) {
  NLClock* clock = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});

  EXPECT_EQ(NLClock::Kind::Primary, clock->getKind());
  EXPECT_EQ(NLName("sys"), clock->getName());
  EXPECT_EQ(top_, clock->getDesign());
  EXPECT_DOUBLE_EQ(10.0, clock->getPeriod());
  EXPECT_DOUBLE_EQ(0.1, clock->getFrequency());
  EXPECT_EQ("ns", clock->getTimeUnit());
  //Default waveform: 50% duty cycle.
  EXPECT_DOUBLE_EQ(0.0, clock->getRiseAt());
  EXPECT_DOUBLE_EQ(5.0, clock->getFallAt());
  EXPECT_EQ(nullptr, clock->getMaster());
  EXPECT_EQ(clock, clock->getRootClock());
  EXPECT_TRUE(clock->isSynchronousWith(clock));
  EXPECT_EQ(1u, clock->getSources().size());
  EXPECT_EQ(clk_, *clock->getSources().begin());
}

TEST_F(NLClockTest, testWaveform) {
  NLClock* clock = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  clock->setWaveform(2.0, 7.0);
  EXPECT_DOUBLE_EQ(2.0, clock->getRiseAt());
  EXPECT_DOUBLE_EQ(7.0, clock->getFallAt());
  EXPECT_THROW(clock->setWaveform(5.0, 2.0), NLException);
  EXPECT_THROW(clock->setWaveform(0.0, 11.0), NLException);
  //A rejected waveform leaves the previous one in place.
  EXPECT_DOUBLE_EQ(7.0, clock->getFallAt());
}

TEST_F(NLClockTest, testGeneratedClockPeriodAndSynchrony) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* divided = NLClock::createGenerated(top_, NLName("div2"), master, clk_, 2, 1, false, {clk2_});

  EXPECT_EQ(NLClock::Kind::Generated, divided->getKind());
  EXPECT_EQ(master, divided->getMaster());
  EXPECT_EQ(clk_, divided->getMasterSource());
  EXPECT_DOUBLE_EQ(20.0, divided->getPeriod());
  EXPECT_DOUBLE_EQ(0.05, divided->getFrequency());
  EXPECT_EQ(master, divided->getRootClock());
  EXPECT_TRUE(divided->isSynchronousWith(master));
  EXPECT_TRUE(master->isSynchronousWith(divided));
}

TEST_F(NLClockTest, testGeneratedClockMultiply) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* multiplied = NLClock::createGenerated(top_, NLName("mul2"), master, clk_, 1, 2, true, {clk2_});
  EXPECT_DOUBLE_EQ(5.0, multiplied->getPeriod());
  EXPECT_TRUE(multiplied->isInverted());
  EXPECT_EQ(1u, multiplied->getDivideBy());
  EXPECT_EQ(2u, multiplied->getMultiplyBy());
}

TEST_F(NLClockTest, testUnrelatedClocksAreNotSynchronous) {
  NLClock* sys = NLClock::createPrimary(top_, NLName("sys"), 20.0, "ns", {clk_});
  NLClock* other = NLClock::createPrimary(top_, NLName("other"), 20.0, "ns", {clk2_});
  //Same period, different roots: not synchronous.
  EXPECT_FALSE(sys->isSynchronousWith(other));
  EXPECT_FALSE(sys->isSynchronousWith(nullptr));
}

TEST_F(NLClockTest, testGeneratedClockOfGeneratedClockKeepsRoot) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* first = NLClock::createGenerated(top_, NLName("g1"), master, clk_, 2, 1, false, {clk2_});
  NLClock* second = NLClock::createGenerated(top_, NLName("g2"), first, clk2_, 2, 1, false, {clk_});
  EXPECT_DOUBLE_EQ(40.0, second->getPeriod());
  EXPECT_EQ(master, second->getRootClock());
  EXPECT_TRUE(second->isSynchronousWith(master));
}

TEST_F(NLClockTest, testRejectsInvalidArguments) {
  EXPECT_THROW(NLClock::createPrimary(top_, NLName(""), 10.0, "ns", {clk_}), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 0.0, "ns", {clk_}), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "", {clk_}), NLException);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {}), NLException);
  EXPECT_THROW(NLClock::createPrimary(nullptr, NLName("sys"), 10.0, "ns", {clk_}), NLException);
}

TEST_F(NLClockTest, testRejectsInstanceTerminalSource) {
  SNLInstTerm* instTerm = instance_->getInstTerm(modelClk_);
  ASSERT_NE(nullptr, instTerm);
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {instTerm}), NLException);
}

TEST_F(NLClockTest, testRejectsTerminalOfAnotherDesign) {
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {otherClk_}), NLException);
}

TEST_F(NLClockTest, testRejectsDuplicateNameInDesign) {
  NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  EXPECT_THROW(NLClock::createPrimary(top_, NLName("sys"), 20.0, "ns", {clk2_}), NLException);
  //Same name in another design is fine.
  EXPECT_NO_THROW(NLClock::createPrimary(other_, NLName("sys"), 20.0, "ns", {otherClk_}));
}

TEST_F(NLClockTest, testRejectsInvalidGeneratedClock) {
  NLClock* master = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), nullptr, clk_, 2, 1, false, {clk2_}), NLException);
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), master, clk_, 0, 1, false, {clk2_}), NLException);
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), master, clk_, 1, 0, false, {clk2_}), NLException);
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), master, clk_, 2, 1, false, {}), NLException);

  NLClock* foreignMaster = NLClock::createPrimary(other_, NLName("foreign"), 10.0, "ns", {otherClk_});
  EXPECT_THROW(NLClock::createGenerated(top_, NLName("g"), foreignMaster, clk_, 2, 1, false, {clk2_}), NLException);
}

TEST_F(NLClockTest, testDesignOwnsClocks) {
  NLClock* sys = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  NLClock* other = NLClock::createPrimary(other_, NLName("other"), 20.0, "ns", {otherClk_});

  EXPECT_EQ(1u, top_->getClocks().size());
  EXPECT_EQ(sys, *top_->getClocks().begin());
  EXPECT_EQ(1u, other_->getClocks().size());

  auto all = db_->getClocks();
  EXPECT_EQ(2u, all.size());
  EXPECT_NE(all.end(), std::find(all.begin(), all.end(), sys));
  EXPECT_NE(all.end(), std::find(all.begin(), all.end(), other));
}

TEST_F(NLClockTest, testDestroyingClockRemovesItFromDesign) {
  NLClock* sys = NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});
  sys->destroy();
  EXPECT_TRUE(top_->getClocks().empty());
  EXPECT_TRUE(db_->getClocks().empty());
}

TEST_F(NLClockTest, testDestroyingDesignDestroysItsClocks) {
  NLClock::createPrimary(other_, NLName("other"), 20.0, "ns", {otherClk_});
  NLClock::createPrimary(top_, NLName("sys"), 10.0, "ns", {clk_});

  other_->destroy();

  auto all = db_->getClocks();
  ASSERT_EQ(1u, all.size());
  EXPECT_EQ(NLName("sys"), (*all.begin())->getName());
}
