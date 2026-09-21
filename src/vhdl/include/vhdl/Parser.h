// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Lexer.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vhdl {

struct Name {
    std::string spelling;
    std::string canonical;
    SourceSpan span;
};

enum class PortMode { In, Out, InOut, Buffer, Linkage };

struct DiscreteRange {
    std::int64_t left = 0;
    std::int64_t right = 0;
    bool ascending = false;
    SourceSpan span;
};

struct TypeMark {
    Name name;
    std::optional<DiscreteRange> constraint;
};

struct PortDeclaration {
    std::vector<Name> names;
    PortMode mode = PortMode::In;
    TypeMark type;
    SourceSpan span;
};

struct Expression {
    enum class Kind {
        Name,
        IntegerLiteral,
        RealLiteral,
        StringLiteral,
        CharacterLiteral,
        Unary,
        Binary
    };
    Kind kind;
    std::string text;
    std::string canonical;
    SourceSpan span;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct ConcurrentAssignment {
    Name target;
    std::unique_ptr<Expression> value;
    SourceSpan span;
};

struct EntityDeclaration {
    Name name;
    std::vector<PortDeclaration> ports;
    SourceSpan span;
};

struct ArchitectureBody {
    Name name;
    Name entity;
    std::vector<ConcurrentAssignment> assignments;
    SourceSpan span;
};

struct DesignFile {
    std::vector<EntityDeclaration> entities;
    std::vector<ArchitectureBody> architectures;
};

struct ParseDiagnostic {
    std::string message;
    SourceSpan span;
};

struct ParseResult {
    DesignFile syntax;
    std::vector<ParseDiagnostic> diagnostics;
    bool hasErrors() const { return !diagnostics.empty(); }
};

/// Parser for the initial RTL syntax slice. Unsupported constructs are diagnosed.
class Parser {
public:
    static ParseResult parse(std::string_view source);
};

} // namespace vhdl
