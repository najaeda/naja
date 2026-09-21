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
