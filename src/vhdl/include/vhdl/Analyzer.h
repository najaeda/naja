// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "vhdl/Parser.h"
#include <string>
#include <vector>

namespace vhdl {

struct AnalysisDiagnostic {
    std::string message;
    SourceSpan span;
};

struct AnalysisResult {
    std::vector<AnalysisDiagnostic> diagnostics;
    bool hasErrors() const { return !diagnostics.empty(); }
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

/// Performs name binding for the parser's initial entity/architecture slice.
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
