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

TEST(VHDLParserTest, SignalInitializerIsPreserved) {
    const auto result = vhdl::Parser::parse(R"(
entity top is end entity top;
architecture rtl of top is
  signal internal : bit := '1';
begin
  internal <= '1';
end architecture rtl;
)");
    ASSERT_FALSE(result.hasErrors());
    const auto& initializer = result.syntax.architectures[0].signals[0].initializer;
    ASSERT_NE(initializer, nullptr);
    EXPECT_EQ(initializer->text, "'1'");

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

TEST(VHDLParserTest, ParenthesizedClockGuardsInSimpleAndStructuredProcesses) {
    for (const auto* guard : {
        "clk'event and clk = '1'", "(CLK'EVENT AND Clk = '1')",
        "((clk'event and clk = '1'))", "(clk'event) and (clk = '1')",
        "((clk = '1') and ((clk'event)))", "(rising_edge(clk))",
        "((rising_edge(clk)))"}) {
        for (const bool structured : {false, true}) {
            SCOPED_TRACE(guard);
            SCOPED_TRACE(structured);
            const std::string source = std::string(
                "entity reg is port(clk, d : in bit; q : out bit); end; "
                "architecture rtl of reg is begin process(clk) begin if ") + guard +
                " then " + (structured ? "for i in 0 to 1 loop q <= d; end loop;" : "q <= d;") +
                " end if; end process; end;";
            const auto parsed = vhdl::Parser::parse(source);
            ASSERT_FALSE(parsed.hasErrors());
            const auto& process = parsed.syntax.architectures.front().processes.front();
            EXPECT_EQ(process.eventSignal.canonical, "clk");
            EXPECT_EQ(process.levelSignal.canonical, "clk");
            EXPECT_EQ(process.level, "'1'");
            EXPECT_EQ(process.edgeForm, std::string(guard).find("rising_edge") != std::string::npos
                ? vhdl::ClockEdgeForm::RisingEdgeCall : vhdl::ClockEdgeForm::EventAndLevel);
            EXPECT_EQ(process.statements.empty(), !structured);
        }
    }
}

TEST(VHDLParserTest, RejectsIncompleteOrCompoundClockGuards) {
    for (const auto* guard : {
        "(clk'event)", "(clk'event or clk = '1')",
        "(clk'event and clk'event)",
        "((clk'event and clk = '1') and d = '1')",
        "(rising_edge(clk) and clk = '1')", "(clk'event and rising_edge(clk))",
        "(clk'event and clk = '1'", "clk'event and clk = '1'))"}) {
        SCOPED_TRACE(guard);
        const std::string source = std::string(
            "entity reg is port(clk, d : in bit; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin if ") + guard +
            " then q <= d; end if; end process; end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
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

TEST(VHDLParserTest, PreservesStructuredClockEnableControlFlow) {
    for (const auto* body : {
        "if en then q <= d; end if;",
        "if en = d then q <= d; end if;",
        "if en = '1' then if d = '1' then q <= d; end if; end if;"}) {
        SCOPED_TRACE(body);
        const std::string source = std::string(
            "entity reg is port(clk, en, d : in bit; q : out bit); end; "
            "architecture rtl of reg is begin process(clk) begin "
            "if rising_edge(clk) then ") + body + " end if; end process; end;";
        const auto parsed = vhdl::Parser::parse(source);
        ASSERT_FALSE(parsed.hasErrors());
        ASSERT_EQ(parsed.syntax.architectures.front().processes.front().statements.size(), 1u);
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

TEST(VHDLParserTest, ParsesArchitectureArrayTypeDeclaration) {
    const auto parsed = vhdl::Parser::parse(R"(entity lfsr is end;
architecture rtl of lfsr is
    signal temp : bit_vector(31 downto 0);
    type inner_taps is array (32 downto 2) of bit_vector(31 downto 0);
    signal taps : inner_taps;
begin
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_EQ(parsed.syntax.architectures.size(), 1u);
    const auto& architecture = parsed.syntax.architectures.front();
    ASSERT_EQ(architecture.arrayTypes.size(), 1u);
    const auto& type = architecture.arrayTypes.front();
    EXPECT_EQ(type.name.canonical, "inner_taps");
    EXPECT_EQ(type.indexRange.left, 32);
    EXPECT_EQ(type.indexRange.right, 2);
    EXPECT_FALSE(type.indexRange.ascending);
    EXPECT_EQ(type.elementType.name.canonical, "bit_vector");
    ASSERT_TRUE(type.elementType.constraint);
    EXPECT_EQ(type.elementType.constraint->left, 31);
    EXPECT_EQ(type.elementType.constraint->right, 0);
    ASSERT_EQ(architecture.signals.size(), 2u);
    EXPECT_EQ(architecture.signals.back().type.name.canonical, "inner_taps");
}

TEST(VHDLParserTest, PreservesNestedIndicesAggregatesAndStaticLoops) {
    const auto parsed = vhdl::Parser::parse(R"(
entity p is end;
architecture rtl of p is begin
  taps(2) <= "1010";
  process(clk, state) variable v : bit; begin
    if rising_edge(clk) then
      state <= (others => '1');
      for i in 3 downto 0 loop
        if taps(2)(i) = '1' then state(i) <= v;
        elsif state(i) = '0' then v := '1';
        else v := '0'; end if;
      end loop;
    end if;
    q <= state;
  end process;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& arch = parsed.syntax.architectures.front();
    ASSERT_EQ(arch.assignments.front().indices.size(), 1u);
    EXPECT_EQ(arch.assignments.front().indices.front()->text, "2");
    const auto& process = arch.processes.front();
    ASSERT_EQ(process.sensitivityList.size(), 2u);
    ASSERT_EQ(process.statements.size(), 2u);
    EXPECT_EQ(process.statements[0].assignment.value->kind, vhdl::Expression::Kind::Others);
    const auto& loop = process.statements[1];
    EXPECT_EQ(loop.kind, vhdl::SequentialStatement::Kind::For);
    EXPECT_EQ(loop.iterator.canonical, "i");
    EXPECT_FALSE(loop.range.ascending);
    const auto& condition = *loop.statements.front().condition;
    EXPECT_EQ(condition.left->kind, vhdl::Expression::Kind::Indexed);
    EXPECT_EQ(condition.left->left->kind, vhdl::Expression::Kind::Indexed);
    EXPECT_EQ(process.assignments.front().target.canonical, "q");
}

TEST(VHDLParserTest, RejectsUnsupportedScheduledSyntax) {
    for (const auto* body : {
        "stage <= d after 1 ns;", "stage <= transport d;",
        "stage <= reject 1 ns inertial d;", "stage <= d, d after 2 ns;",
        "wait;",
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
    for (const auto* mapping : {"open(0), y", "work."}) {
        SCOPED_TRACE(mapping);
        const std::string source = std::string(
            "entity top is port(a : in bit; y : out bit); end; "
            "architecture rtl of top is begin u: entity work.leaf port map(") +
            mapping + "); end;";
        EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    }
}

TEST(VHDLParserTest, PreservesNamedPortAssociations) {
    auto parsed = vhdl::Parser::parse(
        "entity top is port(a : in bit; y : out bit); end; "
        "architecture rtl of top is begin u: entity work.leaf port map(y => y, a => a); end;");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& instance = parsed.syntax.architectures.front().instantiations.front();
    ASSERT_EQ(instance.formals.size(), 2u);
    ASSERT_TRUE(instance.formals[0]);
    EXPECT_EQ(instance.formals[0]->canonical, "y");
    EXPECT_EQ(instance.formals[1]->canonical, "a");
}

TEST(VHDLParserTest, PreservesConstantArraysCallsAndGeneratedInstances) {
    const auto parsed = vhdl::Parser::parse(R"(
entity top is end;
architecture rtl of top is
  type table_type is array(natural range <>) of integer;
  constant coefficients : table_type := (1, 2, 3);
begin
  lanes: for i in 0 to 2 generate begin
    u: entity work.leaf generic map(n => coefficients(i))
      port map(a(i), y(i)(3 downto 0));
  end generate lanes;
  y <= std_logic_vector(to_unsigned(5, 3));
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& architecture = parsed.syntax.architectures.front();
    ASSERT_EQ(architecture.constants.size(), 1u);
    EXPECT_EQ(architecture.constants.front().value->elements.size(), 3u);
    ASSERT_TRUE(architecture.arrayTypes.front().indexSubtype);
    EXPECT_EQ(architecture.arrayTypes.front().indexSubtype->canonical, "natural");
    const auto& generate = architecture.generates.front();
    EXPECT_EQ(generate.label.canonical, "lanes");
    const auto& instance = generate.instantiations.front();
    EXPECT_EQ(instance.actualIndices[0].size(), 1u);
    EXPECT_EQ(instance.actualIndices[1].size(), 2u);
    const auto& conversion = *architecture.assignments.front().value->right;
    EXPECT_EQ(conversion.kind, vhdl::Expression::Kind::Call);
    EXPECT_EQ(conversion.elements.size(), 2u);
}

TEST(VHDLParserTest, RejectsMalformedConstantAndGenerateDeclarations) {
    for (const auto* declarations : {"constant n : integer;", "type t is array(natural range) of integer;"}) {
        EXPECT_TRUE(vhdl::Parser::parse(std::string("entity top is end; architecture rtl of top is ") +
            declarations + " begin end;").hasErrors());
    }
    EXPECT_TRUE(vhdl::Parser::parse("entity top is end; architecture rtl of top is begin "
        "g: for i in 0 to 1 generate begin y <= a; end generate wrong; end;").hasErrors());
}

TEST(VHDLParserTest, RecordsRetainFieldsAssociationsAndSelectedNames) {
    const auto parsed = vhdl::Parser::parse(R"(
package p is
  type t is record a, b : bit; end record t;
  constant c : t := (b => '1', a => '0');
end;
entity top is end;
architecture rtl of top is
  type outer_t is record inner : t; end record;
  signal x : outer_t;
begin
  x.inner.a <= c.b;
  u: entity work.leaf port map(x.inner);
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& package = parsed.syntax.packages.front();
    ASSERT_EQ(package.recordTypes.size(), 1u);
    EXPECT_EQ(package.recordTypes.front().fields.front().names.size(), 2u);
    const auto& aggregate = *package.constants.front().value;
    ASSERT_EQ(aggregate.elements.size(), 2u);
    EXPECT_EQ(aggregate.elements.front()->kind, vhdl::Expression::Kind::Association);
    EXPECT_EQ(aggregate.elements.front()->left->canonical, "b");
    const auto& architecture = parsed.syntax.architectures.front();
    EXPECT_EQ(architecture.recordTypes.size(), 1u);
    const auto& assignment = architecture.assignments.front();
    ASSERT_EQ(assignment.indices.size(), 2u);
    EXPECT_EQ(assignment.indices.front()->kind, vhdl::Expression::Kind::Selected);
    EXPECT_EQ(assignment.indices.front()->canonical, "inner");
    EXPECT_EQ(assignment.value->kind, vhdl::Expression::Kind::Selected);
    EXPECT_EQ(assignment.value->left->canonical, "c");
    EXPECT_EQ(architecture.instantiations.front().actualIndices.front().front()->canonical, "inner");
}

TEST(VHDLParserTest, PackageFunctionBodiesAndInterfaceDefaultsAreRetained) {
    auto parsed = vhdl::Parser::parse(R"(
package p is
  function choose(c : boolean; a, b : integer := 1) return integer;
  component child is generic(mask : bit_vector(3 downto 0) := "0000");
    port(a : in bit := '0'); end component;
end;
package body p is
  function choose(c : boolean; a, b : integer) return integer is
    constant k : integer := 2;
    variable v : integer := 0;
  begin
    if c then return a; elsif a = b then return k; else return b; end if;
  end function choose;
  function reduce(d : bit_vector) return bit is
    variable v : bit := '0';
  begin
    for i in d'range loop v := v or d(i); end loop;
    return v;
  end;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_EQ(parsed.syntax.packages.size(), 2u);
    const auto& declaration = parsed.syntax.packages[0].functions.front();
    EXPECT_FALSE(declaration.body);
    ASSERT_EQ(declaration.parameters.size(), 2u);
    EXPECT_EQ(declaration.parameters[1].names.size(), 2u);
    EXPECT_NE(declaration.parameters[1].defaultValue, nullptr);
    const auto& component = parsed.syntax.packages[0].components.front();
    EXPECT_TRUE(component.generics.front().type.constraint);
    EXPECT_NE(component.ports.front().defaultValue, nullptr);
    const auto& function = parsed.syntax.packages[1].functions.front();
    EXPECT_TRUE(function.body);
    EXPECT_EQ(function.variables.size(), 1u);
    EXPECT_EQ(function.constants.size(), 1u);
    EXPECT_EQ(function.statements.front().statements.front().kind, vhdl::SequentialStatement::Kind::Return);
    const auto& loop = parsed.syntax.packages[1].functions[1].statements.front();
    ASSERT_NE(loop.range.attribute, nullptr);
    EXPECT_EQ(loop.range.attribute->canonical, "range");
}

TEST(VHDLParserTest, RejectsMalformedFunctionsAndProcessReturns) {
    for (const auto* function : {
        "function f return integer is begin return; end;",
        "function f return integer is begin return 1; end wrong;",
        "function f(n : out integer) return integer;",
        "function f return integer is begin return 1 end;"}) {
        SCOPED_TRACE(function);
        EXPECT_TRUE(vhdl::Parser::parse(std::string("package p is ") + function + " end;").hasErrors());
    }
    EXPECT_TRUE(vhdl::Parser::parse("entity e is port(clk : in bit); end; "
        "architecture rtl of e is begin process(clk) begin if rising_edge(clk) then return 1; "
        "end if; end process; end;").hasErrors());
}

TEST(VHDLParserTest, PreservesAsynchronousResetBranchesAndPolarity) {
    for (const auto* predicate : {"rst = '0'", "('1' = rst)"}) {
        auto parsed = vhdl::Parser::parse(std::string(
            "entity e is port(clk, rst, d : in bit; q : out bit); end; "
            "architecture rtl of e is begin process(rst, clk) begin if ") + predicate +
            " then q <= '0'; elsif rising_edge(clk) then q <= d; end if; end process; end;");
        ASSERT_FALSE(parsed.hasErrors());
        const auto& process = parsed.syntax.architectures.front().processes.front();
        EXPECT_TRUE(process.asynchronousReset);
        ASSERT_TRUE(process.resetSignal);
        EXPECT_EQ(process.resetSignal->canonical, "rst");
        EXPECT_EQ(process.resetLevel, std::string(predicate).find("'0'") == std::string::npos ? "'1'" : "'0'");
        EXPECT_EQ(process.eventSignal.canonical, "clk");
        EXPECT_EQ(process.resetStatements.size(), 1u);
        EXPECT_EQ(process.statements.size(), 1u);
        EXPECT_TRUE(process.assignments.empty());
    }
}

TEST(VHDLParserTest, PreservesConditionalGenerateBranchesAndProcesses) {
    const auto parsed = vhdl::Parser::parse(R"(
entity top is end;
architecture rtl of top is begin
  g: if n = 0 generate begin
    p: process(clk) begin if rising_edge(clk) then q <= d; end if; end process;
  elsif n = 1 generate
    h: for i in 0 to 1 generate y(i) <= d(i); end generate h;
  else generate
    h: if true generate u: entity work.leaf port map(d, y); end generate h;
  end generate g;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& generate = parsed.syntax.architectures.front().generates.front();
    EXPECT_TRUE(generate.conditional);
    ASSERT_NE(generate.condition, nullptr);
    EXPECT_EQ(generate.processes.size(), 1u);
    ASSERT_EQ(generate.alternatives.size(), 2u);
    EXPECT_NE(generate.alternatives[0].condition, nullptr);
    EXPECT_EQ(generate.alternatives[1].condition, nullptr);
    EXPECT_FALSE(generate.alternatives[0].generates.front().conditional);
    EXPECT_TRUE(generate.alternatives[1].generates.front().conditional);
}

TEST(VHDLParserTest, RejectsMalformedConditionalGenerates) {
    for (const auto* body : {
        "g: if generate end generate;",
        "g: if true generate else generate elsif true generate end generate;",
        "g: if true generate end generate wrong;",
        "g: if true generate signal x : bit; end generate;",
        "g: for i in 0 to 1 generate else generate end generate;"}) {
        SCOPED_TRACE(body);
        EXPECT_TRUE(vhdl::Parser::parse(std::string("entity top is end; architecture rtl of top is begin ") +
            body + " end;").hasErrors());
    }
}

TEST(VHDLParserTest, PreservesGenerateLocalDeclarations) {
    const auto parsed = vhdl::Parser::parse(R"(
entity top is end;
architecture rtl of top is begin
  g: for i in 0 to 1 generate
    constant width : positive := i + 1;
    type word_t is array(0 to 1) of bit_vector(width-1 downto 0);
    type record_t is record value : bit; end record;
    signal words : word_t;
    signal flag : record_t;
  begin
    words(0) <= (others => '0'); flag.value <= '1';
  end generate;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& generate = parsed.syntax.architectures.front().generates.front();
    EXPECT_EQ(generate.constants.size(), 1u);
    EXPECT_EQ(generate.arrayTypes.size(), 1u);
    EXPECT_EQ(generate.recordTypes.size(), 1u);
    EXPECT_EQ(generate.signals.size(), 2u);
    EXPECT_EQ(generate.assignments.size(), 2u);
}

TEST(VHDLParserTest, PreservesCombinationalProcessesAndSensitivity) {
    for (const auto* sensitivity : {"all", "a, en"}) {
        const auto parsed = vhdl::Parser::parse(std::string(R"(
entity top is port(a, en : in bit; y : out bit); end;
architecture rtl of top is begin
process()") + sensitivity + R"() variable temp : bit; begin
  temp := a; y <= '0'; if en = '1' then y <= temp; end if;
end process; end;
)");
        ASSERT_FALSE(parsed.hasErrors());
        const auto& process = parsed.syntax.architectures.front().processes.front();
        EXPECT_TRUE(process.combinational);
        EXPECT_EQ(process.allSensitivity, std::string(sensitivity) == "all");
        EXPECT_EQ(process.statements.size(), 3u);
        EXPECT_EQ(process.variables.size(), 1u);
    }
    // Level tests are syntactically valid; incomplete assignments are rejected by lowering.
    EXPECT_FALSE(vhdl::Parser::parse("entity top is end; architecture rtl of top is begin "
        "process(clk) begin if clk = '1' then y <= d; end if; end process; end;").hasErrors());
}

TEST(VHDLParserTest, PreservesCaseChoicesRangesNestedBodiesAndNull) {
    const auto parsed = vhdl::Parser::parse(R"(
entity top is end; architecture rtl of top is begin
process(all) begin
  case index is
    when 0 to 2 | 4 => case flag is when '0' => y <= '1'; when others => null; end case;
    when 6 downto 5 => y <= '0';
    when others => null;
  end case;
end process; end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& statement = parsed.syntax.architectures.front().processes.front().statements.front();
    EXPECT_EQ(statement.kind, vhdl::SequentialStatement::Kind::Case);
    ASSERT_EQ(statement.choices.size(), 3u);
    EXPECT_EQ(statement.choices[0].size(), 2u);
    EXPECT_EQ(statement.choices[0][0]->kind, vhdl::Expression::Kind::Range);
    EXPECT_TRUE(statement.choices[2].empty());
    EXPECT_EQ(statement.caseBodies[0][0].kind, vhdl::SequentialStatement::Kind::Case);
    EXPECT_EQ(statement.caseBodies[2][0].kind, vhdl::SequentialStatement::Kind::Null);
}

TEST(VHDLParserTest, RejectsMalformedCaseAlternatives) {
    for (const auto* body : {"case a is end case;", "case a is when '0' => end case;",
        "case a is when others | '0' => null; end case;",
        "case a is when others => null; when '0' => null; end case;",
        "case a is when '0' null; end case;"}) {
        EXPECT_TRUE(vhdl::Parser::parse(std::string("entity top is end; architecture rtl of top is begin process(all) begin ") + body +
            " end process; end;").hasErrors());
    }
}

TEST(VHDLParserTest, PreservesEnumerationsInPackagesArchitecturesAndGenerates) {
    const auto parsed = vhdl::Parser::parse(R"(
package types is type phase_t is (idle, running, done); end;
entity top is end;
architecture rtl of top is type local_t is (low, high); begin
  g: if true generate type inner_t is (first, last); signal state : inner_t; begin state <= first; end generate;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    ASSERT_EQ(parsed.syntax.packages.front().enumerationTypes.size(), 1u);
    EXPECT_EQ(parsed.syntax.packages.front().enumerationTypes.front().literals.size(), 3u);
    ASSERT_EQ(parsed.syntax.architectures.front().enumerationTypes.size(), 1u);
    EXPECT_EQ(parsed.syntax.architectures.front().generates.front().enumerationTypes.size(), 1u);
}

TEST(VHDLParserTest, PreservesNestedRecordSensitivityPaths) {
    const auto parsed = vhdl::Parser::parse(R"(
entity top is end; architecture rtl of top is begin
process(packet.inner.data, packet.flag) begin y <= packet.inner.data; end process;
process(clk, packet.inner.data) begin if rising_edge(clk) then q <= d; end if; y <= packet.inner.data; end process;
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& processes = parsed.syntax.architectures.front().processes;
    ASSERT_EQ(processes.size(), 2u);
    EXPECT_EQ(processes[0].sensitivityList[0].canonical, "packet");
    ASSERT_EQ(processes[0].sensitivityFields[0].size(), 2u);
    EXPECT_EQ(processes[0].sensitivityFields[0][1].canonical, "data");
    EXPECT_TRUE(processes[1].sensitivityFields[0].empty());
    EXPECT_EQ(processes[1].sensitivityFields[1].size(), 2u);
}

TEST(VHDLParserTest, DiagnosticExclusionRequiresSynthesisMode) {
    const auto source = "entity e is end; architecture rtl of e is begin "
        "a: assert false report \"message; text\" severity failure; end;";
    const auto parsed = vhdl::Parser::parse(source, true);
    EXPECT_FALSE(parsed.hasErrors());
    ASSERT_EQ(parsed.warnings.size(), 1);
    EXPECT_EQ(parsed.warnings.front().code, "ignored-assertion");
    EXPECT_TRUE(vhdl::Parser::parse(source).hasErrors());
    EXPECT_TRUE(vhdl::Parser::parse(
        "entity e is end; architecture rtl of e is begin assert false end;", true).hasErrors());
}

TEST(VHDLParserTest, PreservesPortFormalSelectionsSeparatelyFromActualSelections) {
    const auto parsed = vhdl::Parser::parse(R"(
entity e is end;
architecture rtl of e is begin
  u: entity work.leaf port map(a.data(1 + 1 downto 0) => d(4 to 6), y(0) => '1');
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& instance = parsed.syntax.architectures.front().instantiations.front();
    ASSERT_EQ(instance.formalIndices.size(), 2);
    ASSERT_EQ(instance.formalIndices[0].size(), 2);
    EXPECT_EQ(instance.formalIndices[0][0]->kind, vhdl::Expression::Kind::Selected);
    EXPECT_EQ(instance.formalIndices[0][1]->kind, vhdl::Expression::Kind::Range);
    ASSERT_EQ(instance.actualIndices[0].size(), 1);
    EXPECT_EQ(instance.actualIndices[0][0]->kind, vhdl::Expression::Kind::Range);
    EXPECT_EQ(instance.actualLiterals[1]->kind, vhdl::Expression::Kind::CharacterLiteral);
}

TEST(VHDLParserTest, PreservesOpenPortActualsWithoutInventingNames) {
    const auto parsed = vhdl::Parser::parse(R"(
entity e is end;
architecture rtl of e is begin
  p: entity work.leaf port map(d, open);
  n: entity work.leaf port map(y => open, a => '1');
end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& instances = parsed.syntax.architectures.front().instantiations;
    ASSERT_EQ(instances.size(), 2u);
    EXPECT_EQ(instances[0].actualOpen, (std::vector<bool>{false, true}));
    EXPECT_EQ(instances[1].actualOpen, (std::vector<bool>{true, false}));
    EXPECT_TRUE(instances[0].actuals[1].spelling.empty());
    EXPECT_EQ(instances[0].actuals[1].span.start.line, 4);
    EXPECT_EQ(instances[1].formals[0]->canonical, "y");
    EXPECT_NE(instances[1].actualLiterals[1], nullptr);
}

TEST(VHDLParserTest, ProcessEndLabelsMatchAcrossProcessFormsAndScopes) {
    for (const auto* body : {
        "process(all) begin y <= a;",
        "process(clk) begin if rising_edge(clk) then y <= a; end if;",
        "process(clk) begin if rising_edge(clk) then if a = '1' then y <= a; else y <= '0'; end if; end if;",
        "process(clk, rst) begin if rst = '0' then y <= '0'; elsif rising_edge(clk) then y <= a; end if;"
    }) {
        for (const bool generated : {false, true}) {
            for (const auto& [opening, closing] : std::vector<std::pair<std::string, std::string>>{
                {"P", "p"}, {"p", ""}, {"", ""}, {"\\CaseSensitive\\", "\\CaseSensitive\\"}}) {
                SCOPED_TRACE(body);
                SCOPED_TRACE(opening + "/" + closing);
                const auto source = std::string("entity e is end; architecture rtl of e is begin ") +
                    (generated ? "g: if true generate " : "") +
                    (opening.empty() ? "" : opening + ": ") + body + " end process " + closing + "; " +
                    (generated ? "end generate; " : "") + "end;";
                const auto parsed = vhdl::Parser::parse(source);
                ASSERT_FALSE(parsed.hasErrors());
                const auto& architecture = parsed.syntax.architectures.front();
                const auto& process = generated ? architecture.generates.front().processes.front()
                                                : architecture.processes.front();
                EXPECT_EQ(process.label.has_value(), !opening.empty());
                if (process.label) EXPECT_EQ(process.label->spelling, opening);
            }
        }
    }
}

TEST(VHDLParserTest, RejectsMismatchedAndUnintroducedProcessEndLabels) {
    for (const auto* body : {
        "process(all) begin y <= a;",
        "process(clk) begin if rising_edge(clk) then y <= a; end if;"
    }) {
        for (const auto& [opening, closing] : std::vector<std::pair<std::string, std::string>>{
            {"p", "q"}, {"", "p"}, {"\\CaseSensitive\\", "\\casesensitive\\"}}) {
            const auto parsed = vhdl::Parser::parse(std::string(
                "entity e is end; architecture rtl of e is begin ") +
                (opening.empty() ? "" : opening + ": ") + body + " end process " + closing + "; end;");
            EXPECT_TRUE(parsed.hasErrors());
            ASSERT_FALSE(parsed.diagnostics.empty());
            EXPECT_EQ(parsed.diagnostics.front().message, "process end label does not match its declaration");
        }
    }
}

TEST(VHDLParserTest, DistinguishesEnumerationIndexTypeFromUnconstrainedSubtype) {
    const auto parsed = vhdl::Parser::parse(R"(
entity e is end; architecture rtl of e is
  type index_t is (a, b);
  type full_t is array(index_t) of bit;
  type flexible_t is array(index_t range <>) of bit;
  type reverse_t is array(b downto a) of bit;
begin end;
)");
    ASSERT_FALSE(parsed.hasErrors());
    const auto& arrays = parsed.syntax.architectures.front().arrayTypes;
    ASSERT_EQ(arrays.size(), 3u);
    ASSERT_TRUE(arrays[0].indexType.has_value());
    EXPECT_EQ(arrays[0].indexType->canonical, "index_t");
    EXPECT_FALSE(arrays[0].indexSubtype.has_value());
    EXPECT_TRUE(arrays[1].indexSubtype.has_value());
    EXPECT_FALSE(arrays[1].indexType.has_value());
    EXPECT_FALSE(arrays[2].indexRange.ascending);
}
