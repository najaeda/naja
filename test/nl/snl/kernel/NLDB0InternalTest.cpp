// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

// Public DB0 constructors cannot create an invalid sequential interface. Keep
// this invariant check private while exercising it in an isolated test target.
#include "../../../../src/nl/netlist/core/NLDB0.cpp"

using namespace naja::NL;

class NLDB0InternalTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(NLDB0InternalTest, testMalformedSequentialInterface) {
  NLUniverse::create();
  auto* db = NLDB::create(NLUniverse::get());
  auto* primitives = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto* malformed = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("MALFORMED"));
  auto* clock = SNLScalarTerm::create(
      malformed, SNLTerm::Direction::Input, NLName("C"));
  SNLDesignModeling::setTermRole(
      clock, SNLDesignModeling::SNLTermRole::Clock);

  EXPECT_THROW(
      makeDB0SequentialModel(
          malformed,
          clock,
          SNLDesignModeling::SequentialModel::Kind::FlipFlop),
      NLException);
}
