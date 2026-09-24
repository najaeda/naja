// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLRTLPrimitives.h"
#include "SNLVRLDumper.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLSVConstructor.h"
#include "VHDLConstructor.h"
#include "VHDLTestUtils.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

namespace {
using naja::NL::test::evaluateRTL;
using naja::NL::test::dffBits;

std::string lfsrSource(unsigned width) {
  return "library ieee; use ieee.std_logic_1164.all; "
      "use ieee.numeric_std.all; use ieee.std_logic_unsigned.all; "
      "entity lfsr is generic(n : positive := " + std::to_string(width) + R"();
  port(clk, rst, ena : in std_logic; output : out std_logic_vector(n-1 downto 0)); end;
architecture rtl of lfsr is
  type tap_table is array (32 downto 2) of std_logic_vector(31 downto 0);
  signal taps : tap_table;
  signal temp : std_logic_vector(n-1 downto 0);
begin
  taps(n) <= "00000000001000000000000000000011";
  process(clk, rst, temp)
    variable feedback : std_logic;
  begin
    if rising_edge(clk) then
      if rst = '1' then
        temp <= (others => '1'); feedback := '1';
      elsif ena = '1' then
        feedback := temp(0);
        for i in n-2 downto 0 loop
          if taps(n)(i) = '1' then temp(i) <= temp(i+1) XOR feedback;
          else temp(i) <= temp(i+1); end if;
        end loop;
        temp(n-1) <= feedback;
      end if;
    end if;
    output <= temp;
  end process;
end;
)";
}
}

TEST_F(VHDLConstructorTest, IndexedLFSRResetEnableAndCycles) {
  for (const unsigned width : {2u, 4u, 9u, 32u}) {
    SCOPED_TRACE(width);
    auto* design = VHDLConstructor(library_).construct(lfsrSource(width));
    auto* state = dynamic_cast<SNLBusNet*>(design->getNet(NLName("temp")));
    ASSERT_NE(state, nullptr);
    const auto flops = dffBits(design);
    ASSERT_EQ(flops.size(), width);
    const uint64_t mask = (uint64_t(1) << width) - 1;
    uint64_t expected = 0;
    for (unsigned cycle = 0; cycle < 160; ++cycle) {
      const bool reset = cycle == 0 || cycle == 53 || cycle == 104;
      const bool enable = cycle % 5 != 0;
      std::unordered_map<SNLBitNet*, bool> values;
      values[design->getScalarTerm(NLName("rst"))->getNet()] = reset;
      values[design->getScalarTerm(NLName("ena"))->getNet()] = enable;
      for (unsigned bit = 0; bit < width; ++bit) values[state->getBit(bit)] = (expected >> bit) & 1;
      std::unordered_map<SNLBitNet*, bool> next;
      std::unordered_set<SNLBitNet*> visiting;
      for (const auto& flop : flops) {
        EXPECT_EQ(flop.clock,
                  design->getScalarTerm(NLName("clk"))->getNet());
        next[flop.output] =
            evaluateRTL(flop.data, values, visiting);
      }
      if (reset) expected = mask;
      else if (enable) expected = (expected >> 1) ^
          ((expected & 1) ? ((uint64_t(1) << (width - 1)) | (0x200003u & (mask >> 1))) : 0);
      for (unsigned bit = 0; bit < width; ++bit) {
        ASSERT_EQ(next.at(state->getBit(bit)), bool((expected >> bit) & 1)) << cycle << ":" << bit;
        ASSERT_EQ(evaluateRTL(design->getBusTerm(NLName("output"))->getBit(bit)->getNet(),
                              next, visiting), bool((expected >> bit) & 1));
      }
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, IndexedAscendingArraysAndScheduledPriority) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity indexed is port(clk, d : in bit; y : out bit_vector(7 downto 5)); end;
architecture rtl of indexed is
  type table_type is array (-2 to 1) of bit_vector(4 to 6);
  signal table_value : table_type;
  signal state : bit_vector(4 to 6);
begin
  table_value(-2) <= "101";
  process(clk, state)
    variable v : bit;
  begin
    if rising_edge(clk) then
      v := d;
      for i in 4 to 6 loop
        state(i) <= table_value(-2)(i) xor v;
      end loop;
      v := not v;
      state(4) <= v;
      state(5) <= state(4);
      for i in 6 to 4 loop state(i) <= '0'; end loop;
    end if;
    y <= state;
  end process;
end;
)");
  auto* state = dynamic_cast<SNLBusNet*>(design->getNet(NLName("state")));
  ASSERT_NE(state, nullptr);
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    values[design->getScalarTerm(NLName("d"))->getNet()] = pattern & 1;
    for (unsigned i = 0; i < 3; ++i) values[state->getBit(4 + i)] = (pattern >> (i + 1)) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    for (const auto& flop : dffBits(design)) {
      auto* output = flop.output;
      const bool expected = output == state->getBit(5) ? bool((pattern >> 1) & 1) : !(pattern & 1);
      EXPECT_EQ(evaluateRTL(flop.data, values, visiting), expected);
    }
    for (unsigned i = 0; i < 3; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(7-i)->getNet(), values, visiting),
                bool((pattern >> (i+1)) & 1));
  }
}

TEST_F(VHDLConstructorTest, IndexedExtendedIdentifiersRemainDistinct) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity indexed is port(\A\, \a\ : in bit_vector(1 downto 0); y : out bit_vector(1 downto 0)); end;
architecture rtl of indexed is begin
  y(1) <= \A\(0);
  y(0) <= \a\(1);
end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  values[design->getBusTerm(NLName("\\A\\"))->getBit(0)->getNet()] = true;
  values[design->getBusTerm(NLName("\\a\\"))->getBit(1)->getNet()] = false;
  std::unordered_set<SNLBitNet*> visiting;
  EXPECT_TRUE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(1)->getNet(), values, visiting));
  EXPECT_FALSE(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(0)->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, InvalidIndexedRTLPublishesNoDesign) {
  const auto source = lfsrSource(4);
  const std::vector<std::pair<std::string, std::string>> replacements{
      {"taps(n)(i)", "taps(33)(i)"},
      {"temp(i+1)", "temp(i+9)"},
      {"temp(i+1)", "temp(ena)"},
      {"feedback := temp(0);", "feedback := feedback;"},
      {"process(clk, rst, temp)", "process(clk, rst)"},
      {"temp <= (others => '1')", "temp <= (others => 'X')"},
      {"signal taps : tap_table", "signal taps : missing_type"},
      {"taps(n) <=", "taps(n+1) <="},
      {"temp(n-1) <= feedback", "clk <= feedback"},
      {"temp(n-1) <= feedback", "feedback <= temp(0)"},
      {"if taps(n)(i) = '1'", "if taps(n)(i)"},
      {"output <= temp;", "output <= temp; output <= temp;"},
      {"use ieee.std_logic_1164.all;", ""},
      {"n : positive := 4", "n : positive := 0"},
      {"std_logic_vector(n-1 downto 0)", "std_logic_vector(n downto 0)"},
      {"feedback := temp(0);", "feedback := temp;"},
  };
  for (const auto& [before, after] : replacements) {
    SCOPED_TRACE(after);
    auto invalid = source;
    const auto position = invalid.find(before);
    ASSERT_NE(position, std::string::npos);
    invalid.replace(position, before.size(), after);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

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

TEST_F(VHDLConstructorTest, LowersTypedScalarLogicalOperators) {
  const std::vector<std::pair<std::string, NLDB0::GateType::GateTypeEnum>> operators{
      {"and", NLDB0::GateType::And}, {"nand", NLDB0::GateType::Nand},
      {"or", NLDB0::GateType::Or}, {"nor", NLDB0::GateType::Nor},
      {"xor", NLDB0::GateType::Xor}, {"xnor", NLDB0::GateType::Xnor}};
  for (const auto& [op, gateType] : operators) {
    SCOPED_TRACE(op);
    const std::string source =
        "entity logic is port(a, b : in bit; y : out bit); end; "
        "architecture rtl of logic is begin y <= a " + op + " b; end;";
    auto* design = VHDLConstructor(library_).construct(source);
    ASSERT_EQ(design->getInstances().size(), 1);
    EXPECT_EQ((*design->getInstances().begin())->getModel(),
              NLDB0::getOrCreateNInputGate(gateType, 2));
    design->destroy();
  }
  auto* design = VHDLConstructor(library_).construct(
      "entity logic is port(a : in bit; y : out bit); end; "
      "architecture rtl of logic is begin y <= not a; end;");
  ASSERT_EQ(design->getInstances().size(), 1);
  EXPECT_EQ((*design->getInstances().begin())->getModel(),
            NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1));
}

TEST_F(VHDLConstructorTest, PreservesVectorRangesAndPositionalMuxMapping) {
  std::ifstream fixture(SNL_VHDL_VECTORS);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  auto* a = design->getBusTerm(NLName("a"));
  auto* b = design->getBusTerm(NLName("b"));
  auto* y = design->getBusTerm(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(a->getMSB(), 0);
  EXPECT_EQ(a->getLSB(), 3);
  EXPECT_EQ(b->getMSB(), 7);
  EXPECT_EQ(b->getLSB(), 4);
  EXPECT_EQ(y->getMSB(), 3);
  EXPECT_EQ(y->getLSB(), 0);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* mux = *design->getInstances().begin();
  auto* model = NLDB0::getOrCreateMux2(4);
  ASSERT_EQ(mux->getModel(), model);
  for (size_t position = 0; position < 4; ++position) {
    EXPECT_EQ(mux->getInstTerm(
                  NLDB0::getMux2InputA(model)->getBitAtPosition(3 - position))->getNet(),
              a->getBitAtPosition(3 - position)->getNet());
    EXPECT_EQ(mux->getInstTerm(
                  NLDB0::getMux2InputB(model)->getBitAtPosition(3 - position))->getNet(),
              b->getBitAtPosition(3 - position)->getNet());
    EXPECT_EQ(mux->getInstTerm(
                  NLDB0::getMux2Output(model)->getBitAtPosition(3 - position))->getNet(),
              y->getBitAtPosition(3 - position)->getNet());
  }
  if (const auto* path = std::getenv("VHDL_VECTOR_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<std::string> values;
    for (std::string value; reference >> value;)
      values.push_back(value);
    EXPECT_EQ(values, (std::vector<std::string>{"0110", "1001", "0011"}));
  }
}

TEST_F(VHDLConstructorTest, AppliesDefaultGenericToPortRanges) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity generic_wire is
  generic(width : positive := 5);
  port(a : in bit_vector(width-1 downto 0);
       y : out bit_vector(width-1 downto 0));
end;
architecture rtl of generic_wire is begin y <= a; end;
)");
  ASSERT_NE(design, nullptr);
  auto* input = dynamic_cast<SNLBusTerm*>(design->getTerm(NLName("a")));
  auto* output = dynamic_cast<SNLBusTerm*>(design->getTerm(NLName("y")));
  ASSERT_NE(input, nullptr);
  ASSERT_NE(output, nullptr);
  EXPECT_EQ(input->getMSB(), 4);
  EXPECT_EQ(input->getLSB(), 0);
  EXPECT_EQ(output->getMSB(), 4);
  EXPECT_EQ(output->getLSB(), 0);
}

TEST_F(VHDLConstructorTest, ConstructsFromFile) {
  auto* design = VHDLConstructor(library_).constructFile(SNL_VHDL_VECTORS);
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(design->getName(), NLName("vector_mux"));
  EXPECT_THROW(
      VHDLConstructor(library_).constructFile("missing-vhdl-source.vhd"),
      NLException);
}

TEST_F(VHDLConstructorTest, DiagnosticsIncludeSourceLocation) {
  try {
    VHDLConstructor(library_).construct(R"(entity broken is
end entity broken;
architecture rtl of broken is
  signal value : bit;
  value <= '1';
end architecture rtl;
)");
    FAIL() << "expected malformed VHDL to be rejected";
  } catch (const NLException& exception) {
    EXPECT_EQ(exception.getReason(),
        "VHDL constructor: parse failed at line 5, column 3: "
        "expected keyword 'begin'");
  }
}

TEST_F(VHDLConstructorTest, UnusedArrayTypeDeclarationDoesNotBlockConstruction) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity lfsr is port(a : in bit; y : out bit); end;
architecture rtl of lfsr is
    type inner_taps is array (32 downto 2) of bit_vector(31 downto 0);
begin
    y <= a;
end;
)");
  ASSERT_NE(design, nullptr);
  EXPECT_NE(design->getTerm(NLName("a")), nullptr);
  EXPECT_NE(design->getTerm(NLName("y")), nullptr);
}

TEST_F(VHDLConstructorTest, LowersBitwiseVectorExpressionsByPosition) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity vector_logic is port (
  a : in bit_vector(0 to 3);
  b : in bit_vector(7 downto 4);
  y : out bit_vector(3 downto 0));
end;
architecture rtl of vector_logic is begin y <= a xor not b; end;
)");
  std::unordered_map<SNLDesign*, size_t> models;
  for (auto* instance : design->getInstances())
    ++models[instance->getModel()];
  EXPECT_EQ(models[NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1)], 4);
  EXPECT_EQ(models[NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2)], 4);
  EXPECT_EQ(design->getInstances().size(), 8);
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameVectorMuxModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_VECTOR_MUX_SV));
  auto* design = library_->getSNLDesign(NLName("vector_mux_sv"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getInstances().size(), 1);
  EXPECT_EQ((*design->getInstances().begin())->getModel(), NLDB0::getOrCreateMux2(4));
}

TEST_F(VHDLConstructorTest, UnsupportedVectorShapesPublishNoDesign) {
  for (const auto* source : {
      "entity bad_vector is port(a : in bit_vector(0 to 3); "
      "y : out bit_vector(2 downto 0)); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(a : in bit_vector; y : out bit_vector); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(a : in std_logic_vector(3 downto 0); "
      "y : out std_logic_vector(3 downto 0)); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(a : in bit_vector(3 to 0); "
      "y : out bit_vector(3 to 0)); end; "
      "architecture rtl of bad_vector is begin y <= a; end;",
      "entity bad_vector is port(clk : in bit; d : in bit_vector(3 downto 0); "
      "q : out bit_vector(3 downto 0)); end; architecture rtl of bad_vector is begin "
      "process(clk) begin if clk'event and clk = '1' then q <= d; end if; "
      "end process; end;"}) {
    SCOPED_TRACE(source);
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("bad_vector")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, NestedLogicalExpressionConnectivity) {
  std::ifstream fixture(SNL_VHDL_LOGICAL);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 3);
  auto* a = design->getScalarTerm(NLName("a"))->getNet();
  auto* b = design->getScalarTerm(NLName("b"))->getNet();
  auto* c = design->getScalarTerm(NLName("c"))->getNet();
  auto* y = design->getScalarTerm(NLName("y"))->getNet();
  SNLNet* andOutput = nullptr;
  SNLNet* notOutput = nullptr;
  SNLInstance* xorInstance = nullptr;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (model == NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2)) {
      auto* inputs = NLDB0::getGateNTerms(model);
      std::unordered_set<SNLNet*> nets{
          instance->getInstTerm(inputs->getBitAtPosition(0))->getNet(),
          instance->getInstTerm(inputs->getBitAtPosition(1))->getNet()};
      EXPECT_EQ(nets, (std::unordered_set<SNLNet*>{a, b}));
      andOutput = instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet();
    } else if (model == NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1)) {
      EXPECT_EQ(instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet(), c);
      notOutput = instance->getInstTerm(
          NLDB0::getGateNTerms(model)->getBitAtPosition(0))->getNet();
    } else if (model == NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2)) {
      xorInstance = instance;
      EXPECT_EQ(instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet(), y);
    }
  }
  ASSERT_NE(andOutput, nullptr);
  ASSERT_NE(notOutput, nullptr);
  ASSERT_NE(xorInstance, nullptr);
  auto* xorInputs = NLDB0::getGateNTerms(xorInstance->getModel());
  EXPECT_EQ((std::unordered_set<SNLNet*>{
                xorInstance->getInstTerm(xorInputs->getBitAtPosition(0))->getNet(),
                xorInstance->getInstTerm(xorInputs->getBitAtPosition(1))->getNet()}),
            (std::unordered_set<SNLNet*>{andOutput, notOutput}));

  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_LOGICAL_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  for (int stimulus = 0; stimulus < 8; ++stimulus) {
    std::unordered_map<SNLNet*, int> values{
        {a, (stimulus >> 2) & 1}, {b, (stimulus >> 1) & 1}, {c, stimulus & 1}};
    for (std::size_t pass = 0; pass < design->getInstances().size(); ++pass) {
      for (auto* instance : design->getInstances()) {
        auto* model = instance->getModel();
        const auto gate = NLDB0::getGateName(model);
        if (NLDB0::isNInputGate(model)) {
          auto* inputs = NLDB0::getGateNTerms(model);
          auto* lhs = instance->getInstTerm(inputs->getBitAtPosition(0))->getNet();
          auto* rhs = instance->getInstTerm(inputs->getBitAtPosition(1))->getNet();
          if (!values.contains(lhs) || !values.contains(rhs)) continue;
          const int value = gate == "and" ? values[lhs] & values[rhs]
                                           : values[lhs] ^ values[rhs];
          values[instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet()] = value;
        } else {
          auto* input = instance->getInstTerm(NLDB0::getGateSingleTerm(model))->getNet();
          if (!values.contains(input)) continue;
          values[instance->getInstTerm(
              NLDB0::getGateNTerms(model)->getBitAtPosition(0))->getNet()] = 1 - values[input];
        }
      }
    }
    ASSERT_TRUE(values.contains(y));
    EXPECT_EQ(values[y], ((values[a] & values[b]) ^ (1 - values[c])));
    if (reference.is_open()) {
      int expected = -1;
      ASSERT_TRUE(reference >> expected);
      EXPECT_EQ(values[y], expected);
    }
  }
  if (reference.is_open()) {
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameLogicalModels) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_LOGICAL_SV));
  auto* design = library_->getSNLDesign(NLName("logic_nested_sv"));
  ASSERT_NE(design, nullptr);
  std::unordered_map<SNLDesign*, std::size_t> models;
  for (auto* instance : design->getInstances()) ++models[instance->getModel()];
  EXPECT_EQ(models[NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 2)], 1);
  EXPECT_EQ(models[NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 2)], 1);
  EXPECT_EQ(models[NLDB0::getOrCreateNOutputGate(NLDB0::GateType::Not, 1)], 1);
}

TEST_F(VHDLConstructorTest, ScalarExpressionTypeErrorsPublishNoDesign) {
  for (const auto* expression : {
      "a = b", "a and flag", "'Z'", "'1' = '0'", "a + b"}) {
    SCOPED_TRACE(expression);
    const std::string source = std::string(
        "entity bad_logic is port(a, b : in bit; flag : in boolean; y : out bit); end; "
        "architecture rtl of bad_logic is begin y <= ") + expression + "; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("bad_logic")), nullptr);
  }
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
library ieee;
use ieee.std_logic_1164.all;
entity mux_logic is port (a : in std_logic; y : out std_logic); end entity mux_logic;
architecture rtl of mux_logic is begin y <= a; end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("mux_logic")), nullptr);
}

TEST_F(VHDLConstructorTest, NumericStdUnsignedAdditionIsLowered) {
  constexpr auto source = R"(
library ieee;
use ieee.numeric_std.all;
entity add_unsigned is port (
  a, b : in unsigned(3 downto 0); y : out unsigned(3 downto 0));
end entity add_unsigned;
library ieee;
use ieee.numeric_std.all;
architecture rtl of add_unsigned is begin y <= a + b; end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  ASSERT_NE(constructor.construct(source), nullptr);
  EXPECT_NE(library_->getSNLDesign(NLName("add_unsigned")), nullptr);
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameMuxModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_MUX_SV));
  auto* design = library_->getSNLDesign(NLName("mux_sv"));
  ASSERT_NE(design, nullptr);
  auto instances = design->getInstances();
  ASSERT_EQ(instances.size(), 1);
  EXPECT_EQ((*instances.begin())->getModel(), NLDB0::getOrCreateMux2(1));
}

TEST_F(VHDLConstructorTest, ClockedRegisterWiring) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity reg is port (clk, d : in bit; q : out bit); end;
architecture rtl of reg is begin
  process(CLK) is begin
    if clk'event and Clk = '1' then Q <= D; end if;
  end process;
end;
)");
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFF());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
}

TEST_F(VHDLConstructorTest, ParenthesizedClockGuardPipelineWiring) {
  for (const auto* type : {"bit", "std_logic"}) {
    for (const auto* guard : {"(CLK'EVENT AND clk = '1')",
                             "((clk'event) and (clk = '1'))", "(rising_edge(clk))"}) {
      SCOPED_TRACE(type);
      SCOPED_TRACE(guard);
      auto* design = VHDLConstructor(library_).construct(std::string(
          "library ieee; use ieee.std_logic_1164.all; "
          "entity pipeline is ") + (std::string(type) == "std_logic" ? "generic(n : integer range 1 to 1 := 1); " : "") +
          "port(clk, d : in " + type + "; q : out " + type +
          "); end; architecture rtl of pipeline is signal a, b : " + type +
          "; begin process(clk) begin if " + guard +
          " then a <= d; b <= a; q <= b; end if; end process; end;");
      ASSERT_EQ(design->getInstances().size(), 3u);
      const std::unordered_map<SNLBitNet*, SNLBitNet*> expected{
          {design->getScalarNet(NLName("a")), design->getScalarTerm(NLName("d"))->getNet()},
          {design->getScalarNet(NLName("b")), design->getScalarNet(NLName("a"))},
          {design->getScalarTerm(NLName("q"))->getNet(), design->getScalarNet(NLName("b"))}};
      for (auto* flop : design->getInstances()) {
        ASSERT_TRUE(NLDB0::isDFF(flop->getModel()));
        EXPECT_EQ(flop->getInstTerm(NLDB0::getDFFClock())->getNet(),
                  design->getScalarTerm(NLName("clk"))->getNet());
        EXPECT_EQ(flop->getInstTerm(NLDB0::getDFFData())->getNet(),
                  expected.at(flop->getInstTerm(NLDB0::getDFFOutput())->getNet()));
      }
      design->destroy();
    }
  }
}

TEST_F(VHDLConstructorTest, StructuredEventGuardPreservesVectorCycles) {
  for (const auto* guard : {"clk'event and clk = '1'", "((CLK'EVENT) and (clk = '1'))"}) {
    SCOPED_TRACE(guard);
    auto source = lfsrSource(4);
    source.replace(source.find("rising_edge(clk)"), std::string("rising_edge(clk)").size(), guard);
    auto* design = VHDLConstructor(library_).construct(source);
    auto* state = design->getBusNet(NLName("temp"));
    ASSERT_NE(state, nullptr);
    unsigned expected = 0;
    for (unsigned cycle = 0; cycle < 40; ++cycle) {
      const bool reset = cycle == 0 || cycle == 21;
      const bool enable = cycle % 3 != 0;
      std::unordered_map<SNLBitNet*, bool> values, next;
      values[design->getScalarTerm(NLName("rst"))->getNet()] = reset;
      values[design->getScalarTerm(NLName("ena"))->getNet()] = enable;
      for (unsigned bit = 0; bit < 4; ++bit) values[state->getBit(bit)] = (expected >> bit) & 1;
      std::unordered_set<SNLBitNet*> visiting;
      for (const auto& flop : dffBits(design)) {
        EXPECT_EQ(flop.clock,
                  design->getScalarTerm(NLName("clk"))->getNet());
        next[flop.output] =
            evaluateRTL(flop.data, values, visiting);
      }
      if (reset) expected = 15;
      else if (enable) expected = (expected >> 1) ^ ((expected & 1) ? 11 : 0);
      for (unsigned bit = 0; bit < 4; ++bit)
        EXPECT_EQ(next.at(state->getBit(bit)), bool((expected >> bit) & 1));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RejectsInvalidParenthesizedClockSemantics) {
  for (const auto* guard : {"(clk'event and d = '1')", "(clk'event and clk = '0')"}) {
    for (const bool structured : {false, true}) {
      SCOPED_TRACE(guard);
      SCOPED_TRACE(structured);
      const auto source = std::string(
          "entity invalid is port(clk, d : in bit; q : out bit); end; "
          "architecture rtl of invalid is begin process(clk) begin if ") + guard +
          " then " + (structured ? "for i in 0 to 1 loop q <= d; end loop;" : "q <= d;") +
          " end if; end process; end;";
      EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
      EXPECT_TRUE(library_->getSNLDesigns().empty());
    }
  }
}

TEST_F(VHDLConstructorTest, RisingEdgeRegisterWiring) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity rising_reg is port (clk, d : in bit; q : out bit); end;
architecture rtl of rising_reg is begin
  process(clk) begin
    if rising_edge(clk) then q <= d; end if;
  end process;
end;
)");
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFF());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
}

TEST_F(VHDLConstructorTest, ClockEnableWiringAndCycles) {
  std::ifstream fixture(SNL_VHDL_ENABLED);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFFE());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEEnable())->getNet(),
            design->getScalarTerm(NLName("en"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFEOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
  if (const auto* path = std::getenv("VHDL_ENABLE_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<int> values;
    for (int value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<int>{1, 1, 0}));
  }
}

TEST_F(VHDLConstructorTest, SynchronousResetWiringAndCycles) {
  std::ifstream fixture(SNL_VHDL_RESET);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFFSR());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSRClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSRData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSRReset())->getNet(),
            design->getScalarTerm(NLName("rst"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFSROutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
  if (const auto* path = std::getenv("VHDL_RESET_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<int> values;
    for (int value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<int>{1, 1, 0, 1}));
  }

  auto* pair = VHDLConstructor(library_).construct(R"(
entity reset_pair is
  port(clk, rst, d0, d1 : in bit; q0, q1 : out bit);
end;
architecture rtl of reset_pair is begin
  process(clk) begin
    if rising_edge(clk) then
      if rst = '1' then q0 <= '0'; q1 <= '0';
      else q0 <= d0; q1 <= d1; end if;
    end if;
  end process;
end;
)");
  ASSERT_EQ(pair->getInstances().size(), 2);
  for (auto* resetFlop : pair->getInstances()) {
    EXPECT_EQ(resetFlop->getModel(), NLDB0::getDFFSR());
    EXPECT_EQ(resetFlop->getInstTerm(NLDB0::getDFFSRReset())->getNet(),
              pair->getScalarTerm(NLName("rst"))->getNet());
  }
}

TEST_F(VHDLConstructorTest, SynchronousResetWithEnableWiringAndCycles) {
  std::ifstream fixture(SNL_VHDL_RESET_ENABLE);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  auto* model = NLDB0::getDFFSRE();
  EXPECT_EQ(instance->getModel(), model);
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("C")))->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("D")))->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("E")))->getNet(),
            design->getScalarTerm(NLName("en"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("R")))->getNet(),
            design->getScalarTerm(NLName("rst"))->getNet());
  EXPECT_EQ(instance->getInstTerm(model->getScalarTerm(NLName("Q")))->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
  if (const auto* path = std::getenv("VHDL_RESET_ENABLE_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<int> values;
    for (int value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<int>{1, 1, 1, 0, 0, 1}));
  }
}

TEST_F(VHDLConstructorTest, UnsupportedClockedProcessesPublishNoDesign) {
  for (const auto* body : {
      "process(clk) begin if clk'event and clk = '0' then q <= d; end if; end process;",
      "process(d) begin if clk'event and clk = '1' then q <= d; end if; end process;",
      "process(d) begin if rising_edge(clk) then q <= d; end if; end process;",
      "process(clk) begin if rising_edge(missing) then q <= d; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '0' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if missing = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if q = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '0' then q <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '1' then q <= '1'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if missing = '1' then q <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if q = '1' then q <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if d = '1' then d <= '0'; else q <= d; end if; end if; end process;",
      "process(clk) is variable v : bit; begin if rising_edge(clk) then if d = '1' then q <= '0'; else v := d; q <= v; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if rst = '0' then q <= '0'; elsif en = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if rst = '1' then q <= '0'; elsif en = '0' then q <= d; end if; end if; end process;",
      "process(clk) begin if rising_edge(clk) then if rst = '1' then q <= '1'; elsif en = '1' then q <= d; end if; end if; end process;",
      "process(clk) begin if clk'event and d = '1' then q <= d; end if; end process;",
      "process(clk) begin if missing'event and clk = '1' then q <= d; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= missing; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then d <= q; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d after 1 ns; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d; else q <= '0'; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d; q <= clk; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d when clk = '1' else clk; end if; end process;"}) {
    SCOPED_TRACE(body);
    const std::string source = std::string(
        "entity reg is port(clk, d : in bit; q : out bit); end; "
        "architecture rtl of reg is begin ") + body + " end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("reg")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, EquivalentSystemVerilogUsesSameRegisterModel) {
  SNLSVConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VHDL_EQUIVALENT_REGISTER_SV));
  auto* design = library_->getSNLDesign(NLName("reg_sv"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getInstances().size(), 1);
  auto* instance = *design->getInstances().begin();
  EXPECT_EQ(instance->getModel(), NLDB0::getDFF());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(),
            design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFData())->getNet(),
            design->getScalarTerm(NLName("d"))->getNet());
  EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
            design->getScalarTerm(NLName("q"))->getNet());
}

TEST_F(VHDLConstructorTest, ClockedPortTypesAndInitializationAreRejected) {
  for (const auto* ports : {
      "clk, d : in std_logic; q : out std_logic",
      "clk : in bit; d : in bit_vector(1 downto 0); q : out bit",
      "clk, d : in bit; q : out bit := '1'",
      "clk : out bit; d : in bit; q : out bit"}) {
    SCOPED_TRACE(ports);
    const std::string source = std::string("entity reg is port(") + ports +
        "); end; architecture rtl of reg is begin "
        "process(clk) begin if clk'event and clk = '1' then q <= d; "
        "end if; end process; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("reg")), nullptr);
  }
}

namespace {
std::string pipelineSource(const std::string& declarations, const std::string& writes) {
  return "entity pipeline is port(clk, d : in bit; q : out bit); end; "
      "architecture rtl of pipeline is " + declarations +
      " begin process(clk) is begin if clk'event and clk = '1' then " +
      writes + " end if; end process; end;";
}
}

TEST_F(VHDLConstructorTest, PipelineConnectivityAndCycles) {
  std::ifstream fixture(SNL_VHDL_PIPELINE);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  std::vector<std::string> sources{source,
      pipelineSource("signal stage : bit;", "q <= STAGE; Stage <= (d);"),
      pipelineSource("signal stage, extra : bit;", "extra <= stage; q <= stage; stage <= d;")};
  for (const auto& input : sources) {
    auto* design = VHDLConstructor(library_).construct(input);
    const bool extra = input.find("extra") != std::string::npos;
    ASSERT_EQ(design->getInstances().size(), extra ? 3 : 2);
    auto* clk = design->getScalarTerm(NLName("clk"))->getNet();
    auto* d = design->getScalarTerm(NLName("d"))->getNet();
    auto* q = design->getScalarTerm(NLName("q"))->getNet();
    auto* stage = design->getNet(NLName("stage"));
    ASSERT_NE(stage, nullptr);
    EXPECT_EQ(design->getScalarTerm(NLName("stage")), nullptr);
    std::unordered_map<SNLNet*, SNLNet*> drivers;
    for (auto* instance : design->getInstances()) {
      ASSERT_EQ(instance->getModel(), NLDB0::getDFF());
      EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(), clk);
      drivers.emplace(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
                      instance->getInstTerm(NLDB0::getDFFData())->getNet());
    }
    EXPECT_EQ(drivers.at(stage), d);
    EXPECT_EQ(drivers.at(q), stage);
    if (extra) EXPECT_EQ(drivers.at(design->getNet(NLName("extra"))), stage);
    // No DFF power-up state is promised. Start unknown, then compare only
    // after two rising edges have filled both stages with known input data.
    std::unordered_map<SNLNet*, int> values;
    for (const auto& [output, data] : drivers) values[output] = -1;
    const std::vector<int> stimulus{1, 0, 1, 1, 0, 0, 1, 0};
    std::vector<int> observed;
    for (std::size_t cycle = 0; cycle < stimulus.size(); ++cycle) {
      values[d] = stimulus[cycle];
      auto next = values;
      for (const auto& [output, data] : drivers) next[output] = values.at(data);
      values = next;
      if (cycle) {
        EXPECT_EQ(values.at(q), stimulus[cycle - 1]);
        observed.push_back(values.at(q));
      }
      // Changing data with no rising edge leaves the sampled outputs intact.
      values[d] = 1 - stimulus[cycle];
      EXPECT_EQ(values.at(q), cycle ? stimulus[cycle - 1] : -1);
    }
    if (const auto* path = std::getenv("VHDL_PIPELINE_REFERENCE")) {
      std::ifstream reference(path);
      ASSERT_TRUE(reference);
      for (int actual : observed) {
        int expected = -1;
        ASSERT_TRUE(reference >> expected);
        EXPECT_EQ(actual, expected);
      }
      std::string trailing;
      EXPECT_FALSE(reference >> trailing);
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, EventGuardStructuredEnablePreservesScheduling) {
  auto* design = VHDLConstructor(library_).construct(pipelineSource(
      "signal stage : bit;", "stage <= d; if d = '1' then q <= stage; end if;"));
  auto* stage = design->getScalarNet(NLName("stage"));
  auto* data = design->getScalarTerm(NLName("d"))->getNet();
  auto* output = design->getScalarTerm(NLName("q"))->getNet();
  for (unsigned pattern = 0; pattern < 8; ++pattern) {
    const bool d = pattern & 1, previous = pattern & 2, q = pattern & 4;
    std::unordered_map<SNLBitNet*, bool> values{{data, d}, {stage, previous}, {output, q}};
    std::unordered_set<SNLBitNet*> visiting;
    unsigned flops = 0;
    for (auto* flop : design->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
      ++flops;
      auto* target = flop->getInstTerm(NLDB0::getDFFOutput())->getNet();
      EXPECT_EQ(evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting),
                target == stage ? d : (d ? previous : q));
    }
    EXPECT_EQ(flops, 2u);
  }
}

TEST_F(VHDLConstructorTest, PipelineUnsupportedSemanticsPublishNoDesign) {
  for (const auto& [declarations, writes] : std::vector<std::pair<std::string, std::string>>{
      {"signal stage : std_logic;", "stage <= d; q <= stage;"},
      {"signal stage : bit_vector(1 downto 0);", "stage <= d; q <= stage;"},
      {"signal stage : bit bus;", "stage <= d; q <= stage;"},
      {"signal stage, STAGE : bit;", "stage <= d; q <= stage;"},
      {"signal d : bit;", "q <= d;"},
      {"signal stage : bit;", "q <= stage;"},
      {"signal stage : bit;", "stage <= d; q <= missing;"},
      {"signal stage : bit;", "stage <= d; q <= stage; stage <= clk;"},
      {"signal stage : bit;", "stage <= q; q <= stage;"},
      {"signal stage : bit;", "stage <= d; d <= stage;"},
      {"signal stage : bit;", "stage <= d; q <= stage after 1 ns;"},
      {"signal stage : bit;", "stage <= transport d; q <= stage;"},
      {"signal stage : bit;", "stage <= d; q <= stage; else q <= d;"},
      {"signal stage : bit;", "stage <= d; q <= stage; wait;"},
      {"signal stage : bit;", "stage <= d; q <= stage + d;"}}) {
    SCOPED_TRACE(declarations + writes);
    EXPECT_THROW(VHDLConstructor(library_).construct(pipelineSource(declarations, writes)), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
  for (const auto* suffix : {
      "stage <= d;",
      "process(clk) begin if clk'event and clk = '1' then stage <= d; end if; end process;"}) {
    auto source = pipelineSource("signal stage : bit;", "stage <= d; q <= stage;");
    source.insert(source.rfind("end;"), suffix);
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, VariableSchedulingConnectivityAndCycles) {
  std::ifstream fixture(SNL_VHDL_VARIABLES);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 4);
  EXPECT_EQ(design->getNet(NLName("temp")), nullptr);
  EXPECT_EQ(design->getNet(NLName("copy")), nullptr);
  auto* clk = design->getScalarTerm(NLName("clk"))->getNet();
  auto* d = design->getScalarTerm(NLName("d"))->getNet();
  auto* stage = design->getNet(NLName("stage"));
  auto* delayed = design->getScalarTerm(NLName("delayed"))->getNet();
  auto* immediate = design->getScalarTerm(NLName("immediate"))->getNet();
  auto* captured = design->getScalarTerm(NLName("captured"))->getNet();
  std::unordered_map<SNLNet*, SNLNet*> drivers;
  for (auto* instance : design->getInstances()) {
    ASSERT_EQ(instance->getModel(), NLDB0::getDFF());
    EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(), clk);
    drivers.emplace(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
                    instance->getInstTerm(NLDB0::getDFFData())->getNet());
  }
  EXPECT_EQ(drivers.at(stage), d);
  EXPECT_EQ(drivers.at(delayed), stage);
  EXPECT_EQ(drivers.at(immediate), d);
  EXPECT_EQ(drivers.at(captured), stage);
  std::unordered_map<SNLNet*, int> values;
  for (const auto& [output, data] : drivers) values[output] = -1;
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_VARIABLE_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  const std::vector<int> stimulus{1, 0, 1, 1, 0, 0, 1, 0};
  for (std::size_t cycle = 0; cycle < stimulus.size(); ++cycle) {
    values[d] = stimulus[cycle];
    auto next = values;
    for (const auto& [output, data] : drivers) next[output] = values.at(data);
    values = next;
    EXPECT_EQ(values.at(immediate), stimulus[cycle]);
    EXPECT_EQ(values.at(delayed), cycle ? stimulus[cycle - 1] : -1);
    EXPECT_EQ(values.at(captured), values.at(delayed));
    if (cycle && reference.is_open()) {
      for (auto* output : {delayed, immediate, captured}) {
        int expected = -1;
        ASSERT_TRUE(reference >> expected);
        EXPECT_EQ(values.at(output), expected);
      }
    }
  }
  if (reference.is_open()) {
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, RetainedVariableConnectivityAndCycles) {
  std::ifstream fixture(SNL_VHDL_RETAINED_VARIABLES);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* design = VHDLConstructor(library_).construct(source);
  ASSERT_EQ(design->getInstances().size(), 2);
  auto* clk = design->getScalarTerm(NLName("clk"))->getNet();
  auto* d = design->getScalarTerm(NLName("d"))->getNet();
  auto* q = design->getScalarTerm(NLName("q"))->getNet();
  auto* retained = design->getNet(NLName("retained"));
  ASSERT_NE(retained, nullptr);
  EXPECT_EQ(design->getScalarTerm(NLName("retained")), nullptr);
  std::unordered_map<SNLNet*, SNLNet*> drivers;
  for (auto* instance : design->getInstances()) {
    ASSERT_EQ(instance->getModel(), NLDB0::getDFF());
    EXPECT_EQ(instance->getInstTerm(NLDB0::getDFFClock())->getNet(), clk);
    drivers.emplace(instance->getInstTerm(NLDB0::getDFFOutput())->getNet(),
                    instance->getInstTerm(NLDB0::getDFFData())->getNet());
  }
  EXPECT_EQ(drivers.at(retained), d);
  EXPECT_EQ(drivers.at(q), retained);
  std::unordered_map<SNLNet*, int> values{{retained, -1}, {q, -1}};
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_RETAINED_VARIABLE_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  const std::vector<int> stimulus{1, 0, 1, 1, 0, 0, 1, 0};
  for (std::size_t cycle = 0; cycle < stimulus.size(); ++cycle) {
    values[d] = stimulus[cycle];
    auto next = values;
    for (const auto& [output, data] : drivers) next[output] = values.at(data);
    values = next;
    EXPECT_EQ(values.at(retained), stimulus[cycle]);
    EXPECT_EQ(values.at(q), cycle ? stimulus[cycle - 1] : -1);
    if (cycle && reference.is_open()) {
      int expected = -1;
      ASSERT_TRUE(reference >> expected);
      EXPECT_EQ(values.at(q), expected);
    }
  }
  if (reference.is_open()) {
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, UnsupportedVariablesPublishNoDesign) {
  for (const auto& [declarations, body] : std::vector<std::pair<std::string, std::string>>{
      {"variable v : bit;", "q <= v;"},
      {"variable v : bit;", "v := v; q <= v;"},
      {"variable v : bit := '0';", "v := d; q <= v;"},
      {"variable v : std_logic;", "v := d; q <= v;"},
      {"variable v : bit_vector(1 downto 0);", "v := d; q <= v;"},
      {"variable v : bit;", "v <= d; q <= v;"},
      {"variable v : bit;", "d := v; q <= d;"},
      {"variable v : bit;", "v := missing; q <= v;"},
      {"variable v, V : bit;", "v := d; q <= v;"},
      {"variable clk : bit;", "clk := d; q <= clk;"},
      {"variable v : bit;", "v := not d; q <= v;"},
      {"variable v : bit;", "v := d after 1 ns; q <= v;"},
      {"variable v : bit;", "if d = '1' then v := d; end if; q <= v;"},
      {"variable v : bit;", "v := d; q <= v; q <= d;"},
      {"variable v : bit;", "v := q; v := d; q <= v;"},
      {"variable v : bit;", "v := d;"}}) {
    SCOPED_TRACE(declarations + body);
    const std::string source = "entity p is port(clk, d : in bit; q : out bit); end; "
        "architecture rtl of p is begin process(clk) " + declarations +
        " begin if clk'event and clk = '1' then " + body + " end if; end process; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, LowersOneLevelDirectEntityHierarchy) {
  std::ifstream fixture(SNL_VHDL_HIERARCHY);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  auto* top = VHDLConstructor(library_).construct(source, "HIERARCHY_TOP");
  ASSERT_NE(top, nullptr);
  auto* model = library_->getSNLDesign(NLName("inv4"));
  ASSERT_NE(model, nullptr);
  EXPECT_EQ(model->getInstances().size(), 4);
  ASSERT_EQ(top->getInstances().size(), 2);
  auto* u0 = top->getInstance(NLName("u0"));
  auto* u1 = top->getInstance(NLName("u1"));
  ASSERT_NE(u0, nullptr);
  ASSERT_NE(u1, nullptr);
  EXPECT_EQ(u0->getModel(), model);
  EXPECT_EQ(u1->getModel(), model);
  auto* a = top->getBusTerm(NLName("a"));
  auto* y = top->getBusTerm(NLName("y"));
  auto* mid = dynamic_cast<SNLBusNet*>(top->getNet(NLName("mid")));
  auto* modelA = model->getBusTerm(NLName("a"));
  auto* modelY = model->getBusTerm(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_NE(mid, nullptr);
  for (std::size_t position = 0; position < 4; ++position) {
    EXPECT_EQ(u0->getInstTerm(modelA->getBitAtPosition(position))->getNet(),
              a->getBitAtPosition(position)->getNet());
    EXPECT_EQ(u0->getInstTerm(modelY->getBitAtPosition(position))->getNet(),
              static_cast<SNLNet*>(mid->getBitAtPosition(position)));
    EXPECT_EQ(u1->getInstTerm(modelA->getBitAtPosition(position))->getNet(),
              static_cast<SNLNet*>(mid->getBitAtPosition(position)));
    EXPECT_EQ(u1->getInstTerm(modelY->getBitAtPosition(position))->getNet(),
              y->getBitAtPosition(position)->getNet());
  }
  if (const auto* path = std::getenv("VHDL_HIERARCHY_REFERENCE")) {
    std::ifstream reference(path);
    ASSERT_TRUE(reference);
    std::vector<std::string> values;
    for (std::string value; reference >> value;) values.push_back(value);
    EXPECT_EQ(values, (std::vector<std::string>{"1001", "0110", "0011"}));
  }
}

TEST_F(VHDLConstructorTest, SpecializesGenericChildrenPerInstance) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity wire is
  generic(n : positive);
  port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0));
end;
architecture rtl of wire is begin y <= a; end;
entity generic_top is port(
  a3 : in bit_vector(2 downto 0); y3 : out bit_vector(2 downto 0);
  a5 : in bit_vector(4 downto 0); y5 : out bit_vector(4 downto 0));
end;
architecture structural of generic_top is begin
  w3: entity work.wire generic map(n => 3) port map(a3, y3);
  w5: entity work.wire generic map(n => 5) port map(a5, y5);
end;
)", "generic_top");
  ASSERT_NE(top, nullptr);
  auto* w3 = top->getInstance(NLName("w3"));
  auto* w5 = top->getInstance(NLName("w5"));
  ASSERT_NE(w3, nullptr);
  ASSERT_NE(w5, nullptr);
  EXPECT_NE(w3->getModel(), w5->getModel());
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(w3->getModel()->getTerm(NLName("a")))->getWidth(), 3);
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(w5->getModel()->getTerm(NLName("a")))->getWidth(), 5);
}

TEST_F(VHDLConstructorTest, ConstructsHierarchyFromFile) {
  auto* top = VHDLConstructor(library_).constructFile(
      SNL_VHDL_HIERARCHY, "hierarchy_top");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getName(), NLName("hierarchy_top"));
  EXPECT_NE(library_->getSNLDesign(NLName("inv4")), nullptr);
}

TEST_F(VHDLConstructorTest, InvalidHierarchyPublishesNoDesign) {
  constexpr auto valid = R"(
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture structural of top is begin
  u0: entity work.leaf port map(a, y);
end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(valid), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
  EXPECT_THROW(VHDLConstructor(library_).construct(valid, "missing"), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());

  for (const auto* source : {
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u: entity work.leaf port map(y, a); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is signal spare : bit; begin
          u: entity work.leaf port map(a, y); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u0: entity work.leaf port map(a, y);
          u1: entity work.leaf port map(a, y); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          architecture other of leaf is begin y <= a; end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u: entity work.leaf port map(a, y); end;)",
      R"(entity leaf is port(a : in bit; y : out bit); end;
          architecture rtl of leaf is begin y <= not a; end;
          entity middle is port(a : in bit; y : out bit); end;
          architecture structural of middle is begin
          u: entity work.leaf port map(a, y); end;
          entity top is port(a : in bit; y : out bit); end;
          architecture structural of top is begin
          u: entity work.middle port map(a, y); end;)"}) {
    SCOPED_TRACE(source);
    EXPECT_THROW(VHDLConstructor(library_).construct(source, "top"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, PackageROMAndDynamicMemoryCycles) {
  VHDLConstructor constructor(library_);
  EXPECT_EQ(constructor.construct(R"(
library ieee; use ieee.std_logic_1164.all;
package tables is
  type table_type is array (0 to 3) of std_logic_vector(7 downto 0);
  constant rom : table_type := (x"63", x"7C", x"77", x"7B");
end package;
package body tables is end package body;
)"), nullptr);
  auto* design = constructor.construct(R"(
library ieee; use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all; use ieee.std_logic_unsigned.all;
use work.tables.all;
entity memory_test is
  port(clk, we : in std_logic; addr : in std_logic_vector(1 downto 0);
       data : in std_logic_vector(7 downto 0);
       q, lookup, rotated : out std_logic_vector(7 downto 0));
end;
architecture rtl of memory_test is
  signal ram : table_type;
  signal count : integer range 0 to 3;
begin
  lookup <= rom(conv_integer(addr));
  rotated <= data(3 downto 0) & data(7 downto 4);
  write_ram: process(clk) begin
    if rising_edge(clk) then
      if we = '1' then ram(conv_integer(addr)) <= data; end if;
      q <= ram(conv_integer(addr));
    end if;
  end process;
  counter: process(clk) begin
    if rising_edge(clk) then
      if count = 3 then count <= 0; else count <= count + 1; end if;
    end if;
  end process;
end;
)");
  ASSERT_NE(design, nullptr);
  const auto flops = dffBits(design);
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  ASSERT_EQ(flops.size(), 10u);
  auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 1u);
  const auto signature = NLDB0::getMemorySignature(memories.begin()->first);
  EXPECT_EQ(signature.width, 8u);
  EXPECT_EQ(signature.depth, 4u);
  unsigned ram[4]{};
  const unsigned rom[]{0x63, 0x7c, 0x77, 0x7b};
  for (unsigned cycle = 0; cycle < 80; ++cycle) {
    const unsigned address = (cycle * 3) % 4;
    const unsigned data = (cycle * 73 + 5) & 255;
    const bool write = cycle % 3 != 0;
    auto values = state;
    values[design->getScalarTerm(NLName("we"))->getNet()] = write;
    for (unsigned bit = 0; bit < 2; ++bit)
      values[design->getBusTerm(NLName("addr"))->getBit(bit)->getNet()] = (address >> bit) & 1;
    for (unsigned bit = 0; bit < 8; ++bit)
      values[design->getBusTerm(NLName("data"))->getBit(bit)->getNet()] = (data >> bit) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 8; ++bit) {
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("lookup"))->getBit(bit)->getNet(), values, visiting),
                bool((rom[address] >> bit) & 1));
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("rotated"))->getBit(bit)->getNet(), values, visiting),
                bool((data >> ((bit + 4) % 8)) & 1));
    }
    const auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
    for (const auto& flop : flops)
      state[flop.output] =
          evaluateRTL(flop.data, values, visiting, &memories);
    for (unsigned bit = 0; bit < 8; ++bit)
      EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBit(bit)->getNet()), bool((ram[address] >> bit) & 1));
    auto* count = design->getBusNet(NLName("count"));
    for (unsigned bit = 0; bit < 2; ++bit)
      EXPECT_EQ(state.at(count->getBit(bit)), bool((((cycle + 1) % 4) >> bit) & 1));
    if (write) ram[address] = data;
    memories = nextMemories;
  }
}

TEST_F(VHDLConstructorTest, PackageComponentSpecializationAndNestedGenerate) {
  auto* top = VHDLConstructor(library_).construct(R"(
package interfaces is
  component leaf is generic(n : integer range 1 to 8 := 2);
    port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0));
  end component;
end;
entity leaf is generic(n : integer range 1 to 8 := 2);
  port(a : in bit_vector(n-1 downto 0); y : out bit_vector(n-1 downto 0)); end;
architecture rtl of leaf is begin
  g: for i in 0 to n-1 generate
    h: for j in 0 to 0 generate y(i) <= not a(i+j); end generate;
  end generate;
end;
use work.interfaces.all;
entity wrapper is port(a : in bit_vector(3 downto 0); y : out bit_vector(3 downto 0)); end;
architecture rtl of wrapper is begin
  u: leaf generic map(n => 4) port map(y => y, a => a);
end;
)", "wrapper");
  ASSERT_NE(top, nullptr);
  auto* instance = top->getInstance(NLName("u"));
  ASSERT_NE(instance, nullptr);
  auto* leaf = instance->getModel();
  EXPECT_EQ(leaf->getBusTerm(NLName("a"))->getWidth(), 4);
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit) {
      auto* input = leaf->getBusTerm(NLName("a"))->getBit(bit);
      EXPECT_EQ(instance->getInstTerm(input)->getNet(), top->getBusTerm(NLName("a"))->getBit(bit)->getNet());
      values[input->getNet()] = (pattern >> bit) & 1;
    }
    for (unsigned bit = 0; bit < 4; ++bit)
      EXPECT_EQ(evaluateRTL(leaf->getBusTerm(NLName("y"))->getBit(bit)->getNet(), values, visiting),
                !bool((pattern >> bit) & 1));
  }
}

TEST_F(VHDLConstructorTest, InvalidPackageRTLDoesNotPublishDesigns) {
  const std::string source = R"(
library ieee; use ieee.std_logic_1164.all;
package constants is
  type table_type is array (0 to 1) of std_logic_vector(3 downto 0);
  constant rom : table_type := (x"A", x"5");
end;
library ieee; use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all; use ieee.std_logic_unsigned.all;
use work.constants.all;
entity lookup is port(a : in std_logic_vector(0 downto 0); y : out std_logic_vector(3 downto 0)); end;
architecture rtl of lookup is begin y <= rom(conv_integer(a)); end;
)";
  for (const auto& [before, after] : std::vector<std::pair<std::string, std::string>>{
      {"(x\"A\", x\"5\")", "(x\"A\", x\"5\", x\"0\")"},
      {"x\"A\"", "x\"Z\""},
      {"rom(conv_integer(a))", "rom(a)"},
      {"rom(conv_integer(a))", "rom(2)"},
      {"use work.constants.all;", ""},
      {"use ieee.std_logic_arith.all;", ""},
      {"begin y <=", "begin a <= \"0\"; y <="},
  }) {
    SCOPED_TRACE(after);
    auto invalid = source;
    ASSERT_NE(invalid.find(before), std::string::npos);
    invalid.replace(invalid.find(before), before.size(), after);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, UnsignedArithmeticWidthsAndDirections) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity arithmetic is port(signal a : in unsigned(4 downto 2);
  signal b : in unsigned(7 to 8); product : out unsigned(4 downto 0);
  sum, difference : out unsigned(2 downto 0); joined : out unsigned(4 downto 0);
  eq : out bit); end;
architecture rtl of arithmetic is begin
  product <= a * b; sum <= a + b; difference <= a - b;
  joined <= a & b;
  eq <= '1' when a = b else '0';
end;
)");
  for (unsigned a = 0; a < 8; ++a) for (unsigned b = 0; b < 4; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("a"))->getBit(2+i)->getNet()] = (a >> i) & 1;
    for (unsigned i = 0; i < 2; ++i)
      values[design->getBusTerm(NLName("b"))->getBit(8-i)->getNet()] = (b >> i) & 1;
    for (const auto& [name, expected] : std::vector<std::pair<std::string, unsigned>>{
        {"product", a*b}, {"sum", (a+b)&7}, {"difference", (a-b)&7}, {"joined", (a<<2)|b}}) {
      auto* term = design->getBusTerm(NLName(name));
      for (unsigned i = 0; i < term->getWidth(); ++i)
        EXPECT_EQ(evaluateRTL(term->getBit(i)->getNet(), values, visiting), bool((expected >> i)&1))
            << name << ":" << a << ":" << b << ":" << i;
    }
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("eq"))->getNet(), values, visiting), a == b);
  }
}

TEST_F(VHDLConstructorTest, SignalInitializersPreserveRegisterBitOrder) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity initialized is port(clk : in std_logic; d : in unsigned(2 downto 0);
  q : out unsigned(2 downto 0)); end;
architecture rtl of initialized is
  signal a, b : unsigned(4 to 6) := "101";
  signal c : bit := '1';
begin
  process(clk) begin if rising_edge(clk) then a <= d; b <= a; c <= '0'; end if; end process;
  q <= b;
end;
)");
  unsigned count = 0;
  for (auto* flop : design->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
    ++count;
    auto* initial = flop->getInstParameter(NLName("INIT"));
    ASSERT_NE(initial, nullptr);
    EXPECT_EQ(initial->getValue(), flop->getModel()->getBusTerm(NLName("Q")) ? "3'b101" : "1'b1");
  }
  EXPECT_EQ(count, 3u);
}

TEST_F(VHDLConstructorTest, RejectsUnrepresentableSignalInitializers) {
  for (const auto* body : {
      "signal s : bit := d; begin process(clk) begin if clk'event and clk = '1' then s <= d; end if; end process; q <= s;",
      "signal s : bit := '1'; begin s <= d; q <= s;",
      "signal s : bit := 'X'; begin process(clk) begin if clk'event and clk = '1' then s <= d; end if; end process; q <= s;"}) {
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string(
        "entity invalid is port(clk, d : in bit; q : out bit); end; architecture rtl of invalid is ") + body + " end;"), NLException);
    EXPECT_EQ(library_->getSNLDesign(NLName("invalid")), nullptr);
  }
}

TEST_F(VHDLConstructorTest, ArchitectureLocalComponentBindings) {
  const std::string source = R"(
entity leaf is port(signal a : in bit; signal y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity wrapper is port(a : in bit; y : out bit); end;
architecture rtl of wrapper is
  component leaf is port(signal a : in bit; signal y : out bit); end component leaf;
begin u : leaf port map(y => y, a => a); end;
)";
  auto* top = VHDLConstructor(library_).construct(source, "wrapper");
  ASSERT_NE(top, nullptr);
  auto* child = top->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* model = child->getModel();
  EXPECT_EQ(child->getInstTerm(model->getScalarTerm(NLName("a")))->getNet(),
            top->getScalarTerm(NLName("a"))->getNet());
  EXPECT_EQ(child->getInstTerm(model->getScalarTerm(NLName("y")))->getNet(),
            top->getScalarTerm(NLName("y"))->getNet());
  top->destroy();
  model->destroy();
  for (const auto& replacement : {std::string("signal y : in bit"), std::string("signal y : out bit_vector(1 downto 0)")}) {
    auto invalid = source;
    const auto position = invalid.find("signal y : out bit", invalid.find("component leaf"));
    invalid.replace(position, std::string("signal y : out bit").size(), replacement);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid, "wrapper"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, InvalidUnsignedArithmeticPublishesNoDesign) {
  for (const auto* assignment : {"y <= a * b;", "y <= a + b;", "y <= a / b;"}) {
    const auto source = std::string(
        "library ieee; use ieee.numeric_std.all; "
        "entity invalid is port(a : in unsigned(2 downto 0); b : in unsigned(1 downto 0); "
        "y : out unsigned(3 downto 0)); end; architecture rtl of invalid is begin ") + assignment + " end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, RTLTopInferenceRejectsAmbiguousRoots) {
  const auto source = R"(
entity first is port(a : in bit; y : out bit); end;
architecture rtl of first is begin
  g: for i in 0 to 0 generate y <= a; end generate;
end;
entity second is port(a : in bit; y : out bit); end;
architecture rtl of second is begin y <= a; end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
  auto* selected = VHDLConstructor(library_).construct(source, "SECOND");
  ASSERT_NE(selected, nullptr);
  EXPECT_EQ(selected->getName(), NLName("second"));
}

TEST_F(VHDLConstructorTest, RTLTopInferenceRejectsRootlessHierarchy) {
  const auto source = R"(
entity first is port(a : in bit; y : out bit); end;
architecture rtl of first is begin
  g: for i in 0 to 0 generate
    u: entity work.second port map(a, y);
  end generate;
end;
entity second is port(a : in bit; y : out bit); end;
architecture rtl of second is begin
  u: entity work.first port map(a, y);
end;
)";
  EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, GeneratedFIRHierarchyCycles) {
  auto* top = VHDLConstructor(library_).constructFile(SNL_VHDL_GENERATED_FIR);
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getName(), NLName("fir_generated"));
  const auto inputs = naja::NL::test::firInputs(4);
  const auto outputs = naja::NL::test::simulateFIR(top, inputs);
  const unsigned coefficients[3][4]{{1, 3, 6, 10}, {10, 6, 3, 1}, {5, 7, 9, 11}};
  for (size_t cycle = 0; cycle < inputs.size(); ++cycle) {
    unsigned expected = 0;
    for (unsigned age = 0; age < 3; ++age) if (cycle >= age)
      for (unsigned lane = 0; lane < 4; ++lane)
        if ((inputs[cycle-age] >> lane) & 1) expected += coefficients[age][lane];
    EXPECT_EQ(outputs[cycle], expected & 31) << "cycle " << cycle;
  }
  for (unsigned i = 0; i < 4; ++i)
    EXPECT_NE(top->getInstance(NLName("lanes[" + std::to_string(i) + "].singleton[0].f")), nullptr);
  if (const auto* trace = std::getenv("VHDL_FIR_REFERENCE")) {
    std::ifstream reference(trace);
    ASSERT_TRUE(reference);
    for (size_t cycle = 3; cycle < outputs.size(); ++cycle) {
      unsigned expected;
      ASSERT_TRUE(reference >> expected);
      EXPECT_EQ(outputs[cycle], expected) << "NVC cycle " << cycle;
    }
    std::string trailing;
    EXPECT_FALSE(reference >> trailing);
  }
}

TEST_F(VHDLConstructorTest, SignedVectorArithmeticExtendsTheSignBit) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_signed.all;
entity signed_math is port(a : in std_logic_vector(3 downto 0);
  b : in std_logic_vector(5 to 7); sum, diff : out std_logic_vector(8 downto 5); eq : out bit); end;
architecture rtl of signed_math is
begin sum <= a + b; diff <= a - b; eq <= '1' when a = b else '0'; end;
)");
  for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 8; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
    for (unsigned bit = 0; bit < 3; ++bit)
      values[design->getBusTerm(NLName("b"))->getBit(7-bit)->getNet()] = (b >> bit) & 1;
    const int sa = a < 8 ? int(a) : int(a)-16, sb = b < 4 ? int(b) : int(b)-8;
    for (const auto& [name, result] : std::vector<std::pair<std::string, int>>{{"sum", sa+sb}, {"diff", sa-sb}})
      for (unsigned bit = 0; bit < 4; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(bit+5)->getNet(), values, visiting),
                  bool((unsigned(result) >> bit) & 1)) << name << ':' << sa << ':' << sb;
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("eq"))->getNet(), values, visiting), sa == sb);
  }
}

TEST_F(VHDLConstructorTest, StaticConversionsAndIntegerArrayIndexSubtypes) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity conversions is port(a : in std_logic_vector(2 to 5);
  copy : out unsigned(3 downto 0); truncated, wide : out std_logic_vector(4 downto 0)); end;
architecture rtl of conversions is
  constant base : integer := -2147483648;
  type coefficients is array(integer range <>) of integer;
  constant table_value : coefficients := (-7, 47);
  type bounded is array(3 downto 1) of integer;
  constant table_bounded : bounded := (12, 9, 5);
  type masks is array(positive range <>) of std_logic_vector(4 downto 0);
  constant mask : masks := ("11111", "00000");
begin
  copy <= unsigned(a);
  truncated <= std_logic_vector(to_unsigned(table_value(base+1), 5)) and mask(1);
  wide <= std_logic_vector(to_unsigned(table_bounded(2) + table_value(base), 5));
end;
)");
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(5-bit)->getNet()] = (pattern >> bit) & 1;
    for (unsigned bit = 0; bit < 4; ++bit)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("copy"))->getBit(bit)->getNet(), values, visiting),
                bool((pattern >> bit) & 1));
    for (const auto& [name, expected] : std::vector<std::pair<std::string, unsigned>>{{"truncated", 15}, {"wide", 2}})
      for (unsigned bit = 0; bit < 5; ++bit)
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName(name))->getBit(bit)->getNet(), values, visiting),
                  bool((expected >> bit) & 1));
  }
}

TEST_F(VHDLConstructorTest, IndexedPortSlicesPreservePositionAndSharedSpecialization) {
  auto* top = VHDLConstructor(library_).construct(R"(
entity leaf is generic(n : positive); port(a : in bit_vector(n-1 downto 0); y : out bit_vector(1 to n)); end;
architecture rtl of leaf is begin y <= a; end;
entity top is port(a : in bit_vector(3 downto 0); y : out bit_vector(4 to 7)); end;
architecture rtl of top is begin
  g: for i in 0 to 1 generate
    u: entity work.leaf generic map(2) port map(a(3-2*i downto 2-2*i), y(4+2*i to 5+2*i));
  end generate;
end;
)", "top");
  auto* first = top->getInstance(NLName("g[0].u"));
  auto* second = top->getInstance(NLName("g[1].u"));
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->getModel(), second->getModel());
  for (unsigned i = 0; i < 2; ++i) {
    auto* child = i ? second : first;
    auto* model = child->getModel();
    for (unsigned bit = 0; bit < 2; ++bit) {
      EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("a"))->getBit(1-bit))->getNet(),
                top->getBusTerm(NLName("a"))->getBit(3-2*i-bit)->getNet());
      EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("y"))->getBit(1+bit))->getNet(),
                top->getBusTerm(NLName("y"))->getBit(4+2*i+bit)->getNet());
    }
  }
}

TEST_F(VHDLConstructorTest, InvalidGeneratedFIRAndConversionsPublishNoDesigns) {
  std::ifstream fixture(SNL_VHDL_GENERATED_FIR);
  ASSERT_TRUE(fixture);
  const std::string source((std::istreambuf_iterator<char>(fixture)), {});
  for (const auto& [before, after] : std::vector<std::pair<std::string, std::string>>{
      {"  y_out <= result(-1)", "  LANES: for k in 9 to 9 generate begin end generate; y_out <= result(-1)"},
      {"first(i)", "first(i+1)"}, {"result(i-1)", "result(i)"},
      {"regx_in(i+j)", "regx_in"}, {"result(i-1)", "regx_in(i)"},
      {"to_unsigned(c1, n)", "to_unsigned(-1, n)"},
      {"to_unsigned(c1, n)", "to_unsigned(c1, 0)"},
      {"to_unsigned(c1, n)", "to_unsigned(c1, -1)"},
      {"to_unsigned(c1, n)", "to_unsigned(c1, n, 1)"},
      {"to_unsigned(c1, n)", "to_unsigned(reg_in, n)"},
      {"to_unsigned(c1, n)", "unknown_function(c1, n)"},
      {"use ieee.numeric_std.all;", ""},
      {"constant first : coefficients := (1, 3, 6, 10);", "constant first : coefficients(0 to 4) := (1, 3, 6, 10);"},
      {"constant third : positive_indices", "constant third : positive_indices(0 to 3)"},
      {"(1, 3, 6, 10)", "(1, 3, 6, 2147483648)"},
      {"signal result : results;", "signal first : results; signal result : results;"},
      {"first(i), second(i), third(i+1), width", "first(i), second(i), third(i+1)"},
      {"use ieee.std_logic_signed.all;", "use ieee.std_logic_signed.all; use ieee.std_logic_unsigned.all;"}}) {
    SCOPED_TRACE(after);
    auto invalid = source;
    const auto position = invalid.find(before);
    ASSERT_NE(position, std::string::npos);
    invalid.replace(position, before.size(), after);
    EXPECT_THROW(VHDLConstructor(library_).construct(invalid, "fir_generated"), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, MissingGenericDiagnosticIncludesSourceLocation) {
  const std::string source =
      "entity needs_width is\n"
      "generic(\n"
      "  Nin : integer);\n"
      "port(a : in bit_vector(Nin-1 downto 0); y : out bit_vector(Nin-1 downto 0)); end;\n"
      "architecture rtl of needs_width is begin y <= a; end;\n";
  try {
    VHDLConstructor(library_).construct(source, "needs_width");
    FAIL() << "An explicit top requires all generic values";
  } catch (const NLException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(message.find("missing generic value: nin"), std::string::npos);
    EXPECT_NE(message.find("line 3, column 3"), std::string::npos) << message;
  }
  EXPECT_TRUE(library_->getSNLDesigns().empty());
  EXPECT_EQ(VHDLConstructor(library_).construct(source), nullptr);
  auto* top = VHDLConstructor(library_).construct(
      "entity parent is port(a : in bit_vector(2 downto 0); y : out bit_vector(2 downto 0)); end;\n"
      "architecture rtl of parent is begin child : entity work.needs_width\n"
      "generic map(Nin => 3) port map(a, y); end;", "parent");
  ASSERT_NE(top, nullptr);
  auto* child = top->getInstance(NLName("child"));
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->getModel()->getBusTerm(NLName("a"))->getWidth(), 3);
}

TEST_F(VHDLConstructorTest, RetainedDependencyDiagnosticUsesOriginalFileAndLine) {
  // The dependency is only elaborated by its parent, after another source was
  // retained. Neither the parent file nor combined-source line is its location.
  const auto directory = std::filesystem::temp_directory_path() /
      ("naja-vhdl-diagnostic-" + std::to_string(reinterpret_cast<uintptr_t>(this)));
  std::filesystem::create_directories(directory);
  struct Cleanup {
    std::filesystem::path directory;
    ~Cleanup() { std::filesystem::remove_all(directory); }
  } cleanup{directory};
  const auto package = directory / "prefix.vhd";
  const auto child = directory / "child.vhd";
  const auto parent = directory / "parent.vhd";
  std::ofstream(package) << "package prefix is\nconstant unused : integer := 1;\nend;\n";
  std::ofstream(child) <<
      "entity child is generic(n : positive); port(a : in bit; y : out bit); end;\n"
      "architecture rtl of child is\n"
      "begin\n"
      "  y <= unknown;\n"
      "end;\n";
  std::ofstream(parent) <<
      "entity parent is port(a : in bit; y : out bit); end;\n"
      "architecture rtl of parent is begin\n"
      "u : entity work.child generic map(3) port map(a,y); end;\n";
  VHDLConstructor constructor(library_);
  EXPECT_EQ(constructor.constructFile(package), nullptr);
  EXPECT_EQ(constructor.constructFile(child), nullptr);
  try {
    constructor.constructFile(parent, "parent");
    FAIL() << "The invalid dependency must be rejected";
  } catch (const NLException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(message.find("no declaration for object: unknown"), std::string::npos);
    EXPECT_NE(message.find("in '" + child.string() + "' at line 4, column 8"), std::string::npos) << message;
  }
  EXPECT_TRUE(library_->getSNLDesigns().empty());
}

TEST_F(VHDLConstructorTest, NumericSignedArithmeticAndConversions) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
entity numeric_signed_math is port(a : in signed(3 downto 0);
  b : in signed(5 to 7); sum, diff : out signed(8 downto 5);
  product : out signed(6 downto 0); joined : out signed(6 downto 0);
  roundtrip : out unsigned(3 downto 0); eq : out bit); end;
architecture rtl of numeric_signed_math is
begin
  sum <= a + b; diff <= a - b; product <= a * b; joined <= a & b;
  roundtrip <= unsigned(signed(std_logic_vector(a)));
  eq <= '1' when a = b else '0';
end;
)");
  for (unsigned a = 0; a < 16; ++a) for (unsigned b = 0; b < 8; ++b) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned bit = 0; bit < 4; ++bit)
      values[design->getBusTerm(NLName("a"))->getBit(bit)->getNet()] = (a >> bit) & 1;
    for (unsigned bit = 0; bit < 3; ++bit)
      values[design->getBusTerm(NLName("b"))->getBit(7-bit)->getNet()] = (b >> bit) & 1;
    const int sa = a < 8 ? int(a) : int(a)-16, sb = b < 4 ? int(b) : int(b)-8;
    for (const auto& [name, result] : std::vector<std::pair<std::string, int>>{
        {"sum", sa+sb}, {"diff", sa-sb}, {"product", sa*sb},
        {"joined", int((a << 3) | b)}, {"roundtrip", int(a)}}) {
      auto* term = design->getBusTerm(NLName(name));
      for (unsigned bit = 0; bit < term->getWidth(); ++bit)
        EXPECT_EQ(evaluateRTL(term->getBit(term->getLSB()+bit)->getNet(), values, visiting),
                  bool((unsigned(result) >> bit) & 1)) << name << ':' << sa << ':' << sb;
    }
    EXPECT_EQ(evaluateRTL(design->getScalarTerm(NLName("eq"))->getNet(), values, visiting), sa == sb);
  }
}

TEST_F(VHDLConstructorTest, InvalidSignedArithmeticPublishesNoDesign) {
  for (const auto* expression : {"a + b", "a * a", "a & b", "signed(missing)"}) {
    SCOPED_TRACE(expression);
    const auto source = std::string(
        "library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;\n"
        "entity invalid_signed is port(a : in signed(2 downto 0); b : in unsigned(2 downto 0);\n"
        "y : out signed(2 downto 0)); end; architecture rtl of invalid_signed is begin\n"
        "y <= ") + expression + "; end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
    EXPECT_TRUE(library_->getSNLDesigns().empty());
  }
}

TEST_F(VHDLConstructorTest, GroupsArrayBusesAndPartialRegisters) {
  auto* design = VHDLConstructor(library_).construct(R"(
entity grouped is port(clk, sel : in bit; a, b : in bit_vector(2 to 5);
  y : out bit_vector(9 downto 6)); end;
architecture rtl of grouped is
  type words is array (3 downto 2) of bit_vector(-1 to 2);
  signal storage : words;
  signal partial : bit_vector(7 downto 2) := "101001";
begin
  storage(3) <= a;
  storage(2) <= b;
  y <= storage(3) when sel = '1' else storage(2);
  process(clk) begin
    if rising_edge(clk) then
      partial(7 downto 6) <= a(2 to 3);
      partial(3 downto 2) <= b(4 to 5);
    end if;
  end process;
  process(clk) begin
    if rising_edge(clk) then partial(5 downto 4) <= "00"; end if;
  end process;
end;
)");
  for (const auto* name : {"storage(3)", "storage(2)"}) {
    auto* bus = design->getBusNet(NLName(name));
    ASSERT_NE(bus, nullptr);
    EXPECT_EQ(bus->getMSB(), -1);
    EXPECT_EQ(bus->getLSB(), 2);
  }
  unsigned muxes = 0, flops = 0;
  for (auto* instance : design->getInstances()) {
    auto* model = instance->getModel();
    if (NLDB0::isMux2(model)) {
      ++muxes;
      EXPECT_EQ(NLDB0::getMux2Output(model)->getWidth(), 4);
    }
    if (NLDB0::isDFF(model)) {
      ++flops;
      ASSERT_NE(model->getBusTerm(NLName("Q")), nullptr);
      EXPECT_EQ(model->getBusTerm(NLName("Q"))->getWidth(), 2);
      ASSERT_NE(instance->getInstParameter(NLName("INIT")), nullptr);
      const auto first = instance->getInstTerm(model->getBusTerm(NLName("Q"))->getBit(1))->getNet();
      EXPECT_EQ(instance->getInstParameter(NLName("INIT"))->getValue(),
          first == design->getBusNet(NLName("partial"))->getBit(3) ? "2'b01" : "2'b10");
    }
  }
  EXPECT_EQ(muxes, 1u);
  EXPECT_EQ(flops, 3u);
  for (unsigned pattern = 0; pattern < 512; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    values[design->getScalarTerm(NLName("sel"))->getNet()] = pattern & 1;
    for (unsigned i = 0; i < 4; ++i) {
      values[design->getBusTerm(NLName("a"))->getBit(2+i)->getNet()] = (pattern >> (1+i)) & 1;
      values[design->getBusTerm(NLName("b"))->getBit(2+i)->getNet()] = (pattern >> (5+i)) & 1;
    }
    std::unordered_set<SNLBitNet*> visiting;
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBit(9-i)->getNet(), values, visiting),
          bool((pattern >> ((pattern & 1 ? 1 : 5) + i)) & 1));
    for (const auto& flop : dffBits(design)) {
      const auto index = static_cast<SNLBusNetBit*>(flop.output)->getBit();
      const auto inputPosition = index >= 6 ? 1 + 7 - index : 7 + 3 - index;
      EXPECT_EQ(evaluateRTL(flop.data, values, visiting), index == 5 || index == 4 ? false : bool((pattern >> inputPosition) & 1));
    }
  }
}

TEST_F(VHDLConstructorTest, InferredMemoryReadBeforeWriteAndEnableCycles) {
  auto* design = VHDLConstructor(library_).constructFile(SNL_VHDL_MEMORY);
  auto memories = naja::NL::test::zeroMemoryState(design);
  ASSERT_EQ(memories.size(), 1u);
  auto* memory = memories.begin()->first;
  const auto signature = NLDB0::getMemorySignature(memory);
  EXPECT_EQ(signature.width, 8u);
  EXPECT_EQ(signature.depth, 4u);
  EXPECT_EQ(signature.readPorts, 1u);
  EXPECT_EQ(signature.writePorts, 1u);
  EXPECT_EQ(signature.resetMode, NLDB0::MemoryResetMode::None);
  EXPECT_EQ(memory->getInstTerm(NLDB0::getMemoryClock(memory->getModel()))->getNet(),
      design->getScalarTerm(NLName("clk"))->getNet());
  EXPECT_TRUE(memory->getInstTerm(NLDB0::getMemoryReset(memory->getModel()))->getNet()->isConstant0());
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 12u); // Addresses and read register, no storage DFFs.
  std::unordered_map<SNLBitNet*, bool> state;
  for (const auto& flop : flops) state[flop.output] = false;
  unsigned words[4]{}, writeAddress = 0, readAddress = 0, registered = 0;
  std::ifstream reference;
  if (const auto* path = std::getenv("VHDL_MEMORY_REFERENCE")) {
    reference.open(path);
    ASSERT_TRUE(reference);
  }
  for (unsigned cycle = 0; cycle < 80; ++cycle) {
    const bool reset = cycle == 0 || cycle == 17 || cycle == 39;
    const bool enable = cycle > 0 && (cycle < 5 || cycle % 5 != 0);
    const bool write = cycle < 5 || cycle % 3 != 0;
    const unsigned data = (cycle * 37 + 11) & 255;
    auto values = state;
    for (const auto& [name, value] : std::vector<std::pair<std::string, bool>>{
        {"ce", enable}, {"we", write}, {"reset", reset}})
      values[design->getScalarTerm(NLName(name))->getNet()] = value;
    for (unsigned bit = 0; bit < 8; ++bit)
      values[design->getBusTerm(NLName("d"))->getBit(bit)->getNet()] = (data >> bit) & 1;
    std::unordered_set<SNLBitNet*> visiting;
    const auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
    for (const auto& flop : flops)
      state[flop.output] = evaluateRTL(flop.data, values, visiting, &memories);
    memories = nextMemories;
    if (enable) {
      registered = words[readAddress];
      if (write) words[writeAddress] = data;
    }
    if (reset) writeAddress = readAddress = 0;
    else if (enable) {
      if (write) writeAddress = (writeAddress + 1) % 4;
      readAddress = (readAddress + 1) % 4;
    }
    values = state;
    unsigned actual[2]{};
    for (unsigned output = 0; output < 2; ++output)
      for (unsigned bit = 0; bit < 8; ++bit)
        if (evaluateRTL(design->getBusTerm(NLName(output ? "async_q" : "q"))->getBit(bit)->getNet(),
                        values, visiting, &memories)) actual[output] |= 1u << bit;
    EXPECT_EQ(actual[0], registered) << cycle;
    EXPECT_EQ(actual[1], words[readAddress]) << cycle;
    if (reference.is_open() && cycle >= 6) {
      unsigned q, asynchronous;
      ASSERT_TRUE(reference >> q >> asynchronous);
      EXPECT_EQ(actual[0], q) << cycle;
      EXPECT_EQ(actual[1], asynchronous) << cycle;
    }
  }
  if (reference.is_open()) { std::string trailing; EXPECT_FALSE(reference >> trailing); }
  if (const auto* directory = std::getenv("VHDL_MEMORY_DUMP")) {
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.setTopFileName("memory_test.v");
    dumper.dumpDesign(design, directory);
  }
}

TEST_F(VHDLConstructorTest, InferredMemoryBoundsWordOrderAndReadPorts) {
  for (const auto& [bounds, low, high, width] :
       std::vector<std::tuple<std::string, unsigned, unsigned, unsigned>>{
           {"0 to 3", 0, 3, 4}, {"3 downto 1", 1, 3, 4},
           {"2 to 4", 2, 4, 4}, {"7 downto 7", 7, 7, 1}}) {
    SCOPED_TRACE(bounds);
    const auto wordRange = "4 to " + std::to_string(3 + width);
    auto* design = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all; "
        "use ieee.std_logic_unsigned.all; "
        "entity bounded_ram is port(clk, ce, we : in std_logic; "
        "wa, ra : in std_logic_vector(3 downto 0); d : in std_logic_vector(" + wordRange + "); "
        "q, direct, first_word : out std_logic_vector(" + wordRange + ")); end; "
        "architecture rtl of bounded_ram is type words is array(" + bounds + ") of std_logic_vector(" +
        wordRange + "); signal ram, copied : words; begin "
        "copied <= ram; direct <= ram(conv_integer(ra)); first_word <= copied(" + std::to_string(low) + "); "
        "process(clk) variable address : integer range 0 to 15; begin if rising_edge(clk) then "
        "address := conv_integer(wa); if ce = '1' then if we = '1' then ram(address) <= d; end if; "
        "q <= ram(conv_integer(ra)); end if; end if; end process; end;");
    auto memories = naja::NL::test::zeroMemoryState(design);
    ASSERT_EQ(memories.size(), 1u);
    const auto signature = NLDB0::getMemorySignature(memories.begin()->first);
    EXPECT_EQ(signature.width, width);
    EXPECT_EQ(signature.depth, high - low + 1);
    EXPECT_GE(signature.readPorts, 2u);
    const auto flops = dffBits(design);
    ASSERT_EQ(flops.size(), width);
    std::unordered_map<SNLBitNet*, bool> state;
    for (const auto& flop : flops) state[flop.output] = false;
    std::vector<unsigned> words(high - low + 1);
    unsigned registered = 0;
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
      // Includes indices outside the array and with nonzero truncated high bits.
      const auto wa = cycle % 16, ra = cycle % 3 ? wa : (cycle + 5) % 16;
      const unsigned data = (cycle * 3 + 1) & ((1u << width) - 1);
      const bool ce = cycle % 5 != 0, we = cycle % 7 != 0;
      auto values = state;
      values[design->getScalarTerm(NLName("ce"))->getNet()] = ce;
      values[design->getScalarTerm(NLName("we"))->getNet()] = we;
      for (const auto& [name, value] : std::vector<std::pair<std::string, unsigned>>{
          {"wa", wa}, {"ra", ra}, {"d", data}}) {
        auto* term = design->getBusTerm(NLName(name));
        for (unsigned i = 0; i < term->getWidth(); ++i)
          values[term->getBitAtPosition(i)->getNet()] = (value >> (term->getWidth()-1-i)) & 1;
      }
      const unsigned read = ra >= low && ra <= high ? words[ra-low] : 0;
      std::unordered_set<SNLBitNet*> visiting;
      for (unsigned i = 0; i < width; ++i) {
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("direct"))->getBitAtPosition(i)->getNet(),
                             values, visiting, &memories), bool((read >> (width-1-i)) & 1));
        EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("first_word"))->getBitAtPosition(i)->getNet(),
                             values, visiting, &memories), bool((words.front() >> (width-1-i)) & 1));
      }
      const auto nextMemories = naja::NL::test::nextMemoryState(memories, values);
      for (const auto& flop : flops)
        state[flop.output] = evaluateRTL(flop.data, values, visiting, &memories);
      memories = nextMemories;
      if (ce) {
        registered = read;
        if (we && wa >= low && wa <= high) words[wa-low] = data;
      }
      for (unsigned i = 0; i < width; ++i)
        EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBitAtPosition(i)->getNet()),
                  bool((registered >> (width-1-i)) & 1)) << cycle;
      EXPECT_EQ(memories.begin()->second, std::vector<uint64_t>(words.begin(), words.end()));
    }
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, MemoryInferenceFallsBackForMultiplePartialAndLoopWrites) {
  for (const auto& [writes, initializer] : std::vector<std::pair<std::string, std::string>>{
      {"ram(conv_integer(a)) <= d; ram(conv_integer(b)) <= d;", ""},
      {"ram(conv_integer(a))(3 downto 2) <= d(3 downto 2); ram(conv_integer(a))(1 downto 0) <= d(1 downto 0);", ""},
      {"for i in 0 to 1 loop ram(conv_integer(a)) <= d; end loop;", ""},
      {"ram(conv_integer(a)) <= d;", " := (others => (others => '0'))"}}) {
    SCOPED_TRACE(writes);
    auto* design = VHDLConstructor(library_).construct(
        "library ieee; use ieee.std_logic_1164.all; use ieee.std_logic_arith.all; "
        "use ieee.std_logic_unsigned.all; entity fallback is port(clk : in std_logic; "
        "a, b : in std_logic_vector(1 downto 0); d : in std_logic_vector(3 downto 0); "
        "q : out std_logic_vector(3 downto 0)); end; architecture rtl of fallback is "
        "type words is array(0 to 3) of std_logic_vector(3 downto 0); signal ram : words" + initializer + "; "
        "begin q <= ram(conv_integer(a)); process(clk) begin if rising_edge(clk) then " +
        std::string(writes) + " end if; end process; end;");
    EXPECT_TRUE(naja::NL::test::zeroMemoryState(design).empty());
    EXPECT_EQ(dffBits(design).size(), 16u);
    design->destroy();
  }
}

TEST_F(VHDLConstructorTest, RecordsPreserveFieldOrderAndScheduledWrites) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
package bus_types is
  type request_t is record
    valid : std_ulogic;
    data : std_ulogic_vector(2 to 4);
  end record request_t;
  type envelope_t is record
    request : request_t;
    tag : bit;
  end record;
  constant idle : request_t := (data => (others => '0'), valid => '0');
end;
library ieee; use ieee.std_logic_1164.all; use work.bus_types.all;
entity record_test is port(clk, valid : in std_ulogic;
  data : in std_ulogic_vector(7 downto 5); y : out std_ulogic_vector(3 downto 0)); end;
architecture rtl of record_test is
  type entries_t is array (1 downto 0) of envelope_t;
  signal entries : entries_t;
  signal state : request_t;
begin
  entries(1) <= (request => (valid, data), tag => '1');
  entries(0) <= (request => idle, tag => '0');
  process(clk) begin
    if rising_edge(clk) then
      state <= entries(1).request;
      state.data(3) <= entries(0).request.valid;
    end if;
  end process;
  y <= state.valid & state.data;
end;
)");
  ASSERT_NE(design, nullptr);
  const auto flops = dffBits(design);
  ASSERT_EQ(flops.size(), 4u);
  auto* state = design->getBusNet(NLName("state"));
  ASSERT_NE(state, nullptr);
  for (unsigned pattern = 0; pattern < 16; ++pattern) {
    std::unordered_map<SNLBitNet*, bool> values;
    std::unordered_set<SNLBitNet*> visiting;
    values[design->getScalarTerm(NLName("valid"))->getNet()] = pattern & 8;
    for (unsigned i = 0; i < 3; ++i)
      values[design->getBusTerm(NLName("data"))->getBitAtPosition(i)->getNet()] = pattern & (4 >> i);
    for (const auto& flop : flops) {
      unsigned position = 0;
      while (state->getBitAtPosition(position) != flop.output) ++position;
      const bool expected = position == 2 ? false : bool(pattern & (8 >> position));
      EXPECT_EQ(evaluateRTL(flop.data, values, visiting), expected);
      values[flop.output] = expected;
    }
    for (unsigned i = 0; i < 4; ++i)
      EXPECT_EQ(evaluateRTL(design->getBusTerm(NLName("y"))->getBitAtPosition(i)->getNet(), values, visiting),
          i == 2 ? false : bool(pattern & (8 >> i)));
  }
}

TEST_F(VHDLConstructorTest, RecordPortsBindAcrossHierarchy) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type pair_t is record a, b : bit; end record; end;
use work.types.all;
entity leaf is port(d : in pair_t; q : out pair_t); end;
architecture rtl of leaf is begin q <= (b => d.a, a => d.b); end;
use work.types.all;
entity parent is port(d : in pair_t; q : out pair_t); end;
architecture rtl of parent is begin u: entity work.leaf port map(d, q); end;
)", "parent");
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(design->getBusTerm(NLName("d"))->getWidth(), 2u);
  auto* child = design->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  auto* model = child->getModel();
  for (unsigned i = 0; i < 2; ++i) {
    EXPECT_EQ(child->getInstTerm(model->getBusTerm(NLName("d"))->getBitAtPosition(i))->getNet(),
        design->getBusTerm(NLName("d"))->getBitAtPosition(i)->getNet());
  }
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  values[model->getBusTerm(NLName("d"))->getBitAtPosition(0)->getNet()] = false;
  values[model->getBusTerm(NLName("d"))->getBitAtPosition(1)->getNet()] = true;
  EXPECT_TRUE(evaluateRTL(model->getBusTerm(NLName("q"))->getBitAtPosition(0)->getNet(), values, visiting));
  EXPECT_FALSE(evaluateRTL(model->getBusTerm(NLName("q"))->getBitAtPosition(1)->getNet(), values, visiting));
}

TEST_F(VHDLConstructorTest, RejectsMalformedOrIncompatibleRecords) {
  for (const auto* body : {
      "r <= (a => '0', a => '1');",
      "r <= (a => '0');",
      "r <= (missing => '0', b => '1');",
      "r <= (a => '0', '1');",
      "r.missing <= '0';",
      "r <= not d;",
      "r <= d and d;",
      "r <= other;",
      "r.a <= d.b(0);",
      "r <= ('0', '1', '0');"}) {
    SCOPED_TRACE(body);
    const auto source = std::string(R"(
package types is
  type pair_t is record a, b : bit; end record;
  type other_t is record a, b : bit; end record;
end;
use work.types.all;
entity invalid is port(d : in pair_t; other : in other_t; r : out pair_t); end;
architecture rtl of invalid is begin
)") + body + " end;";
    EXPECT_THROW(VHDLConstructor(library_).construct(source), NLException);
  }
  for (const auto* declaration : {
      "type pair_t is record a, a : bit; end record;",
      "type pair_t is record end record;",
      "type pair_t is record a : bit; end record wrong;",
      "type pair_t is record a : pair_t; end record;"}) {
    SCOPED_TRACE(declaration);
    EXPECT_THROW(VHDLConstructor(library_).construct(std::string("entity invalid is end; architecture rtl of invalid is ") +
        declaration + " begin end;"), NLException);
  }
}

TEST_F(VHDLConstructorTest, RecordDefaultsAndFieldPortActuals) {
  auto* design = VHDLConstructor(library_).construct(R"(
package types is type pair_t is record a, b : bit; end record; end;
entity leaf is port(d : in bit; q : out bit); end;
architecture rtl of leaf is begin q <= d; end;
use work.types.all;
entity parent is port(d : in pair_t; q : out pair_t); end;
architecture rtl of parent is
  constant zero : pair_t := (others => '0');
begin
  u: entity work.leaf port map(d.a, q.b);
  q.a <= zero.b;
end;
)", "parent");
  auto* child = design->getInstance(NLName("u"));
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->getInstTerm(child->getModel()->getScalarTerm(NLName("d")))->getNet(),
      design->getBusTerm(NLName("d"))->getBitAtPosition(0)->getNet());
  EXPECT_EQ(child->getInstTerm(child->getModel()->getScalarTerm(NLName("q")))->getNet(),
      design->getBusTerm(NLName("q"))->getBitAtPosition(1)->getNet());
  EXPECT_TRUE(design->getBusTerm(NLName("q"))->getBitAtPosition(0)->getNet()->isConstant0());
}

TEST_F(VHDLConstructorTest, UnresolvedLogicRequiresVisibilityAndSingleDriver) {
  auto* design = VHDLConstructor(library_).construct(R"(
library ieee; use ieee.std_logic_1164.all;
entity unresolved is port(a : in std_ulogic; y : out std_ulogic); end;
architecture rtl of unresolved is begin y <= not a; end;
)");
  std::unordered_map<SNLBitNet*, bool> values;
  std::unordered_set<SNLBitNet*> visiting;
  values[design->getScalarTerm(NLName("a"))->getNet()] = false;
  EXPECT_TRUE(evaluateRTL(design->getScalarTerm(NLName("y"))->getNet(), values, visiting));
  EXPECT_THROW(VHDLConstructor(library_).construct(
      "entity invisible is port(a : in std_ulogic; y : out std_ulogic); end; "
      "architecture rtl of invisible is begin y <= a; end;"), NLException);
  EXPECT_THROW(VHDLConstructor(library_).construct(
      "library ieee; use ieee.std_logic_1164.all; "
      "entity multiple is port(a : in std_ulogic; y : out std_ulogic); end; "
      "architecture rtl of multiple is begin y <= a; y <= '0'; end;"), NLException);
}
