// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Parser.h"
#include <string>
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
    bool hasErrors() const { return !diagnostics.empty(); }
    ScalarType getType(const Expression& expression) const {
        const auto found = expressionTypes.find(&expression);
        return found == expressionTypes.end() ? ScalarType::Unknown : found->second;
    }
    const DiscreteRange* getRange(const Expression& expression) const {
        const auto found = expressionRanges.find(&expression);
        return found == expressionRanges.end() ? nullptr : &found->second;
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
