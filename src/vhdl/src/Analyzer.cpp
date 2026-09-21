// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Analyzer.h"

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

std::string_view key(const Name& name) {
    return name.canonical.empty() ? std::string_view(name.spelling)
                                  : std::string_view(name.canonical);
}

CheckedType declarationType(const TypeMark& type) {
    if (type.name.canonical == "bit_vector" && type.constraint)
        return {ScalarType::BitVector, type.constraint};
    if (type.constraint)
        return {};
    if (type.name.canonical == "bit")
        return {ScalarType::Bit, std::nullopt};
    if (type.name.canonical == "boolean")
        return {ScalarType::Boolean, std::nullopt};
    if (type.name.canonical == "integer")
        return {ScalarType::Integer, std::nullopt};
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
    if (left.kind == ScalarType::Unknown || left.kind != right.kind)
        return false;
    if (left.kind != ScalarType::BitVector)
        return true;
    return left.range && right.range && rangeWidth(*left.range) == rangeWidth(*right.range);
}

bool isLogical(std::string_view op) {
    return op == "and" || op == "nand" || op == "or" || op == "nor" ||
           op == "xor" || op == "xnor";
}

const char* typeName(ScalarType type) {
    switch (type) {
        case ScalarType::Bit: return "bit";
        case ScalarType::BitVector: return "bit_vector";
        case ScalarType::Boolean: return "boolean";
        case ScalarType::Integer: return "integer";
        case ScalarType::Real: return "real";
        case ScalarType::String: return "string";
        case ScalarType::Unknown: return "unknown";
    }
    return "unknown";
}

CheckedType checkExpression(const Expression& expression,
                            const Declarations& declarations,
                            CheckedType expected,
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
        case Expression::Kind::IntegerLiteral:
            return record({ScalarType::Integer, std::nullopt});
        case Expression::Kind::RealLiteral:
            return record({ScalarType::Real, std::nullopt});
        case Expression::Kind::StringLiteral:
            return record({ScalarType::String, std::nullopt});
        case Expression::Kind::CharacterLiteral:
            if (expected.kind == ScalarType::Bit &&
                (expression.text == "'0'" || expression.text == "'1'"))
                return record({ScalarType::Bit, std::nullopt});
            if (expected.kind == ScalarType::Unknown)
                return record({});
            result.diagnostics.push_back(
                {"character literal requires a supported scalar bit context", expression.span});
            return record({});
        case Expression::Kind::Unary: {
            const auto operand = checkExpression(*expression.left, declarations, expected, result);
            if (expression.text == "not" &&
                (operand.kind == ScalarType::Bit || operand.kind == ScalarType::BitVector ||
                 operand.kind == ScalarType::Boolean))
                return record(operand);
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
                    expected.kind == ScalarType::Bit || expected.kind == ScalarType::BitVector ||
                    expected.kind == ScalarType::Boolean ? expected : CheckedType{};
                const auto left = checkExpression(
                    *expression.left, declarations, operandExpected, result);
                const auto rightExpected = left.kind == ScalarType::Unknown ? operandExpected : left;
                const auto right = checkExpression(
                    *expression.right, declarations, rightExpected, result);
                if ((left.kind == ScalarType::Bit || left.kind == ScalarType::BitVector ||
                     left.kind == ScalarType::Boolean) && compatible(left, right))
                    return record(left);
                if (left.kind == ScalarType::Unknown || right.kind == ScalarType::Unknown)
                    return record({});
                result.diagnostics.push_back(
                    {"logical operator '" + expression.text +
                         "' requires matching bit, bit_vector or boolean operands", expression.span});
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
                                    {}, result);
                    checkExpression(*expression.right, declarations,
                                    {}, result);
                    return record({});
                }
                CheckedType left;
                CheckedType right;
                if (leftCharacter && !rightCharacter) {
                    right = checkExpression(*expression.right, declarations,
                                            {}, result);
                    left = checkExpression(*expression.left, declarations, right, result);
                } else {
                    left = checkExpression(*expression.left, declarations,
                                           {}, result);
                    right = checkExpression(*expression.right, declarations, left, result);
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
            // Visit operands so name errors are still complete before reporting
            // the unsupported operator itself.
            checkExpression(*expression.left, declarations, {}, result);
            checkExpression(*expression.right, declarations, {}, result);
            result.diagnostics.push_back(
                {"binary operator '" + expression.text +
                     "' is not supported by scalar type analysis", expression.span});
            return record({});
        }
        case Expression::Kind::Conditional: {
            const auto condition = checkExpression(
                *expression.condition, declarations,
                {ScalarType::Boolean, std::nullopt}, result);
            const auto whenTrue = checkExpression(
                *expression.left, declarations, expected, result);
            const auto whenFalse = checkExpression(
                *expression.right, declarations, expected, result);
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
            std::vector<std::pair<const Name*, CheckedType>> formals;
            for (const auto& port : model->second->ports) {
                const auto type = declarationType(port.type);
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
            const auto target = declarations.find(std::string(key(assignment.target)));
            if (target == declarations.end()) {
                result.diagnostics.push_back(
                    {"no declaration for assignment target '" + assignment.target.spelling + "'",
                     assignment.target.span});
                checkExpression(*assignment.value, declarations, {}, result);
                return;
            }
            const auto value = checkExpression(
                *assignment.value, declarations, target->second, result);
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
                else if (found->second.kind != ScalarType::Bit)
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
                    ? CheckedType{} : targetDeclaration->second;
                const auto value = checkExpression(
                    *assignment.value, localDeclarations, expected, result);
                if (expected.kind != ScalarType::Unknown &&
                    value.kind != ScalarType::Unknown && !compatible(expected, value))
                    result.diagnostics.push_back(
                        {"assignment type mismatch: target is '" +
                             std::string(typeName(expected.kind)) + "' but value is '" +
                             typeName(value.kind) + "'", assignment.span});
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
