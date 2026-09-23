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
#include "SNLInstTerm.h"
#include "SNLRTLPrimitives.h"
#include "SNLScalarTerm.h"
#include "SNLSVConstructor.h"
#include "VHDLConstructor.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
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
// Evaluate the actual canonical primitive connectivity, with flop outputs
// supplied as the current state. This also catches undriven data and cycles.
bool evaluateRTL(SNLBitNet* net, std::unordered_map<SNLBitNet*, bool>& values,
                 std::unordered_set<SNLBitNet*>& visiting) {
  if (net->isConstant0()) return false;
  if (net->isConstant1()) return true;
  if (const auto found = values.find(net); found != values.end()) return found->second;
  if (!visiting.insert(net).second) throw std::runtime_error("combinational cycle");
  SNLInstance* driver = nullptr;
  for (auto* term : net->getInstTerms())
    if (term->getDirection() == SNLTerm::Direction::Output) {
      if (driver) throw std::runtime_error("multiple drivers");
      driver = term->getInstance();
    }
  if (!driver || NLDB0::isDFF(driver->getModel())) throw std::runtime_error("missing data/state");
  auto* model = driver->getModel();
  const auto read = [&](SNLBitTerm* term) {
    return evaluateRTL(driver->getInstTerm(term)->getNet(), values, visiting);
  };
  bool value;
  if (NLDB0::isMux2(model)) {
    value = read(NLDB0::getMux2Select(model))
        ? read(NLDB0::getMux2InputB(model)->getBit(0))
        : read(NLDB0::getMux2InputA(model)->getBit(0));
  } else {
    if (!NLDB0::isGate(model)) throw std::runtime_error("unexpected primitive");
    SNLTruthTable::ConstantInputs inputs;
    for (auto* term : driver->getInstTerms())
      if (term->getDirection() == SNLTerm::Direction::Input)
        inputs.emplace_back(inputs.size(), evaluateRTL(term->getNet(), values, visiting));
    const auto table = NLDB0::getPrimitiveTruthTable(model);
    const auto dependencies = SNLTruthTable::fullDependencies(inputs.size());
    const auto normalized = table.getGenericType() == SNLTruthTable::GenericType::NONE
        ? SNLTruthTable(inputs.size(), static_cast<uint64_t>(table.bits()), dependencies)
        : SNLTruthTable(inputs.size(), table.getGenericType(), dependencies);
    value = normalized.getReducedWithConstants(inputs).all1();
  }
  visiting.erase(net);
  values[net] = value;
  return value;
}

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
    std::vector<SNLInstance*> flops;
    for (auto* instance : design->getInstances())
      if (NLDB0::isDFF(instance->getModel())) flops.push_back(instance);
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
      for (auto* flop : flops) {
        EXPECT_EQ(flop->getInstTerm(NLDB0::getDFFClock())->getNet(),
                  design->getScalarTerm(NLName("clk"))->getNet());
        next[flop->getInstTerm(NLDB0::getDFFOutput())->getNet()] =
            evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting);
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
    for (auto* flop : design->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
      auto* output = flop->getInstTerm(NLDB0::getDFFOutput())->getNet();
      const bool expected = output == state->getBit(5) ? bool((pattern >> 1) & 1) : !(pattern & 1);
      EXPECT_EQ(evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting), expected);
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

TEST_F(VHDLConstructorTest, NumericStdPortIsRejectedBeforeDesignCreation) {
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
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("add_unsigned")), nullptr);
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

TEST_F(VHDLConstructorTest, PipelineUnsupportedSemanticsPublishNoDesign) {
  for (const auto& [declarations, writes] : std::vector<std::pair<std::string, std::string>>{
      {"signal stage : bit := '1';", "stage <= d; q <= stage;"},
      {"signal stage : bit := '0';", "stage <= d; q <= stage;"},
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
      {"signal stage : bit;", "stage <= d; if d = '1' then q <= stage; end if;"},
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
  std::vector<SNLInstance*> flops;
  std::unordered_map<SNLBitNet*, bool> state;
  for (auto* instance : design->getInstances()) if (NLDB0::isDFF(instance->getModel())) {
    flops.push_back(instance);
    state[instance->getInstTerm(NLDB0::getDFFOutput())->getNet()] = false;
  }
  ASSERT_EQ(flops.size(), 42u);
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
    for (auto* flop : flops)
      state[flop->getInstTerm(NLDB0::getDFFOutput())->getNet()] =
          evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting);
    for (unsigned bit = 0; bit < 8; ++bit)
      EXPECT_EQ(state.at(design->getBusTerm(NLName("q"))->getBit(bit)->getNet()), bool((ram[address] >> bit) & 1));
    auto* count = design->getBusNet(NLName("count"));
    for (unsigned bit = 0; bit < 2; ++bit)
      EXPECT_EQ(state.at(count->getBit(bit)), bool((((cycle + 1) % 4) >> bit) & 1));
    if (write) ram[address] = data;
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
