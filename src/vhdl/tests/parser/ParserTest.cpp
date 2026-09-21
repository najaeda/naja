// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Parser.h"
#include <gtest/gtest.h>

TEST(VHDLParserTest, EntityArchitectureAndExpressionTree) {
    const auto result = vhdl::Parser::parse(R"(
entity top is
  port (
    clk : in std_logic;
    a, b : in bit;
    data : in std_logic_vector(7 downto 0);
    y : out bit
  );
end entity top;

architecture rtl of top is
begin
  y <= a + b * 2;
end architecture rtl;
)");
    EXPECT_FALSE(result.hasErrors());
    ASSERT_EQ(result.syntax.entities.size(), 1);
    const auto& entity = result.syntax.entities[0];
    EXPECT_EQ(entity.name.canonical, "top");
    ASSERT_EQ(entity.ports.size(), 4);
    EXPECT_EQ(entity.ports[1].names.size(), 2);
    EXPECT_EQ(entity.ports[1].mode, vhdl::PortMode::In);
    const auto& vectorType = entity.ports[2].type;
    EXPECT_EQ(vectorType.name.canonical, "std_logic_vector");
    ASSERT_TRUE(vectorType.constraint.has_value());
    EXPECT_EQ(vectorType.constraint->left, 7);
    EXPECT_EQ(vectorType.constraint->right, 0);
    EXPECT_FALSE(vectorType.constraint->ascending);

    ASSERT_EQ(result.syntax.architectures.size(), 1);
    const auto& architecture = result.syntax.architectures[0];
    EXPECT_EQ(architecture.name.canonical, "rtl");
    EXPECT_EQ(architecture.entity.canonical, "top");
    ASSERT_EQ(architecture.assignments.size(), 1);
    const auto& expression = *architecture.assignments[0].value;
    ASSERT_EQ(expression.kind, vhdl::Expression::Kind::Binary);
    EXPECT_EQ(expression.text, "+");
    ASSERT_TRUE(expression.right);
    EXPECT_EQ(expression.right->kind, vhdl::Expression::Kind::Binary);
    EXPECT_EQ(expression.right->text, "*");
}

TEST(VHDLParserTest, UnsupportedConstructIsDiagnosed) {
    const auto result = vhdl::Parser::parse(R"(
entity top is end entity top;
architecture rtl of top is
  signal internal : bit;
begin
  internal <= '1';
end architecture rtl;
)");
    EXPECT_TRUE(result.hasErrors());
    EXPECT_FALSE(result.syntax.architectures.empty());
}

TEST(VHDLParserTest, ConditionalSignalAssignment) {
    const auto result = vhdl::Parser::parse(R"(
entity mux is
  port (a, b, sel : in bit; y : out bit);
end entity mux;
architecture rtl of mux is
begin
  y <= a when sel = '1' else b;
end architecture rtl;
)");
    ASSERT_FALSE(result.hasErrors());
    ASSERT_EQ(result.syntax.architectures.size(), 1);
    const auto& assignment = result.syntax.architectures[0].assignments[0];
    ASSERT_EQ(assignment.value->kind, vhdl::Expression::Kind::Conditional);
    ASSERT_EQ(assignment.value->left->text, "a");
    ASSERT_EQ(assignment.value->right->text, "b");
    ASSERT_EQ(assignment.value->condition->kind, vhdl::Expression::Kind::Binary);
    EXPECT_EQ(assignment.value->condition->text, "=");
}

TEST(VHDLParserTest, MalformedPortProgresses) {
    const auto result = vhdl::Parser::parse("entity top is port (a in bit); end entity top;");
    EXPECT_TRUE(result.hasErrors());
    EXPECT_FALSE(result.diagnostics.empty());
}

TEST(VHDLParserTest, PreservesClockedProcessNamesAndLocations) {
    const auto parsed = vhdl::Parser::parse(R"(
entity reg is port(clk, d : in bit; q : out bit); end;
architecture rtl of reg is begin
process(CLK) begin if clk'event and clk = '1' then q <= d; end if; end process;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& processes = parsed.syntax.architectures.front().processes;
    ASSERT_EQ(processes.size(), 1);
    EXPECT_EQ(processes.front().sensitivity.canonical, "clk");
    EXPECT_EQ(processes.front().eventSignal.canonical, "clk");
    EXPECT_EQ(processes.front().assignment.target.canonical, "q");
    EXPECT_LT(processes.front().span.start.offset, processes.front().span.end.offset);
}
