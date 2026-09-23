// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Parser.h"
#include <gtest/gtest.h>

TEST(VHDLParserTest, PreservesEntityGenericInterfacesAndDefaults) {
    const auto parsed = vhdl::Parser::parse(R"(
entity configurable is
  generic (width : positive := 32; low, high : integer; lanes : natural := 2);
  port (data : out bit_vector(width - 1 downto 0));
end entity configurable;
)");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_EQ(parsed.syntax.entities.size(), 1);
    const auto& entity = parsed.syntax.entities.front();
    ASSERT_EQ(entity.generics.size(), 3);
    ASSERT_EQ(entity.generics[0].names.size(), 1);
    EXPECT_EQ(entity.generics[0].names[0].canonical, "width");
    EXPECT_EQ(entity.generics[0].type.name.canonical, "positive");
    ASSERT_NE(entity.generics[0].defaultValue, nullptr);
    EXPECT_EQ(entity.generics[0].defaultValue->text, "32");
    ASSERT_EQ(entity.generics[1].names.size(), 2);
    EXPECT_EQ(entity.generics[1].defaultValue, nullptr);
    ASSERT_NE(entity.generics[2].defaultValue, nullptr);
    ASSERT_TRUE(entity.ports.front().type.constraint.has_value());
    const auto& range = *entity.ports.front().type.constraint;
    ASSERT_NE(range.leftExpression, nullptr);
    EXPECT_EQ(range.leftExpression->kind, vhdl::Expression::Kind::Binary);
    EXPECT_EQ(range.leftExpression->left->canonical, "width");
}

TEST(VHDLParserTest, PreservesGenericMapsBeforePortMaps) {
    const auto parsed = vhdl::Parser::parse(R"(
entity leaf is generic(n : positive); port(q : out bit_vector(n-1 downto 0)); end;
entity top is port(q : out bit_vector(4 downto 0)); end;
architecture rtl of top is begin
  u0: entity work.leaf(rtl) generic map (n => 5) port map(q);
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& instance = parsed.syntax.architectures.front().instantiations.front();
    ASSERT_EQ(instance.generics.size(), 1);
    ASSERT_TRUE(instance.generics.front().formal.has_value());
    EXPECT_EQ(instance.generics.front().formal->canonical, "n");
    EXPECT_EQ(instance.generics.front().actual->text, "5");
    ASSERT_TRUE(instance.architecture.has_value());
    EXPECT_EQ(instance.architecture->canonical, "rtl");
}

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
  signal internal : bit := '1';
begin
  internal <= '1';
end architecture rtl;
)");
    EXPECT_TRUE(result.hasErrors());

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

TEST(VHDLParserTest, PreservesPerDesignUnitLibraryAndUseContext) {
    const auto parsed = vhdl::Parser::parse(R"(
library IEEE, vendor;
use IEEE.STD_LOGIC_1164.ALL, ieee.numeric_std.all;
entity top is port(a : in std_logic; y : out std_logic); end;
library WORK;
use work.helpers.all;
architecture rtl of top is begin y <= a; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& entityContext = parsed.syntax.entities.front().context;
    ASSERT_EQ(entityContext.libraries.size(), 1);
    ASSERT_EQ(entityContext.libraries.front().names.size(), 2);
    EXPECT_EQ(entityContext.libraries.front().names[0].canonical, "ieee");
    EXPECT_EQ(entityContext.libraries.front().names[1].canonical, "vendor");
    ASSERT_EQ(entityContext.uses.size(), 2);
    ASSERT_EQ(entityContext.uses[0].selectedName.size(), 3);
    EXPECT_EQ(entityContext.uses[0].selectedName[1].canonical, "std_logic_1164");
    EXPECT_EQ(entityContext.uses[0].selectedName[2].canonical, "all");

    const auto& architectureContext = parsed.syntax.architectures.front().context;
    ASSERT_EQ(architectureContext.libraries.size(), 1);
    EXPECT_EQ(architectureContext.libraries.front().names.front().canonical, "work");
    ASSERT_EQ(architectureContext.uses.size(), 1);
    EXPECT_EQ(architectureContext.uses.front().selectedName[1].canonical, "helpers");
}

TEST(VHDLParserTest, RejectsMalformedContextClauses) {
    for (const auto* context : {
        "library ;", "library ieee use ieee.std_logic_1164.all;",
        "use ieee..all;"}) {
        SCOPED_TRACE(context);
        const std::string source = std::string(context) +
            " entity top is end; architecture rtl of top is begin end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
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
    EXPECT_EQ(processes.front().assignments.front().target.canonical, "q");
    EXPECT_LT(processes.front().span.start.offset, processes.front().span.end.offset);
}

TEST(VHDLParserTest, PreservesRisingEdgeCall) {
    const auto parsed = vhdl::Parser::parse(R"(
entity reg is port(clk, d : in bit; q : out bit); end;
architecture rtl of reg is begin
process(CLK) begin if rising_edge(clk) then q <= d; end if; end process;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& process = parsed.syntax.architectures.front().processes.front();
    EXPECT_EQ(process.edgeForm, vhdl::ClockEdgeForm::RisingEdgeCall);
    EXPECT_EQ(process.sensitivity.canonical, "clk");
    EXPECT_EQ(process.eventSignal.canonical, "clk");
    EXPECT_EQ(process.levelSignal.canonical, "clk");
    EXPECT_EQ(process.level, "'1'");
}

TEST(VHDLParserTest, RejectsMalformedRisingEdgeCalls) {
    for (const auto* condition : {
        "rising_edge()", "rising_edge(clk, d)", "rising_edge('1')",
        "rising_edge(clk) and d = '1'"}) {
        SCOPED_TRACE(condition);
        const std::string source = std::string(
            "entity reg is port(clk, d : in bit; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin if ") + condition +
            " then q <= d; end if; end process; end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
}

TEST(VHDLParserTest, PreservesNestedActiveHighClockEnable) {
    const auto parsed = vhdl::Parser::parse(R"(
entity reg is port(clk, en, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if rising_edge(clk) then if EN = '1' then q <= d; end if; end if;
end process; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& process = parsed.syntax.architectures.front().processes.front();
    ASSERT_TRUE(process.enableSignal);
    EXPECT_EQ(process.enableSignal->canonical, "en");
    EXPECT_EQ(process.enableLevel, "'1'");
    ASSERT_EQ(process.assignments.size(), 1);
    EXPECT_EQ(process.assignments.front().target.canonical, "q");

    const auto explicitGuard = vhdl::Parser::parse(R"(
entity reg is port(clk, en, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if clk'event and clk = '1' then if en = '1' then q <= d; end if; end if;
end process; end;
)");
    ASSERT_FALSE(explicitGuard.hasErrors());
    ASSERT_TRUE(explicitGuard.syntax.architectures.front().processes.front().enableSignal);
}

TEST(VHDLParserTest, RejectsUnsupportedClockEnableControlFlow) {
    for (const auto* body : {
        "if en then q <= d; end if;",
        "if en = d then q <= d; end if;",
        "if en = '1' then if d = '1' then q <= d; end if; end if;"}) {
        SCOPED_TRACE(body);
        const std::string source = std::string(
            "entity reg is port(clk, en, d : in bit; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin "
            "if rising_edge(clk) then ") + body + " end if; end process; end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
}

TEST(VHDLParserTest, PreservesActiveHighSynchronousResetBranches) {
    const auto parsed = vhdl::Parser::parse(R"(
entity reg is port(clk, rst, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if rising_edge(clk) then
  if RST = '1' then q <= '0'; else q <= d; end if;
end if; end process; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& process = parsed.syntax.architectures.front().processes.front();
    EXPECT_FALSE(process.enableSignal);
    ASSERT_TRUE(process.resetSignal);
    EXPECT_EQ(process.resetSignal->canonical, "rst");
    EXPECT_EQ(process.resetLevel, "'1'");
    ASSERT_EQ(process.resetAssignments.size(), 1);
    EXPECT_EQ(process.resetAssignments.front().target.canonical, "q");
    EXPECT_EQ(process.resetAssignments.front().value->kind,
              vhdl::Expression::Kind::CharacterLiteral);
    EXPECT_EQ(process.resetAssignments.front().value->text, "'0'");
    ASSERT_EQ(process.assignments.size(), 1);
    EXPECT_EQ(process.assignments.front().value->canonical, "d");
}

TEST(VHDLParserTest, PreservesSynchronousResetWithClockEnable) {
    const auto parsed = vhdl::Parser::parse(R"(
entity reg is port(clk, rst, en, d : in bit; q : out bit); end;
architecture rtl of reg is begin process(clk) begin
if rising_edge(clk) then
  if RST = '1' then q <= '0'; elsif EN = '1' then q <= d; end if;
end if; end process; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& process = parsed.syntax.architectures.front().processes.front();
    ASSERT_TRUE(process.resetSignal);
    EXPECT_EQ(process.resetSignal->canonical, "rst");
    EXPECT_EQ(process.resetLevel, "'1'");
    ASSERT_TRUE(process.enableSignal);
    EXPECT_EQ(process.enableSignal->canonical, "en");
    EXPECT_EQ(process.enableLevel, "'1'");
    ASSERT_EQ(process.resetAssignments.size(), 1);
    EXPECT_EQ(process.resetAssignments.front().value->text, "'0'");
    ASSERT_EQ(process.assignments.size(), 1);
    EXPECT_EQ(process.assignments.front().value->canonical, "d");
}

TEST(VHDLParserTest, InternalDeclarationsAndOrderedScheduledWrites) {
    const auto parsed = vhdl::Parser::parse(R"(
entity pipeline is port(clk, d : in bit; q : out bit); end;
architecture rtl of pipeline is
signal Stage, spare : bit;
signal other : bit;
begin process(CLK) is begin
if clk'event and Clk = '1' then
  Stage <= d;
  q <= (STAGE);
end if; end process; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& arch = parsed.syntax.architectures.front();
    ASSERT_EQ(arch.signals.size(), 2);
    ASSERT_EQ(arch.signals[0].names.size(), 2);
    EXPECT_EQ(arch.signals[0].names[0].canonical, "stage");
    EXPECT_EQ(arch.signals[0].type.name.canonical, "bit");
    EXPECT_LT(arch.signals[0].span.start.offset, arch.signals[0].span.end.offset);
    const auto& writes = arch.processes.front().assignments;
    ASSERT_EQ(writes.size(), 2);
    EXPECT_EQ(writes[0].target.canonical, "stage");
    EXPECT_EQ(writes[1].value->canonical, "stage");
    EXPECT_LT(writes[0].span.end.offset, writes[1].span.start.offset);
}

TEST(VHDLParserTest, RejectsUnsupportedScheduledSyntax) {
    for (const auto* body : {
        "stage <= d after 1 ns;", "stage <= transport d;",
        "stage <= reject 1 ns inertial d;", "stage <= d, d after 2 ns;",
        "wait;", "null;",
        "stage <= d; else stage <= d;", ""}) {
        SCOPED_TRACE(body);
        const auto source = std::string("entity p is end; architecture rtl of p is "
            "signal stage : bit; begin process(clk) begin "
            "if clk'event and clk = '1' then ") + body + " end if; end process; end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
}

TEST(VHDLParserTest, PreservesVariablesAndAssignmentKindsInSourceOrder) {
    const auto parsed = vhdl::Parser::parse(R"(
entity p is port(clk, d : in bit; q : out bit); end;
architecture rtl of p is begin
process(clk) is variable Temp, copy : bit; variable other : bit;
begin if clk'event and clk = '1' then
Temp := d; copy := (TEMP); q <= copy; Temp := other;
end if; end process; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& process = parsed.syntax.architectures.front().processes.front();
    ASSERT_EQ(process.variables.size(), 2);
    ASSERT_EQ(process.variables[0].names.size(), 2);
    EXPECT_EQ(process.variables[0].names[0].canonical, "temp");
    EXPECT_EQ(process.variables[0].type.name.canonical, "bit");
    ASSERT_EQ(process.assignments.size(), 4);
    EXPECT_EQ(process.assignments[0].kind, vhdl::AssignmentKind::Variable);
    EXPECT_EQ(process.assignments[2].kind, vhdl::AssignmentKind::Signal);
    EXPECT_LT(process.variables[0].span.end.offset, process.assignments[0].span.start.offset);
    EXPECT_LT(process.assignments[2].span.end.offset, process.assignments[3].span.start.offset);
}

TEST(VHDLParserTest, RejectsVariableInitializationAndUnsupportedDeclarations) {
    for (const auto* declaration : {"variable v : bit := '0';",
         "shared variable v : bit;", "constant v : bit := '1';"}) {
        const std::string source = std::string("entity p is end; architecture rtl of p is begin "
            "process(clk) ") + declaration +
            " begin if clk'event and clk = '1' then v := d; end if; end process; end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
    EXPECT_TRUE(vhdl::Parser::parse("entity p is end; architecture rtl of p is begin v := d; end;").hasErrors());
}

TEST(VHDLParserTest, PreservesDirectEntityInstantiationAndPositionalActuals) {
    const auto parsed = vhdl::Parser::parse(R"(
entity leaf is port(a : in bit; y : out bit); end;
architecture rtl of leaf is begin y <= not a; end;
entity top is port(a : in bit; y : out bit); end;
architecture structural of top is signal Mid : bit; begin
  U0: entity WORK.Leaf port map(a, MID);
  u1: entity work.leaf port map(mid, y);
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_EQ(parsed.syntax.architectures.size(), 2);
    const auto& instances = parsed.syntax.architectures[1].instantiations;
    ASSERT_EQ(instances.size(), 2);
    EXPECT_EQ(instances[0].label.canonical, "u0");
    EXPECT_EQ(instances[0].library.canonical, "work");
    EXPECT_EQ(instances[0].entity.canonical, "leaf");
    ASSERT_EQ(instances[0].actuals.size(), 2);
    EXPECT_EQ(instances[0].actuals[1].canonical, "mid");
    EXPECT_LT(instances[0].span.start.offset, instances[0].span.end.offset);
    EXPECT_LT(instances[0].span.end.offset, instances[1].span.start.offset);
}

TEST(VHDLParserTest, RejectsUnsupportedEntityAssociationForms) {
    for (const auto* mapping : {"a => a, y => y", "open, y", "work.leaf(rtl)"}) {
        SCOPED_TRACE(mapping);
        const std::string source = std::string(
            "entity top is port(a : in bit; y : out bit); end; "
            "architecture rtl of top is begin u: entity work.leaf port map(") +
            mapping + "); end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
}
