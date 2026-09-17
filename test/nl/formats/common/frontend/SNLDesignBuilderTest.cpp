// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <utility>
#include <vector>

#include "SNLDesignBuilder.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

using namespace naja::NL;

class SNLDesignBuilderTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto* universe = NLUniverse::create();
    auto* db = NLDB::create(universe);
    auto* library = NLLibrary::create(db, NLName("WORK"));
    design_ = SNLDesign::create(library, NLName("top"));
  }

  void TearDown() override {
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
    design_ = nullptr;
  }

  SNLDesignBuilder::Bits createBits(size_t width) {
    SNLDesignBuilder::Bits bits;
    bits.reserve(width);
    for (size_t bit = 0; bit < width; ++bit) {
      bits.push_back(SNLScalarNet::create(design_));
    }
    return bits;
  }

  SNLDesign* design_{nullptr};
};

TEST_F(SNLDesignBuilderTest, AddCreatesRippleCarryChain) {
  auto left = createBits(4);
  auto right = createBits(4);
  size_t createdObjects = 0;
  SNLDesignBuilder::Hooks hooks;
  hooks.objectCreated = [&createdObjects](NLObject*) { ++createdObjects; };
  SNLDesignBuilder builder(design_, std::move(hooks));

  SNLDesignBuilder::Bits result;
  ASSERT_TRUE(builder.add(left, right, result));
  ASSERT_EQ(4, result.size());
  ASSERT_EQ(4, design_->getInstances().size());
  EXPECT_EQ(13, createdObjects);

  auto instances = std::vector<SNLInstance*>(design_->getInstances().begin(),
                                             design_->getInstances().end());
  for (size_t bit = 0; bit < instances.size(); ++bit) {
    auto* instance = instances[bit];
    ASSERT_EQ(NLDB0::getFA(), instance->getModel());
    EXPECT_EQ(left[bit], instance->getInstTerm(NLDB0::getFAInputA())->getNet());
    EXPECT_EQ(right[bit],
              instance->getInstTerm(NLDB0::getFAInputB())->getNet());
    EXPECT_EQ(result[bit],
              instance->getInstTerm(NLDB0::getFAOutputS())->getNet());
    if (bit != 0) {
      EXPECT_EQ(
          instances[bit - 1]->getInstTerm(NLDB0::getFAOutputCO())->getNet(),
          instance->getInstTerm(NLDB0::getFAInputCI())->getNet());
    }
  }
  EXPECT_EQ(SNLNet::Type::Assign0, instances.front()
                                       ->getInstTerm(NLDB0::getFAInputCI())
                                       ->getNet()
                                       ->getType());
}

TEST_F(SNLDesignBuilderTest, SubtractCreatesTwosComplementChain) {
  auto left = createBits(4);
  auto right = createBits(4);
  SNLDesignBuilder builder(design_);

  SNLDesignBuilder::Bits result;
  ASSERT_TRUE(builder.subtract(left, right, result));
  ASSERT_EQ(4, result.size());
  ASSERT_EQ(8, design_->getInstances().size());

  size_t notCount = 0;
  size_t fullAdderCount = 0;
  SNLInstance* firstFullAdder = nullptr;
  for (auto* instance : design_->getInstances()) {
    if (instance->getModel() == NLDB0::getFA()) {
      ++fullAdderCount;
      if (!firstFullAdder) {
        firstFullAdder = instance;
      }
    } else if (NLDB0::getGateName(instance->getModel()) == "not") {
      ++notCount;
    }
  }
  EXPECT_EQ(4, notCount);
  EXPECT_EQ(4, fullAdderCount);
  ASSERT_NE(nullptr, firstFullAdder);
  EXPECT_EQ(
      SNLNet::Type::Assign1,
      firstFullAdder->getInstTerm(NLDB0::getFAInputCI())->getNet()->getType());
}

TEST_F(SNLDesignBuilderTest, MuxConnectsWidthExplicitVectors) {
  auto input0 = createBits(4);
  auto input1 = createBits(4);
  auto* select = SNLScalarNet::create(design_);
  SNLDesignBuilder builder(design_);

  SNLDesignBuilder::Bits result;
  ASSERT_TRUE(builder.mux(select, input0, input1, result));
  ASSERT_EQ(4, result.size());
  ASSERT_EQ(1, design_->getInstances().size());
  auto* instance = *design_->getInstances().begin();
  EXPECT_TRUE(NLDB0::isMux2(instance->getModel()));
  EXPECT_EQ(select,
            instance->getInstTerm(NLDB0::getMux2Select(instance->getModel()))
                ->getNet());
  for (size_t bit = 0; bit < result.size(); ++bit) {
    auto getLSBFirstTerm = [bit](SNLBusTerm* term) {
      const auto step = term->getMSB() >= term->getLSB() ? 1 : -1;
      return term->getBit(term->getLSB() + step * static_cast<NLID::Bit>(bit));
    };
    auto* input0Term =
        getLSBFirstTerm(NLDB0::getMux2InputA(instance->getModel()));
    auto* input1Term =
        getLSBFirstTerm(NLDB0::getMux2InputB(instance->getModel()));
    auto* outputTerm =
        getLSBFirstTerm(NLDB0::getMux2Output(instance->getModel()));
    EXPECT_EQ(input0[bit], instance->getInstTerm(input0Term)->getNet());
    EXPECT_EQ(input1[bit], instance->getInstTerm(input1Term)->getNet());
    EXPECT_EQ(result[bit], instance->getInstTerm(outputTerm)->getNet());
  }
}

TEST_F(SNLDesignBuilderTest, RejectsInvalidWidthsWithoutCreatingInstances) {
  SNLDesignBuilder builder(design_);
  SNLDesignBuilder::Bits result;
  EXPECT_FALSE(builder.add(createBits(2), createBits(3), result));
  EXPECT_FALSE(builder.mux(SNLScalarNet::create(design_), createBits(1),
                           createBits(2), result));
  EXPECT_TRUE(design_->getInstances().empty());
}
