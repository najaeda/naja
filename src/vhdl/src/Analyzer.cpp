// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Analyzer.h"

#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace vhdl {
namespace {

using Declarations = std::unordered_map<std::string, ScalarType>;

std::string_view key(const Name& name) {
    return name.canonical.empty() ? std::string_view(name.spelling)
                                  : std::string_view(name.canonical);
}

ScalarType declarationType(const TypeMark& type) {
    if (type.constraint)
        return ScalarType::Unknown;
    if (type.name.canonical == "bit")
        return ScalarType::Bit;
    if (type.name.canonical == "boolean")
        return ScalarType::Boolean;
    if (type.name.canonical == "integer")
        return ScalarType::Integer;
    if (type.name.canonical == "real")
        return ScalarType::Real;
    if (type.name.canonical == "string")
        return ScalarType::String;
    return ScalarType::Unknown;
}

bool isLogical(std::string_view op) {
    return op == "and" || op == "nand" || op == "or" || op == "nor" ||
           op == "xor" || op == "xnor";
}

const char* typeName(ScalarType type) {
    switch (type) {
        case ScalarType::Bit: return "bit";
        case ScalarType::Boolean: return "boolean";
        case ScalarType::Integer: return "integer";
        case ScalarType::Real: return "real";
        case ScalarType::String: return "string";
        case ScalarType::Unknown: return "unknown";
    }
    return "unknown";
}

ScalarType checkExpression(const Expression& expression,
                           const Declarations& declarations,
                           ScalarType expected,
                           AnalysisResult& result) {
    const auto record = [&](ScalarType type) {
        result.expressionTypes[&expression] = type;
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
                return record(ScalarType::Unknown);
            }
            if (found->second == ScalarType::Unknown) {
                result.diagnostics.push_back(
                    {"unsupported scalar type for name '" + expression.text + "'", expression.span});
            }
            return record(found->second);
        }
        case Expression::Kind::IntegerLiteral:
            return record(ScalarType::Integer);
        case Expression::Kind::RealLiteral:
            return record(ScalarType::Real);
        case Expression::Kind::StringLiteral:
            return record(ScalarType::String);
        case Expression::Kind::CharacterLiteral:
            if (expected == ScalarType::Bit &&
                (expression.text == "'0'" || expression.text == "'1'"))
                return record(ScalarType::Bit);
            if (expected == ScalarType::Unknown)
                return record(ScalarType::Unknown);
            result.diagnostics.push_back(
                {"character literal requires a supported scalar bit context", expression.span});
            return record(ScalarType::Unknown);
        case Expression::Kind::Unary: {
            const auto operand = checkExpression(*expression.left, declarations, expected, result);
            if (expression.text == "not" &&
                (operand == ScalarType::Bit || operand == ScalarType::Boolean))
                return record(operand);
            if (operand == ScalarType::Unknown)
                return record(ScalarType::Unknown);
            result.diagnostics.push_back(
                {"operator '" + expression.text + "' is not supported for scalar type '" +
                     typeName(operand) + "'", expression.span});
            return record(ScalarType::Unknown);
        }
        case Expression::Kind::Binary: {
            if (isLogical(expression.text)) {
                const auto operandExpected =
                    expected == ScalarType::Bit || expected == ScalarType::Boolean
                    ? expected : ScalarType::Unknown;
                const auto left = checkExpression(
                    *expression.left, declarations, operandExpected, result);
                const auto rightExpected = left == ScalarType::Unknown ? operandExpected : left;
                const auto right = checkExpression(
                    *expression.right, declarations, rightExpected, result);
                if ((left == ScalarType::Bit || left == ScalarType::Boolean) && left == right)
                    return record(left);
                if (left == ScalarType::Unknown || right == ScalarType::Unknown)
                    return record(ScalarType::Unknown);
                result.diagnostics.push_back(
                    {"logical operator '" + expression.text +
                         "' requires matching bit or boolean operands", expression.span});
                return record(ScalarType::Unknown);
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
                                    ScalarType::Unknown, result);
                    checkExpression(*expression.right, declarations,
                                    ScalarType::Unknown, result);
                    return record(ScalarType::Unknown);
                }
                ScalarType left = ScalarType::Unknown;
                ScalarType right = ScalarType::Unknown;
                if (leftCharacter && !rightCharacter) {
                    right = checkExpression(*expression.right, declarations,
                                            ScalarType::Unknown, result);
                    left = checkExpression(*expression.left, declarations, right, result);
                } else {
                    left = checkExpression(*expression.left, declarations,
                                           ScalarType::Unknown, result);
                    right = checkExpression(*expression.right, declarations, left, result);
                }
                if (left != ScalarType::Unknown && left == right)
                    return record(ScalarType::Boolean);
                if (left == ScalarType::Unknown || right == ScalarType::Unknown)
                    return record(ScalarType::Unknown);
                result.diagnostics.push_back(
                    {"equality operator requires matching supported scalar operands",
                     expression.span});
                return record(ScalarType::Unknown);
            }
            // Visit operands so name errors are still complete before reporting
            // the unsupported operator itself.
            checkExpression(*expression.left, declarations, ScalarType::Unknown, result);
            checkExpression(*expression.right, declarations, ScalarType::Unknown, result);
            result.diagnostics.push_back(
                {"binary operator '" + expression.text +
                     "' is not supported by scalar type analysis", expression.span});
            return record(ScalarType::Unknown);
        }
        case Expression::Kind::Conditional: {
            const auto condition = checkExpression(
                *expression.condition, declarations, ScalarType::Boolean, result);
            const auto whenTrue = checkExpression(
                *expression.left, declarations, expected, result);
            const auto whenFalse = checkExpression(
                *expression.right, declarations, expected, result);
            if (condition == ScalarType::Boolean && whenTrue != ScalarType::Unknown &&
                whenTrue == whenFalse)
                return record(whenTrue);
            if (condition == ScalarType::Unknown || whenTrue == ScalarType::Unknown ||
                whenFalse == ScalarType::Unknown)
                return record(ScalarType::Unknown);
            result.diagnostics.push_back(
                {"conditional expression requires a boolean condition and matching branches",
                 expression.span});
            return record(ScalarType::Unknown);
        }
    }
    return record(ScalarType::Unknown);
}

} // namespace

AnalysisResult Analyzer::analyze(const DesignFile& syntax) {
    AnalysisResult result;
    std::unordered_map<std::string, const EntityDeclaration*> entities;
    for (const auto& entity : syntax.entities) {
        const std::string entityName(key(entity.name));
        if (!entities.emplace(entityName, &entity).second) {
            result.diagnostics.push_back(
                {"duplicate entity declaration '" + entity.name.spelling + "'",
                 entity.name.span});
        }

        std::unordered_set<std::string> ports;
        for (const auto& port : entity.ports) {
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
        for (const auto& port : entity->second->ports) {
            for (const auto& name : port.names)
                declarations.emplace(std::string(key(name)), declarationType(port.type));
        }
        for (const auto& signal : architecture.signals) {
            for (const auto& name : signal.names) {
                if (!declarations.emplace(std::string(key(name)),
                                          declarationType(signal.type)).second)
                    result.diagnostics.push_back(
                        {"duplicate signal declaration '" + name.spelling + "'", name.span});
            }
        }
        const auto checkAssignment = [&](const Assignment& assignment) {
            const auto target = declarations.find(std::string(key(assignment.target)));
            if (target == declarations.end()) {
                result.diagnostics.push_back(
                    {"no declaration for assignment target '" + assignment.target.spelling + "'",
                     assignment.target.span});
                checkExpression(*assignment.value, declarations, ScalarType::Unknown, result);
                return;
            }
            const auto value = checkExpression(
                *assignment.value, declarations, target->second, result);
            if (target->second != ScalarType::Unknown && value != ScalarType::Unknown &&
                target->second != value) {
                result.diagnostics.push_back(
                    {"assignment type mismatch: target is '" +
                         std::string(typeName(target->second)) + "' but value is '" +
                         typeName(value) + "'", assignment.span});
            }
        };
        for (const auto& assignment : architecture.assignments)
            checkAssignment(assignment);
        for (const auto& process : architecture.processes) {
            auto localDeclarations = declarations;
            std::unordered_set<std::string> variables;
            for (const auto& declaration : process.variables) {
                for (const auto& name : declaration.names) {
                    const std::string variable(key(name));
                    if (!variables.insert(variable).second)
                        result.diagnostics.push_back(
                            {"duplicate variable declaration '" + name.spelling + "'", name.span});
                    // Shadowing needs proper scoped binding throughout the adapter.
                    if (!localDeclarations.emplace(variable, declarationType(declaration.type)).second &&
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
                else if (found->second != ScalarType::Bit)
                    result.diagnostics.push_back(
                        {"clock name must have scalar bit type: '" + name->spelling + "'",
                         name->span});
            }
            for (const auto& assignment : process.assignments) {
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
                    ? ScalarType::Unknown : targetDeclaration->second;
                const auto value = checkExpression(
                    *assignment.value, localDeclarations, expected, result);
                if (expected != ScalarType::Unknown && value != ScalarType::Unknown &&
                    expected != value)
                    result.diagnostics.push_back(
                        {"assignment type mismatch: target is '" +
                             std::string(typeName(expected)) + "' but value is '" +
                             typeName(value) + "'", assignment.span});
            }
        }
    }
    return result;
}

ScheduleResult Analyzer::schedule(const ClockedProcess& process) {
    ScheduleResult result;
    std::unordered_set<std::string> variables;
    for (const auto& declaration : process.variables)
        for (const auto& name : declaration.names)
            variables.emplace(key(name));
    std::unordered_map<std::string, std::string> values;
    std::unordered_set<std::string> retained;
    for (const auto& assignment : process.assignments) {
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
