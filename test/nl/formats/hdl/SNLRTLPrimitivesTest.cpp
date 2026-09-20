// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLRTLPrimitives.h"

using namespace naja::NL;

class SNLRTLPrimitivesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto* db = NLDB::create(NLUniverse::create());
      library_ = NLLibrary::create(db, NLLibrary::Type::Standard);
      design_ = SNLDesign::create(library_, NLName("top"));
    }
    void TearDown() override { NLUniverse::get()->destroy(); }
    NLLibrary* library_ {};
    SNLDesign* design_ {};
};

TEST_F(SNLRTLPrimitivesTest, MuxPreservesReorderedInputsAndAscendingOutput) {
  auto* select = SNLScalarNet::create(design_);
  auto* a = SNLBusNet::create(design_, 7, 4);
  auto* b = SNLBusNet::create(design_, 2, 5);
  auto* out = SNLBusNet::create(design_, 10, 13);
  SNLRTLPrimitives::Bits aBits {a->getBit(6), a->getBit(4), a->getBit(7), a->getBit(5)};
  SNLRTLPrimitives::Bits bBits {b->getBit(5), b->getBit(4), b->getBit(3), b->getBit(2)};
  auto* instance = SNLRTLPrimitives::createMux(design_, select, aBits, bBits, out);
  auto* model = NLDB0::getOrCreateMux2(4);
  ASSERT_EQ(model, instance->getModel());
  EXPECT_EQ(select, instance->getInstTerm(NLDB0::getMux2Select(model))->getNet());
  for (NLID::Bit bit = 0; bit < 4; ++bit) {
    EXPECT_EQ(aBits[bit], instance->getInstTerm(NLDB0::getMux2InputA(model)->getBit(bit))->getNet());
    EXPECT_EQ(bBits[bit], instance->getInstTerm(NLDB0::getMux2InputB(model)->getBit(bit))->getNet());
    EXPECT_EQ(out->getBit(13 - bit),
      instance->getInstTerm(NLDB0::getMux2Output(model)->getBit(bit))->getNet());
  }
}

TEST_F(SNLRTLPrimitivesTest, MuxAcceptsScalarOutputAndConstantInput) {
  auto* select = SNLScalarNet::create(design_);
  auto* a = SNLScalarNet::create(design_);
  a->setType(SNLNet::Type::Assign0);
  auto* b = SNLScalarNet::create(design_);
  auto* out = SNLScalarNet::create(design_);
  auto* instance = SNLRTLPrimitives::createMux(design_, select, {a}, {b}, out);
  EXPECT_EQ(a, instance->getInstTerm(NLDB0::getMux2InputA()->getBit(0))->getNet());
  EXPECT_EQ(out, instance->getInstTerm(NLDB0::getMux2Output()->getBit(0))->getNet());
}

TEST_F(SNLRTLPrimitivesTest, DFFAcceptsOneBitBusesWithoutFrontendState) {
  auto* clock = SNLScalarNet::create(design_);
  auto* data = SNLBusNet::create(design_, 7, 7);
  auto* out = SNLBusNet::create(design_, -2, -2);
  auto* instance = SNLRTLPrimitives::createDFF(design_, clock, data, out);
  EXPECT_EQ(NLDB0::getDFF(), instance->getModel());
  EXPECT_EQ(clock, instance->getInstTerm(NLDB0::getDFFClock())->getNet());
  EXPECT_EQ(data->getBit(7), instance->getInstTerm(NLDB0::getDFFData())->getNet());
  EXPECT_EQ(out->getBit(-2), instance->getInstTerm(NLDB0::getDFFOutput())->getNet());
}

TEST_F(SNLRTLPrimitivesTest, RejectsInvalidInputsBeforeCreatingInstances) {
  auto* bit = SNLScalarNet::create(design_);
  auto* bus = SNLBusNet::create(design_, 1, 0);
  auto* other = SNLDesign::create(library_, NLName("other"));
  auto* foreign = SNLScalarNet::create(other);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {}, {}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {bit, bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, nullptr, {bit}, {bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {nullptr}, {bit}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {foreign}, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {bit}, bus), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createMux(design_, bit, {bit}, {bit}, foreign), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(nullptr, bit, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(design_, bit, bus, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(design_, foreign, bit, bit), NLException);
  EXPECT_THROW(SNLRTLPrimitives::createDFF(design_, bit, bit, nullptr), NLException);
  EXPECT_TRUE(design_->getInstances().empty());
}
