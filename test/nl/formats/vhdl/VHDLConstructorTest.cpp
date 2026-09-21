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
entity mux_logic is port (a : in std_logic); end entity mux_logic;
architecture rtl of mux_logic is begin end architecture rtl;
)";
  VHDLConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(source), NLException);
  EXPECT_EQ(library_->getSNLDesign(NLName("mux_logic")), nullptr);
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

TEST_F(VHDLConstructorTest, UnsupportedClockedProcessesPublishNoDesign) {
  for (const auto* body : {
      "process(clk) begin if clk'event and clk = '0' then q <= d; end if; end process;",
      "process(d) begin if clk'event and clk = '1' then q <= d; end if; end process;",
      "process(clk) begin if clk'event and d = '1' then q <= d; end if; end process;",
      "process(clk) begin if missing'event and clk = '1' then q <= d; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= missing; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then d <= q; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d after 1 ns; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d; else q <= '0'; end if; end process;",
      "process(clk) begin if clk'event and clk = '1' then q <= d; q <= clk; end if; end process;",
      "process(clk) begin if rising_edge(clk) then q <= d; end if; end process;",
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
