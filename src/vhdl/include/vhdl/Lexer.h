// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace vhdl {

struct SourcePosition {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct SourceSpan {
    SourcePosition start;
    SourcePosition end;
};

enum class TokenKind {
    Identifier,
    ExtendedIdentifier,
    IntegerLiteral,
    RealLiteral,
    StringLiteral,
    CharacterLiteral,
    Symbol,
    Invalid,
    EndOfFile,
};

struct Token {
    TokenKind kind;
    std::string text;
    // Basic identifiers are case-insensitive; other tokens have no canonical form.
    std::string canonical;
    SourceSpan span;
};

struct LexDiagnostic {
    std::string message;
    SourceSpan span;
};

struct LexResult {
    std::vector<Token> tokens;
    std::vector<LexDiagnostic> diagnostics;
};

/// Handwritten VHDL scanner. Source spelling and byte-based locations are retained.
class Lexer {
public:
    static LexResult scan(std::string_view source, bool synthesis = false);
};

} // namespace vhdl
