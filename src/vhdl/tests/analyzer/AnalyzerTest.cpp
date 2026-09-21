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

} // namespace
