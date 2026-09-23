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

struct Expression {
    enum class Kind {
        Name,
        IntegerLiteral,
        RealLiteral,
        StringLiteral,
        CharacterLiteral,
        Unary,
        Binary,
        Conditional,
        Indexed,
        Others
    };
    Kind kind;
    std::string text;
    std::string canonical;
    SourceSpan span;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::unique_ptr<Expression> condition;
};

struct DiscreteRange {
    std::int64_t left = 0;
    std::int64_t right = 0;
    bool ascending = false;
    SourceSpan span;
    std::shared_ptr<Expression> leftExpression;
    std::shared_ptr<Expression> rightExpression;
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

struct GenericDeclaration {
    std::vector<Name> names;
    TypeMark type;
    std::unique_ptr<Expression> defaultValue;
    SourceSpan span;
};

struct ObjectDeclaration {
    std::vector<Name> names;
    TypeMark type;
    SourceSpan span;
};

using SignalDeclaration = ObjectDeclaration;
using VariableDeclaration = ObjectDeclaration;

struct ArrayTypeDeclaration {
    Name name;
    DiscreteRange indexRange;
    TypeMark elementType;
    SourceSpan span;
};

enum class AssignmentKind { Signal, Variable };

struct Assignment {
    Name target;
    std::unique_ptr<Expression> value;
    SourceSpan span;
    AssignmentKind kind = AssignmentKind::Signal;
    std::vector<std::unique_ptr<Expression>> indices;
};

struct SequentialStatement {
    enum class Kind { Assignment, If, For };
    Kind kind = Kind::Assignment;
    Assignment assignment;
    std::unique_ptr<Expression> condition;
    Name iterator;
    DiscreteRange range;
    std::vector<SequentialStatement> statements;
    std::vector<SequentialStatement> alternative;
    SourceSpan span;
};

struct LibraryClause {
    std::vector<Name> names;
    SourceSpan span;
};

struct UseClause {
    std::vector<Name> selectedName;
    SourceSpan span;
};

struct ContextClause {
    std::vector<LibraryClause> libraries;
    std::vector<UseClause> uses;
};

// Restricted positive-edge process syntax; names remain unresolved here.
enum class ClockEdgeForm { EventAndLevel, RisingEdgeCall };

struct ClockedProcess {
    Name sensitivity;
    Name eventSignal;
    Name levelSignal;
    std::string level;
    std::vector<Assignment> assignments;
    std::vector<VariableDeclaration> variables;
    std::optional<Name> enableSignal;
    std::string enableLevel;
    std::optional<Name> resetSignal;
    std::string resetLevel;
    std::vector<Assignment> resetAssignments;
    ClockEdgeForm edgeForm = ClockEdgeForm::EventAndLevel;
    SourceSpan span;
    // Structured bodies retain nested control flow until static elaboration.
    std::vector<SequentialStatement> statements;
    std::vector<Name> sensitivityList;
};

struct EntityDeclaration {
    Name name;
    std::vector<GenericDeclaration> generics;
    std::vector<PortDeclaration> ports;
    SourceSpan span;
    ContextClause context;
};

struct GenericAssociation {
    std::optional<Name> formal;
    std::unique_ptr<Expression> actual;
    SourceSpan span;
};

struct EntityInstantiation {
    Name label;
    Name library;
    Name entity;
    std::optional<Name> architecture;
    std::vector<GenericAssociation> generics;
    std::vector<Name> actuals;
    SourceSpan span;
};

struct ArchitectureBody {
    Name name;
    Name entity;
    std::vector<Assignment> assignments;
    std::vector<ClockedProcess> processes;
    std::vector<ArrayTypeDeclaration> arrayTypes;
    std::vector<SignalDeclaration> signals;
    std::vector<EntityInstantiation> instantiations;
    SourceSpan span;
    ContextClause context;
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
