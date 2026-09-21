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

/// Performs name binding for the parser's initial entity/architecture slice.
class Analyzer {
public:
    static AnalysisResult analyze(const DesignFile& syntax);
};

} // namespace vhdl
