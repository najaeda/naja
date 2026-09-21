// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "vhdl/Analyzer.h"

#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace vhdl {
namespace {

std::string_view key(const Name& name) {
    return name.canonical.empty() ? std::string_view(name.spelling)
                                  : std::string_view(name.canonical);
}

void checkExpression(const Expression& expression,
                     const std::unordered_set<std::string>& declarations,
                     AnalysisResult& result) {
    if (expression.kind == Expression::Kind::Name &&
        !declarations.contains(expression.canonical.empty()
                                   ? expression.text
                                   : expression.canonical)) {
        result.diagnostics.push_back(
            {"no declaration for name '" + expression.text + "'", expression.span});
    }
    if (expression.left)
        checkExpression(*expression.left, declarations, result);
    if (expression.right)
        checkExpression(*expression.right, declarations, result);
    if (expression.condition)
        checkExpression(*expression.condition, declarations, result);
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

        std::unordered_set<std::string> declarations;
        for (const auto& port : entity->second->ports) {
            for (const auto& name : port.names)
                declarations.insert(std::string(key(name)));
        }
        for (const auto& signal : architecture.signals) {
            for (const auto& name : signal.names) {
                if (!declarations.insert(std::string(key(name))).second)
                    result.diagnostics.push_back(
                        {"duplicate signal declaration '" + name.spelling + "'", name.span});
            }
        }
        const auto checkAssignment = [&](const Assignment& assignment) {
            if (!declarations.contains(std::string(key(assignment.target)))) {
                result.diagnostics.push_back(
                    {"no declaration for assignment target '" + assignment.target.spelling + "'",
                     assignment.target.span});
            }
            checkExpression(*assignment.value, declarations, result);
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
                    if (!localDeclarations.insert(variable).second && declarations.contains(variable))
                        result.diagnostics.push_back(
                            {"variable shadowing is not supported: '" + name.spelling + "'", name.span});
                }
            }
            for (const auto* name : {&process.sensitivity, &process.eventSignal,
                                     &process.levelSignal}) {
                if (!declarations.contains(std::string(key(*name))))
                    result.diagnostics.push_back(
                        {"no declaration for clock name '" + name->spelling + "'", name->span});
            }
            for (const auto& assignment : process.assignments) {
                const std::string target(key(assignment.target));
                if (!localDeclarations.contains(target))
                    result.diagnostics.push_back(
                        {"no declaration for assignment target '" + assignment.target.spelling + "'",
                         assignment.target.span});
                else if ((assignment.kind == AssignmentKind::Variable) != variables.contains(target))
                    result.diagnostics.push_back(
                        {"assignment operator does not match object class for '" +
                             assignment.target.spelling + "'", assignment.target.span});
                checkExpression(*assignment.value, localDeclarations, result);
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
