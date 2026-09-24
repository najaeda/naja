// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Analyzer.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace vhdl {
namespace {

struct CheckedType {
    ScalarType kind = ScalarType::Unknown;
    std::optional<DiscreteRange> range;
};

using Declarations = std::unordered_map<std::string, CheckedType>;
using GenericValues = std::unordered_map<std::string, std::int64_t>;

struct Visibility {
    bool stdLogic1164 = false;
    bool numericStd = false;
};

std::string_view key(const Name& name) {
    return name.canonical.empty() ? std::string_view(name.spelling)
                                  : std::string_view(name.canonical);
}

Visibility analyzeContext(const ContextClause& context, AnalysisResult& result) {
    std::unordered_set<std::string> libraries{"std", "work"};
    for (const auto& clause : context.libraries) {
        for (const auto& name : clause.names)
            libraries.emplace(key(name));
    }
    Visibility visibility;
    for (const auto& clause : context.uses) {
        if (clause.selectedName.size() != 3 ||
            key(clause.selectedName.back()) != "all") {
            result.diagnostics.push_back(
                {"only whole-package use clauses are supported", clause.span});
            continue;
        }
        const auto library = key(clause.selectedName[0]);
        const auto package = key(clause.selectedName[1]);
        if (!libraries.contains(std::string(library))) {
            result.diagnostics.push_back(
                {"use clause names library '" + clause.selectedName[0].spelling +
                     "' without a matching library clause",
                 clause.selectedName[0].span});
            continue;
        }
        if (library != "ieee") {
            result.diagnostics.push_back(
                {"package imports are currently supported only from library 'ieee'",
                 clause.selectedName[0].span});
            continue;
        }
        if (package == "std_logic_1164")
            visibility.stdLogic1164 = true;
        else if (package == "numeric_std")
            visibility.numericStd = true;
        else
            result.diagnostics.push_back(
                {"unsupported IEEE package '" + clause.selectedName[1].spelling + "'",
                 clause.selectedName[1].span});
    }
    return visibility;
}

std::optional<std::int64_t> evaluateInteger(
        const Expression& expression, const GenericValues& values) {
    switch (expression.kind) {
        case Expression::Kind::Name: {
            const auto name = expression.canonical.empty()
                ? expression.text : expression.canonical;
            const auto found = values.find(name);
            return found == values.end() ? std::nullopt
                                         : std::optional<std::int64_t>(found->second);
        }
        case Expression::Kind::IntegerLiteral: {
            if (expression.text.find('#') != std::string::npos)
                return std::nullopt;
            std::string digits;
            for (const auto character : expression.text)
                if (character != '_')
                    digits.push_back(character);
            std::int64_t value = 0;
            const auto [end, error] = std::from_chars(
                digits.data(), digits.data() + digits.size(), value);
            if (error != std::errc{} || end != digits.data() + digits.size())
                return std::nullopt;
            return value;
        }
        case Expression::Kind::Unary: {
            if (!expression.left)
                return std::nullopt;
            const auto operand = evaluateInteger(*expression.left, values);
            if (!operand)
                return std::nullopt;
            if (expression.text == "+")
                return operand;
            if (expression.text == "-" && *operand != std::numeric_limits<std::int64_t>::min())
                return -*operand;
            return std::nullopt;
        }
        case Expression::Kind::Binary: {
            if (!expression.left || !expression.right)
                return std::nullopt;
            const auto left = evaluateInteger(*expression.left, values);
            const auto right = evaluateInteger(*expression.right, values);
            if (!left || !right)
                return std::nullopt;
            const auto min = std::numeric_limits<std::int64_t>::min();
            const auto max = std::numeric_limits<std::int64_t>::max();
            if (expression.text == "+") {
                if ((*right > 0 && *left > max - *right) ||
                    (*right < 0 && *left < min - *right))
                    return std::nullopt;
                return *left + *right;
            }
            if (expression.text == "-") {
                if ((*right > 0 && *left < min + *right) ||
                    (*right < 0 && *left > max + *right))
                    return std::nullopt;
                return *left - *right;
            }
            if (expression.text == "*") {
                if (*left > 0) {
                    if ((*right > 0 && *left > max / *right) ||
                        (*right < 0 && *right < min / *left))
                        return std::nullopt;
                } else if (*left < 0) {
                    if ((*right > 0 && *left < min / *right) ||
                        (*right < 0 && *left < max / *right))
                        return std::nullopt;
                }
                return *left * *right;
            }
            if ((expression.text == "/" || expression.text == "mod" ||
                 expression.text == "rem") && *right == 0)
                return std::nullopt;
            // Signed division and remainder both overflow for min / -1.
            if (*left == min && *right == -1) {
                if (expression.text == "/")
                    return std::nullopt;
                if (expression.text == "rem" || expression.text == "mod")
                    return 0;
            }
            if (expression.text == "/")
                return *left / *right;
            if (expression.text == "rem")
                return *left % *right;
            if (expression.text == "mod") {
                auto remainder = *left % *right;
                if (remainder != 0 && ((remainder < 0) != (*right < 0)))
                    remainder += *right;
                return remainder;
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

std::optional<DiscreteRange> resolveRange(
        const DiscreteRange& range, const GenericValues& values) {
    if (range.attribute) return std::nullopt;
    if (!range.leftExpression || !range.rightExpression)
        return range;
    const auto left = evaluateInteger(*range.leftExpression, values);
    const auto right = evaluateInteger(*range.rightExpression, values);
    if (!left || !right)
        return std::nullopt;
    auto resolved = range;
    resolved.left = *left;
    resolved.right = *right;
    return resolved;
}

CheckedType declarationType(const TypeMark& type, Visibility visibility = {},
                            const GenericValues& values = {}) {
    const auto range = type.constraint
        ? resolveRange(*type.constraint, values) : std::nullopt;
    if (type.name.canonical == "bit_vector" && type.constraint)
        return {ScalarType::BitVector, range};
    if (visibility.stdLogic1164 && type.name.canonical == "std_logic_vector" &&
        type.constraint)
        return {ScalarType::StdLogicVector, range};
    if (visibility.numericStd && type.name.canonical == "unsigned" && type.constraint)
        return {ScalarType::Unsigned, range};
    if (visibility.numericStd && type.name.canonical == "signed" && type.constraint)
        return {ScalarType::Signed, range};
    if (type.constraint)
        return {};
    if (type.name.canonical == "bit")
        return {ScalarType::Bit, std::nullopt};
    if (visibility.stdLogic1164 && type.name.canonical == "std_logic")
        return {ScalarType::StdLogic, std::nullopt};
    if (type.name.canonical == "boolean")
        return {ScalarType::Boolean, std::nullopt};
    if (type.name.canonical == "integer")
        return {ScalarType::Integer, std::nullopt};
    if (type.name.canonical == "natural")
        return {ScalarType::Natural, std::nullopt};
    if (type.name.canonical == "positive")
        return {ScalarType::Natural, std::nullopt};
    if (type.name.canonical == "real")
        return {ScalarType::Real, std::nullopt};
    if (type.name.canonical == "string")
        return {ScalarType::String, std::nullopt};
    return {};
}

std::uint64_t rangeWidth(const DiscreteRange& range) {
    if ((range.ascending && range.left > range.right) ||
        (!range.ascending && range.left < range.right))
        return 0;
    const auto left = static_cast<std::uint64_t>(range.left);
    const auto right = static_cast<std::uint64_t>(range.right);
    return (range.left > range.right ? left - right : right - left) + 1;
}

bool compatible(const CheckedType& left, const CheckedType& right) {
    if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
        return false;
    if ((left.kind == ScalarType::Integer || left.kind == ScalarType::Natural) &&
        (right.kind == ScalarType::Integer || right.kind == ScalarType::Natural))
        return true;
    if (left.kind != right.kind)
        return false;
    if (left.kind != ScalarType::BitVector && left.kind != ScalarType::StdLogicVector &&
        left.kind != ScalarType::Unsigned && left.kind != ScalarType::Signed)
        return true;
    if (!left.range || !right.range)
        return !left.range && !right.range;
    return rangeWidth(*left.range) == rangeWidth(*right.range);
}

bool isLogical(std::string_view op) {
    return op == "and" || op == "nand" || op == "or" || op == "nor" ||
           op == "xor" || op == "xnor";
}

const char* typeName(ScalarType type) {
    switch (type) {
        case ScalarType::Bit: return "bit";
        case ScalarType::BitVector: return "bit_vector";
        case ScalarType::StdLogic: return "std_logic";
        case ScalarType::StdLogicVector: return "std_logic_vector";
        case ScalarType::Unsigned: return "unsigned";
        case ScalarType::Signed: return "signed";
        case ScalarType::Boolean: return "boolean";
        case ScalarType::Integer: return "integer";
        case ScalarType::Natural: return "natural";
        case ScalarType::Real: return "real";
        case ScalarType::String: return "string";
        case ScalarType::Unknown: return "unknown";
    }
    return "unknown";
}

bool isLogicalType(ScalarType type) {
    return type == ScalarType::Bit || type == ScalarType::BitVector ||
           type == ScalarType::StdLogic || type == ScalarType::StdLogicVector ||
           type == ScalarType::Boolean;
}

bool isNumericVectorType(ScalarType type) {
    return type == ScalarType::Unsigned || type == ScalarType::Signed;
}

std::optional<DiscreteRange> canonicalRange(
    std::uint64_t width, SourceSpan span) {
    constexpr auto maxWidth =
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1;
    if (width == 0 || width > maxWidth)
        return std::nullopt;
    return DiscreteRange{
        static_cast<std::int64_t>(width - 1), 0, false, span};
}

bool isStdLogicLiteral(std::string_view literal) {
    if (literal.size() != 3 || literal.front() != '\'' || literal.back() != '\'')
        return false;
    switch (literal[1]) {
        case 'U': case 'X': case '0': case '1': case 'Z': case 'W':
        case 'L': case 'H': case '-':
            return true;
        default:
            return false;
    }
}

CheckedType checkExpression(const Expression& expression,
                            const Declarations& declarations,
                            CheckedType expected,
                            Visibility visibility,
                            AnalysisResult& result) {
    const auto record = [&](CheckedType type) {
        result.expressionTypes[&expression] = type.kind;
        if (type.range)
            result.expressionRanges[&expression] = *type.range;
        return type;
    };
    switch (expression.kind) {
        case Expression::Kind::Name: {
            const auto expressionName = expression.canonical.empty()
                ? expression.text : expression.canonical;
            const auto found = declarations.find(expressionName);
            if (found == declarations.end()) {
                result.diagnostics.push_back(
                    {"no declaration for name '" + expression.text + "'", expression.span});
                return record({});
            }
            if (found->second.kind == ScalarType::Unknown) {
                result.diagnostics.push_back(
                    {"unsupported type for name '" + expression.text + "'", expression.span});
            }
            return record(found->second);
        }
        case Expression::Kind::Attribute:
        case Expression::Kind::Selected:
        case Expression::Kind::Association:
        case Expression::Kind::Call:
        case Expression::Kind::Indexed:
        case Expression::Kind::Others:
        case Expression::Kind::Aggregate:
        case Expression::Kind::Range:
        case Expression::Kind::BitStringLiteral:
            result.diagnostics.push_back(
                {"indexed expressions and aggregates require RTL elaboration", expression.span});
            return record({});
        case Expression::Kind::IntegerLiteral:
            return record({expected.kind == ScalarType::Natural
                               ? ScalarType::Natural : ScalarType::Integer,
                           std::nullopt});
        case Expression::Kind::RealLiteral:
            return record({ScalarType::Real, std::nullopt});
        case Expression::Kind::StringLiteral:
            return record({ScalarType::String, std::nullopt});
        case Expression::Kind::CharacterLiteral:
            if (expected.kind == ScalarType::Bit &&
                (expression.text == "'0'" || expression.text == "'1'"))
                return record({ScalarType::Bit, std::nullopt});
            if (expected.kind == ScalarType::StdLogic && isStdLogicLiteral(expression.text))
                return record({ScalarType::StdLogic, std::nullopt});
            if (expected.kind == ScalarType::Unknown)
                return record({});
            result.diagnostics.push_back(
                {"character literal requires a supported scalar bit context", expression.span});
            return record({});
        case Expression::Kind::Unary: {
            const auto operand = checkExpression(
                *expression.left, declarations, expected, visibility, result);
            if (expression.text == "not" && isLogicalType(operand.kind))
                return record(operand);
            if (expression.text == "not" && isNumericVectorType(operand.kind)) {
                if (!visibility.numericStd) {
                    result.diagnostics.push_back(
                        {"unary 'not' for numeric vectors requires ieee.numeric_std.all",
                         expression.span});
                    return record({});
                }
                if (!operand.range || rangeWidth(*operand.range) == 0) {
                    result.diagnostics.push_back(
                        {"unary 'not' requires a non-null numeric vector operand",
                         expression.span});
                    return record({});
                }
                const auto range = canonicalRange(
                    rangeWidth(*operand.range), expression.span);
                if (!range) {
                    result.diagnostics.push_back(
                        {"unary 'not' result width exceeds the supported range",
                         expression.span});
                    return record({});
                }
                return record({operand.kind, *range});
            }
            if ((expression.text == "-" || expression.text == "abs") &&
                operand.kind == ScalarType::Signed) {
                if (!visibility.numericStd) {
                    result.diagnostics.push_back(
                        {"unary '" + expression.text +
                             "' for signed vectors requires ieee.numeric_std.all",
                         expression.span});
                    return record({});
                }
                if (!operand.range || rangeWidth(*operand.range) == 0) {
                    result.diagnostics.push_back(
                        {"unary '" + expression.text +
                             "' requires a non-null signed vector operand",
                         expression.span});
                    return record({});
                }
                const auto range = canonicalRange(
                    rangeWidth(*operand.range), expression.span);
                if (!range) {
                    result.diagnostics.push_back(
                        {"unary '" + expression.text +
                             "' result width exceeds the supported range",
                         expression.span});
                    return record({});
                }
                return record({ScalarType::Signed, *range});
            }
            if (operand.kind == ScalarType::Unknown)
                return record({});
            result.diagnostics.push_back(
                {"operator '" + expression.text + "' is not supported for type '" +
                     typeName(operand.kind) + "'", expression.span});
            return record({});
        }
        case Expression::Kind::Binary: {
            if (isLogical(expression.text)) {
                const auto operandExpected =
                    isLogicalType(expected.kind) ? expected : CheckedType{};
                const auto left = checkExpression(
                    *expression.left, declarations, operandExpected, visibility, result);
                const auto rightExpected = left.kind == ScalarType::Unknown ? operandExpected : left;
                const auto right = checkExpression(
                    *expression.right, declarations, rightExpected, visibility, result);
                if (isLogicalType(left.kind) && compatible(left, right))
                    return record(left);
                if (isNumericVectorType(left.kind) || isNumericVectorType(right.kind)) {
                    if (!visibility.numericStd) {
                        result.diagnostics.push_back(
                            {"logical operator '" + expression.text +
                                 "' for numeric vectors requires ieee.numeric_std.all",
                             expression.span});
                        return record({});
                    }
                    if (left.kind == right.kind && left.range && right.range) {
                        const auto leftWidth = rangeWidth(*left.range);
                        const auto rightWidth = rangeWidth(*right.range);
                        if (leftWidth == 0 || rightWidth == 0) {
                            result.diagnostics.push_back(
                                {"logical operator '" + expression.text +
                                     "' requires non-null numeric vector operands",
                                 expression.span});
                            return record({});
                        }
                        if (leftWidth != rightWidth) {
                            result.diagnostics.push_back(
                                {"logical operator '" + expression.text +
                                     "' requires equally sized numeric vector operands",
                                 expression.span});
                            return record({});
                        }
                        const auto range = canonicalRange(leftWidth, expression.span);
                        if (!range) {
                            result.diagnostics.push_back(
                                {"logical operator '" + expression.text +
                                     "' result width exceeds the supported range",
                                 expression.span});
                            return record({});
                        }
                        return record({left.kind, *range});
                    }
                    if (left.kind == ScalarType::Unknown ||
                        right.kind == ScalarType::Unknown)
                        return record({});
                    result.diagnostics.push_back(
                        {"logical operator '" + expression.text +
                             "' requires matching numeric vector operands",
                         expression.span});
                    return record({});
                }
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"logical operator '" + expression.text +
                         "' requires matching logical operands", expression.span});
                return record({});
            }
            if (expression.text == "=" || expression.text == "/=") {
                const bool leftCharacter =
                    expression.left->kind == Expression::Kind::CharacterLiteral;
                const bool rightCharacter =
                    expression.right->kind == Expression::Kind::CharacterLiteral;
                if (leftCharacter && rightCharacter) {
                    result.diagnostics.push_back(
                        {"equality between character literals needs a declared scalar context",
                         expression.span});
                    checkExpression(*expression.left, declarations,
                                    {}, visibility, result);
                    checkExpression(*expression.right, declarations,
                                    {}, visibility, result);
                    return record({});
                }
                CheckedType left;
                CheckedType right;
                if (leftCharacter && !rightCharacter) {
                    right = checkExpression(*expression.right, declarations,
                                            {}, visibility, result);
                    left = checkExpression(
                        *expression.left, declarations, right, visibility, result);
                } else {
                    left = checkExpression(*expression.left, declarations,
                                           {}, visibility, result);
                    right = checkExpression(
                        *expression.right, declarations, left, visibility, result);
                }
                if (isNumericVectorType(left.kind) || isNumericVectorType(right.kind)) {
                    if (!visibility.numericStd) {
                        result.diagnostics.push_back(
                            {"operator '" + expression.text +
                                 "' for numeric vectors requires ieee.numeric_std.all",
                             expression.span});
                        return record({});
                    }
                    const bool leftVector = isNumericVectorType(left.kind);
                    const bool rightVector = isNumericVectorType(right.kind);
                    if (leftVector && rightVector && left.kind == right.kind &&
                        left.range && right.range) {
                        if (rangeWidth(*left.range) == 0 ||
                            rangeWidth(*right.range) == 0) {
                            result.diagnostics.push_back(
                                {"operator '" + expression.text +
                                     "' requires non-null numeric vector operands",
                                 expression.span});
                            return record({});
                        }
                        return record({ScalarType::Boolean, std::nullopt});
                    }
                    if (leftVector != rightVector) {
                        const auto& vector = leftVector ? left : right;
                        const auto& scalar = leftVector ? right : left;
                        const auto& scalarExpression = leftVector
                            ? *expression.right : *expression.left;
                        const bool naturalScalar = scalar.kind == ScalarType::Natural ||
                            (scalar.kind == ScalarType::Integer &&
                             scalarExpression.kind == Expression::Kind::IntegerLiteral);
                        const bool integerScalar = scalar.kind == ScalarType::Integer ||
                                                   scalar.kind == ScalarType::Natural;
                        const bool validScalar = vector.kind == ScalarType::Unsigned
                            ? naturalScalar : integerScalar;
                        if (validScalar && vector.range) {
                            if (rangeWidth(*vector.range) == 0) {
                                result.diagnostics.push_back(
                                    {"operator '" + expression.text +
                                         "' requires a non-null numeric vector operand",
                                     expression.span});
                                return record({});
                            }
                            if (vector.kind == ScalarType::Unsigned &&
                                scalarExpression.kind == Expression::Kind::IntegerLiteral)
                                result.expressionTypes[&scalarExpression] =
                                    ScalarType::Natural;
                            return record({ScalarType::Boolean, std::nullopt});
                        }
                    }
                    if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                        return record({});
                    result.diagnostics.push_back(
                        {"operator '" + expression.text +
                             "' requires matching numeric vectors or a compatible scalar operand",
                         expression.span});
                    return record({});
                }
                if (compatible(left, right))
                    return record({ScalarType::Boolean, std::nullopt});
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"equality operator requires matching supported scalar operands",
                     expression.span});
                return record({});
            }
            if (expression.text == "<" || expression.text == "<=" ||
                expression.text == ">" || expression.text == ">=") {
                const auto left = checkExpression(
                    *expression.left, declarations, {}, visibility, result);
                const auto rightExpected = isNumericVectorType(left.kind) ? left : CheckedType{};
                const auto right = checkExpression(
                    *expression.right, declarations, rightExpected, visibility, result);
                const bool hasNumericOperand =
                    isNumericVectorType(left.kind) || isNumericVectorType(right.kind);
                if (!hasNumericOperand) {
                    if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                        return record({});
                    result.diagnostics.push_back(
                        {"relational operator '" + expression.text +
                             "' is currently supported only for numeric vectors",
                         expression.span});
                    return record({});
                }
                if (!visibility.numericStd) {
                    result.diagnostics.push_back(
                        {"operator '" + expression.text +
                             "' for numeric vectors requires ieee.numeric_std.all",
                         expression.span});
                    return record({});
                }
                const bool leftVector = isNumericVectorType(left.kind);
                const bool rightVector = isNumericVectorType(right.kind);
                if (leftVector && rightVector && left.kind == right.kind &&
                    left.range && right.range) {
                    if (rangeWidth(*left.range) == 0 ||
                        rangeWidth(*right.range) == 0) {
                        result.diagnostics.push_back(
                            {"operator '" + expression.text +
                                 "' requires non-null numeric vector operands",
                             expression.span});
                        return record({});
                    }
                    return record({ScalarType::Boolean, std::nullopt});
                }
                if (leftVector != rightVector) {
                    const auto& vector = leftVector ? left : right;
                    const auto& scalar = leftVector ? right : left;
                    const auto& scalarExpression = leftVector
                        ? *expression.right : *expression.left;
                    const bool naturalScalar = scalar.kind == ScalarType::Natural ||
                        (scalar.kind == ScalarType::Integer &&
                         scalarExpression.kind == Expression::Kind::IntegerLiteral);
                    const bool integerScalar = scalar.kind == ScalarType::Integer ||
                                               scalar.kind == ScalarType::Natural;
                    const bool validScalar = vector.kind == ScalarType::Unsigned
                        ? naturalScalar : integerScalar;
                    if (validScalar && vector.range) {
                        if (rangeWidth(*vector.range) == 0) {
                            result.diagnostics.push_back(
                                {"operator '" + expression.text +
                                     "' requires a non-null numeric vector operand",
                                 expression.span});
                            return record({});
                        }
                        if (vector.kind == ScalarType::Unsigned &&
                            scalarExpression.kind == Expression::Kind::IntegerLiteral)
                            result.expressionTypes[&scalarExpression] = ScalarType::Natural;
                        return record({ScalarType::Boolean, std::nullopt});
                    }
                }
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"operator '" + expression.text +
                         "' requires matching numeric vectors or a compatible scalar operand",
                     expression.span});
                return record({});
            }
            if (expression.text == "+" || expression.text == "-") {
                const auto left = checkExpression(
                    *expression.left, declarations, {}, visibility, result);
                const auto right = checkExpression(
                    *expression.right, declarations, {}, visibility, result);
                const bool hasNumericOperand =
                    isNumericVectorType(left.kind) || isNumericVectorType(right.kind);
                if (hasNumericOperand && !visibility.numericStd) {
                    result.diagnostics.push_back(
                        {"operator '" + expression.text +
                             "' for numeric vectors requires ieee.numeric_std.all",
                         expression.span});
                    return record({});
                }
                if (isNumericVectorType(left.kind) &&
                    left.kind == right.kind && left.range && right.range) {
                    const auto leftWidth = rangeWidth(*left.range);
                    const auto rightWidth = rangeWidth(*right.range);
                    if (leftWidth == 0 || rightWidth == 0) {
                        result.diagnostics.push_back(
                            {"operator '" + expression.text +
                                 "' requires non-null numeric vector operands",
                             expression.span});
                        return record({});
                    }
                    const auto width = std::max(leftWidth, rightWidth);
                    const auto range = canonicalRange(width, expression.span);
                    if (!range) {
                        result.diagnostics.push_back(
                            {"operator '" + expression.text +
                                 "' result width exceeds the supported range",
                             expression.span});
                        return record({});
                    }
                    return record({left.kind, *range});
                }
                const bool leftVector = isNumericVectorType(left.kind);
                const bool rightVector = isNumericVectorType(right.kind);
                if (leftVector != rightVector) {
                    const auto& vector = leftVector ? left : right;
                    const auto& scalar = leftVector ? right : left;
                    const auto& scalarExpression = leftVector
                        ? *expression.right : *expression.left;
                    const bool naturalScalar = scalar.kind == ScalarType::Natural ||
                        (scalar.kind == ScalarType::Integer &&
                         scalarExpression.kind == Expression::Kind::IntegerLiteral);
                    const bool integerScalar = scalar.kind == ScalarType::Integer ||
                                               scalar.kind == ScalarType::Natural;
                    const bool validScalar = vector.kind == ScalarType::Unsigned
                        ? naturalScalar : integerScalar;
                    if (validScalar && vector.range) {
                        const auto width = rangeWidth(*vector.range);
                        if (width == 0) {
                            result.diagnostics.push_back(
                                {"operator '" + expression.text +
                                     "' requires a non-null numeric vector operand",
                                 expression.span});
                            return record({});
                        }
                        const auto range = canonicalRange(width, expression.span);
                        if (!range) {
                            result.diagnostics.push_back(
                                {"operator '" + expression.text +
                                     "' result width exceeds the supported range",
                                 expression.span});
                            return record({});
                        }
                        if (vector.kind == ScalarType::Unsigned &&
                            scalarExpression.kind == Expression::Kind::IntegerLiteral)
                            result.expressionTypes[&scalarExpression] = ScalarType::Natural;
                        return record({vector.kind, *range});
                    }
                }
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"operator '" + expression.text +
                         "' requires matching numeric vectors or a compatible scalar operand",
                     expression.span});
                return record({});
            }
            if (expression.text == "*") {
                const auto operandExpected =
                    isNumericVectorType(expected.kind) ? expected : CheckedType{};
                const auto left = checkExpression(
                    *expression.left, declarations, operandExpected, visibility, result);
                const auto rightExpected =
                    left.kind == ScalarType::Unknown ? operandExpected : left;
                const auto right = checkExpression(
                    *expression.right, declarations, rightExpected, visibility, result);
                const bool hasNumericOperand =
                    isNumericVectorType(left.kind) || isNumericVectorType(right.kind);
                if (!hasNumericOperand) {
                    if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                        return record({});
                    result.diagnostics.push_back(
                        {"binary operator '*' is not supported by scalar type analysis",
                         expression.span});
                    return record({});
                }
                if (!visibility.numericStd) {
                    result.diagnostics.push_back(
                        {"operator '*' for numeric vectors requires ieee.numeric_std.all",
                         expression.span});
                    return record({});
                }
                if (left.kind == right.kind && left.range && right.range) {
                    const auto leftWidth = rangeWidth(*left.range);
                    const auto rightWidth = rangeWidth(*right.range);
                    if (leftWidth == 0 || rightWidth == 0) {
                        result.diagnostics.push_back(
                            {"operator '*' requires non-null numeric vector operands",
                             expression.span});
                        return record({});
                    }
                    constexpr auto maxResultWidth = static_cast<std::uint64_t>(
                        std::numeric_limits<std::int64_t>::max()) + 1;
                    if (rightWidth > maxResultWidth ||
                        leftWidth > maxResultWidth - rightWidth) {
                        result.diagnostics.push_back(
                            {"operator '*' result width exceeds the supported range",
                             expression.span});
                        return record({});
                    }
                    const auto width = leftWidth + rightWidth;
                    return record({left.kind, *canonicalRange(width, expression.span)});
                }
                const bool leftVector = isNumericVectorType(left.kind);
                const bool rightVector = isNumericVectorType(right.kind);
                if (leftVector != rightVector) {
                    const auto& vector = leftVector ? left : right;
                    const auto& scalar = leftVector ? right : left;
                    const auto& scalarExpression = leftVector
                        ? *expression.right : *expression.left;
                    const bool naturalScalar = scalar.kind == ScalarType::Natural ||
                        (scalar.kind == ScalarType::Integer &&
                         scalarExpression.kind == Expression::Kind::IntegerLiteral);
                    const bool integerScalar = scalar.kind == ScalarType::Integer ||
                                               scalar.kind == ScalarType::Natural;
                    const bool validScalar = vector.kind == ScalarType::Unsigned
                        ? naturalScalar : integerScalar;
                    if (validScalar && vector.range) {
                        const auto vectorWidth = rangeWidth(*vector.range);
                        if (vectorWidth == 0) {
                            result.diagnostics.push_back(
                                {"operator '*' requires a non-null numeric vector operand",
                                 expression.span});
                            return record({});
                        }
                        constexpr auto maxResultWidth = static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max()) + 1;
                        if (vectorWidth > maxResultWidth / 2) {
                            result.diagnostics.push_back(
                                {"operator '*' result width exceeds the supported range",
                                 expression.span});
                            return record({});
                        }
                        if (vector.kind == ScalarType::Unsigned &&
                            scalarExpression.kind == Expression::Kind::IntegerLiteral)
                            result.expressionTypes[&scalarExpression] = ScalarType::Natural;
                        return record(
                            {vector.kind,
                             *canonicalRange(vectorWidth * 2, expression.span)});
                    }
                }
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"operator '*' requires matching numeric vectors or a compatible scalar operand",
                     expression.span});
                return record({});
            }
            if (expression.text == "/" || expression.text == "rem" ||
                expression.text == "mod") {
                const auto operandExpected =
                    isNumericVectorType(expected.kind) ? expected : CheckedType{};
                const auto left = checkExpression(
                    *expression.left, declarations, operandExpected, visibility, result);
                const auto rightExpected =
                    left.kind == ScalarType::Unknown ? operandExpected : left;
                const auto right = checkExpression(
                    *expression.right, declarations, rightExpected, visibility, result);
                const bool hasNumericOperand =
                    isNumericVectorType(left.kind) || isNumericVectorType(right.kind);
                if (!hasNumericOperand) {
                    if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                        return record({});
                    result.diagnostics.push_back(
                        {"binary operator '" + expression.text +
                             "' is not supported by scalar type analysis",
                         expression.span});
                    return record({});
                }
                if (!visibility.numericStd) {
                    result.diagnostics.push_back(
                        {"operator '" + expression.text +
                             "' for numeric vectors requires ieee.numeric_std.all",
                         expression.span});
                    return record({});
                }
                if (left.kind == right.kind && left.range && right.range) {
                    const auto leftWidth = rangeWidth(*left.range);
                    const auto rightWidth = rangeWidth(*right.range);
                    if (leftWidth == 0 || rightWidth == 0) {
                        result.diagnostics.push_back(
                            {"operator '" + expression.text +
                                 "' requires non-null numeric vector operands",
                             expression.span});
                        return record({});
                    }
                    const auto width = expression.text == "/" ? leftWidth : rightWidth;
                    const auto range = canonicalRange(width, expression.span);
                    if (!range) {
                        result.diagnostics.push_back(
                            {"operator '" + expression.text +
                                 "' result width exceeds the supported range",
                             expression.span});
                        return record({});
                    }
                    return record({left.kind, *range});
                }
                const bool leftVector = isNumericVectorType(left.kind);
                const bool rightVector = isNumericVectorType(right.kind);
                if (leftVector != rightVector) {
                    const auto& vector = leftVector ? left : right;
                    const auto& scalar = leftVector ? right : left;
                    const auto& scalarExpression = leftVector
                        ? *expression.right : *expression.left;
                    const bool naturalScalar = scalar.kind == ScalarType::Natural ||
                        (scalar.kind == ScalarType::Integer &&
                         scalarExpression.kind == Expression::Kind::IntegerLiteral);
                    const bool integerScalar = scalar.kind == ScalarType::Integer ||
                                               scalar.kind == ScalarType::Natural;
                    const bool validScalar = vector.kind == ScalarType::Unsigned
                        ? naturalScalar : integerScalar;
                    if (validScalar && vector.range) {
                        const auto width = rangeWidth(*vector.range);
                        if (width == 0) {
                            result.diagnostics.push_back(
                                {"operator '" + expression.text +
                                     "' requires a non-null numeric vector operand",
                                 expression.span});
                            return record({});
                        }
                        const auto range = canonicalRange(width, expression.span);
                        if (!range) {
                            result.diagnostics.push_back(
                                {"operator '" + expression.text +
                                     "' result width exceeds the supported range",
                                 expression.span});
                            return record({});
                        }
                        if (vector.kind == ScalarType::Unsigned &&
                            scalarExpression.kind == Expression::Kind::IntegerLiteral)
                            result.expressionTypes[&scalarExpression] = ScalarType::Natural;
                        return record({vector.kind, *range});
                    }
                }
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"operator '" + expression.text +
                         "' requires matching numeric vectors or a compatible scalar operand",
                     expression.span});
                return record({});
            }
            // Visit operands so name errors are still complete before reporting
            // the unsupported operator itself.
            checkExpression(*expression.left, declarations, {}, visibility, result);
            checkExpression(*expression.right, declarations, {}, visibility, result);
            result.diagnostics.push_back(
                {"binary operator '" + expression.text +
                     "' is not supported by scalar type analysis", expression.span});
            return record({});
        }
        case Expression::Kind::Conditional: {
            const auto condition = checkExpression(
                *expression.condition, declarations,
                {ScalarType::Boolean, std::nullopt}, visibility, result);
            const auto whenTrue = checkExpression(
                *expression.left, declarations, expected, visibility, result);
            const auto whenFalse = checkExpression(
                *expression.right, declarations, expected, visibility, result);
            if (condition.kind == ScalarType::Boolean && compatible(whenTrue, whenFalse))
                return record(expected.kind != ScalarType::Unknown &&
                              compatible(expected, whenTrue) ? expected : whenTrue);
            if (condition.kind == ScalarType::Unknown ||
                whenTrue.kind == ScalarType::Unknown || whenFalse.kind == ScalarType::Unknown)
                return record({});
            result.diagnostics.push_back(
                {"conditional expression requires a boolean condition and matching branches",
                 expression.span});
            return record({});
        }
    }
    return record({});
}

} // namespace

AnalysisResult Analyzer::analyze(const DesignFile& syntax) {
    AnalysisResult result;
    for (const auto& package : syntax.packages)
        result.diagnostics.push_back({"packages require RTL elaboration", package.name.span});
    for (const auto& architecture : syntax.architectures) {
        if (!architecture.functions.empty())
            result.diagnostics.push_back({"architecture functions require RTL elaboration", architecture.span});
        if (!architecture.enumerationTypes.empty())
            result.diagnostics.push_back({"enumerated types require RTL elaboration", architecture.span});
        if (!architecture.recordTypes.empty())
            result.diagnostics.push_back({"record types require RTL elaboration", architecture.span});
        if (!architecture.constants.empty())
            result.diagnostics.push_back({"architecture constants require RTL elaboration", architecture.span});
        if (!architecture.generates.empty())
            result.diagnostics.push_back({"generate statements require RTL elaboration", architecture.span});
        for (const auto& instance : architecture.instantiations)
            if (std::any_of(instance.actualOpen.begin(), instance.actualOpen.end(),
                [](bool open) { return open; }) || std::any_of(instance.actualLiterals.begin(), instance.actualLiterals.end(),
                [](const auto& literal) { return bool(literal); }) || std::any_of(instance.actualIndices.begin(), instance.actualIndices.end(),
                    [](const auto& indices) { return !indices.empty(); }) || instance.component || std::any_of(instance.formals.begin(), instance.formals.end(),
                    [](const auto& formal) { return formal.has_value(); }))
                result.diagnostics.push_back({"named/component binding requires RTL elaboration", instance.span});
    }
    std::unordered_map<std::string, const EntityDeclaration*> entities;
    std::unordered_map<const EntityDeclaration*, Visibility> entityVisibility;
    std::unordered_map<const EntityDeclaration*, GenericValues> entityDefaults;
    for (const auto& entity : syntax.entities) {
        for (const auto& port : entity.ports)
            if (port.defaultValue) result.diagnostics.push_back({"port defaults require RTL elaboration", port.span});
        const auto visibility = analyzeContext(entity.context, result);
        entityVisibility.emplace(&entity, visibility);
        const std::string entityName(key(entity.name));
        if (!entities.emplace(entityName, &entity).second) {
            result.diagnostics.push_back(
                {"duplicate entity declaration '" + entity.name.spelling + "'",
                 entity.name.span});
        }

        GenericValues defaults;
        std::unordered_set<std::string> genericNames;
        for (const auto& generic : entity.generics) {
            const auto genericType = declarationType(generic.type, visibility);
            if (genericType.kind != ScalarType::Integer &&
                genericType.kind != ScalarType::Natural) {
                result.diagnostics.push_back(
                    {"entity generics currently require integer, natural, or positive type",
                     generic.type.name.span});
            }
            for (const auto& name : generic.names) {
                const auto genericName = std::string(key(name));
                if (!genericNames.insert(genericName).second) {
                    result.diagnostics.push_back(
                        {"duplicate generic declaration '" + name.spelling + "'", name.span});
                    continue;
                }
                if (generic.defaultValue) {
                    const auto value = evaluateInteger(*generic.defaultValue, defaults);
                    if (!value) {
                        result.diagnostics.push_back(
                            {"generic default for '" + name.spelling +
                                 "' is not a locally static integer expression",
                             generic.defaultValue->span});
                    } else if ((generic.type.name.canonical == "natural" && *value < 0) ||
                               (generic.type.name.canonical == "positive" && *value <= 0)) {
                        result.diagnostics.push_back(
                            {"generic default for '" + name.spelling +
                                 "' is outside its subtype",
                             generic.defaultValue->span});
                    } else {
                        defaults.emplace(genericName, *value);
                    }
                }
            }
        }
        entityDefaults.emplace(&entity, defaults);

        std::unordered_set<std::string> ports = genericNames;
        for (const auto& port : entity.ports) {
            const auto portType = declarationType(port.type, visibility, defaults);
            if (portType.kind == ScalarType::Unknown) {
                result.diagnostics.push_back(
                    {"unsupported or invisible type mark '" + port.type.name.spelling + "'",
                     port.type.name.span});
            }
            if (port.type.constraint && portType.range)
                result.resolvedTypeRanges[&port.type] = *portType.range;
            for (const auto& name : port.names) {
                if (!ports.insert(std::string(key(name))).second) {
                    result.diagnostics.push_back(
                        {"duplicate port declaration '" + name.spelling + "'", name.span});
                }
            }
        }
    }

    std::unordered_set<std::string> architectures;
    for (const auto& architecture : syntax.architectures) {
        const auto architectureVisibility = analyzeContext(architecture.context, result);
        if (!architecture.components.empty())
            result.diagnostics.push_back({"local components require RTL elaboration", architecture.span});
        const std::string entityName(key(architecture.entity));
        const auto entity = entities.find(entityName);
        if (entity == entities.end()) {
            result.diagnostics.push_back(
                {"no entity declaration for '" + architecture.entity.spelling + "'",
                 architecture.entity.span});
            continue;
        }

        std::string architectureKey = entityName;
        architectureKey.push_back('\0');
        architectureKey.append(key(architecture.name));
        if (!architectures.insert(architectureKey).second) {
            result.diagnostics.push_back(
                {"duplicate architecture '" + architecture.name.spelling + "' for entity '" +
                     architecture.entity.spelling + "'",
                 architecture.name.span});
        }

        Declarations declarations;
        const auto& architectureGenericValues = entityDefaults.at(entity->second);
        const auto portVisibility = entityVisibility.at(entity->second);
        for (const auto& generic : entity->second->generics) {
            const auto type = declarationType(generic.type, portVisibility);
            for (const auto& name : generic.names)
                declarations.emplace(std::string(key(name)), type);
        }
        for (const auto& port : entity->second->ports) {
            const auto type = declarationType(
                port.type, portVisibility, architectureGenericValues);
            if (port.type.constraint && type.range)
                result.resolvedTypeRanges[&port.type] = *type.range;
            for (const auto& name : port.names)
                declarations.emplace(std::string(key(name)), type);
        }
        for (const auto& signal : architecture.signals) {
            if (signal.initializer)
                result.diagnostics.push_back({"signal initialization requires RTL elaboration", signal.span});
            const auto signalType = declarationType(
                signal.type, architectureVisibility, architectureGenericValues);
            if (signalType.kind == ScalarType::Unknown) {
                result.diagnostics.push_back(
                    {"unsupported or invisible type mark '" + signal.type.name.spelling + "'",
                     signal.type.name.span});
            }
            if (signal.type.constraint && signalType.range)
                result.resolvedTypeRanges[&signal.type] = *signalType.range;
            for (const auto& name : signal.names) {
                if (!declarations.emplace(std::string(key(name)), signalType).second)
                    result.diagnostics.push_back(
                        {"duplicate signal declaration '" + name.spelling + "'", name.span});
            }
        }
        std::unordered_set<std::string> instanceLabels;
        for (const auto& instantiation : architecture.instantiations) {
            if (!instanceLabels.insert(std::string(key(instantiation.label))).second) {
                result.diagnostics.push_back(
                    {"duplicate instance label '" + instantiation.label.spelling + "'",
                     instantiation.label.span});
            }
            if (key(instantiation.library) != "work") {
                result.diagnostics.push_back(
                    {"only direct entity instantiation from library 'work' is supported",
                     instantiation.library.span});
                continue;
            }
            const auto model = entities.find(std::string(key(instantiation.entity)));
            if (model == entities.end()) {
                result.diagnostics.push_back(
                    {"no entity declaration for instantiated entity '" +
                         instantiation.entity.spelling + "'",
                     instantiation.entity.span});
                continue;
            }
            GenericValues genericValues;
            std::unordered_map<std::string, const GenericAssociation*> namedActuals;
            std::vector<const GenericAssociation*> positionalActuals;
            bool sawNamed = false;
            for (const auto& association : instantiation.generics) {
                if (association.formal) {
                    sawNamed = true;
                    if (!namedActuals.emplace(
                            std::string(key(*association.formal)), &association).second) {
                        result.diagnostics.push_back(
                            {"duplicate generic association for '" +
                                 association.formal->spelling + "'",
                             association.formal->span});
                    }
                } else {
                    if (sawNamed) {
                        result.diagnostics.push_back(
                            {"positional generic association cannot follow a named association",
                             association.span});
                    }
                    positionalActuals.push_back(&association);
                }
            }
            std::size_t positionalIndex = 0;
            std::unordered_set<std::string> formalGenericNames;
            for (const auto& generic : model->second->generics) {
                for (const auto& name : generic.names) {
                    const auto genericName = std::string(key(name));
                    formalGenericNames.insert(genericName);
                    const GenericAssociation* association = nullptr;
                    const auto named = namedActuals.find(genericName);
                    if (named != namedActuals.end())
                        association = named->second;
                    else if (positionalIndex < positionalActuals.size())
                        association = positionalActuals[positionalIndex++];
                    std::optional<std::int64_t> value;
                    SourceSpan valueSpan = name.span;
                    if (association) {
                        value = evaluateInteger(
                            *association->actual, architectureGenericValues);
                        valueSpan = association->actual->span;
                    } else if (generic.defaultValue) {
                        value = evaluateInteger(*generic.defaultValue, genericValues);
                        valueSpan = generic.defaultValue->span;
                    }
                    if (!value) {
                        result.diagnostics.push_back(
                            {"generic '" + name.spelling +
                                 "' requires a locally static integer actual or default",
                             valueSpan});
                        continue;
                    }
                    if ((generic.type.name.canonical == "natural" && *value < 0) ||
                        (generic.type.name.canonical == "positive" && *value <= 0)) {
                        result.diagnostics.push_back(
                            {"generic actual for '" + name.spelling +
                                 "' is outside its subtype",
                             valueSpan});
                        continue;
                    }
                    genericValues.emplace(genericName, *value);
                }
            }
            if (positionalIndex != positionalActuals.size()) {
                result.diagnostics.push_back(
                    {"too many positional generic actuals for entity '" +
                         instantiation.entity.spelling + "'",
                     instantiation.span});
            }
            for (const auto& [name, association] : namedActuals) {
                if (!formalGenericNames.contains(name)) {
                    result.diagnostics.push_back(
                        {"no generic named '" + association->formal->spelling +
                             "' on entity '" + instantiation.entity.spelling + "'",
                         association->formal->span});
                }
            }
            result.genericValues[&instantiation] = genericValues;
            std::vector<std::pair<const Name*, CheckedType>> formals;
            const auto modelVisibility = entityVisibility.at(model->second);
            for (const auto& port : model->second->ports) {
                const auto type = declarationType(
                    port.type, modelVisibility, genericValues);
                if (port.type.constraint && type.range)
                    result.specializedTypeRanges[&instantiation][&port.type] = *type.range;
                for (const auto& name : port.names)
                    formals.emplace_back(&name, type);
            }
            if (formals.size() != instantiation.actuals.size()) {
                result.diagnostics.push_back(
                    {"port map for entity '" + instantiation.entity.spelling + "' has " +
                         std::to_string(instantiation.actuals.size()) + " actuals but " +
                         std::to_string(formals.size()) + " are required",
                     instantiation.span});
                continue;
            }
            for (std::size_t index = 0; index < formals.size(); ++index) {
                const auto& actual = instantiation.actuals[index];
                const auto declaration = declarations.find(std::string(key(actual)));
                if (declaration == declarations.end()) {
                    result.diagnostics.push_back(
                        {"no declaration for port-map actual '" + actual.spelling + "'",
                         actual.span});
                    continue;
                }
                if (!compatible(formals[index].second, declaration->second)) {
                    result.diagnostics.push_back(
                        {"port-map type mismatch for formal '" +
                             formals[index].first->spelling + "'",
                         actual.span});
                }
            }
        }
        const auto checkAssignment = [&](const Assignment& assignment) {
            if (assignment.selector) {
                result.diagnostics.push_back({"selected assignments require RTL elaboration", assignment.span});
                return;
            }
            if (!assignment.indices.empty()) {
                result.diagnostics.push_back(
                    {"indexed assignments require RTL elaboration", assignment.span});
                return;
            }
            const auto target = declarations.find(std::string(key(assignment.target)));
            if (target == declarations.end()) {
                result.diagnostics.push_back(
                    {"no declaration for assignment target '" + assignment.target.spelling + "'",
                     assignment.target.span});
                checkExpression(
                    *assignment.value, declarations, {}, architectureVisibility, result);
                return;
            }
            const auto value = checkExpression(
                *assignment.value, declarations, target->second,
                architectureVisibility, result);
            if (target->second.kind != ScalarType::Unknown &&
                value.kind != ScalarType::Unknown && !compatible(target->second, value)) {
                result.diagnostics.push_back(
                    {"assignment type mismatch: target is '" +
                         std::string(typeName(target->second.kind)) + "' but value is '" +
                         typeName(value.kind) + "'", assignment.span});
            }
        };
        for (const auto& assignment : architecture.assignments)
            checkAssignment(assignment);
        for (const auto& process : architecture.processes) {
            if (process.combinational || !process.sensitivityList.empty()) {
                result.diagnostics.push_back(
                    {"structured processes require RTL elaboration", process.span});
                continue;
            }
            auto localDeclarations = declarations;
            std::unordered_set<std::string> variables;
            for (const auto& declaration : process.variables) {
                const auto variableType = declarationType(
                    declaration.type, architectureVisibility);
                if (variableType.kind == ScalarType::Unknown) {
                    result.diagnostics.push_back(
                        {"unsupported or invisible type mark '" +
                             declaration.type.name.spelling + "'",
                         declaration.type.name.span});
                }
                for (const auto& name : declaration.names) {
                    const std::string variable(key(name));
                    if (!variables.insert(variable).second)
                        result.diagnostics.push_back(
                            {"duplicate variable declaration '" + name.spelling + "'", name.span});
                    // Shadowing needs proper scoped binding throughout the adapter.
                    if (!localDeclarations.emplace(variable, variableType).second &&
                        declarations.contains(variable))
                        result.diagnostics.push_back(
                            {"variable shadowing is not supported: '" + name.spelling + "'", name.span});
                }
            }
            for (const auto* name : {&process.sensitivity, &process.eventSignal,
                                     &process.levelSignal}) {
                const auto found = declarations.find(std::string(key(*name)));
                if (found == declarations.end())
                    result.diagnostics.push_back(
                        {"no declaration for clock name '" + name->spelling + "'", name->span});
                else if (found->second.kind != ScalarType::Bit &&
                         found->second.kind != ScalarType::StdLogic)
                    result.diagnostics.push_back(
                        {"clock name must have scalar logic type: '" + name->spelling + "'",
                         name->span});
            }
            if (process.enableSignal) {
                const auto found = declarations.find(std::string(key(*process.enableSignal)));
                if (found == declarations.end())
                    result.diagnostics.push_back(
                        {"no declaration for enable name '" +
                             process.enableSignal->spelling + "'",
                         process.enableSignal->span});
                else if (found->second.kind != ScalarType::Bit &&
                         found->second.kind != ScalarType::StdLogic)
                    result.diagnostics.push_back(
                        {"enable name must have scalar logic type: '" +
                             process.enableSignal->spelling + "'",
                         process.enableSignal->span});
            }
            if (process.resetSignal) {
                const auto found = declarations.find(std::string(key(*process.resetSignal)));
                if (found == declarations.end())
                    result.diagnostics.push_back(
                        {"no declaration for reset name '" +
                             process.resetSignal->spelling + "'",
                         process.resetSignal->span});
                else if (found->second.kind != ScalarType::Bit &&
                         found->second.kind != ScalarType::StdLogic)
                    result.diagnostics.push_back(
                        {"reset name must have scalar logic type: '" +
                             process.resetSignal->spelling + "'",
                         process.resetSignal->span});
            }
            const auto checkProcessAssignment = [&](const Assignment& assignment) {
                if (!assignment.indices.empty()) {
                    result.diagnostics.push_back(
                        {"indexed assignments require RTL elaboration", assignment.span});
                    return;
                }
                const std::string target(key(assignment.target));
                const auto targetDeclaration = localDeclarations.find(target);
                if (targetDeclaration == localDeclarations.end())
                    result.diagnostics.push_back(
                        {"no declaration for assignment target '" + assignment.target.spelling + "'",
                         assignment.target.span});
                else if ((assignment.kind == AssignmentKind::Variable) != variables.contains(target))
                    result.diagnostics.push_back(
                        {"assignment operator does not match object class for '" +
                             assignment.target.spelling + "'", assignment.target.span});
                const auto expected = targetDeclaration == localDeclarations.end()
                    ? CheckedType{} : targetDeclaration->second;
                const auto value = checkExpression(
                    *assignment.value, localDeclarations, expected,
                    architectureVisibility, result);
                if (expected.kind != ScalarType::Unknown &&
                    value.kind != ScalarType::Unknown && !compatible(expected, value))
                    result.diagnostics.push_back(
                        {"assignment type mismatch: target is '" +
                             std::string(typeName(expected.kind)) + "' but value is '" +
                             typeName(value.kind) + "'", assignment.span});
            };
            for (const auto& assignment : process.assignments)
                checkProcessAssignment(assignment);
            for (const auto& assignment : process.resetAssignments)
                checkProcessAssignment(assignment);
        }
    }
    return result;
}

ScheduleResult Analyzer::schedule(const ClockedProcess& process) {
    ScheduleResult result;
    if (process.combinational || !process.sensitivityList.empty()) {
        result.diagnostics.push_back({"structured processes require RTL elaboration", process.span});
        return result;
    }
    std::unordered_set<std::string> variables;
    for (const auto& declaration : process.variables)
        for (const auto& name : declaration.names)
            variables.emplace(key(name));
    std::unordered_map<std::string, std::string> values;
    std::unordered_set<std::string> retained;
    for (const auto& assignment : process.assignments) {
        if (!assignment.indices.empty()) {
            result.diagnostics.push_back({"indexed assignments require RTL elaboration", assignment.span});
            result.writes.clear();
            return result;
        }
        const auto& expression = *assignment.value;
        if (expression.kind != Expression::Kind::Name) {
            result.diagnostics.push_back({"scheduled data values must be scalar names", expression.span});
            continue;
        }
        std::string source = expression.canonical.empty() ? expression.text : expression.canonical;
        if (variables.contains(source)) {
            const auto value = values.find(source);
            if (value == values.end()) {
                retained.insert(source);
            } else {
                source = value->second;
            }
        }
        const std::string target(key(assignment.target));
        if (assignment.kind == AssignmentKind::Variable)
            values[target] = source;
        else
            result.writes.push_back({target, source, assignment.span, AssignmentKind::Signal});
    }
    for (const auto& declaration : process.variables) {
        for (const auto& name : declaration.names) {
            const std::string variable(key(name));
            if (!retained.contains(variable))
                continue;
            result.retainedVariables.push_back(variable);
            const auto value = values.find(variable);
            if (value == values.end()) {
                result.diagnostics.push_back(
                    {"retained variable must be assigned on every activation: '" +
                         name.spelling + "'", name.span});
                continue;
            }
            if (retained.contains(value->second)) {
                result.diagnostics.push_back(
                    {"retained variable next value must resolve to a non-retained scalar name: '" +
                         name.spelling + "'", name.span});
                continue;
            }
            result.writes.push_back(
                {variable, value->second, name.span, AssignmentKind::Variable});
        }
    }
    if (result.hasErrors()) {
        result.writes.clear();
        result.retainedVariables.clear();
    }
    return result;
}

} // namespace vhdl
