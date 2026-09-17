// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <string>

#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLXLSConstructor.h"
#include "SNLXLSConstructorException.h"

using namespace naja::NL;

class SNLXLSConstructorTest : public ::testing::Test {
  protected:
    void SetUp() override {
      auto* universe = NLUniverse::create();
      auto* db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("WORK"));
    }

    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
      library_ = nullptr;
    }

    NLLibrary* library_{nullptr};
};

TEST_F(SNLXLSConstructorTest, ConstructsBitsOnlyAddSubSelectFunction) {
  SNLXLSIRFunction function;
  function.name = "add_select";
  function.parameters = {{"a", 8}, {"b", 8}, {"select", 1}};
  function.nodes = {
    {6, "selected", "sel", 8, {"select", "sum", "difference"}, "fixture.ir:9"},
    {5, "difference", "sub", 8, {"a", "b"}, "fixture.ir:8"},
    {4, "sum", "add", 8, {"a", "b"}, "fixture.ir:7"},
  };
  function.result = "selected";
  function.outputName = "result";

  SNLXLSConstructor constructor(library_);
  auto* design = constructor.construct(function);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(NLName("add_select"), design->getName());

  auto* a = design->getBusTerm(NLName("a"));
  auto* b = design->getBusTerm(NLName("b"));
  auto* select = design->getScalarTerm(NLName("select"));
  auto* result = design->getBusTerm(NLName("result"));
  ASSERT_NE(nullptr, a);
  ASSERT_NE(nullptr, b);
  ASSERT_NE(nullptr, select);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, select->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, result->getDirection());
  EXPECT_EQ(8, a->getWidth());
  EXPECT_EQ(8, b->getWidth());
  EXPECT_EQ(8, result->getWidth());

  size_t fullAdders = 0;
  size_t notGates = 0;
  size_t muxes = 0;
  for (auto* instance : design->getInstances()) {
    if (instance->getModel() == NLDB0::getFA()) {
      ++fullAdders;
    } else if (NLDB0::isMux2(instance->getModel())) {
      ++muxes;
    } else if (NLDB0::getGateName(instance->getModel()) == "not") {
      ++notGates;
    }
  }
  EXPECT_EQ(16, fullAdders);
  EXPECT_EQ(8, notGates);
  EXPECT_EQ(1, muxes);
  for (auto* bit : result->getBits()) {
    EXPECT_NE(nullptr, bit->getNet());
  }
}

TEST_F(SNLXLSConstructorTest, RejectsUnsupportedOperationWithoutCreatingDesign) {
  SNLXLSIRFunction function;
  function.name = "unsupported";
  function.parameters = {{"a", 8}, {"b", 8}};
  function.nodes = {
    {3, "product", "umul", 8, {"a", "b"}, "unsupported.ir:7"},
  };
  function.result = "product";

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(function);
    FAIL() << "unsupported operation was accepted";
  } catch (const SNLXLSConstructorException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(std::string::npos, message.find("umul"));
    EXPECT_NE(std::string::npos, message.find("unsupported.ir:7"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("unsupported")));
}

TEST_F(SNLXLSConstructorTest, RejectsWidthMismatchWithoutCreatingDesign) {
  SNLXLSIRFunction function;
  function.name = "width_mismatch";
  function.parameters = {{"a", 8}, {"b", 4}};
  function.nodes = {
    {3, "sum", "add", 8, {"a", "b"}, "width.ir:7"},
  };
  function.result = "sum";

  SNLXLSConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(function), SNLXLSConstructorException);
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("width_mismatch")));
}

TEST_F(SNLXLSConstructorTest, RejectsCyclicNodeGraphWithoutCreatingDesign) {
  SNLXLSIRFunction function;
  function.name = "cycle";
  function.parameters = {{"a", 8}};
  function.nodes = {
    {2, "left", "add", 8, {"a", "right"}, "cycle.ir:7"},
    {3, "right", "sub", 8, {"left", "a"}, "cycle.ir:8"},
  };
  function.result = "left";

  SNLXLSConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(function), SNLXLSConstructorException);
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("cycle")));
}
