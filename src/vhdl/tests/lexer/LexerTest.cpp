// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Lexer.h"
#include <gtest/gtest.h>
#include <vector>

using vhdl::Lexer;
using vhdl::TokenKind;

TEST(VHDLLexerTest, IdentifiersAndLocations) {
    const auto result = Lexer::scan(
        "ENTITY Top_1 is\n  signal CLK : bit; -- ignored\nend ENTITY top_1;");
    EXPECT_TRUE(result.diagnostics.empty());
    ASSERT_GE(result.tokens.size(), 2);
    EXPECT_EQ(result.tokens[0].kind, TokenKind::Identifier);
    EXPECT_EQ(result.tokens[0].text, "ENTITY");
    EXPECT_EQ(result.tokens[0].canonical, "entity");
    EXPECT_EQ(result.tokens[1].canonical, "top_1");
    bool foundClk = false;
    for (const auto& token : result.tokens) {
        if (token.text == "CLK") {
            foundClk = true;
            EXPECT_EQ(token.span.start.line, 2);
            EXPECT_EQ(token.span.start.column, 10);
        }
    }
    EXPECT_TRUE(foundClk);
    EXPECT_EQ(result.tokens.back().kind, TokenKind::EndOfFile);
}

TEST(VHDLLexerTest, LiteralsAndApostrophes) {
    const auto result = Lexer::scan(
        "2#1010# 16#FF# 2#10.1#E+2 12.5e-2 \"a\"\"b\" \\Case Sensitive\\ q'event '1' ''''");
    EXPECT_TRUE(result.diagnostics.empty());
    const std::vector<TokenKind> expected = {
        TokenKind::IntegerLiteral,   TokenKind::IntegerLiteral,   TokenKind::RealLiteral,
        TokenKind::RealLiteral,      TokenKind::StringLiteral,    TokenKind::ExtendedIdentifier,
        TokenKind::Identifier,       TokenKind::Symbol,           TokenKind::Identifier,
        TokenKind::CharacterLiteral, TokenKind::CharacterLiteral, TokenKind::EndOfFile};
    ASSERT_EQ(result.tokens.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_EQ(result.tokens[i].kind, expected[i]) << "token " << i;
    EXPECT_EQ(result.tokens[5].text, "\\Case Sensitive\\");
    EXPECT_TRUE(result.tokens[5].canonical.empty());
}

TEST(VHDLLexerTest, CompoundSymbols) {
    const auto result = Lexer::scan(":= <= >= /= => ** <> ?= ?/= ?<= ?< ?");
    EXPECT_TRUE(result.diagnostics.empty());
    const std::vector<std::string> expected = {
        ":=", "<=", ">=", "/=", "=>", "**", "<>", "?", "=", "?", "/=", "?", "<=", "?", "<", "?",
    };
    ASSERT_EQ(result.tokens.size(), expected.size() + 1);
    for (std::size_t i = 0; i < expected.size(); ++i)
        EXPECT_EQ(result.tokens[i].text, expected[i]) << "token " << i;
}

TEST(VHDLLexerTest, MalformedInputRecovers) {
    const auto result = Lexer::scan("a__b 1__2 \"unterminated");
    ASSERT_EQ(result.diagnostics.size(), 3);
    ASSERT_GE(result.tokens.size(), 4);
    EXPECT_EQ(result.tokens[0].kind, TokenKind::Invalid);
    EXPECT_EQ(result.tokens[1].kind, TokenKind::Invalid);
    EXPECT_EQ(result.tokens[2].kind, TokenKind::Invalid);
    EXPECT_EQ(result.tokens.back().kind, TokenKind::EndOfFile);
}

TEST(VHDLLexerTest, SynthesisExclusionsPreserveLocationsAndDefaultTokens) {
    const std::string source = "a\n-- PrAgMa translate_off\nfile f : text;\n\"-- pragma translate_on\"\n-- pragma translate_on\nb";
    const auto normal = Lexer::scan(source);
    const auto synthesis = Lexer::scan(source, true);
    EXPECT_TRUE(normal.diagnostics.empty());
    EXPECT_GT(normal.tokens.size(), 3u);
    EXPECT_TRUE(synthesis.diagnostics.empty());
    ASSERT_EQ(synthesis.tokens.size(), 3u);
    EXPECT_EQ(synthesis.tokens[1].canonical, "b");
    EXPECT_EQ(synthesis.tokens[1].span.start.line, 6u);
    EXPECT_EQ(synthesis.tokens[1].span.start.offset, source.size() - 1);
}

TEST(VHDLLexerTest, RejectsUnbalancedSynthesisExclusions) {
    for (const auto* source : {"-- pragma translate_off\na", "-- synthesis translate_on\na",
         "-- synopsys translate_off\n-- synopsys translate_off\n-- synopsys translate_on"}) {
        EXPECT_FALSE(Lexer::scan(source, true).diagnostics.empty()) << source;
        EXPECT_TRUE(Lexer::scan(source).diagnostics.empty()) << source;
    }
}
