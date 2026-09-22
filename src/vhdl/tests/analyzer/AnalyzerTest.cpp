// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Analyzer.h"
#include <gtest/gtest.h>

namespace {

vhdl::AnalysisResult analyze(std::string_view source) {
    const auto parsed = vhdl::Parser::parse(source);
    EXPECT_FALSE(parsed.hasErrors());
    return vhdl::Analyzer::analyze(parsed.syntax);
}


TEST(VHDLAnalyzerTest, ResolvesEntityArchitectureAndConcurrentAssignments) {
    const auto result = analyze(R"(
entity top is
  port (a, b : in bit; y : out bit);
end entity top;
architecture rtl of TOP is
begin
  y <= A and b;
end architecture rtl;
)");
    EXPECT_FALSE(result.hasErrors());
}

TEST(VHDLAnalyzerTest, ReportsMissingEntityAndNames) {
    const auto result = analyze(R"(
entity top is port (a : in bit; y : out bit); end entity top;
architecture rtl of missing is begin y <= a; end architecture rtl;
architecture other of top is begin y <= unknown; end architecture other;
architecture third of top is begin missing <= a; end architecture third;
)");
    ASSERT_EQ(result.diagnostics.size(), 3);
    EXPECT_NE(result.diagnostics[0].message.find("no entity declaration"), std::string::npos);
    EXPECT_NE(result.diagnostics[1].message.find("no declaration for name"), std::string::npos);
    EXPECT_NE(result.diagnostics[2].message.find("no declaration for assignment target"),
              std::string::npos);
}

TEST(VHDLAnalyzerTest, RejectsDuplicatePortsAndArchitectures) {
    const auto result = analyze(R"(
entity top is port (a : in bit; A : out bit); end entity top;
architecture rtl of top is begin a <= '1'; end architecture rtl;
architecture RTL of top is begin a <= '0'; end architecture RTL;
)");
    ASSERT_EQ(result.diagnostics.size(), 2);
    EXPECT_NE(result.diagnostics[0].message.find("duplicate port"), std::string::npos);
    EXPECT_NE(result.diagnostics[1].message.find("duplicate architecture"), std::string::npos);
}

TEST(VHDLAnalyzerTest, ExtendedIdentifiersPreserveCase) {
    const auto result = analyze(R"(
entity top is port (\Data\ : in bit; y : out bit); end entity top;
architecture rtl of top is begin y <= \data\; end architecture rtl;
)");
    ASSERT_EQ(result.diagnostics.size(), 1);
    EXPECT_NE(result.diagnostics[0].message.find("no declaration for name"), std::string::npos);
}

TEST(VHDLAnalyzerTest, ResolvesNamesInConditionalAssignments) {
    const auto result = analyze(R"(
entity mux is port (a, b, sel : in bit; y : out bit); end entity mux;
architecture rtl of mux is begin
  y <= a when missing = '1' else b;
end architecture rtl;
)");
    ASSERT_EQ(result.diagnostics.size(), 1);
    EXPECT_NE(result.diagnostics[0].message.find("no declaration for name"), std::string::npos);
    EXPECT_NE(result.diagnostics[0].message.find("missing"), std::string::npos);
}

TEST(VHDLAnalyzerTest, TypeChecksScalarBitLogicalExpressions) {
    const auto parsed = vhdl::Parser::parse(R"(
entity logic is port (a, b, c : in bit; y : out bit); end;
architecture rtl of logic is begin y <= (a and b) xor not c; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto result = vhdl::Analyzer::analyze(parsed.syntax);
    ASSERT_FALSE(result.hasErrors());
    const auto& expression = *parsed.syntax.architectures[0].assignments[0].value;
    EXPECT_EQ(result.getType(expression), vhdl::ScalarType::Bit);
    EXPECT_EQ(result.getType(*expression.left), vhdl::ScalarType::Bit);
    EXPECT_EQ(result.getType(*expression.right), vhdl::ScalarType::Bit);
}

TEST(VHDLAnalyzerTest, ResolvesImportedNineValuedLogicWithoutBinaryCollapse) {
    const auto parsed = vhdl::Parser::parse(R"(
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
entity logic is port(a, b : in std_logic; y : out std_logic); end;
architecture rtl of logic is begin y <= (a xor b) and 'X'; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto result = vhdl::Analyzer::analyze(parsed.syntax);
    ASSERT_FALSE(result.hasErrors());
    const auto& expression = *parsed.syntax.architectures[0].assignments[0].value;
    EXPECT_EQ(result.getType(expression), vhdl::ScalarType::StdLogic);
    EXPECT_EQ(result.getType(*expression.right), vhdl::ScalarType::StdLogic);

    for (const auto* literal : {
        "'U'", "'X'", "'0'", "'1'", "'Z'", "'W'", "'L'", "'H'", "'-'"}) {
        SCOPED_TRACE(literal);
        const std::string source = std::string(
            "library ieee; use ieee.std_logic_1164.all; "
            "entity p is port(y : out std_logic); end; "
            "architecture rtl of p is begin y <= ") + literal + "; end;";
        EXPECT_FALSE(analyze(source).hasErrors());
    }
    EXPECT_TRUE(analyze(
        "library ieee; use ieee.std_logic_1164.all; "
        "entity p is port(y : out std_logic); end; "
        "architecture rtl of p is begin y <= 'x'; end;").hasErrors());

    const auto vectors = vhdl::Parser::parse(R"(
library ieee; use ieee.std_logic_1164.all;
entity vectors is port(a, b : in std_logic_vector(3 downto 0);
                       y : out std_logic_vector(0 to 3)); end;
architecture rtl of vectors is begin y <= a xor b; end;
)");
    ASSERT_FALSE(vectors.hasErrors());
    const auto vectorResult = vhdl::Analyzer::analyze(vectors.syntax);
    ASSERT_FALSE(vectorResult.hasErrors());
    EXPECT_EQ(vectorResult.getType(
                  *vectors.syntax.architectures[0].assignments[0].value),
              vhdl::ScalarType::StdLogicVector);
}

TEST(VHDLAnalyzerTest, DiagnosesInvisibleAndUnsupportedPackageTypes) {
    for (const auto* source : {
        "entity p is port(a : in std_logic; y : out std_logic); end; "
        "architecture rtl of p is begin y <= a; end;",
        "use ieee.std_logic_1164.all; entity p is end;",
        "library ieee; use ieee.unknown.all; entity p is end;",
        "library ieee; use ieee.std_logic_1164.std_logic; entity p is end;",
        "library ieee; use ieee.std_logic_1164.all; entity p is end; "
        "entity q is port(a : in std_logic); end;"}) {
        SCOPED_TRACE(source);
        const auto parsed = vhdl::Parser::parse(source);
        ASSERT_FALSE(parsed.hasErrors());
        EXPECT_TRUE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
    }
}

TEST(VHDLAnalyzerTest, ResolvesNumericStdVectorsAndAdditionWidth) {
    const auto parsed = vhdl::Parser::parse(R"(
library ieee; use ieee.numeric_std.all;
entity adders is port (
  ua : in unsigned(0 to 2); ub : in unsigned(7 downto 4);
  sy : out signed(1 downto 0); sa, sb : in signed(1 downto 0));
end;
library ieee; use ieee.numeric_std.all;
architecture rtl of adders is
  signal uy : unsigned(3 downto 0);
begin
  uy <= ua + ub;
  sy <= sa - sb;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto result = vhdl::Analyzer::analyze(parsed.syntax);
    ASSERT_FALSE(result.hasErrors());

    const auto& addition = *parsed.syntax.architectures[0].assignments[0].value;
    EXPECT_EQ(result.getType(addition), vhdl::ScalarType::Unsigned);
    ASSERT_NE(result.getRange(addition), nullptr);
    EXPECT_EQ(result.getRange(addition)->left, 3);
    EXPECT_EQ(result.getRange(addition)->right, 0);

    const auto& subtraction = *parsed.syntax.architectures[0].assignments[1].value;
    EXPECT_EQ(result.getType(subtraction), vhdl::ScalarType::Signed);
    ASSERT_NE(result.getRange(subtraction), nullptr);
    EXPECT_EQ(result.getRange(subtraction)->left, 1);
    EXPECT_EQ(result.getRange(subtraction)->right, 0);
}

TEST(VHDLAnalyzerTest, DiagnosesNumericStdVisibilityAndTypeMismatches) {
    EXPECT_TRUE(analyze(R"(
library ieee; use ieee.numeric_std.all;
entity p is port(a, b : in unsigned(3 downto 0); y : out unsigned(3 downto 0)); end;
architecture rtl of p is begin y <= a + b; end;
)").hasErrors());

    EXPECT_TRUE(analyze(R"(
library ieee; use ieee.numeric_std.all;
entity p is port(a : in unsigned(3 downto 0); b : in signed(3 downto 0);
                 y : out unsigned(3 downto 0)); end;
library ieee; use ieee.numeric_std.all;
architecture rtl of p is begin y <= a + b; end;
)").hasErrors());

    EXPECT_TRUE(analyze(R"(
entity p is port(a : in unsigned(3 downto 0)); end;
)").hasErrors());

    EXPECT_TRUE(analyze(R"(
library ieee; use ieee.numeric_std.all;
entity p is port(a, b : in unsigned(3 to 0); y : out unsigned(3 to 0)); end;
library ieee; use ieee.numeric_std.all;
architecture rtl of p is begin y <= a + b; end;
)").hasErrors());
}

TEST(VHDLAnalyzerTest, RejectsScalarExpressionTypeMismatches) {
    for (const auto* expression : {"a and flag", "a = b", "'Z'", "'1' = '0'"}) {
        const auto parsed = vhdl::Parser::parse(std::string(
            "entity logic is port(a, b : in bit; flag : in boolean; y : out bit); end; "
            "architecture rtl of logic is begin y <= ") + expression + "; end;");
        ASSERT_FALSE(parsed.hasErrors());
        EXPECT_TRUE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
    }
}

TEST(VHDLAnalyzerTest, TypeChecksConstrainedBitVectorsByLength) {
    const auto parsed = vhdl::Parser::parse(R"(
entity vectors is port (
  a : in bit_vector(0 to 3);
  b : in bit_vector(7 downto 4);
  y : out bit_vector(3 downto 0));
end;
architecture rtl of vectors is begin y <= a xor not b; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto result = vhdl::Analyzer::analyze(parsed.syntax);
    ASSERT_FALSE(result.hasErrors());
    const auto& expression = *parsed.syntax.architectures[0].assignments[0].value;
    EXPECT_EQ(result.getType(expression), vhdl::ScalarType::BitVector);
    ASSERT_NE(result.getRange(expression), nullptr);
    EXPECT_EQ(result.getRange(expression)->left, 0);
    EXPECT_EQ(result.getRange(expression)->right, 3);

    EXPECT_TRUE(analyze(R"(
entity vectors is port (
  a : in bit_vector(0 to 3);
  b : in bit_vector(2 downto 0);
  y : out bit_vector(3 downto 0));
end;
architecture rtl of vectors is begin y <= a and b; end;
)").hasErrors());
}

} // namespace

TEST(VHDLAnalyzerTest, ResolvesAllClockedProcessNames) {
    const auto result = analyze(R"(
entity reg is port(clk, d : in bit; q : out bit); end;
architecture rtl of reg is begin
process(missing_sensitivity) begin
if missing_event'event and missing_level = '1' then missing_target <= missing_data;
end if; end process;
end;
)");
    EXPECT_EQ(result.diagnostics.size(), 5);
}

TEST(VHDLAnalyzerTest, ResolvesAndTypeChecksClockEnable) {
    EXPECT_FALSE(analyze(R"(
entity reg is port(clk, en, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if rising_edge(clk) then if en = '1' then q <= d; end if; end if;
end process; end;
)").hasErrors());
    for (const auto* enable : {"missing", "count"}) {
        SCOPED_TRACE(enable);
        const std::string source = std::string(
            "entity reg is port(clk, d : in bit; count : in integer; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin "
            "if rising_edge(clk) then if ") + enable +
            " = '1' then q <= d; end if; end if; end process; end;";
        EXPECT_TRUE(analyze(source).hasErrors());
    }
}

TEST(VHDLAnalyzerTest, ResolvesAndTypeChecksSynchronousReset) {
    EXPECT_FALSE(analyze(R"(
entity reg is port(clk, rst, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if rising_edge(clk) then
  if rst = '1' then q <= '0'; else q <= d; end if;
end if; end process; end;
)").hasErrors());
    for (const auto* reset : {"missing", "count"}) {
        SCOPED_TRACE(reset);
        const std::string source = std::string(
            "entity reg is port(clk, d : in bit; count : in integer; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin "
            "if rising_edge(clk) then if ") + reset +
            " = '1' then q <= '0'; else q <= d; end if; end if; end process; end;";
        EXPECT_TRUE(analyze(source).hasErrors());
    }
}

TEST(VHDLAnalyzerTest, ResolvesAndTypeChecksResetWithEnable) {
    EXPECT_FALSE(analyze(R"(
entity reg is port(clk, rst, en, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if rising_edge(clk) then
  if rst = '1' then q <= '0'; elsif en = '1' then q <= d; end if;
end if; end process; end;
)").hasErrors());
    for (const auto* enable : {"missing", "count"}) {
        SCOPED_TRACE(enable);
        const std::string source = std::string(
            "entity reg is port(clk, rst, d : in bit; count : in integer; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin "
            "if rising_edge(clk) then if rst = '1' then q <= '0'; elsif ") + enable +
            " = '1' then q <= d; end if; end if; end process; end;";
        EXPECT_TRUE(analyze(source).hasErrors());
    }
}

TEST(VHDLAnalyzerTest, InternalSignalsResolveAcrossAllScheduledWrites) {
    EXPECT_FALSE(analyze(R"(
entity p is port(clk, d : in bit; q : out bit); end;
architecture rtl of p is signal Stage : bit;
begin process(clk) begin if clk'event and clk = '1' then
q <= STAGE; stage <= d; end if; end process; end;
)").hasErrors());
    const auto result = analyze(R"(
entity p is port(clk, d : in bit; q : out bit); end;
architecture rtl of p is signal stage, STAGE : bit; signal D : bit;
begin process(clk) begin if clk'event and clk = '1' then
stage <= missing; unknown <= stage; end if; end process; end;
)");
    ASSERT_EQ(result.diagnostics.size(), 4);
    EXPECT_NE(result.diagnostics[0].message.find("duplicate signal"), std::string::npos);
    EXPECT_NE(result.diagnostics[1].message.find("duplicate signal"), std::string::npos);
    EXPECT_NE(result.diagnostics[2].message.find("missing"), std::string::npos);
    EXPECT_NE(result.diagnostics[3].message.find("unknown"), std::string::npos);
}

namespace {
vhdl::ParseResult variableProcess(const std::string& declarations, const std::string& body) {
    return vhdl::Parser::parse("entity p is port(clk, d, e : in bit; q, r : out bit); end; "
        "architecture rtl of p is signal stage : bit; begin process(clk) " + declarations +
        " begin if clk'event and clk = '1' then " + body + " end if; end process; end;");
}
}

TEST(VHDLAnalyzerTest, SchedulesImmediateVariablesAndCurrentSignalsSeparately) {
    const auto parsed = variableProcess("variable v, copy : bit;",
        "stage <= d; v := stage; copy := v; q <= copy; v := e; r <= v; copy := d;");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_FALSE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
    const auto scheduled = vhdl::Analyzer::schedule(parsed.syntax.architectures[0].processes[0]);
    ASSERT_FALSE(scheduled.hasErrors());
    ASSERT_EQ(scheduled.writes.size(), 3);
    EXPECT_EQ(scheduled.writes[0].target, "stage");
    EXPECT_EQ(scheduled.writes[0].source, "d");
    EXPECT_EQ(scheduled.writes[1].target, "q");
    EXPECT_EQ(scheduled.writes[1].source, "stage");
    EXPECT_EQ(scheduled.writes[2].target, "r");
    EXPECT_EQ(scheduled.writes[2].source, "e");
    EXPECT_LT(scheduled.writes[0].span.end.offset, scheduled.writes[1].span.start.offset);
}

TEST(VHDLAnalyzerTest, RejectsVariableObjectClassAndScopeErrors) {
    for (const auto& [decl, body] : std::vector<std::pair<std::string, std::string>>{
        {"variable v : bit;", "v <= d; q <= v;"},
        {"variable v : bit;", "stage := d; q <= stage;"},
        {"variable v : bit;", "missing := d; q <= v;"},
        {"variable v, V : bit;", "v := d; q <= v;"},
        {"variable stage : bit;", "stage := d; q <= stage;"},
        {"variable clk : bit;", "clk := d; q <= clk;"},
        {"variable v : bit;", "v := missing; q <= v;"}}) {
        const auto parsed = variableProcess(decl, body);
        ASSERT_FALSE(parsed.hasErrors());
        EXPECT_TRUE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
    }
    const auto parsed = vhdl::Parser::parse(R"(
entity p is port(clk, d : in bit; q, r : out bit); end;
architecture rtl of p is begin
process(clk) variable v : bit; begin if clk'event and clk = '1' then
v := d; q <= v; end if; end process;
process(clk) begin if clk'event and clk = '1' then r <= v; end if; end process;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    EXPECT_TRUE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
}

TEST(VHDLAnalyzerTest, RejectsRetainedVariableReadsWithoutPartialSchedule) {
    for (const auto* body : {"q <= v;", "v := v; q <= v;",
                            "v := not d; q <= v;"}) {
        const auto parsed = variableProcess("variable v, copy : bit;", body);
        ASSERT_FALSE(parsed.hasErrors());
        ASSERT_FALSE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
        const auto scheduled = vhdl::Analyzer::schedule(parsed.syntax.architectures[0].processes[0]);
        EXPECT_TRUE(scheduled.hasErrors());
        EXPECT_TRUE(scheduled.writes.empty());
        EXPECT_LT(scheduled.diagnostics[0].span.start.offset, scheduled.diagnostics[0].span.end.offset);
    }
}

TEST(VHDLAnalyzerTest, SchedulesRetainedVariableStateAfterSignalReads) {
    const auto parsed = variableProcess("variable v : bit;", "q <= v; v := d;");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_FALSE(vhdl::Analyzer::analyze(parsed.syntax).hasErrors());
    const auto scheduled = vhdl::Analyzer::schedule(
        parsed.syntax.architectures[0].processes[0]);
    ASSERT_FALSE(scheduled.hasErrors());
    ASSERT_EQ(scheduled.retainedVariables, std::vector<std::string>{"v"});
    ASSERT_EQ(scheduled.writes.size(), 2);
    EXPECT_EQ(scheduled.writes[0].target, "q");
    EXPECT_EQ(scheduled.writes[0].source, "v");
    EXPECT_EQ(scheduled.writes[0].kind, vhdl::AssignmentKind::Signal);
    EXPECT_EQ(scheduled.writes[1].target, "v");
    EXPECT_EQ(scheduled.writes[1].source, "d");
    EXPECT_EQ(scheduled.writes[1].kind, vhdl::AssignmentKind::Variable);
}

TEST(VHDLAnalyzerTest, BindsDirectEntityInstancesByPositionAndWidth) {
    EXPECT_FALSE(analyze(R"(
entity leaf is port(a : in bit_vector(0 to 3); y : out bit_vector(7 downto 4)); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit_vector(3 downto 0); y : out bit_vector(0 to 3)); end;
architecture structural of top is signal mid : bit_vector(-2 to 1); begin
  u0: entity work.leaf port map(a, mid);
  u1: entity WORK.LEAF port map(mid, y);
end;
)").hasErrors());
}

TEST(VHDLAnalyzerTest, RejectsInvalidDirectEntityBindings) {
    for (const auto* instance : {
        "u: entity other.leaf port map(a, y);",
        "u: entity work.missing port map(a, y);",
        "u: entity work.leaf port map(a);",
        "u: entity work.leaf port map(a, missing);",
        "u: entity work.leaf port map(a, narrow);",
        "u: entity work.leaf port map(a, y); U: entity work.leaf port map(a, y);"}) {
        SCOPED_TRACE(instance);
        const std::string source = std::string(R"(
entity leaf is port(a : in bit_vector(3 downto 0); y : out bit_vector(3 downto 0)); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit_vector(3 downto 0); y : out bit_vector(3 downto 0)); end;
architecture structural of top is signal narrow : bit_vector(2 downto 0); begin
)") + instance + " end;";
        EXPECT_TRUE(analyze(source).hasErrors());
    }
}
