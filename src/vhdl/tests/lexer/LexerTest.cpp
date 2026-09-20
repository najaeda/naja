// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Lexer.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using vhdl::Lexer;
using vhdl::TokenKind;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

void testIdentifiersAndLocations() {
    const auto result = Lexer::scan(
        "ENTITY Top_1 is\n  signal CLK : bit; -- ignored\nend ENTITY top_1;");
    require(result.diagnostics.empty(), "valid entity source produced diagnostics");
    require(result.tokens[0].kind == TokenKind::Identifier, "ENTITY must be an identifier token");
    require(result.tokens[0].text == "ENTITY" && result.tokens[0].canonical == "entity",
            "basic identifier spelling or canonical form was lost");
    require(result.tokens[1].canonical == "top_1", "basic identifier case was not normalized");
    bool foundClk = false;
    for (const auto& token : result.tokens) {
        if (token.text == "CLK") {
            foundClk = true;
            require(token.span.start.line == 2 && token.span.start.column == 10,
                    "token source location is incorrect");
        }
    }
    require(foundClk, "signal identifier was not emitted");
    require(result.tokens.back().kind == TokenKind::EndOfFile, "EOF token is missing");
}

void testLiteralsAndApostrophes() {
    const auto result = Lexer::scan(
        "2#1010# 16#FF# 2#10.1#E+2 12.5e-2 \"a\"\"b\" \\Case Sensitive\\ q'event '1' ''''");
    require(result.diagnostics.empty(), "valid literals produced diagnostics");
    const std::vector<TokenKind> expected = {
        TokenKind::IntegerLiteral,   TokenKind::IntegerLiteral,   TokenKind::RealLiteral,
        TokenKind::RealLiteral,      TokenKind::StringLiteral,    TokenKind::ExtendedIdentifier,
        TokenKind::Identifier,       TokenKind::Symbol,           TokenKind::Identifier,
        TokenKind::CharacterLiteral, TokenKind::CharacterLiteral, TokenKind::EndOfFile};
    require(result.tokens.size() == expected.size(), "unexpected token count for literals");
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(result.tokens[i].kind == expected[i],
                "literal or apostrophe token kind is incorrect");
    require(result.tokens[5].text == "\\Case Sensitive\\", "extended identifier spelling changed");
    require(result.tokens[5].canonical.empty(), "extended identifiers must remain case-sensitive");
}

void testCompoundSymbols() {
    const auto result = Lexer::scan(":= <= >= /= => ** <> ?= ?/= ?<= ?< ?");
    require(result.diagnostics.empty(), "valid symbols produced diagnostics");
    const std::vector<std::string> expected = {
        ":=", "<=", ">=", "/=", "=>", "**", "<>", "?", "=", "?", "/=", "?", "<=", "?", "<", "?",
    };
    require(result.tokens.size() == expected.size() + 1, "unexpected symbol token count");
    for (std::size_t i = 0; i < expected.size(); ++i)
        require(result.tokens[i].text == expected[i], "compound symbol was split incorrectly");
}

void testMalformedInputRecovers() {
    const auto result = Lexer::scan("a__b 1__2 \"unterminated");
    require(result.diagnostics.size() == 3, "malformed input diagnostic count is incorrect");
    require(result.tokens[0].kind == TokenKind::Invalid, "malformed identifier was not rejected");
    require(result.tokens[1].kind == TokenKind::Invalid,
            "malformed decimal literal was not rejected");
    require(result.tokens[2].kind == TokenKind::Invalid, "unterminated string was not rejected");
    require(result.tokens.back().kind == TokenKind::EndOfFile,
            "lexer failed to make forward progress");
}

} // namespace

int main() {
    try {
        testIdentifiersAndLocations();
        testLiteralsAndApostrophes();
        testCompoundSymbols();
        testMalformedInputRecovers();
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
