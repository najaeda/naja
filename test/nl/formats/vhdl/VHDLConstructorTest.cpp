// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLRTLPrimitives.h"
#include "SNLScalarTerm.h"
#include "SNLSVConstructor.h"
#include "VHDLConstructor.h"

#include <filesystem>

using namespace naja::NL;

class VHDLConstructorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto* db = NLDB::create(NLUniverse::create());
    library_ = NLLibrary::create(db, NLLibrary::Type::Standard);
  }
  void TearDown() override { NLUniverse::get()->destroy(); }

  NLLibrary* library_ {};
};

TEST_F(VHDLConstructorTest, LowersConditionalAssignmentThroughSharedMux) {
  constexpr auto source = R"(
entity mux is
  port (a, b, sel : in bit; y : out bit);
end entity mux;
architecture rtl of mux is
begin
  y <= a when sel = '1' else b;
end architecture rtl;
)";

  VHDLConstructor constructor(library_);
  auto* design = constructor.construct(source);
  ASSERT_NE(design, nullptr);

  auto* muxTermA = NLDB0::getMux2InputA();
  auto* muxTermB = NLDB0::getMux2InputB();
  auto* muxSelect = NLDB0::getMux2Select();
  auto* muxOutput = NLDB0::getMux2Output();
  auto instances = design->getInstances();
  ASSERT_EQ(instances.size(), 1);
  auto* instance = *instances.begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getOrCreateMux2(1));
  EXPECT_EQ(instance->getInstTerm(muxTermA->getBit(0))->getNet(),
            design->getScalarTerm(NLName("a"))->getNet());
  EXPECT_EQ(instance->getInstTerm(muxTermB->getBit(0))->getNet(),
            design->getScalarTerm(NLName("b"))->getNet());
  EXPECT_EQ(instance->getInstTerm(muxSelect)->getNet(),
            design->getScalarTerm(NLName("sel"))->getNet());
  EXPECT_EQ(instance->getInstTerm(muxOutput->getBit(0))->getNet(),
            design->getScalarTerm(NLName("y"))->getNet());
}

TEST_F(VHDLConstructorTest, UnsupportedPortShapeDoesNotCreateDesign) {
  constexpr auto source = R"(
entity mux is port (a : in bit_vector(1 downto 0)); end entity mux;
architecture rtl of mux is begin end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("mux")), nullptr);
}

TEST_F(VHDLConstructorTest, NineValuedPortIsRejectedBeforeDesignCreation) {
  constexpr auto source = R"(
entity mux_logic is port (a : in std_logic); end entity mux_logic;
architecture rtl of mux_logic is begin end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("mux_logic")), nullptr);
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameMuxModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_SV));
  auto* design = library_->getSNLDesign(NLName("mux_sv"));
  ASSERT_NE(design, nullptr);
  auto instances = design->getInstances();
  ASSERT_EQ(instances.size(), 1);
  EXPECT_EQ((*instances.begin())->getModel(), NLDB0::getOrCreateMux2(1));
}
