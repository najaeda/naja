// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Parser.h"
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vhdl {

struct AnalysisDiagnostic {
    std::string message;
    SourceSpan span;
};

enum class ScalarType {
    Unknown,
    Bit,
    BitVector,
    StdLogic,
    StdLogicVector,
    Unsigned,
    Signed,
    Boolean,
    Integer,
    Natural,
    Real,
    String
};

struct AnalysisResult {
    std::vector<AnalysisDiagnostic> diagnostics;
    std::unordered_map<const Expression*, ScalarType> expressionTypes;
    std::unordered_map<const Expression*, DiscreteRange> expressionRanges;
    std::unordered_map<const TypeMark*, DiscreteRange> resolvedTypeRanges;
    std::unordered_map<const EntityInstantiation*,
        std::unordered_map<std::string, std::int64_t>> genericValues;
    std::unordered_map<const EntityInstantiation*,
        std::unordered_map<const TypeMark*, DiscreteRange>> specializedTypeRanges;
    bool hasErrors() const { return !diagnostics.empty(); }
    ScalarType getType(const Expression& expression) const {
        const auto found = expressionTypes.find(&expression);
        return found == expressionTypes.end() ? ScalarType::Unknown : found->second;
    }
    const DiscreteRange* getRange(const Expression& expression) const {
        const auto found = expressionRanges.find(&expression);
        return found == expressionRanges.end() ? nullptr : &found->second;
    }
    const DiscreteRange* getRange(const TypeMark& type) const {
        const auto found = resolvedTypeRanges.find(&type);
        return found == resolvedTypeRanges.end() ? nullptr : &found->second;
    }
    std::optional<std::int64_t> getGenericValue(
            const EntityInstantiation& instantiation, std::string_view name) const {
        const auto values = genericValues.find(&instantiation);
        if (values == genericValues.end())
            return std::nullopt;
        std::string canonical(name);
        for (auto& character : canonical)
            character = static_cast<char>(std::tolower(
                static_cast<unsigned char>(character)));
        const auto value = values->second.find(canonical);
        return value == values->second.end() ? std::nullopt
                                             : std::optional<std::int64_t>(value->second);
    }
    const DiscreteRange* getRange(
            const EntityInstantiation& instantiation, const TypeMark& type) const {
        const auto ranges = specializedTypeRanges.find(&instantiation);
        if (ranges == specializedTypeRanges.end())
            return nullptr;
        const auto range = ranges->second.find(&type);
        return range == ranges->second.end() ? nullptr : &range->second;
    }
};

struct ScheduledWrite {
    std::string target;
    std::string source;
    SourceSpan span;
    AssignmentKind kind = AssignmentKind::Signal;
};

struct ScheduleResult {
    std::vector<ScheduledWrite> writes;
    std::vector<std::string> retainedVariables;
    std::vector<AnalysisDiagnostic> diagnostics;
    bool hasErrors() const { return !diagnostics.empty(); }
};

/// Performs name binding and restricted scalar/constrained-vector expression
/// type checking. Expression type and range entries remain valid only
/// while the analyzed DesignFile and its expression nodes remain alive.
class Analyzer {
public:
    static AnalysisResult analyze(const DesignFile& syntax);
    /// Resolve immediate variable values and freeze scheduled signal RHS names.
    /// Variables read before their first assignment are retained state; their
    /// final value is returned as one variable write. Call after name analysis;
    /// only scalar-name, straight-line bodies qualify.
    static ScheduleResult schedule(const ClockedProcess& process);
};

} // namespace vhdl
