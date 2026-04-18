// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSVConstructor.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "NajaLog.h"
#include "NajaPerf.h"

#include "NLID.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLName.h"
#include "NLLibrary.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTermBit.h"
#include "SNLBusTerm.h"
#include "SNLAttributes.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLInstParameter.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLParameter.h"
#include "SNLRTLInfos.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

#include "SNLSVConstructorException.h"

#include "slang/ast/Compilation.h"
#include "slang/ast/ASTSerializer.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/EvalContext.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/Statement.h"
#include "slang/ast/TimingControl.h"
#include "slang/ast/expressions/AssignmentExpressions.h"
#include "slang/ast/expressions/CallExpression.h"
#include "slang/ast/expressions/ConversionExpression.h"
#include "slang/ast/expressions/LiteralExpressions.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/expressions/OperatorExpressions.h"
#include "slang/ast/expressions/SelectExpressions.h"
#include "slang/ast/statements/ConditionalStatements.h"
#include "slang/ast/statements/LoopStatements.h"
#include "slang/ast/statements/MiscStatements.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/MemberSymbols.h"
#include "slang/ast/symbols/PortSymbols.h"
#include "slang/ast/symbols/SubroutineSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/Type.h"
#include "slang/diagnostics/DiagnosticEngine.h"
#include "slang/diagnostics/Diagnostics.h"
#include "slang/driver/Driver.h"
#include "slang/syntax/SyntaxTree.h"
#include "slang/text/Json.h"
#include "slang/text/SourceManager.h"
#include "slang/util/ScopeGuard.h"

namespace naja::NL {

namespace {

using slang::ast::ArgumentDirection;
using slang::ast::Expression;
using slang::ast::InstanceBodySymbol;
using slang::ast::InstanceSymbol;
using slang::ast::PortSymbol;
using slang::ast::Statement;
using slang::ast::Symbol;
using slang::ast::SymbolKind;
using slang::ast::TimingControl;
using slang::ast::Type;
using slang::ast::ValueSymbol;

std::string getCompilationDiagnosticsReport(slang::ast::Compilation& compilation) {
  const auto& diags = compilation.getAllDiagnostics();
  if (diags.empty()) {
    return {};
  }
  if (const auto* sourceManager = compilation.getSourceManager()) {
    return slang::DiagnosticEngine::reportAll(*sourceManager, diags);
  }
  return {}; // LCOV_EXCL_LINE
}

std::optional<std::string> getCompilationFailureDetails(
  slang::ast::Compilation& compilation) {
  const auto& diags = compilation.getAllDiagnostics();
  bool hasError = false;
  for (const auto& diag : diags) {
    if (diag.isError()) {
      hasError = true;
      break;
    }
  }
  if (!hasError) {
    return std::nullopt;
  }

  std::ostringstream reason;
  reason << "SystemVerilog compilation failed";
  if (const auto* sourceManager = compilation.getSourceManager()) {
    const auto details = slang::DiagnosticEngine::reportAll(*sourceManager, diags);
    if (!details.empty()) {
      reason << ":\n" << details;
    }
  }
  return reason.str();
}

std::optional<std::string> getDriverFailureDetails(
  const slang::driver::Driver& driver) {
  std::ostringstream details;
  bool hasDetails = false;

  for (const auto& error : driver.sourceLoader.getErrors()) {
    details << error << "\n";
    hasDetails = true;
  }

  // LCOV_EXCL_START
  slang::Diagnostics parseDiags;
  for (const auto& tree : driver.syntaxTrees) {
    parseDiags.append_range(tree->diagnostics());
  }
  if (!parseDiags.empty()) {
    parseDiags.sort(driver.sourceManager);
    const auto parseDetails =
      slang::DiagnosticEngine::reportAll(driver.sourceManager, parseDiags);
    if (!parseDetails.empty()) {
      details << parseDetails;
      hasDetails = true;
    }
  }
  // LCOV_EXCL_STOP

  if (!hasDetails) {
    return std::nullopt;
  }

  std::ostringstream reason;
  reason << "SystemVerilog compilation failed:\n" << details.str();
  return reason.str();
}

bool allNetsArePortNets(SNLDesign* design) {
  for (auto net : design->getBitNets()) {
    if (net->isAssignConstant()) {
      continue;
    }
    if (net->getBitTerms().empty()) {
      return false;
    }
  }
  return true;
}

const Expression* stripConversions(const Expression& expr) {
  const Expression* current = &expr;
  while (current && current->kind == slang::ast::ExpressionKind::Conversion) {
    current = &current->as<slang::ast::ConversionExpression>().operand();
  }
  return current;
}

void collectBinaryOperands(const Expression& expr, slang::ast::BinaryOperator op,
                           std::vector<const Expression*>& operands) {
  const Expression* current = stripConversions(expr);
  if (!current) {
    // LCOV_EXCL_START
    throw SNLSVConstructorException(
      "Internal error: null expression while collecting binary operands");
    // LCOV_EXCL_STOP
  }
  if (current->kind == slang::ast::ExpressionKind::BinaryOp) {
    const auto& binaryExpr = current->as<slang::ast::BinaryExpression>();
    if (binaryExpr.op == op) {
      collectBinaryOperands(binaryExpr.left(), op, operands);
      collectBinaryOperands(binaryExpr.right(), op, operands);
      return;
    }
  }
  operands.push_back(current);
}

std::optional<NLDB0::GateType> gateTypeFromBinary(slang::ast::BinaryOperator op) {
  switch (op) {
    case slang::ast::BinaryOperator::BinaryAnd:
    case slang::ast::BinaryOperator::LogicalAnd:
      return NLDB0::GateType(NLDB0::GateType::And);
    case slang::ast::BinaryOperator::BinaryOr:
    case slang::ast::BinaryOperator::LogicalOr:
      return NLDB0::GateType(NLDB0::GateType::Or);
    case slang::ast::BinaryOperator::BinaryXor:
      return NLDB0::GateType(NLDB0::GateType::Xor);
    case slang::ast::BinaryOperator::BinaryXnor:
      return NLDB0::GateType(NLDB0::GateType::Xnor);
    default:
      return std::nullopt;
  }
}

bool isEqualityBinaryOp(slang::ast::BinaryOperator op) {
  switch (op) {
    case slang::ast::BinaryOperator::Equality:
    case slang::ast::BinaryOperator::CaseEquality:
      return true;
    default:
      return false;
  }
}

bool isInequalityBinaryOp(slang::ast::BinaryOperator op) {
  switch (op) {
    case slang::ast::BinaryOperator::Inequality:
    case slang::ast::BinaryOperator::CaseInequality:
      return true;
    default:
      return false;
  }
}

bool isCaseComparisonBinaryOp(slang::ast::BinaryOperator op) {
  switch (op) {
    case slang::ast::BinaryOperator::CaseEquality:
    case slang::ast::BinaryOperator::CaseInequality:
      return true;
    default:
      return false;
  }
}

bool isRelationalBinaryOp(slang::ast::BinaryOperator op) {
  switch (op) {
    case slang::ast::BinaryOperator::LessThan:
    case slang::ast::BinaryOperator::LessThanEqual:
    case slang::ast::BinaryOperator::GreaterThan:
    case slang::ast::BinaryOperator::GreaterThanEqual:
      return true;
    default:
      return false;
  }
}

bool isKnown2StateConstantExpr(const Expression& expr) {
  const auto* stripped = stripConversions(expr);
  if (!stripped) {
    // LCOV_EXCL_START
    throw SNLSVConstructorException(
      "Internal error: null expression in isKnown2StateConstantExpr");
    // LCOV_EXCL_STOP
  }
  const auto* symbol = stripped->getSymbolReference();
  return symbol && symbol->kind == SymbolKind::EnumValue;
}

std::optional<SNLTerm::Direction> toSNLDirection(ArgumentDirection direction) {
  switch (direction) {
    case ArgumentDirection::In:
      return SNLTerm::Direction::Input;
    case ArgumentDirection::Out:
      return SNLTerm::Direction::Output;
    case ArgumentDirection::InOut:
      return SNLTerm::Direction::InOut;
    default:
      return std::nullopt;
  }
}

const char* getPortKindLabel(SymbolKind kind) {
  switch (kind) {
    case SymbolKind::InterfacePort:
      return "interface port";
    case SymbolKind::MultiPort:
      return "multi-port";
    // LCOV_EXCL_START
    default:
      // createTerms only calls this helper for non-Port entries from getPortList.
      return "non-port";
    // LCOV_EXCL_STOP
  }
}

bool tryGetInt64FromSVInt(const slang::SVInt& svInt, int64_t& value) {
  if (svInt.isSigned()) {
    const auto maybeSigned = svInt.as<int64_t>();
    if (!maybeSigned) {
      return false;
    }
    value = *maybeSigned;
    return true;
  }

  const auto maybeUnsigned = svInt.as<uint64_t>();
  if (!maybeUnsigned ||
      *maybeUnsigned > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    return false;
  }
  value = static_cast<int64_t>(*maybeUnsigned);
  return true;
}

std::optional<std::string> getUnsupportedTypeReason(const Type& type) {
  const auto& canonical = type.getCanonicalType();
  if (canonical.isString()) {
    return "Unsupported SystemVerilog type 'string'";
  }
  if (canonical.isFloating()) {
    return "Unsupported SystemVerilog floating-point type";
  }
  // Keep support conservative but include fixed-size unpacked arrays of representable
  // element types by flattening them to a single SNL bus.
  std::function<std::optional<slang::bitwidth_t>(const Type&)> getRepresentableBitWidth =
    [&](const Type& currentType) -> std::optional<slang::bitwidth_t> {
    const auto& current = currentType.getCanonicalType();
    if (current.isIntegral()) {
      return current.getBitWidth();
    }
    if (!current.isUnpackedArray() || !current.hasFixedRange()) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    const auto* elementType = current.getArrayElementType();
    if (!elementType) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    auto elementWidth = getRepresentableBitWidth(*elementType);
    if (!elementWidth) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    const auto dimensionWidth = static_cast<uint64_t>(current.getFixedRange().width());
    const auto totalWidth = static_cast<uint64_t>(*elementWidth) * dimensionWidth;
    if (totalWidth > std::numeric_limits<slang::bitwidth_t>::max()) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    return static_cast<slang::bitwidth_t>(totalWidth);
  };

  if (!getRepresentableBitWidth(canonical)) {
    return "Unsupported SystemVerilog type not representable in SNL";
  }
  return std::nullopt;
}

std::optional<slang::ConstantRange> getRangeFromType(const Type& type) {
  const auto& canonical = type.getCanonicalType();

  std::function<std::optional<slang::bitwidth_t>(const Type&)> getRepresentableBitWidth =
    [&](const Type& currentType) -> std::optional<slang::bitwidth_t> {
    const auto& current = currentType.getCanonicalType();
    if (current.isIntegral()) {
      return current.getBitWidth();
    }
    if (!current.isUnpackedArray() || !current.hasFixedRange()) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    const auto* elementType = current.getArrayElementType();
    if (!elementType) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    auto elementWidth = getRepresentableBitWidth(*elementType);
    if (!elementWidth) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    const auto dimensionWidth = static_cast<uint64_t>(current.getFixedRange().width());
    const auto totalWidth = static_cast<uint64_t>(*elementWidth) * dimensionWidth;
    if (totalWidth > std::numeric_limits<slang::bitwidth_t>::max()) {
      return std::nullopt; // LCOV_EXCL_LINE
    }
    return static_cast<slang::bitwidth_t>(totalWidth);
  };

  // Preserve declared packed ranges whenever available.
  if (!canonical.isUnpackedArray() && canonical.hasFixedRange()) {
    const auto fixedRange = canonical.getFixedRange();
    auto totalWidth = getRepresentableBitWidth(canonical);
    if (totalWidth && static_cast<uint64_t>(*totalWidth) == fixedRange.fullWidth()) {
      return fixedRange;
    }
  }

  auto width = getRepresentableBitWidth(canonical);
  if (!width || *width <= 1) {
    return std::nullopt; // LCOV_EXCL_LINE
  }
  const auto maxBit = static_cast<uint64_t>(*width) - 1;
  if (maxBit > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
    return std::nullopt; // LCOV_EXCL_LINE
  }
  return slang::ConstantRange(static_cast<int32_t>(maxBit), 0);
}

SNLNet* getOrCreateNet(SNLDesign* design, const std::string& name, const Type& type) {
  if (auto existing = design->getNet(NLName(name))) {
    return existing;
  }
  auto range = getRangeFromType(type);
  if (range && range->width() > 1) {
    return SNLBusNet::create(design,
      static_cast<NLID::Bit>(range->left),
      static_cast<NLID::Bit>(range->right),
      NLName(name));
  }
  return SNLScalarNet::create(design, NLName(name));
}

#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
double toMilliseconds(std::chrono::nanoseconds duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

std::string toSingleLine(const std::string& text) {
  std::string line = text;
  std::replace(line.begin(), line.end(), '\n', ' ');
  std::replace(line.begin(), line.end(), '\r', ' ');
  return line;
}
#endif

}  // namespace

class SNLSVConstructorImpl {
  public:
    explicit SNLSVConstructorImpl(
      NLLibrary* library,
      const SNLSVConstructor::Config& config,
      const SNLSVConstructor::ConstructOptions& options):
      library_(library),
      config_(config),
      options_(options)
    {}

#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
    struct SVPerfReport {
      bool enabled {false};
      std::filesystem::path reportPath {};
      std::chrono::steady_clock::time_point constructStart {};

      std::chrono::nanoseconds constructDuration {0};
      std::chrono::nanoseconds parseDuration {0};
      std::chrono::nanoseconds diagnosticsReportDuration {0};
      std::chrono::nanoseconds buildTopDesignsDuration {0};
      std::chrono::nanoseconds elaboratedASTDumpDuration {0};
      std::chrono::nanoseconds unsupportedCheckDuration {0};

      std::chrono::nanoseconds buildDesignDuration {0};
      std::chrono::nanoseconds createTermsDuration {0};
      std::chrono::nanoseconds createNetsDuration {0};
      std::chrono::nanoseconds connectTermsToNetsDuration {0};
      std::chrono::nanoseconds createContinuousAssignsDuration {0};
      std::chrono::nanoseconds createInstancesDuration {0};
      std::chrono::nanoseconds createSequentialLogicDuration {0};
      std::chrono::nanoseconds annotateSourceInfoDuration {0};
      std::chrono::nanoseconds cloneRTLInfosDuration {0};

      size_t inputPathCount {0};
      size_t topInstanceCount {0};
      size_t buildDesignCalls {0};
      size_t buildDesignCacheHits {0};
      size_t createTermsCalls {0};
      size_t createNetsCalls {0};
      size_t connectTermsToNetsCalls {0};
      size_t createContinuousAssignsCalls {0};
      size_t createInstancesCalls {0};
      size_t createSequentialLogicCalls {0};
      size_t annotateSourceInfoCalls {0};
      size_t rtlInfoCreateDesignCalls {0};
      size_t rtlInfoCreateDesignObjectCalls {0};
      size_t rtlInfoSetCalls {0};
      size_t rtlInfoCloneCalls {0};
      size_t rtlInfoClonedEntries {0};
      size_t portSymbolsVisited {0};
      size_t portsCreated {0};
      size_t netOrVariableSymbolsVisited {0};
      size_t netsCreated {0};
      size_t continuousAssignsVisited {0};
      size_t instanceSymbolsVisited {0};
      size_t instancesCreated {0};
      size_t proceduralBlocksVisited {0};
      size_t alwaysCombBlocksVisited {0};
      size_t alwaysCombBlocksLowered {0};
      size_t sequentialBlocksVisited {0};
      size_t sequentialBlocksLowered {0};
      size_t warningCount {0};
      size_t unsupportedCount {0};

      bool success {false};
      std::string failureReason {};
      std::string firstUnsupported {};
      std::string firstWarning {};
    };

    class SVPerfScopedTimer {
      public:
        SVPerfScopedTimer(SVPerfReport& report, std::chrono::nanoseconds& bucket):
          report_((report.enabled) ? &report : nullptr),
          bucket_((report.enabled) ? &bucket : nullptr) {
          if (report_) {
            start_ = std::chrono::steady_clock::now();
          }
        }

        ~SVPerfScopedTimer() {
          if (report_) {
            *bucket_ += std::chrono::steady_clock::now() - start_;
          }
        }

      private:
        SVPerfReport* report_ {nullptr};
        std::chrono::nanoseconds* bucket_ {nullptr};
        std::chrono::steady_clock::time_point start_ {};
    };

    void initializeSVPerfReport(const SNLSVConstructor::Paths& paths) {
      svPerfReport_ = SVPerfReport {};

      std::string reportPath = "sv_constructor_perf.log";
      const char* reportEnv = std::getenv("NAJA_SV_CONSTRUCTOR_REPORT");
      if (reportEnv) {
        std::string overridePath(reportEnv);
        if (!overridePath.empty() && overridePath != "1") {
          reportPath = std::move(overridePath);
        }
      }

      svPerfReport_.enabled = true;
      svPerfReport_.reportPath = reportPath;
      svPerfReport_.constructStart = std::chrono::steady_clock::now();
      svPerfReport_.inputPathCount = paths.size();
    }

    void finalizeSVPerfReport() {
      if (!svPerfReport_.enabled) {
        return;
      }

      svPerfReport_.constructDuration =
        std::chrono::steady_clock::now() - svPerfReport_.constructStart;
      svPerfReport_.warningCount = warnings_.size();
      svPerfReport_.unsupportedCount = unsupportedElements_.size();
      if (svPerfReport_.firstWarning.empty() && !warnings_.empty()) {
        svPerfReport_.firstWarning = warnings_.front();
      }
      if (svPerfReport_.firstUnsupported.empty() && !unsupportedElements_.empty()) {
        svPerfReport_.firstUnsupported = unsupportedElements_.front();
      }

      std::ofstream output(
        svPerfReport_.reportPath,
        std::ios::out | std::ios::app);
      if (!output) {
        NAJA_LOG_WARN(
          "Unable to write SV constructor performance report: {}",
          svPerfReport_.reportPath.string());
        return;
      }

      const auto now = std::chrono::system_clock::now();
      const auto nowTime = std::chrono::system_clock::to_time_t(now);
      std::tm localTime {};
#if defined(_WIN32)
      localtime_s(&localTime, &nowTime);
#else
      if (const auto* tm = std::localtime(&nowTime)) {
        localTime = *tm;
      }
#endif

      output << "=== SNLSVConstructor Perf Report "
             << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << " ===\n";
      output << "result=" << (svPerfReport_.success ? "success" : "failure") << "\n";
      if (!svPerfReport_.failureReason.empty()) {
        output << "failure_reason=" << toSingleLine(svPerfReport_.failureReason) << "\n";
      }
      output << "inputs.path_count=" << svPerfReport_.inputPathCount << "\n";
      output << "inputs.top_instance_count=" << svPerfReport_.topInstanceCount << "\n";

      output << std::fixed << std::setprecision(3);
      output << "time.construct.total_ms=" << toMilliseconds(svPerfReport_.constructDuration)
             << "\n";
      output << "time.phase.parse_ms=" << toMilliseconds(svPerfReport_.parseDuration) << "\n";
      output << "time.phase.diagnostics_report_ms="
             << toMilliseconds(svPerfReport_.diagnosticsReportDuration) << "\n";
      output << "time.phase.build_top_designs_ms="
             << toMilliseconds(svPerfReport_.buildTopDesignsDuration) << "\n";
      output << "time.phase.dump_elaborated_ast_ms="
             << toMilliseconds(svPerfReport_.elaboratedASTDumpDuration) << "\n";
      output << "time.phase.unsupported_check_ms="
             << toMilliseconds(svPerfReport_.unsupportedCheckDuration) << "\n";
      output << "time.lowering.build_design_ms="
             << toMilliseconds(svPerfReport_.buildDesignDuration) << "\n";
      output << "time.lowering.create_terms_ms="
             << toMilliseconds(svPerfReport_.createTermsDuration) << "\n";
      output << "time.lowering.create_nets_ms="
             << toMilliseconds(svPerfReport_.createNetsDuration) << "\n";
      output << "time.lowering.connect_terms_to_nets_ms="
             << toMilliseconds(svPerfReport_.connectTermsToNetsDuration) << "\n";
      output << "time.lowering.create_continuous_assigns_ms="
             << toMilliseconds(svPerfReport_.createContinuousAssignsDuration) << "\n";
      output << "time.lowering.create_instances_ms="
             << toMilliseconds(svPerfReport_.createInstancesDuration) << "\n";
      output << "time.lowering.create_sequential_logic_ms="
             << toMilliseconds(svPerfReport_.createSequentialLogicDuration) << "\n";
      output << "time.lowering.annotate_source_info_ms="
             << toMilliseconds(svPerfReport_.annotateSourceInfoDuration) << "\n";
      output << "time.lowering.clone_rtl_infos_ms="
             << toMilliseconds(svPerfReport_.cloneRTLInfosDuration) << "\n";

      output << "count.design.build_calls=" << svPerfReport_.buildDesignCalls << "\n";
      output << "count.design.build_cache_hits=" << svPerfReport_.buildDesignCacheHits << "\n";
      output << "count.design.unique_built="
             << (svPerfReport_.buildDesignCalls - svPerfReport_.buildDesignCacheHits) << "\n";
      output << "count.lowering.create_terms_calls=" << svPerfReport_.createTermsCalls << "\n";
      output << "count.lowering.create_nets_calls=" << svPerfReport_.createNetsCalls << "\n";
      output << "count.lowering.connect_terms_to_nets_calls="
             << svPerfReport_.connectTermsToNetsCalls << "\n";
      output << "count.lowering.create_continuous_assigns_calls="
             << svPerfReport_.createContinuousAssignsCalls << "\n";
      output << "count.lowering.create_instances_calls="
             << svPerfReport_.createInstancesCalls << "\n";
      output << "count.lowering.create_sequential_logic_calls="
             << svPerfReport_.createSequentialLogicCalls << "\n";
      output << "count.lowering.annotate_source_info_calls="
             << svPerfReport_.annotateSourceInfoCalls << "\n";
      output << "count.rtl_info.create_design_calls="
             << svPerfReport_.rtlInfoCreateDesignCalls << "\n";
      output << "count.rtl_info.create_design_object_calls="
             << svPerfReport_.rtlInfoCreateDesignObjectCalls << "\n";
      output << "count.rtl_info.set_calls=" << svPerfReport_.rtlInfoSetCalls << "\n";
      output << "count.rtl_info.clone_calls=" << svPerfReport_.rtlInfoCloneCalls << "\n";
      output << "count.rtl_info.cloned_entries="
             << svPerfReport_.rtlInfoClonedEntries << "\n";
      output << "count.symbol.port_visited=" << svPerfReport_.portSymbolsVisited << "\n";
      output << "count.symbol.port_created=" << svPerfReport_.portsCreated << "\n";
      output << "count.symbol.net_or_variable_visited="
             << svPerfReport_.netOrVariableSymbolsVisited << "\n";
      output << "count.symbol.net_created=" << svPerfReport_.netsCreated << "\n";
      output << "count.symbol.continuous_assign_visited="
             << svPerfReport_.continuousAssignsVisited << "\n";
      output << "count.symbol.instance_visited=" << svPerfReport_.instanceSymbolsVisited << "\n";
      output << "count.symbol.instance_created=" << svPerfReport_.instancesCreated << "\n";
      output << "count.symbol.procedural_block_visited="
             << svPerfReport_.proceduralBlocksVisited << "\n";
      output << "count.block.always_comb_visited="
             << svPerfReport_.alwaysCombBlocksVisited << "\n";
      output << "count.block.always_comb_lowered="
             << svPerfReport_.alwaysCombBlocksLowered << "\n";
      output << "count.block.sequential_visited="
             << svPerfReport_.sequentialBlocksVisited << "\n";
      output << "count.block.sequential_lowered="
             << svPerfReport_.sequentialBlocksLowered << "\n";
      output << "count.warning=" << svPerfReport_.warningCount << "\n";
      output << "count.unsupported=" << svPerfReport_.unsupportedCount << "\n";

      if (!svPerfReport_.firstWarning.empty()) {
        output << "first_warning=" << toSingleLine(svPerfReport_.firstWarning) << "\n";
      }
      if (!svPerfReport_.firstUnsupported.empty()) {
        output << "first_unsupported=" << toSingleLine(svPerfReport_.firstUnsupported) << "\n";
      }
      output << "\n";
    }
#endif

    void construct(const SNLSVConstructor::Paths& paths) {
      if (!library_) {
        throw SNLSVConstructorException("SNLSVConstructor requires a valid NLLibrary");
      }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      initializeSVPerfReport(paths);
      const auto perfReportGuard = slang::ScopeGuard([&]() {
        finalizeSVPerfReport();
      });
#endif

      driver_.reset();
      syntaxTrees_.clear();
      warnings_.clear();
      emittedWarnings_.clear();
      unsupportedElements_.clear();
      dynamicElementSelectCache_.clear();
      try {
        {
          NajaPerf::Scope scope("SNLSVConstructorImpl::constructWithSlangDriver");
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          const SVPerfScopedTimer timer(svPerfReport_, svPerfReport_.parseDuration);
#endif
          constructWithSlangDriver(paths);
        }

        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          const SVPerfScopedTimer timer(
            svPerfReport_,
            svPerfReport_.diagnosticsReportDuration);
#endif
          dumpDiagnosticsReport(getCompilationDiagnosticsReport(*compilation_));
        }
        if (auto failure = getCompilationFailureDetails(*compilation_)) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          if (svPerfReport_.failureReason.empty()) {
            svPerfReport_.failureReason = *failure;
          }
#endif
          throw SNLSVConstructorException(*failure);
        }

        const auto& root = compilation_->getRoot();
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        svPerfReport_.topInstanceCount = root.topInstances.size();
#endif
        {
          NajaPerf::Scope scope("SNLSVConstructorImpl::buildTopDesigns");
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          const SVPerfScopedTimer timer(
            svPerfReport_,
            svPerfReport_.buildTopDesignsDuration);
#endif
          for (const auto* top : root.topInstances) {
            buildDesign(top->body);
          }
        }

        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          const SVPerfScopedTimer timer(
            svPerfReport_,
            svPerfReport_.elaboratedASTDumpDuration);
#endif
          dumpElaboratedASTJson(root);
        }

        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          const SVPerfScopedTimer timer(
            svPerfReport_,
            svPerfReport_.unsupportedCheckDuration);
#endif
          throwIfUnsupportedElements();
        }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        svPerfReport_.success = true;
#endif
      } catch (const std::exception& e) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        if (svPerfReport_.failureReason.empty()) {
          svPerfReport_.failureReason = e.what();
        }
#endif
        throw;
      } catch (...) { // LCOV_EXCL_LINE
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        if (svPerfReport_.failureReason.empty()) {
          svPerfReport_.failureReason = "unknown non-std exception";
        }
#endif
        throw; // LCOV_EXCL_LINE
      }
    }

  private:
    void constructWithSlangDriver(const SNLSVConstructor::Paths& paths) {
      driver_ = std::make_unique<slang::driver::Driver>();
      auto& driver = *driver_;
      driver.addStandardArgs();

      std::vector<std::string> args;
      args.reserve(paths.size() + 7);
      args.emplace_back("snl_sv_constructor");
      args.emplace_back("--translate-off-format");
      args.emplace_back("pragma,translate_off,translate_on");
      args.emplace_back("--translate-off-format");
      args.emplace_back("synthesis,translate_off,translate_on");
      args.emplace_back("--translate-off-format");
      args.emplace_back("synopsys,translate_off,translate_on");
      for (const auto& path : paths) {
        args.emplace_back(path.string());
      }

      std::vector<const char*> argv;
      argv.reserve(args.size());
      for (const auto& arg : args) {
        argv.push_back(arg.c_str());
      }

      if (!driver.parseCommandLine(static_cast<int>(argv.size()), argv.data()) ||
          !driver.processOptions() ||
          !driver.parseAllSources()) {
        if (auto failure = getDriverFailureDetails(driver)) {
          dumpDiagnosticsReport(*failure);
          throw SNLSVConstructorException(*failure);
        }
        dumpDiagnosticsReport("SystemVerilog compilation failed");
        throw SNLSVConstructorException("SystemVerilog compilation failed");
      }

      for (const auto& tree : driver.syntaxTrees) {
        syntaxTrees_.push_back(tree);
      }
      // Preserve driver-resolved compilation options from command files
      // (for example: --top, -G parameter overrides, library maps).
      compilation_ = driver.createCompilation();
    }

    void dumpDiagnosticsReport(const std::string& report) const {
      if (!options_.diagnosticsReportPath) {
        return;
      }
      const auto& reportPath = *options_.diagnosticsReportPath;
      if (reportPath.empty()) {
        throw SNLSVConstructorException("Empty path for diagnostics report dump");
      }

      auto parent = reportPath.parent_path();
      if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        // LCOV_EXCL_START
        if (ec) {
          std::ostringstream reason;
          reason << "Failed to create diagnostics report directory: " << parent.string();
          throw SNLSVConstructorException(reason.str());
        }
        // LCOV_EXCL_STOP
      }

      std::ofstream output(reportPath, std::ios::out | std::ios::trunc);
      // LCOV_EXCL_START
      if (!output) {
        std::ostringstream reason;
        reason << "Failed to create diagnostics report file: " << reportPath.string();
        throw SNLSVConstructorException(reason.str());
      }
      // LCOV_EXCL_STOP

      if (!report.empty()) {
        output << report;
      } else {
        output << "No SystemVerilog diagnostics.\n";
      }

      // LCOV_EXCL_START
      if (!output.good()) {
        std::ostringstream reason;
        reason << "Failed to write diagnostics report file: " << reportPath.string();
        throw SNLSVConstructorException(reason.str());
      }
      // LCOV_EXCL_STOP
    }

    void dumpElaboratedASTJson(const slang::ast::RootSymbol& root) const {
      if (!options_.elaboratedASTJsonPath) {
        return;
      }
      const auto& jsonPath = *options_.elaboratedASTJsonPath;
      if (jsonPath.empty()) {
        throw SNLSVConstructorException("Empty path for elaborated AST JSON dump");
      }

      auto parent = jsonPath.parent_path();
      if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        // LCOV_EXCL_START
        if (ec) {
          std::ostringstream reason;
          reason << "Failed to create elaborated AST JSON directory: " << parent.string();
          throw SNLSVConstructorException(reason.str());
        }
        // LCOV_EXCL_STOP
      }

      slang::JsonWriter writer;
      writer.setPrettyPrint(options_.prettyPrintElaboratedASTJson);
      auto& compilation = *compilation_;
      slang::ast::ASTSerializer serializer(compilation, writer);
      serializer.setIncludeSourceInfo(options_.includeSourceInfoInElaboratedASTJson);
      serializer.serialize(root);

      std::ofstream output(jsonPath, std::ios::out | std::ios::trunc);
      // LCOV_EXCL_START
      if (!output) {
        std::ostringstream reason;
        reason << "Failed to create elaborated AST JSON file: " << jsonPath.string();
        throw SNLSVConstructorException(reason.str());
      }
      // LCOV_EXCL_STOP
      output << writer.view();
      // LCOV_EXCL_START
      if (!output.good()) {
        std::ostringstream reason;
        reason << "Failed to write elaborated AST JSON file: " << jsonPath.string();
        throw SNLSVConstructorException(reason.str());
      }
      // LCOV_EXCL_STOP
    }

    std::string allocateElaboratedDesignName(const std::string& defName) {
      auto& nextOrdinal = elaboratedDesignOrdinals_[defName];
      while (true) {
        std::string candidate = defName;
        if (nextOrdinal != 0) {
          candidate += "__elab" + std::to_string(nextOrdinal);
        }
        ++nextOrdinal;
        if (!library_->getSNLDesign(NLName(candidate))) {
          return candidate;
        }
      }
    }

    SNLDesign* buildDesign(const InstanceBodySymbol& body) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      ++svPerfReport_.buildDesignCalls;
#endif
      const auto& definition = body.getDefinition();
      std::string defName(definition.name);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      const auto makePerfScopeName = [&](const char* phase) {
        return std::string("SNLSVConstructorImpl::") + phase + "(" + defName + ")";
      };
#endif
      auto existingIt = bodyToDesign_.find(&body);
      if (existingIt != bodyToDesign_.end()) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.buildDesignCacheHits;
#endif
        return existingIt->second;
      }
      auto& representativeBodies = representativeBodiesByDefinition_[&definition];
      for (const auto* representativeBody : representativeBodies) {
        if (!representativeBody || !representativeBody->hasSameType(body)) {
          continue;
        }
        auto representativeIt = bodyToDesign_.find(representativeBody);
        if (representativeIt == bodyToDesign_.end()) {
          continue; // LCOV_EXCL_LINE
        }
        bodyToDesign_[&body] = representativeIt->second;
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.buildDesignCacheHits;
#endif
        return representativeIt->second;
      }

#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      const SVPerfScopedTimer designTimer(svPerfReport_, svPerfReport_.buildDesignDuration);
      NajaPerf::Scope buildDesignScope(makePerfScopeName("buildDesign"));
#endif
      auto savedInferredMemories = std::move(inferredMemories_);
      auto savedInferredMemoryByStateSymbol = std::move(inferredMemoryByStateSymbol_);
      auto savedInferredMemoryCombBlocks = std::move(inferredMemoryCombBlocks_);
      auto savedInferredMemorySequentialBlocks = std::move(inferredMemorySequentialBlocks_);
      inferredMemories_.clear();
      inferredMemoryByStateSymbol_.clear();
      inferredMemoryCombBlocks_.clear();
      inferredMemorySequentialBlocks_.clear();
      const auto inferredMemoryStateGuard = slang::ScopeGuard([&]() {
        inferredMemories_ = std::move(savedInferredMemories);
        inferredMemoryByStateSymbol_ = std::move(savedInferredMemoryByStateSymbol);
        inferredMemoryCombBlocks_ = std::move(savedInferredMemoryCombBlocks);
        inferredMemorySequentialBlocks_ = std::move(savedInferredMemorySequentialBlocks);
      });

      auto design = SNLDesign::create(library_, NLName(allocateElaboratedDesignName(defName)));
      bodyToDesign_[&body] = design;
      representativeBodies.push_back(&body);
      annotateSourceInfo(design, getSourceRange(definition));

      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("createTerms"));
        ++svPerfReport_.createTermsCalls;
        const SVPerfScopedTimer timer(svPerfReport_, svPerfReport_.createTermsDuration);
#endif
        createTerms(design, body);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("createNets"));
        ++svPerfReport_.createNetsCalls;
        const SVPerfScopedTimer timer(svPerfReport_, svPerfReport_.createNetsDuration);
#endif
        createNets(design, body);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("connectTermsToNets"));
        ++svPerfReport_.connectTermsToNetsCalls;
        const SVPerfScopedTimer timer(
          svPerfReport_,
          svPerfReport_.connectTermsToNetsDuration);
#endif
        connectTermsToNets(design);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("inferMemories"));
#endif
        inferMemories(design, body);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("prepareInferredMemories"));
#endif
        prepareInferredMemories(design);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("createContinuousAssigns"));
        ++svPerfReport_.createContinuousAssignsCalls;
        const SVPerfScopedTimer timer(
          svPerfReport_,
          svPerfReport_.createContinuousAssignsDuration);
#endif
        createContinuousAssigns(design, body);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("createInstances"));
        ++svPerfReport_.createInstancesCalls;
        const SVPerfScopedTimer timer(svPerfReport_, svPerfReport_.createInstancesDuration);
#endif
        createInstances(design, body);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("createSequentialLogic"));
        ++svPerfReport_.createSequentialLogicCalls;
        const SVPerfScopedTimer timer(
          svPerfReport_,
          svPerfReport_.createSequentialLogicDuration);
#endif
        createSequentialLogic(design, body);
      }
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(makePerfScopeName("finalizeInferredMemories"));
#endif
        finalizeInferredMemories(design);
      }
      validateLoweringCoverage(body, defName, LoweringCoverageScope::ModuleBody);

      if (config_.blackboxDetection_ and design->isStandard()) {
        if (design->getInstances().empty() and allNetsArePortNets(design)) {
          design->setType(SNLDesign::Type::UserBlackBox);
        }
      }

      return design;
    }

    struct SourceInfo {
      std::string file;
      size_t line {0};
      size_t column {0};
      size_t endLine {0};
      size_t endColumn {0};
    };

    struct InferredMemoryReadPort {
      const slang::ast::Expression* selectorExpr {nullptr};
      SNLBusNet* addrNet {nullptr};
      SNLBusNet* dataNet {nullptr};
    };

    struct InferredMemoryGuard {
      enum class Kind {
        Condition,
        CaseItemMatch
      };

      Kind kind {Kind::Condition};
      bool polarity {true};
      const slang::ast::Expression* expr {nullptr};
      const slang::ast::Expression* caseExpr {nullptr};
      const slang::ast::CaseStatement* caseStmt {nullptr};
      const slang::ast::CaseStatement::ItemGroup* caseItem {nullptr};
      std::optional<slang::SourceRange> sourceRange {};
    };

    struct InferredMemoryWriteAction {
      const slang::ast::Expression* lhsExpr {nullptr};
      const slang::ast::Expression* selectorExpr {nullptr};
      const slang::ast::Expression* rhsExpr {nullptr};
      int8_t stepDelta {0};
      std::optional<slang::ast::BinaryOperator> compoundOp {};
      size_t bitOffset {0};
      size_t bitWidth {0};
      std::vector<InferredMemoryGuard> guards {};
      std::optional<slang::SourceRange> sourceRange {};
    };

    struct InferredMemoryCommitGuard {
      bool polarity {true};
      const slang::ast::Expression* expr {nullptr};
      std::optional<slang::SourceRange> sourceRange {};
    };

    struct InferredMemoryCommitAction {
      const slang::ast::Expression* rhsExpr {nullptr};
      const slang::ast::ValueSymbol* selectorSymbol {nullptr};
      size_t bitOffset {0};
      size_t bitWidth {0};
      std::vector<InferredMemoryCommitGuard> guards {};
      std::optional<slang::SourceRange> sourceRange {};
    };

    struct InferredMemoryWritePort {
      const slang::ast::Expression* selectorExpr {nullptr};
      const slang::ast::Expression* rhsExpr {nullptr};
      SNLBusNet* addrNet {nullptr};
      SNLBusNet* dataNet {nullptr};
      SNLScalarNet* weNet {nullptr};
      std::vector<SNLBitNet*> dataBits {};
      std::optional<slang::SourceRange> sourceRange {};
    };

    struct InferredMemory {
      const slang::ast::ValueSymbol* stateSymbol {nullptr};
      const slang::ast::ValueSymbol* shadowSymbol {nullptr};
      const slang::ast::ValueSymbol* commitSymbol {nullptr};
      const slang::ast::ProceduralBlockSymbol* combBlock {nullptr};
      const slang::ast::ProceduralBlockSymbol* commitBlock {nullptr};
      const slang::ast::ProceduralBlockSymbol* seqBlock {nullptr};
      const slang::ast::Expression* clockExpr {nullptr};
      const slang::ast::Expression* resetSignalExpr {nullptr};
      NLDB0::MemorySignature signature {};
      std::vector<bool> initBits {};
      std::map<int32_t, std::vector<InferredMemoryCommitAction>> commitActionsByIndex {};
      std::vector<InferredMemoryWriteAction> directWriteActions {};
      std::vector<InferredMemoryWritePort> writePorts {};
      std::vector<InferredMemoryReadPort> readPorts {};
      std::unordered_map<const slang::ast::Expression*, size_t> readPortBySelectorExpr {};
      std::optional<slang::SourceRange> sourceRange {};
      bool lowered {false};
    };

    std::optional<slang::SourceRange> getSourceRange(const Symbol& symbol) const {
      std::optional<slang::SourceRange> range;
      if (symbol.location.valid()) {
        range = slang::SourceRange(symbol.location, symbol.location);
      }
      if (auto syntax = symbol.getSyntax()) {
        auto syntaxRange = syntax->sourceRange();
        if (syntaxRange.start().valid()) {
          range = syntaxRange;
        }
      }
      return range;
    }

    std::optional<slang::SourceRange> getSourceRange(const Expression& expression) const {
      if (expression.sourceRange.start().valid()) {
        return expression.sourceRange;
      }
      return std::nullopt; // LCOV_EXCL_LINE
    }

    std::optional<slang::SourceRange> getSourceRange(const Statement& statement) const {
      if (statement.sourceRange.start().valid()) {
        return statement.sourceRange;
      }
      return std::nullopt; // LCOV_EXCL_LINE
    }

    std::optional<slang::SourceRange> getSourceRange(const TimingControl& timing) const {
      if (!timing.sourceRange.start().valid()) {
        return std::nullopt; // LCOV_EXCL_LINE
      }
      return timing.sourceRange;
    }

    std::optional<SourceInfo> getSourceInfo(
      const std::optional<slang::SourceRange>& maybeRange) const {
      if (!maybeRange || !compilation_) {
        return std::nullopt; // LCOV_EXCL_LINE
      }
      auto sourceManager = compilation_->getSourceManager();
      if (!sourceManager) {
        return std::nullopt; // LCOV_EXCL_LINE
      }

      auto sourceRange = sourceManager->getFullyOriginalRange(*maybeRange);
      auto start = sourceRange.start();
      auto end = sourceRange.end();
      if (!start.valid()) {
        return std::nullopt; // LCOV_EXCL_LINE
      }

      start = sourceManager->getFullyOriginalLoc(start);
      if (!start.valid() || !sourceManager->isFileLoc(start)) {
        return std::nullopt; // LCOV_EXCL_LINE
      }

      if (end.valid()) {
        end = sourceManager->getFullyOriginalLoc(end);
      }
      if (!end.valid() || !sourceManager->isFileLoc(end) || end < start) {
        end = start; // LCOV_EXCL_LINE
      } else if (end > start) {
        // sourceRange end is exclusive: map to the previous character.
        end -= 1;
      }

      SourceInfo sourceInfo;
      sourceInfo.file = sourceManager->getFileName(start);
      // LCOV_EXCL_START
      if (sourceInfo.file.empty()) {
        sourceInfo.file = sourceManager->getRawFileName(start.buffer());
      }
      // LCOV_EXCL_STOP
      sourceInfo.line = sourceManager->getLineNumber(start);
      sourceInfo.column = sourceManager->getColumnNumber(start);
      sourceInfo.endLine = sourceManager->getLineNumber(end);
      sourceInfo.endColumn = sourceManager->getColumnNumber(end);
      return sourceInfo;
    }

    std::optional<std::string> getSourceExcerpt(
      const std::optional<slang::SourceRange>& maybeRange,
      size_t maxLength = 160) const {
      if (!maybeRange || !compilation_) {
        return std::nullopt;
      }
      auto* sourceManager = compilation_->getSourceManager();
      if (!sourceManager) {
        return std::nullopt; // LCOV_EXCL_LINE
      }

      auto sourceRange = sourceManager->getFullyOriginalRange(*maybeRange);
      auto start = sourceManager->getFullyOriginalLoc(sourceRange.start());
      auto end = sourceRange.end().valid()
                   ? sourceManager->getFullyOriginalLoc(sourceRange.end())
                   : sourceRange.end();
      if (!start.valid() || !sourceManager->isFileLoc(start)) {
        return std::nullopt;
      }
      if (!end.valid() || !sourceManager->isFileLoc(end) || end.buffer() != start.buffer()) {
        end = start + 1;
      } else if (end <= start) {
        end = start + 1;
      }

      auto sourceText = sourceManager->getSourceText(start.buffer());
      if (sourceText.empty()) {
        return std::nullopt;
      }
      auto startOffset = start.offset();
      auto endOffset = std::min(end.offset(), sourceText.size());
      if (startOffset >= sourceText.size() || startOffset >= endOffset) {
        return std::nullopt;
      }

      std::string excerpt(sourceText.substr(startOffset, endOffset - startOffset));
      std::string normalized;
      normalized.reserve(excerpt.size());
      bool previousWasSpace = false;
      for (unsigned char c : excerpt) {
        if (std::isspace(c)) {
          if (!normalized.empty() && !previousWasSpace) {
            normalized.push_back(' ');
          }
          previousWasSpace = true;
          continue;
        }
        normalized.push_back(static_cast<char>(c));
        previousWasSpace = false;
      }
      while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
      }
      if (normalized.empty()) {
        return std::nullopt;
      }
      if (normalized.size() > maxLength) {
        normalized.resize(maxLength - 3);
        normalized += "...";
      }
      return normalized;
    }

    std::string formatReasonWithSourceExcerpt(
      const std::string& reason,
      const std::optional<slang::SourceRange>& maybeRange) const {
      auto excerpt = getSourceExcerpt(maybeRange);
      if (!excerpt) {
        return reason;
      }
      std::ostringstream message;
      message << reason << " [RTL: " << *excerpt << "]";
      return message.str();
    }

    void reportUnsupportedElement(
      const std::string& reason,
      const std::optional<slang::SourceRange>& maybeRange = std::nullopt) {
      std::ostringstream message;
      if (auto sourceInfo = getSourceInfo(maybeRange)) {
        message << sourceInfo->file << ":" << sourceInfo->line << ":" << sourceInfo->column
                << ": ";
      }
      message << reason;
      auto unsupported = message.str();
      unsupportedElements_.push_back(unsupported);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      if (svPerfReport_.enabled && svPerfReport_.firstUnsupported.empty()) {
        svPerfReport_.firstUnsupported = unsupported;
      }
#endif
    }

    void reportWarning(
      const std::string& reason,
      const std::optional<slang::SourceRange>& maybeRange = std::nullopt) {
      std::ostringstream message;
      if (auto sourceInfo = getSourceInfo(maybeRange)) {
        message << sourceInfo->file << ":" << sourceInfo->line << ":" << sourceInfo->column
                << ": ";
      }
      message << "Warning: " << reason;
      auto warning = message.str();
      if (!emittedWarnings_.insert(warning).second) {
        return;
      }
      warnings_.push_back(warning);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      if (svPerfReport_.enabled && svPerfReport_.firstWarning.empty()) {
        svPerfReport_.firstWarning = warning;
      }
#endif
      NAJA_LOG_WARN("{}", warning);
    }

    void reportCaseComparison2StateWarning(
      slang::ast::BinaryOperator op,
      const std::optional<slang::SourceRange>& maybeRange = std::nullopt) {
      if (!isCaseComparisonBinaryOp(op)) {
        return;
      }
      std::ostringstream reason;
      reason << "Case comparison operator '" << slang::ast::OpInfo::getText(op)
             << "' lowered as 2-state comparison in SNL (X/Z distinction is not preserved)";
      reportWarning(reason.str(), maybeRange);
    }

    bool shouldSuppressCaseComparison2StateWarning(
      const slang::ast::CaseStatement& caseStmt) const {
      if (!caseStmt.expr.type->getCanonicalType().isEnum()) {
        return false;
      }
      for (const auto& item : caseStmt.items) {
        for (const auto* itemExpr : item.expressions) {
          if (!itemExpr || !isKnown2StateConstantExpr(*itemExpr)) {
            return false;
          }
        }
      }
      return true;
    }

    void throwIfUnsupportedElements() const {
      if (unsupportedElements_.empty()) {
        return;
      }
      std::ostringstream reason;
      reason << "Unsupported SystemVerilog elements encountered (" << unsupportedElements_.size()
             << "):";
      for (const auto& unsupported : unsupportedElements_) {
        reason << "\n - " << unsupported;
      }
      throw SNLSVConstructorException(reason.str());
    }

    SNLRTLInfos* getOrCreateRTLInfos(SNLDesign* design) const {
      if (!design) {
        return nullptr; // LCOV_EXCL_LINE
      }
      auto* rtlInfos = design->getRTLInfos();
      if (!rtlInfos) {
        rtlInfos = SNLRTLInfos::create(design);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.rtlInfoCreateDesignCalls;
#endif
      }
      return rtlInfos;
    }

    enum class LoweringCoverageScope {
      ModuleBody,
      GenerateBody
    };

    const char* getLoweringCoverageScopeLabel(LoweringCoverageScope scope) const {
      switch (scope) {
        case LoweringCoverageScope::ModuleBody:
          return "module body";
        case LoweringCoverageScope::GenerateBody:
          return "generate block";
      }
      return "scope"; // LCOV_EXCL_LINE
    }

    bool isIgnorableLoweringCoverageMember(const Symbol& sym) const {
      if (sym.isType()) {
        return true;
      }

      switch (sym.kind) {
        case SymbolKind::TransparentMember:
        case SymbolKind::EmptyMember:
        case SymbolKind::StatementBlock:
        case SymbolKind::Parameter:
        case SymbolKind::TypeParameter:
        case SymbolKind::Port:
        case SymbolKind::MultiPort:
        case SymbolKind::InterfacePort:
        case SymbolKind::Modport:
        case SymbolKind::ModportPort:
        case SymbolKind::ModportClocking:
        case SymbolKind::ExplicitImport:
        case SymbolKind::WildcardImport:
        case SymbolKind::Attribute:
        case SymbolKind::Genvar:
        case SymbolKind::DefParam:
        case SymbolKind::Specparam:
        case SymbolKind::Subroutine:
        case SymbolKind::ElabSystemTask:
          return true;
        default:
          return false;
      }
    }

    bool isLoweredInCoverageScope(SymbolKind kind, LoweringCoverageScope scope) const {
      switch (scope) {
        case LoweringCoverageScope::ModuleBody:
          switch (kind) {
            case SymbolKind::Net:
            case SymbolKind::Variable:
            case SymbolKind::ContinuousAssign:
            case SymbolKind::Instance:
            case SymbolKind::ProceduralBlock:
              return true;
            default:
              return false;
          }
        case LoweringCoverageScope::GenerateBody:
          switch (kind) {
            case SymbolKind::Net:
            case SymbolKind::Variable:
            case SymbolKind::ContinuousAssign:
            case SymbolKind::Instance:
            case SymbolKind::ProceduralBlock:
              return true;
            default:
              return false;
          }
      }
      return false; // LCOV_EXCL_LINE
    }

    void validateLoweringCoverage(
      const slang::ast::Scope& scope,
      const std::string& moduleName,
      LoweringCoverageScope coverageScope) {
      for (const auto& sym : scope.members()) {
        if (isIgnorableLoweringCoverageMember(sym)) {
          continue;
        }

        if (sym.kind == SymbolKind::GenerateBlock) {
          const auto& generateBlock = sym.as<slang::ast::GenerateBlockSymbol>();
          if (!generateBlock.isUninstantiated) {
            validateLoweringCoverage(
              generateBlock,
              moduleName,
              LoweringCoverageScope::GenerateBody);
          }
          continue;
        }

        if (sym.kind == SymbolKind::GenerateBlockArray) {
          const auto& generateBlockArray =
            sym.as<slang::ast::GenerateBlockArraySymbol>();
          for (const auto* entry : generateBlockArray.entries) {
            if (entry && !entry->isUninstantiated) {
              validateLoweringCoverage(
                *entry,
                moduleName,
                LoweringCoverageScope::GenerateBody);
            }
          }
          continue;
        }

        if (isLoweredInCoverageScope(sym.kind, coverageScope)) {
          continue;
        }

        std::ostringstream reason;
        reason << "Unsupported elaborated member in "
               << getLoweringCoverageScopeLabel(coverageScope)
               << " of module '" << moduleName << "': " << toString(sym.kind);
        if (!sym.name.empty()) {
          reason << " '" << sym.name << "'";
        }
        reportUnsupportedElement(reason.str(), getSourceRange(sym));
      }
    }

    template <typename Visitor>
    void visitElaboratedNonGenerateMembers(
      const slang::ast::Scope& scope,
      Visitor& visitor) const {
      for (const auto& sym : scope.members()) {
        if (sym.kind == SymbolKind::GenerateBlock) {
          const auto& generateBlock = sym.as<slang::ast::GenerateBlockSymbol>();
          if (!generateBlock.isUninstantiated) {
            visitElaboratedNonGenerateMembers(generateBlock, visitor);
          }
          continue;
        }
        if (sym.kind == SymbolKind::GenerateBlockArray) {
          const auto& generateBlockArray =
            sym.as<slang::ast::GenerateBlockArraySymbol>();
          for (const auto* entry : generateBlockArray.entries) {
            if (entry && !entry->isUninstantiated) {
              visitElaboratedNonGenerateMembers(*entry, visitor);
            }
          }
          continue;
        }
        visitor(sym);
      }
    }

    SNLRTLInfos* getOrCreateRTLInfos(SNLDesignObject* designObject) const {
      if (!designObject) {
        return nullptr; // LCOV_EXCL_LINE
      }
      auto* rtlInfos = designObject->getRTLInfos();
      if (!rtlInfos) {
        rtlInfos = SNLRTLInfos::create(designObject);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.rtlInfoCreateDesignObjectCalls;
#endif
      }
      return rtlInfos;
    }

    void addRTLInfo(
      SNLRTLInfos* rtlInfos,
      const char* name,
      const std::string& value) const {
      if (!rtlInfos) {
        return; // LCOV_EXCL_LINE
      }
      rtlInfos->setInfo(NLName(name), value);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      ++svPerfReport_.rtlInfoSetCalls;
#endif
    }

    void cloneRTLInfos(
      const SNLDesignObject* from,
      SNLDesignObject* to) const {
      if (!from || !to) {
        return; // LCOV_EXCL_LINE
      }
      auto* fromInfos = from->getRTLInfos();
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      ++svPerfReport_.rtlInfoCloneCalls;
      svPerfReport_.rtlInfoClonedEntries += fromInfos->getInfos().size();
      const SVPerfScopedTimer timer(svPerfReport_, svPerfReport_.cloneRTLInfosDuration);
#endif
      auto* toInfos = to->getRTLInfos();
      if (!toInfos) {
        toInfos = SNLRTLInfos::create(to);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.rtlInfoCreateDesignObjectCalls;
#endif
      }
      toInfos->cloneInfos(*fromInfos);
    }

    template<typename TObject>
    void annotateSourceInfo(
      TObject* object,
      const SourceInfo& sourceInfo) const {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      ++svPerfReport_.annotateSourceInfoCalls;
      const SVPerfScopedTimer timer(svPerfReport_, svPerfReport_.annotateSourceInfoDuration);
#endif
      auto* rtlInfos = getOrCreateRTLInfos(object);
      addRTLInfo(rtlInfos, "sv_src_file", sourceInfo.file);
      const auto line = std::to_string(sourceInfo.line);
      addRTLInfo(rtlInfos, "sv_src_line", line);
      const auto column = std::to_string(sourceInfo.column);
      addRTLInfo(rtlInfos, "sv_src_column", column);
      const auto endLine = std::to_string(sourceInfo.endLine);
      addRTLInfo(rtlInfos, "sv_src_end_line", endLine);
      const auto endColumn = std::to_string(sourceInfo.endColumn);
      addRTLInfo(rtlInfos, "sv_src_end_column", endColumn);
    }

    void annotateSourceInfo(
      NLObject* object,
      const std::optional<slang::SourceRange>& maybeRange) const {
      auto sourceInfo = getSourceInfo(maybeRange);
      if (!sourceInfo) {
        return; // LCOV_EXCL_LINE
      }
      if (auto* design = dynamic_cast<SNLDesign*>(object)) {
        annotateSourceInfo(design, *sourceInfo);
        return;
      }
      if (auto* designObject = dynamic_cast<SNLDesignObject*>(object)) {
        annotateSourceInfo(designObject, *sourceInfo);
      }
    }

    const Expression* stripConnectionLValueArgConversions(const Expression& expr) const {
      const Expression* current = &expr;
      while (current) {
        if (current->kind == slang::ast::ExpressionKind::Conversion) {
          current = &current->as<slang::ast::ConversionExpression>().operand();
          continue;
        }
        if (current->kind == slang::ast::ExpressionKind::Assignment) {
          const auto& assign = current->as<slang::ast::AssignmentExpression>();
          if (assign.isLValueArg()) {
            current = &assign.left();
            continue;
          }
        } // LCOV_EXCL_LINE
        break;
      }
      return current ? stripConversions(*current) : nullptr;
    }

    SNLNet* resolveExpressionNet(SNLDesign* design, const Expression& expr) {
      const Expression* current = stripConnectionLValueArgConversions(expr);
      if (current && slang::ast::ValueExpressionBase::isKind(current->kind)) {
        const auto& valueExpr = current->as<slang::ast::ValueExpressionBase>();
        const auto& symbol = valueExpr.symbol;
        if (auto found = loweredValueSymbolNets_.find(&symbol);
            found != loweredValueSymbolNets_.end()) {
          return found->second;
        }
        for (auto it = activeFunctionArgumentNets_.rbegin();
             it != activeFunctionArgumentNets_.rend();
             ++it) {
          auto found = it->find(&symbol);
          if (found != it->end()) {
            return found->second;
          }
        }
        if (auto unsupportedTypeReason = getUnsupportedTypeReason(symbol.getType())) {
          std::ostringstream reason;
          reason << *unsupportedTypeReason << " for symbol: " << std::string(symbol.name);
          reportUnsupportedElement(reason.str(), getSourceRange(expr));
          return nullptr;
        }
        return getOrCreateNet(design, std::string(symbol.name), symbol.getType());
      }
      return nullptr;
    }

    size_t getMemoryAddressWidth(size_t depth) const {
      size_t abits = 0;
      size_t limit = 1;
      while (limit < depth) {
        ++abits;
        limit <<= 1;
      }
      return std::max<size_t>(1, abits);
    }

    bool getSupportedMemorySignature(
      const Type& type,
      NLDB0::MemorySignature& signature) const {
      const auto& canonical = type.getCanonicalType();
      if (!canonical.isUnpackedArray() || !canonical.hasFixedRange()) {
        return false;
      }
      const auto* elementType = canonical.getArrayElementType();
      if (!elementType) {
        return false; // LCOV_EXCL_LINE
      }
      const auto& elementCanonical = elementType->getCanonicalType();
      if (!elementCanonical.isBitstreamType()) {
        return false; // LCOV_EXCL_LINE
      }
      const auto width = static_cast<size_t>(elementCanonical.getBitstreamWidth());
      const auto depth = static_cast<size_t>(canonical.getFixedRange().width());
      if (!width || !depth) {
        return false; // LCOV_EXCL_LINE
      }
      signature.width = width;
      signature.depth = depth;
      signature.abits = getMemoryAddressWidth(depth);
      return true;
    }

    bool tryGetValueSymbolReference(
      const Expression& expr,
      const slang::ast::ValueSymbol*& symbol) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped || !slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        return false;
      }
      symbol = &stripped->as<slang::ast::ValueExpressionBase>().symbol;
      return true;
    }

    bool tryGetRootValueSymbolReference(
      const Expression& expr,
      const slang::ast::ValueSymbol*& symbol) const {
      const Expression* current = stripConversions(expr);
      while (current) {
        if (slang::ast::ValueExpressionBase::isKind(current->kind)) {
          symbol = &current->as<slang::ast::ValueExpressionBase>().symbol;
          return true;
        }
        switch (current->kind) {
          case slang::ast::ExpressionKind::ElementSelect:
            current = &current->as<slang::ast::ElementSelectExpression>().value();
            continue;
          case slang::ast::ExpressionKind::RangeSelect:
            current = &current->as<slang::ast::RangeSelectExpression>().value();
            continue;
          case slang::ast::ExpressionKind::MemberAccess:
            current = &current->as<slang::ast::MemberAccessExpression>().value();
            continue;
          // LCOV_EXCL_START
          default:
            return false;
          // LCOV_EXCL_STOP
        }
      }
      return false; // LCOV_EXCL_LINE
    }

    const Expression* getSelectionBaseExpression(const Expression& expr) const {
      const Expression* current = stripConversions(expr);
      while (current) {
        switch (current->kind) {
          case slang::ast::ExpressionKind::ElementSelect:
            current =
              stripConversions(current->as<slang::ast::ElementSelectExpression>().value());
            continue;
          case slang::ast::ExpressionKind::RangeSelect:
            current =
              stripConversions(current->as<slang::ast::RangeSelectExpression>().value());
            continue;
          case slang::ast::ExpressionKind::MemberAccess:
            current =
              stripConversions(current->as<slang::ast::MemberAccessExpression>().value());
            continue;
          default:
            return current;
        }
      }
      return nullptr; // LCOV_EXCL_LINE
    }

    bool isValueSymbolReference(
      const Expression& expr,
      const slang::ast::ValueSymbol& symbol) const {
      const slang::ast::ValueSymbol* referenced = nullptr;
      return tryGetValueSymbolReference(expr, referenced) && referenced == &symbol;
    }

    bool shouldIgnoreTrackedLHS(
      const Expression* expr,
      const std::unordered_set<const slang::ast::ValueSymbol*>* ignoredSymbols) const {
      if (!expr || !ignoredSymbols || ignoredSymbols->empty()) {
        return false;
      }
      const slang::ast::ValueSymbol* symbol = nullptr;
      return tryGetRootValueSymbolReference(*expr, symbol) && ignoredSymbols->contains(symbol);
    }

    SNLBusNet* getOrCreateNamedBusNet(
      SNLDesign* design,
      const std::string& baseName,
      size_t width,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto name = baseName.empty() ? std::string("tmp_bus") : baseName;
      int suffix = 0;
      while (true) {
        if (auto* existing = design->getNet(NLName(name))) {
          // LCOV_EXCL_START
          if (auto* bus = dynamic_cast<SNLBusNet*>(existing); bus && bus->getWidth() == width) {
            return bus;
          }
          // LCOV_EXCL_STOP
        } else {
          auto* net = SNLBusNet::create(
            design,
            static_cast<NLID::Bit>(width - 1),
            0,
            NLName(name));
          annotateSourceInfo(net, sourceRange);
          return net;
        }
        name = baseName + "_" + std::to_string(suffix++); // LCOV_EXCL_LINE
      }
    }

    SNLScalarNet* getOrCreateNamedScalarNet(
      SNLDesign* design,
      const std::string& baseName,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto name = baseName.empty() ? std::string("tmp_scalar") : baseName;
      int suffix = 0;
      while (true) {
        if (auto* existing = design->getNet(NLName(name))) {
          // LCOV_EXCL_START
          if (auto* scalar = dynamic_cast<SNLScalarNet*>(existing)) {
            return scalar;
          }
          // LCOV_EXCL_STOP
        } else {
          auto* net = SNLScalarNet::create(design, NLName(name));
          annotateSourceInfo(net, sourceRange);
          return net;
        }
        name = baseName + "_" + std::to_string(suffix++); // LCOV_EXCL_LINE
      }
    }

    bool flattenConstantValueToBits(
      const slang::ConstantValue& value,
      std::vector<bool>& bits) const {
      if (!value) {
        return false; // LCOV_EXCL_LINE
      }
      if (value.isInteger()) {
        const auto& intValue = value.integer();
        if (intValue.hasUnknown()) {
          return false;
        }
        const size_t width = intValue.getBitWidth();
        bits.reserve(bits.size() + width);
        for (size_t bit = 0; bit < width; ++bit) {
          bits.push_back(static_cast<bool>(intValue[bit]));
        }
        return true;
      }
      if (value.isUnpacked()) {
        auto elements = value.elements();
        for (const auto& element : elements) {
          if (!flattenConstantValueToBits(element, bits)) {
            return false;
          }
        }
        return true;
      }
      slang::ConstantValue converted = value.convertToInt();
      if (converted && converted.isInteger()) {
        return flattenConstantValueToBits(converted, bits);
      }
      return false; // LCOV_EXCL_LINE
    }

    std::optional<size_t> getExpressionBitstreamWidth(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return std::nullopt; // LCOV_EXCL_LINE
      }
      const auto& canonical = stripped->type->getCanonicalType();
      const auto bitWidth = canonical.getBitstreamWidth();
      if (bitWidth <= 0) {
        return std::nullopt;
      }
      return static_cast<size_t>(bitWidth);
    }

    std::optional<std::vector<bool>> getConstantInitBits(
      const Expression& expr,
      const Symbol& evalSymbol,
      size_t totalWidth) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return std::nullopt; // LCOV_EXCL_LINE
      }

      const slang::ConstantValue* value = stripped->getConstant();
      slang::ConstantValue evaluatedValue;
      if (!value) {
        slang::ast::EvalContext evalContext(evalSymbol);
        evaluatedValue = stripped->eval(evalContext);
        if (evaluatedValue) {
          value = &evaluatedValue;
        }
      }
      if (!value) {
        return std::nullopt;
      }

      std::vector<bool> bits;
      if (!flattenConstantValueToBits(*value, bits)) {
        return std::nullopt;
      }
      if (bits.size() < totalWidth) {
        bits.resize(totalWidth, false);
      }
      if (bits.size() != totalWidth) {
        return std::nullopt;
      }
      return bits;
    }

    std::string encodeInitBits(const std::vector<bool>& bits) const {
      std::string encoded;
      encoded.reserve(bits.size() + 32);
      encoded += std::to_string(bits.size());
      encoded += "'b";
      for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        encoded += *it ? '1' : '0';
      }
      return encoded;
    }

    void connectBusNetBits(
      SNLDesign* design,
      SNLBusNet* busNet,
      const std::vector<SNLBitNet*>& bits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto busBits = collectBits(busNet);
      if (busBits.size() != bits.size()) {
        // LCOV_EXCL_START
        throw SNLSVConstructorException(
          "Internal error: bus width mismatch while connecting inferred memory net");
        // LCOV_EXCL_STOP
      }
      for (size_t i = 0; i < bits.size(); ++i) {
        createAssignInstance(design, bits[i], busBits[i], sourceRange);
      }
    }

    bool tryGetTopLevelWholeArrayCopy(
      const slang::ast::ProceduralBlockSymbol& block,
      const slang::ast::ValueSymbol*& lhsSymbol,
      const slang::ast::ValueSymbol*& rhsSymbol) const {
      lhsSymbol = nullptr;
      rhsSymbol = nullptr;

      const Statement* stmt = unwrapStatement(block.getBody());
      if (!stmt) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<const Statement*> topLevelStatements;
      if (stmt->kind == slang::ast::StatementKind::List) {
        for (const auto* item : stmt->as<slang::ast::StatementList>().list) {
          if (item) {
            topLevelStatements.push_back(item);
          }
        }
      } else {
        topLevelStatements.push_back(stmt);
      }

      for (const auto* topLevelStmt : topLevelStatements) {
        const auto* current = topLevelStmt ? unwrapStatement(*topLevelStmt) : nullptr;
        if (!current ||
            current->kind != slang::ast::StatementKind::ExpressionStatement) {
          continue;
        }
        const auto& expr = current->as<slang::ast::ExpressionStatement>().expr;
        if (expr.kind != slang::ast::ExpressionKind::Assignment) {
          continue;
        }
        const auto& assign = expr.as<slang::ast::AssignmentExpression>();
        if (assign.op) {
          continue;
        }
        if (!tryGetValueSymbolReference(assign.left(), lhsSymbol) ||
            !tryGetValueSymbolReference(assign.right(), rhsSymbol) ||
            lhsSymbol == rhsSymbol) {
          continue;
        }
        return true;
      }
      lhsSymbol = nullptr;
      rhsSymbol = nullptr;
      return false;
    }

    const slang::ast::ProceduralBlockSymbol* findWholeArrayCopyBlock(
      const std::vector<const slang::ast::ProceduralBlockSymbol*>& combinationalBlocks,
      const slang::ast::ValueSymbol& lhsSymbol,
      const slang::ast::ValueSymbol& rhsSymbol,
      const slang::ast::ProceduralBlockSymbol* excludeBlock = nullptr) const {
      for (const auto* block : combinationalBlocks) {
        if (!block || block == excludeBlock) {
          continue;
        }
        const slang::ast::ValueSymbol* blockLhs = nullptr;
        const slang::ast::ValueSymbol* blockRhs = nullptr;
        if (!tryGetTopLevelWholeArrayCopy(*block, blockLhs, blockRhs)) {
          continue;
        }
        if (blockLhs == &lhsSymbol && blockRhs == &rhsSymbol) {
          return block;
        }
      }
      return nullptr;
    }

    bool tryEvaluateConstantConditionBit(const Expression& expr, bool& value) const {
      if (getConstantBit(expr, value)) {
        return true;
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }

      if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
        if (unaryExpr.op == slang::ast::UnaryOperator::LogicalNot ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
          bool operandValue = false;
          if (!tryEvaluateConstantConditionBit(unaryExpr.operand(), operandValue)) {
            return false;
          }
          value = !operandValue;
          return true;
        }
        return false;
      }

      if (stripped->kind != slang::ast::ExpressionKind::BinaryOp) {
        return false;
      }

      const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
      int64_t leftValue = 0;
      int64_t rightValue = 0;
      if (!getConstantInt64(binaryExpr.left(), leftValue) ||
          !getConstantInt64(binaryExpr.right(), rightValue)) {
        return false;
      }

      switch (binaryExpr.op) {
        case slang::ast::BinaryOperator::LessThan:
          value = leftValue < rightValue;
          return true;
        case slang::ast::BinaryOperator::LessThanEqual:
          value = leftValue <= rightValue;
          return true;
        case slang::ast::BinaryOperator::GreaterThan:
          value = leftValue > rightValue;
          return true;
        case slang::ast::BinaryOperator::GreaterThanEqual:
          value = leftValue >= rightValue;
          return true;
        case slang::ast::BinaryOperator::Equality:
        case slang::ast::BinaryOperator::CaseEquality:
          value = leftValue == rightValue;
          return true;
        case slang::ast::BinaryOperator::Inequality:
        case slang::ast::BinaryOperator::CaseInequality:
          value = leftValue != rightValue;
          return true;
        default:
          return false;
      }
    }

    bool tryExtractInferredMemoryEntryTarget(
      const Expression& lhsExpr,
      const slang::ast::ValueSymbol& entrySymbol,
      size_t entryWidth,
      const slang::ast::Expression*& selectorExpr,
      size_t& bitOffset,
      size_t& bitWidth,
      std::string& failureReason) const {
      const auto* stripped = stripConversions(lhsExpr);
      if (!stripped) {
        // LCOV_EXCL_START
        failureReason = "unsupported inferred memory null write target";
        return false;
        // LCOV_EXCL_STOP
      }

      auto widthBits = getIntegralExpressionBitWidth(*stripped);
      if (!widthBits || !*widthBits) {
        failureReason = "unsupported inferred memory write target without integral width";
        return false;
      }
      bitWidth = *widthBits;

      if (stripped->kind == slang::ast::ExpressionKind::ElementSelect) {
        const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
        if (isValueSymbolReference(elementExpr.value(), entrySymbol)) {
          selectorExpr = &elementExpr.selector();
          bitOffset = 0;
          return bitWidth <= entryWidth;
        }

        const auto* baseExpr = stripConversions(elementExpr.value());
        if (!baseExpr) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory packed element-select base";
          return false;
          // LCOV_EXCL_STOP
        }

        const auto& baseType = baseExpr->type->getCanonicalType();
        if (!baseType.hasFixedRange()) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory packed element-select without fixed range";
          return false;
          // LCOV_EXCL_STOP
        }

        size_t baseOffset = 0;
        size_t baseWidth = 0;
        if (!tryExtractInferredMemoryEntryTarget(
              *baseExpr,
              entrySymbol,
              entryWidth,
              selectorExpr,
              baseOffset,
              baseWidth,
              failureReason)) {
          return false;
        }

        int32_t selectedIndex = 0;
        if (!getConstantInt32(elementExpr.selector(), selectedIndex)) {
          failureReason =
            "unsupported inferred memory dynamic packed element-select in shadow write";
          return false;
        }
        const auto translated = baseType.getFixedRange().translateIndex(selectedIndex);
        if (translated < 0 ||
            translated >= static_cast<int32_t>(baseType.getFixedRange().width())) {
          failureReason = "unsupported inferred memory out-of-range packed element-select";
          return false;
        }
        bitOffset = baseOffset + static_cast<size_t>(translated) * bitWidth;
        return bitOffset + bitWidth <= entryWidth &&
               bitOffset + bitWidth <= baseOffset + baseWidth;
      }

      if (stripped->kind == slang::ast::ExpressionKind::MemberAccess) {
        const auto& memberExpr = stripped->as<slang::ast::MemberAccessExpression>();
        if (memberExpr.member.kind != SymbolKind::Field) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory non-field member write";
          return false;
          // LCOV_EXCL_STOP
        }
        size_t baseOffset = 0;
        size_t baseWidth = 0;
        if (!tryExtractInferredMemoryEntryTarget(
              memberExpr.value(),
              entrySymbol,
              entryWidth,
              selectorExpr,
              baseOffset,
              baseWidth,
              failureReason)) {
          return false;
        }
        const auto& field = memberExpr.member.as<slang::ast::FieldSymbol>();
        bitOffset = baseOffset + static_cast<size_t>(field.bitOffset);
        return bitOffset + bitWidth <= entryWidth &&
               bitOffset + bitWidth <= baseOffset + baseWidth;
      }

      if (stripped->kind == slang::ast::ExpressionKind::RangeSelect) {
        const auto& rangeExpr = stripped->as<slang::ast::RangeSelectExpression>();
        const auto* baseExpr = stripConversions(rangeExpr.value());
        if (!baseExpr) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory range-select base";
          return false;
          // LCOV_EXCL_STOP
        }
        const auto& baseType = baseExpr->type->getCanonicalType();
        if (!baseType.hasFixedRange()) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory range-select without fixed range";
          return false;
          // LCOV_EXCL_STOP
        }

        size_t baseOffset = 0;
        size_t baseWidth = 0;
        if (!tryExtractInferredMemoryEntryTarget(
              *baseExpr,
              entrySymbol,
              entryWidth,
              selectorExpr,
              baseOffset,
              baseWidth,
              failureReason)) {
          return false; // LCOV_EXCL_LINE
        }

        int64_t lsbIndex = 0;
        const auto selectionKind = rangeExpr.getSelectionKind();
        if (selectionKind == slang::ast::RangeSelectionKind::IndexedUp ||
            selectionKind == slang::ast::RangeSelectionKind::IndexedDown) {
          int32_t constantStartIndex = 0;
          int32_t constantSliceWidth = 0;
          if (!getConstantInt32(rangeExpr.left(), constantStartIndex) ||
              !getConstantInt32(rangeExpr.right(), constantSliceWidth) ||
              constantSliceWidth <= 0 ||
              static_cast<size_t>(constantSliceWidth) != bitWidth) {
            failureReason = "unsupported inferred memory dynamic indexed range-select";
            return false;
          }
          lsbIndex = static_cast<int64_t>(constantStartIndex);
          if (selectionKind == slang::ast::RangeSelectionKind::IndexedDown) {
            lsbIndex -= static_cast<int64_t>(constantSliceWidth - 1);
          }
        } else {
          int32_t left = 0;
          int32_t right = 0;
          if (!getConstantInt32(rangeExpr.left(), left) ||
              !getConstantInt32(rangeExpr.right(), right)) {
            // LCOV_EXCL_START
            failureReason = "unsupported inferred memory dynamic simple range-select";
            return false;
            // LCOV_EXCL_STOP
          }
          lsbIndex = std::min<int64_t>(left, right);
        }

        if (lsbIndex < std::numeric_limits<int32_t>::min() ||
            lsbIndex > std::numeric_limits<int32_t>::max()) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory range-select index overflow";
          return false;
          // LCOV_EXCL_STOP
        }
        const auto translated = baseType.getFixedRange().translateIndex(
          static_cast<int32_t>(lsbIndex));
        if (translated < 0 ||
            translated >= static_cast<int32_t>(entryWidth)) {
          failureReason = "unsupported inferred memory out-of-range range-select";
          return false;
        }
        bitOffset = baseOffset + static_cast<size_t>(translated);
        return bitOffset + bitWidth <= entryWidth &&
               bitOffset + bitWidth <= baseOffset + baseWidth;
      }

      failureReason = "unsupported inferred memory shadow write shape";
      return false;
    }

    bool tryExtractInferredMemoryWriteTarget(
      const Expression& lhsExpr,
      const InferredMemory& memory,
      const slang::ast::Expression*& selectorExpr,
      size_t& bitOffset,
      size_t& bitWidth,
      std::string& failureReason) const {
      return tryExtractInferredMemoryEntryTarget(
        lhsExpr,
        *memory.shadowSymbol,
        memory.signature.width,
        selectorExpr,
        bitOffset,
        bitWidth,
        failureReason);
    }

    bool analyzeDirectSequentialMemoryWriteStatement(
      const Statement& stmt,
      const slang::ast::ValueSymbol*& stateSymbol,
      NLDB0::MemorySignature& signature,
      std::vector<InferredMemoryGuard>& guards,
      std::vector<InferredMemoryWriteAction>& actions,
      std::string& failureReason) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty ||
          current->kind == slang::ast::StatementKind::VariableDeclaration) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        for (const auto* item : current->as<slang::ast::StatementList>().list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!analyzeDirectSequentialMemoryWriteStatement(
                *item,
                stateSymbol,
                signature,
                guards,
                actions,
                failureReason)) {
            return false;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        return unrollForLoopStatement(
          forStmt,
          [&]() {
            return analyzeDirectSequentialMemoryWriteStatement(
              forStmt.body,
              stateSymbol,
              signature,
              guards,
              actions,
              failureReason);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        bool constantBit = false;
        if (getConstantBit(*condStmt.conditions[0].expr, constantBit)) {
          const Statement* selectedStmt = constantBit ? &condStmt.ifTrue : condStmt.ifFalse;
          if (!selectedStmt) {
            return true;
          }
          return analyzeDirectSequentialMemoryWriteStatement(
            *selectedStmt,
            stateSymbol,
            signature,
            guards,
            actions,
            failureReason);
        }

        const size_t baseGuardCount = guards.size();
        guards.push_back({
          InferredMemoryGuard::Kind::Condition,
          true,
          condStmt.conditions[0].expr,
          nullptr,
          nullptr,
          nullptr,
          getSourceRange(*condStmt.conditions[0].expr)});
        if (!analyzeDirectSequentialMemoryWriteStatement(
              condStmt.ifTrue,
              stateSymbol,
              signature,
              guards,
              actions,
              failureReason)) {
          return false;
        }
        guards.resize(baseGuardCount);

        if (condStmt.ifFalse) {
          guards.push_back({
            InferredMemoryGuard::Kind::Condition,
            false,
            condStmt.conditions[0].expr,
            nullptr,
            nullptr,
            nullptr,
            getSourceRange(*condStmt.conditions[0].expr)});
          if (!analyzeDirectSequentialMemoryWriteStatement(
                *condStmt.ifFalse,
                stateSymbol,
                signature,
                guards,
                actions,
                failureReason)) {
            return false;
          }
          guards.resize(baseGuardCount);
        }
        return true;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (!extractAssignment(*current, lhsExpr, action)) {
        failureReason = "unsupported direct inferred memory sequential statement";
        return false;
      }
      if (!action.rhs || action.stepDelta != 0 || action.compoundOp) {
        failureReason = "unsupported direct inferred memory assignment action";
        return false;
      }

      const slang::ast::ValueSymbol* assignedSymbol = nullptr;
      if (!lhsExpr || !tryGetRootValueSymbolReference(*lhsExpr, assignedSymbol)) {
        failureReason = "unsupported direct inferred memory assignment target";
        return false;
      }

      NLDB0::MemorySignature assignedSignature;
      if (!getSupportedMemorySignature(assignedSymbol->getType(), assignedSignature)) {
        failureReason = "direct sequential assignment target is not a supported memory";
        return false;
      }

      if (!stateSymbol) {
        stateSymbol = assignedSymbol;
        signature = assignedSignature;
      } else if (assignedSymbol != stateSymbol || !(signature == assignedSignature)) {
        failureReason = "direct inferred memory sequential block writes multiple targets";
        return false;
      }

      const slang::ast::Expression* selectorExpr = nullptr;
      size_t bitOffset = 0;
      size_t bitWidth = 0;
      if (!tryExtractInferredMemoryEntryTarget(
            *lhsExpr,
            *stateSymbol,
            signature.width,
            selectorExpr,
            bitOffset,
            bitWidth,
            failureReason)) {
        return false;
      }

      InferredMemoryWriteAction writeAction;
      writeAction.lhsExpr = lhsExpr;
      writeAction.selectorExpr = selectorExpr;
      writeAction.rhsExpr = action.rhs;
      writeAction.stepDelta = action.stepDelta;
      writeAction.compoundOp = action.compoundOp;
      writeAction.bitOffset = bitOffset;
      writeAction.bitWidth = bitWidth;
      writeAction.guards = guards;
      writeAction.sourceRange = getSourceRange(*lhsExpr);
      actions.push_back(std::move(writeAction));
      return true;
    }

    bool analyzeIndexedCommitStatement(
      const Statement& stmt,
      const slang::ast::ValueSymbol& commitSymbol,
      const slang::ast::ValueSymbol& shadowSymbol,
      const slang::ast::ValueSymbol& stateSymbol,
      size_t entryWidth,
      std::vector<InferredMemoryCommitGuard>& guards,
      std::map<int32_t, std::vector<InferredMemoryCommitAction>>& commitActionsByIndex,
      std::string& failureReason) const {
      (void)shadowSymbol;
      (void)stateSymbol;
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty ||
          current->kind == slang::ast::StatementKind::VariableDeclaration) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        for (const auto* item : current->as<slang::ast::StatementList>().list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!analyzeIndexedCommitStatement(
                *item,
                commitSymbol,
                shadowSymbol,
                stateSymbol,
                entryWidth,
                guards,
                commitActionsByIndex,
                failureReason)) {
            return false;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        return unrollForLoopStatement(
          forStmt,
          [&]() {
            return analyzeIndexedCommitStatement(
              forStmt.body,
              commitSymbol,
              shadowSymbol,
              stateSymbol,
              entryWidth,
              guards,
              commitActionsByIndex,
              failureReason);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        bool constantBit = false;
        if (tryEvaluateConstantConditionBit(*condStmt.conditions[0].expr, constantBit)) {
          const Statement* selectedStmt = constantBit ? &condStmt.ifTrue : condStmt.ifFalse;
          if (!selectedStmt) {
            return true;
          }
          return analyzeIndexedCommitStatement(
            *selectedStmt,
            commitSymbol,
            shadowSymbol,
            stateSymbol,
            entryWidth,
            guards,
            commitActionsByIndex,
            failureReason);
        }

        const size_t baseGuardCount = guards.size();
        guards.push_back({true, condStmt.conditions[0].expr, getSourceRange(*condStmt.conditions[0].expr)});
        if (!analyzeIndexedCommitStatement(
              condStmt.ifTrue,
              commitSymbol,
              shadowSymbol,
              stateSymbol,
              entryWidth,
              guards,
              commitActionsByIndex,
              failureReason)) {
          return false;
        }
        guards.resize(baseGuardCount);
        if (condStmt.ifFalse) {
          guards.push_back(
            {false, condStmt.conditions[0].expr, getSourceRange(*condStmt.conditions[0].expr)});
          if (!analyzeIndexedCommitStatement(
                *condStmt.ifFalse,
                commitSymbol,
                shadowSymbol,
                stateSymbol,
                entryWidth,
                guards,
                commitActionsByIndex,
                failureReason)) {
            return false;
          }
          guards.resize(baseGuardCount);
        }
        return true;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (!extractAssignment(*current, lhsExpr, action)) {
        return true; // LCOV_EXCL_LINE
      }
      if (!action.rhs || action.stepDelta != 0 || action.compoundOp) {
        failureReason = "unsupported inferred memory indexed commit action";
        return false;
      }

      const slang::ast::ValueSymbol* assignedSymbol = nullptr;
      if (!lhsExpr || !tryGetRootValueSymbolReference(*lhsExpr, assignedSymbol) ||
          assignedSymbol != &commitSymbol) {
        return true;
      }

      const slang::ast::Expression* selectorExpr = nullptr;
      size_t bitOffset = 0;
      size_t bitWidth = 0;
      if (!tryExtractInferredMemoryEntryTarget(
            *lhsExpr,
            commitSymbol,
            entryWidth,
            selectorExpr,
            bitOffset,
            bitWidth,
            failureReason)) {
        return false;
      }

      int32_t entryIndex = 0;
      if (!selectorExpr || !getConstantInt32(*selectorExpr, entryIndex)) {
        failureReason = "unsupported inferred memory dynamic indexed commit target";
        return false;
      }

      InferredMemoryCommitAction commitAction;
      commitAction.rhsExpr = action.rhs;
      const slang::ast::ValueSymbol* selectorSymbol = nullptr;
      if (selectorExpr) {
        tryGetValueSymbolReference(*selectorExpr, selectorSymbol);
      }
      commitAction.selectorSymbol = selectorSymbol;
      commitAction.bitOffset = bitOffset;
      commitAction.bitWidth = bitWidth;
      commitAction.sourceRange = getSourceRange(*lhsExpr);
      commitAction.guards = guards;
      commitActionsByIndex[entryIndex].push_back(std::move(commitAction));
      return true;
    }

    const slang::ast::ProceduralBlockSymbol* findIndexedCommitBlock(
      const std::vector<const slang::ast::ProceduralBlockSymbol*>& combinationalBlocks,
      const slang::ast::ValueSymbol& commitSymbol,
      const slang::ast::ValueSymbol& shadowSymbol,
      const slang::ast::ValueSymbol& stateSymbol,
      size_t entryWidth,
      const slang::ast::ProceduralBlockSymbol* excludeBlock,
      std::map<int32_t, std::vector<InferredMemoryCommitAction>>& commitActionsByIndex,
      std::string& failureReason) const {
      for (const auto* block : combinationalBlocks) {
        if (!block || block == excludeBlock) {
          continue;
        }
        std::vector<InferredMemoryCommitGuard> guards;
        std::map<int32_t, std::vector<InferredMemoryCommitAction>> blockCommitActionsByIndex;
        std::string blockFailureReason;
        if (!analyzeIndexedCommitStatement(
              block->getBody(),
              commitSymbol,
              shadowSymbol,
              stateSymbol,
              entryWidth,
              guards,
              blockCommitActionsByIndex,
              blockFailureReason)) {
          continue;
        }
        if (blockCommitActionsByIndex.empty()) {
          continue;
        }
        commitActionsByIndex = std::move(blockCommitActionsByIndex);
        return block;
      }
      failureReason = "unable to match indexed inferred memory commit block";
      return nullptr;
    }

    void collectTopLevelStatements(
      const Statement& stmt,
      std::vector<const Statement*>& topLevelStatements) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return; // LCOV_EXCL_LINE
      }
      if (current->kind == slang::ast::StatementKind::List) {
        for (const auto* item : current->as<slang::ast::StatementList>().list) {
          if (item) {
            topLevelStatements.push_back(item);
          }
        }
        return;
      }
      topLevelStatements.push_back(current);
    }

    bool findTopLevelTargetAssignment(
      const Statement& stmt,
      const slang::ast::ValueSymbol& targetSymbol,
      const Statement*& targetStmt,
      bool& found,
      std::string& failureReason) const {
      targetStmt = nullptr;
      found = false;

      std::vector<const Statement*> topLevelStatements;
      collectTopLevelStatements(stmt, topLevelStatements);
      for (const auto* topLevelStmt : topLevelStatements) {
        const Statement* current = topLevelStmt ? unwrapStatement(*topLevelStmt) : nullptr;
        if (!current || current->kind != slang::ast::StatementKind::ExpressionStatement) {
          continue;
        }
        const auto& expr = current->as<slang::ast::ExpressionStatement>().expr;
        if (expr.kind != slang::ast::ExpressionKind::Assignment) {
          continue;
        }
        const auto& assign = expr.as<slang::ast::AssignmentExpression>();
        const Expression* candidateLhs = &assign.left();
        const slang::ast::ValueSymbol* assignedSymbol = nullptr;
        if (!candidateLhs ||
            !tryGetValueSymbolReference(*candidateLhs, assignedSymbol) ||
            assignedSymbol != &targetSymbol) {
          continue;
        }
        if (found) {
          failureReason = "multiple inferred memory state assignments in shared sequential block";
          return false;
        }
        targetStmt = topLevelStmt;
        found = true;
      }
      return true;
    }

    bool analyzeSharedSequentialResetInitStatement(
      const Statement& stmt,
      const slang::ast::ValueSymbol& stateSymbol,
      size_t entryWidth,
      size_t depth,
      std::vector<bool>& initBits,
      std::vector<bool>& assignedBits,
      bool& sawTargetAssignment,
      std::string& failureReason) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty ||
          current->kind == slang::ast::StatementKind::VariableDeclaration) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        for (const auto* item : current->as<slang::ast::StatementList>().list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!analyzeSharedSequentialResetInitStatement(
                *item,
                stateSymbol,
                entryWidth,
                depth,
                initBits,
                assignedBits,
                sawTargetAssignment,
                failureReason)) {
            return false;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        return unrollForLoopStatement(
          forStmt,
          [&]() {
            return analyzeSharedSequentialResetInitStatement(
              forStmt.body,
              stateSymbol,
              entryWidth,
              depth,
              initBits,
              assignedBits,
              sawTargetAssignment,
              failureReason);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        if (condStmt.conditions.empty()) {
          // LCOV_EXCL_START
          failureReason =
            "unsupported inferred memory reset conditional without condition";
          return false;
          // LCOV_EXCL_STOP
        }
        bool constantBit = false;
        if (!tryEvaluateConstantConditionBit(*condStmt.conditions[0].expr, constantBit)) {
          failureReason = "unsupported non-constant inferred memory reset conditional";
          return false;
        }
        const Statement* selectedStmt = constantBit ? &condStmt.ifTrue : condStmt.ifFalse;
        if (!selectedStmt) {
          return true;
        }
        return analyzeSharedSequentialResetInitStatement(
          *selectedStmt,
          stateSymbol,
          entryWidth,
          depth,
          initBits,
          assignedBits,
          sawTargetAssignment,
          failureReason);
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (!extractAssignment(*current, lhsExpr, action)) {
        return true;
      }

      const slang::ast::ValueSymbol* assignedSymbol = nullptr;
      if (!lhsExpr ||
          !tryGetRootValueSymbolReference(*lhsExpr, assignedSymbol) ||
          assignedSymbol != &stateSymbol) {
        return true;
      }
      sawTargetAssignment = true;

      if (!action.rhs || action.stepDelta != 0 || action.compoundOp) {
        failureReason = "unsupported inferred memory reset assignment action";
        return false;
      }

      if (isValueSymbolReference(*lhsExpr, stateSymbol)) {
        auto fullBits = getConstantInitBits(*action.rhs, stateSymbol, entryWidth * depth);
        if (!fullBits) {
          failureReason = "unable to resolve inferred memory reset init bits";
          return false;
        }
        initBits = std::move(*fullBits);
        std::fill(assignedBits.begin(), assignedBits.end(), true);
        return true;
      }

      const slang::ast::Expression* selectorExpr = nullptr;
      size_t bitOffset = 0;
      size_t bitWidth = 0;
      if (!tryExtractInferredMemoryEntryTarget(
            *lhsExpr,
            stateSymbol,
            entryWidth,
            selectorExpr,
            bitOffset,
            bitWidth,
            failureReason)) {
        return false; // LCOV_EXCL_LINE
      }

      int32_t entryIndex = 0;
      if (!selectorExpr || !getConstantInt32(*selectorExpr, entryIndex) ||
          entryIndex < 0 ||
          entryIndex >= static_cast<int32_t>(depth)) {
        failureReason = "unsupported inferred memory dynamic reset entry index";
        return false;
      }

      auto rhsBits = getConstantInitBits(*action.rhs, stateSymbol, bitWidth);
      if (!rhsBits) {
        failureReason = "unable to resolve inferred memory reset entry bits";
        return false;
      }

      const auto start = static_cast<size_t>(entryIndex) * entryWidth + bitOffset;
      if (start + bitWidth > initBits.size()) {
        // LCOV_EXCL_START
        failureReason = "inferred memory reset entry exceeds init image width";
        return false;
        // LCOV_EXCL_STOP
      }
      for (size_t bit = 0; bit < bitWidth; ++bit) {
        initBits[start + bit] = (*rhsBits)[bit];
        assignedBits[start + bit] = true;
      }
      return true;
    }

    bool tryExtractSharedSequentialResetInitBits(
      const Statement& resetStmt,
      const slang::ast::ValueSymbol& stateSymbol,
      size_t entryWidth,
      size_t depth,
      std::vector<bool>& initBits,
      std::string& failureReason) const {
      initBits.assign(entryWidth * depth, false);
      std::vector<bool> assignedBits(entryWidth * depth, false);
      bool sawTargetAssignment = false;
      if (!analyzeSharedSequentialResetInitStatement(
            resetStmt,
            stateSymbol,
            entryWidth,
            depth,
            initBits,
            assignedBits,
            sawTargetAssignment,
            failureReason)) {
        return false;
      }
      if (!sawTargetAssignment) {
        return false;
      }
      if (std::find(assignedBits.begin(), assignedBits.end(), false) != assignedBits.end()) {
        failureReason = "inferred memory reset branch does not fully initialize the state array";
        return false;
      }
      return true;
    }

    bool tryResolveInferredMemoryCommitTarget(
      const slang::ast::ValueSymbol& stateSymbol,
      const slang::ast::ValueSymbol& shadowSymbol,
      const std::vector<const slang::ast::ProceduralBlockSymbol*>& combinationalBlocks,
      const Expression& rhsExpr,
      InferredMemory& memory) {
      if (isValueSymbolReference(rhsExpr, shadowSymbol)) {
        return true;
      }

      const slang::ast::ValueSymbol* commitSymbol = nullptr;
      if (!tryGetValueSymbolReference(rhsExpr, commitSymbol) ||
          commitSymbol == &stateSymbol || commitSymbol == &shadowSymbol) {
        return false;
      }
      NLDB0::MemorySignature commitSignature;
      if (!getSupportedMemorySignature(commitSymbol->getType(), commitSignature) ||
          !(commitSignature == memory.signature)) {
        return false; // LCOV_EXCL_LINE
      }

      auto* commitBlock =
        findWholeArrayCopyBlock(combinationalBlocks, *commitSymbol, shadowSymbol, memory.combBlock);
      if (!commitBlock) {
        std::map<int32_t, std::vector<InferredMemoryCommitAction>> commitActionsByIndex;
        std::string indexedFailureReason;
        commitBlock = findIndexedCommitBlock(
          combinationalBlocks,
          *commitSymbol,
          shadowSymbol,
          stateSymbol,
          memory.signature.width,
          memory.combBlock,
          commitActionsByIndex,
          indexedFailureReason);
        if (commitBlock) {
          memory.commitActionsByIndex = std::move(commitActionsByIndex);
        }
      }
      if (!commitBlock) {
        return false;
      }
      memory.commitSymbol = commitSymbol;
      memory.commitBlock = commitBlock;
      return true;
    }

    bool tryConfigureInferredMemoryResetMode(
      const Expression& resetCondition,
      const Expression* asyncResetEventExpr,
      const std::optional<slang::ast::EdgeKind>& asyncResetEventEdge,
      InferredMemory& memory) const {
      if (asyncResetEventExpr && asyncResetEventEdge) {
        if (*asyncResetEventEdge == slang::ast::EdgeKind::NegEdge &&
            isActiveLowResetConditionForSignal(resetCondition, *asyncResetEventExpr)) {
          memory.signature.resetMode = NLDB0::MemoryResetMode::AsyncLow;
          memory.resetSignalExpr = asyncResetEventExpr;
          return true;
        }
        if (*asyncResetEventEdge == slang::ast::EdgeKind::PosEdge &&
            isActiveHighResetConditionForSignal(resetCondition, *asyncResetEventExpr)) {
          memory.signature.resetMode = NLDB0::MemoryResetMode::AsyncHigh;
          memory.resetSignalExpr = asyncResetEventExpr;
          return true;
        }
        return false;
      }

      const slang::ast::ValueSymbol* resetSignal = nullptr;
      if (tryGetValueSymbolReference(resetCondition, resetSignal)) {
        memory.signature.resetMode = NLDB0::MemoryResetMode::SyncHigh;
        memory.resetSignalExpr = &resetCondition;
        return true;
      }
      if (const auto* strippedReset = stripConversions(resetCondition);
          strippedReset &&
          strippedReset->kind == slang::ast::ExpressionKind::UnaryOp &&
          strippedReset->as<slang::ast::UnaryExpression>().op ==
            slang::ast::UnaryOperator::BitwiseNot &&
          tryGetValueSymbolReference(
            strippedReset->as<slang::ast::UnaryExpression>().operand(),
            resetSignal)) {
        memory.signature.resetMode = NLDB0::MemoryResetMode::SyncLow;
        memory.resetSignalExpr = &strippedReset->as<slang::ast::UnaryExpression>().operand();
        return true;
      }
      return false;
    }

    bool tryMatchSharedSequentialMemoryBlock(
      const slang::ast::ProceduralBlockSymbol& block,
      const Statement& stmt,
      const slang::ast::ValueSymbol& stateSymbol,
      const slang::ast::ValueSymbol& shadowSymbol,
      const std::vector<const slang::ast::ProceduralBlockSymbol*>& combinationalBlocks,
      const Expression* asyncResetEventExpr,
      const std::optional<slang::ast::EdgeKind>& asyncResetEventEdge,
      InferredMemory& memory) {
      const Statement* current = unwrapStatement(stmt);
      if (!current || current->kind != slang::ast::StatementKind::Conditional) {
        return false; // LCOV_EXCL_LINE
      }
      const auto& topCond = current->as<slang::ast::ConditionalStatement>();
      if (topCond.conditions.size() != 1 || !topCond.ifFalse) {
        return false;
      }

      const Statement* defaultStmt = nullptr;
      bool foundDefault = false;
      std::string searchFailureReason;
      if (!findTopLevelTargetAssignment(
            *topCond.ifFalse,
            stateSymbol,
            defaultStmt,
            foundDefault,
            searchFailureReason) ||
          !foundDefault) {
        return false;
      }
      const Expression* defaultLhs = nullptr;
      AssignAction defaultAction;
      if (!defaultStmt || !extractAssignment(*defaultStmt, defaultLhs, defaultAction)) {
        return false; // LCOV_EXCL_LINE
      }
      if (!defaultAction.rhs || defaultAction.stepDelta != 0 || defaultAction.compoundOp) {
        return false;
      }
      if (!tryResolveInferredMemoryCommitTarget(
            stateSymbol,
            shadowSymbol,
            combinationalBlocks,
            *defaultAction.rhs,
            memory)) {
        return false;
      }

      std::vector<bool> initBits;
      std::string initFailureReason;
      if (!tryExtractSharedSequentialResetInitBits(
            topCond.ifTrue,
            stateSymbol,
            memory.signature.width,
            memory.signature.depth,
            initBits,
            initFailureReason)) {
        return false;
      }
      memory.initBits = std::move(initBits);
      if (!tryConfigureInferredMemoryResetMode(
            *topCond.conditions[0].expr,
            asyncResetEventExpr,
            asyncResetEventEdge,
            memory)) {
        return false;
      }
      memory.seqBlock = &block;
      return true;
    }

    SNLBitNet* combineConditionAnd(
      SNLDesign* design,
      SNLBitNet* lhs,
      SNLBitNet* rhs,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      if (lhs == const0 || rhs == const0) {
        return const0;
      }
      if (lhs == const1) return rhs;
      if (rhs == const1) return lhs;
      if (lhs == rhs) return lhs;
      return getSingleBitNet(createBinaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::And),
        lhs,
        rhs,
        nullptr,
        sourceRange));
    }

    SNLBitNet* combineConditionOr(
      SNLDesign* design,
      SNLBitNet* lhs,
      SNLBitNet* rhs,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      if (lhs == const1 || rhs == const1) return const1;
      if (lhs == const0) return rhs;
      if (rhs == const0) return lhs;
      if (lhs == rhs) return lhs;
      return getSingleBitNet(createBinaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::Or),
        lhs,
        rhs,
        nullptr,
        sourceRange));
    }

    SNLBitNet* negateCondition(
      SNLDesign* design,
      SNLBitNet* bit,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      if (bit == const0) {
        return const1;
      }
      if (bit == const1) {
        return const0;
      }
      return getSingleBitNet(createUnaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::Not),
        bit,
        nullptr,
        sourceRange));
    }

    bool tryMatchSequentialMemoryBlock(
      const slang::ast::ProceduralBlockSymbol& block,
      const slang::ast::ValueSymbol& stateSymbol,
      const slang::ast::ValueSymbol& shadowSymbol,
      const std::vector<const slang::ast::ProceduralBlockSymbol*>& combinationalBlocks,
      InferredMemory& memory) {
      if (block.procedureKind != slang::ast::ProceduralBlockKind::AlwaysFF &&
          block.procedureKind != slang::ast::ProceduralBlockKind::Always) { // LCOV_EXCL_LINE
        return false; // LCOV_EXCL_LINE
      }

      const Statement* stmt = &block.getBody();
      const TimingControl* timing = nullptr;
      if (const auto* timed = findTimedStatement(*stmt)) {
        timing = &timed->timing;
        stmt = &timed->stmt;
      }
      stmt = stmt ? unwrapStatement(*stmt) : nullptr;
      if (!stmt || !timing) {
        return false;
      }
      if (isIgnorableSequentialStatementTree(*stmt)) {
        return false;
      }

      memory.signature.resetMode = NLDB0::MemoryResetMode::None;
      memory.initBits.assign(memory.signature.width * memory.signature.depth, false);
      memory.commitSymbol = nullptr;
      memory.commitBlock = nullptr;
      memory.commitActionsByIndex.clear();
      memory.resetSignalExpr = nullptr;

      const Expression* directLhs = nullptr;
      AssignAction directAction;
      if (extractAssignment(*stmt, directLhs, directAction)) {
        if (!directAction.rhs || directAction.stepDelta != 0 || directAction.compoundOp) return false;
        if (!isValueSymbolReference(*directLhs, stateSymbol)) {
          return false;
        }
        if (!tryResolveInferredMemoryCommitTarget(
              stateSymbol,
              shadowSymbol,
              combinationalBlocks,
              *directAction.rhs,
              memory)) {
          return false;
        }
        const auto* clockExpr = getClockExpression(*timing);
        if (!clockExpr) {
          return false;
        }
        memory.clockExpr = clockExpr;
        memory.seqBlock = &block; // LCOV_EXCL_LINE
        return true; // LCOV_EXCL_LINE
      }

      const auto* clockExpr = getClockExpression(*timing);
      if (!clockExpr) {
        return false;
      }

      auto asyncEvent = getSingleAsyncEventExpression(*timing, *clockExpr);
      const Expression* asyncResetEventExpr = asyncEvent.multiple ? nullptr : asyncEvent.expr;
      std::optional<slang::ast::EdgeKind> asyncResetEventEdge =
        asyncEvent.multiple ? std::optional<slang::ast::EdgeKind>{} : asyncEvent.edge;

      memory.clockExpr = clockExpr;
      if (tryMatchSharedSequentialMemoryBlock(
            block,
            *stmt,
            stateSymbol,
            shadowSymbol,
            combinationalBlocks,
            asyncResetEventExpr,
            asyncResetEventEdge,
            memory)) {
        return true;
      }

      AlwaysFFChain chain;
      if (extractAlwaysFFChain(*stmt, chain) && !chain.enableCond) {
        if (!isValueSymbolReference(*chain.lhs, stateSymbol)) {
          return false;
        }
        if (!chain.hasDefault || !chain.defaultAction.rhs ||
            chain.defaultAction.stepDelta != 0 ||
            chain.defaultAction.compoundOp) {
          return false;
        }
        if (!tryResolveInferredMemoryCommitTarget(
              stateSymbol,
              shadowSymbol,
              combinationalBlocks,
              *chain.defaultAction.rhs,
              memory)) {
          return false;
        }
        if (!chain.resetCond || !chain.resetAction.rhs ||
            chain.resetAction.stepDelta != 0 ||
            chain.resetAction.compoundOp) {
          return false; // LCOV_EXCL_LINE
        }

        auto initBits = getConstantInitBits(
          *chain.resetAction.rhs,
          stateSymbol,
          memory.signature.width * memory.signature.depth);
        if (!initBits) {
          return false;
        }
        memory.initBits = std::move(*initBits);

        const auto* resetCondition = chain.resetCond;
        if (!tryConfigureInferredMemoryResetMode(
              *resetCondition,
              asyncResetEventExpr,
              asyncResetEventEdge,
              memory)) {
          return false;
        }
        // LCOV_EXCL_START
        memory.seqBlock = &block;
        return true;
        // LCOV_EXCL_STOP
      }

      return false;
    }

    bool tryMatchDirectSequentialMemoryBlock(
      const slang::ast::ProceduralBlockSymbol& block,
      InferredMemory& memory) {
      if (block.procedureKind != slang::ast::ProceduralBlockKind::AlwaysFF &&
          block.procedureKind != slang::ast::ProceduralBlockKind::Always) {
        return false;
      }

      const Statement* stmt = &block.getBody();
      const TimingControl* timing = nullptr;
      if (const auto* timed = findTimedStatement(*stmt)) {
        timing = &timed->timing;
        stmt = &timed->stmt;
      }
      stmt = stmt ? unwrapStatement(*stmt) : nullptr;
      if (!stmt || !timing) {
        return false;
      }
      if (isIgnorableSequentialStatementTree(*stmt)) {
        return false;
      }

      const slang::ast::ValueSymbol* stateSymbol = nullptr;
      NLDB0::MemorySignature signature;
      std::vector<InferredMemoryGuard> guards;
      std::vector<InferredMemoryWriteAction> directWriteActions;
      std::string failureReason;
      if (!analyzeDirectSequentialMemoryWriteStatement(
            *stmt,
            stateSymbol,
            signature,
            guards,
            directWriteActions,
            failureReason) ||
          !stateSymbol ||
          directWriteActions.empty()) {
        return false;
      }

      const auto* clockExpr = getClockExpression(*timing);
      if (!clockExpr) {
        return false;
      }

      auto asyncEvent = getSingleAsyncEventExpression(*timing, *clockExpr);
      if (asyncEvent.multiple || asyncEvent.expr) {
        return false;
      }

      memory.stateSymbol = stateSymbol;
      memory.shadowSymbol = nullptr;
      memory.commitSymbol = nullptr;
      memory.combBlock = nullptr;
      memory.commitBlock = nullptr;
      memory.seqBlock = &block;
      memory.clockExpr = clockExpr;
      memory.resetSignalExpr = nullptr;
      memory.signature = signature;
      memory.signature.resetMode = NLDB0::MemoryResetMode::None;
      memory.initBits.assign(signature.width * signature.depth, false);
      memory.commitActionsByIndex.clear();
      memory.directWriteActions = std::move(directWriteActions);
      memory.sourceRange = getSourceRange(block);
      return true;
    }

    bool analyzeInferredMemoryCombinationalStatement(
      const Statement& stmt,
      const InferredMemory& memory,
      bool topLevel,
      std::vector<InferredMemoryGuard>& guards,
      bool& sawBaseCopy,
      std::vector<InferredMemoryWriteAction>& actions,
      std::string& failureReason) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty ||
          current->kind == slang::ast::StatementKind::VariableDeclaration) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        for (const auto* item : current->as<slang::ast::StatementList>().list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!analyzeInferredMemoryCombinationalStatement(
                *item,
                memory,
                topLevel,
                guards,
                sawBaseCopy,
                actions,
                failureReason)) {
            return false;
          }
          if (isCurrentForLoopBreakRequested()) {
            break;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        return unrollForLoopStatement(
          forStmt,
          [&]() {
            return analyzeInferredMemoryCombinationalStatement(
              forStmt.body,
              memory,
              false,
              guards,
              sawBaseCopy,
              actions,
              failureReason);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        bool constantBit = false;
        if (getConstantBit(*condStmt.conditions[0].expr, constantBit)) {
          const Statement* selectedStmt = constantBit ? &condStmt.ifTrue : condStmt.ifFalse;
          if (!selectedStmt) {
            return true;
          }
          return analyzeInferredMemoryCombinationalStatement(
            *selectedStmt,
            memory,
            false,
            guards,
            sawBaseCopy,
            actions,
            failureReason);
        }

        const bool hasLoopContext = hasActiveForLoopContext();
        const bool incomingBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
        auto guardSourceRange = getSourceRange(*condStmt.conditions[0].expr);

        if (hasLoopContext) {
          setCurrentForLoopBreakRequested(false);
        }
        const size_t baseGuardCount = guards.size();
        guards.push_back({
          InferredMemoryGuard::Kind::Condition,
          true,
          condStmt.conditions[0].expr,
          nullptr,
          nullptr,
          nullptr,
          guardSourceRange});
        if (!analyzeInferredMemoryCombinationalStatement(
              condStmt.ifTrue,
              memory,
              false,
              guards,
              sawBaseCopy,
              actions,
              failureReason)) {
          return false;
        }
        guards.resize(baseGuardCount);
        const bool trueBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;

        if (hasLoopContext) {
          setCurrentForLoopBreakRequested(false);
        }
        bool falseBreak = false;
        if (condStmt.ifFalse) {
          guards.push_back({
            InferredMemoryGuard::Kind::Condition,
            false,
            condStmt.conditions[0].expr,
            nullptr,
            nullptr,
            nullptr,
            guardSourceRange});
          if (!analyzeInferredMemoryCombinationalStatement(
                *condStmt.ifFalse,
                memory,
                false,
                guards,
                sawBaseCopy,
                actions,
                failureReason)) {
            return false;
          }
          guards.resize(baseGuardCount);
        }
        if (hasLoopContext) {
          falseBreak = isCurrentForLoopBreakRequested();
          setCurrentForLoopBreakRequested(incomingBreak || (trueBreak && falseBreak));
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Case) {
        const auto& caseStmt = current->as<slang::ast::CaseStatement>();
        if (caseStmt.condition != slang::ast::CaseStatementCondition::Normal) {
          std::ostringstream reason;
          reason << "unsupported inferred memory case condition kind "
                 << static_cast<int>(caseStmt.condition);
          failureReason = reason.str();
          return false;
        }

        std::vector<const slang::ast::CaseStatement::ItemGroup*> previousItems;
        for (const auto& item : caseStmt.items) {
          const size_t baseGuardCount = guards.size();
          for (const auto* previous : previousItems) {
            guards.push_back({
              InferredMemoryGuard::Kind::CaseItemMatch,
              false,
              nullptr,
              &caseStmt.expr,
              &caseStmt,
              previous,
              getSourceRange(*previous->stmt)});
          }
          guards.push_back({
            InferredMemoryGuard::Kind::CaseItemMatch,
            true,
            nullptr,
            &caseStmt.expr,
            &caseStmt,
            &item,
            getSourceRange(*item.stmt)});
          if (!analyzeInferredMemoryCombinationalStatement(
                *item.stmt,
                memory,
                false,
                guards,
                sawBaseCopy,
                actions,
                failureReason)) {
            return false;
          }
          guards.resize(baseGuardCount);
          previousItems.push_back(&item);
        }

        if (caseStmt.defaultCase) {
          const size_t baseGuardCount = guards.size();
          for (const auto* previous : previousItems) {
            guards.push_back({
              InferredMemoryGuard::Kind::CaseItemMatch,
              false,
              nullptr,
              &caseStmt.expr,
              &caseStmt,
              previous,
              getSourceRange(*previous->stmt)});
          }
          if (!analyzeInferredMemoryCombinationalStatement(
                *caseStmt.defaultCase,
                memory,
                false,
                guards,
                sawBaseCopy,
                actions,
                failureReason)) {
            return false;
          }
          guards.resize(baseGuardCount);
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Break) {
        requestCurrentForLoopBreak();
        return true;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (!extractAssignment(*current, lhsExpr, action)) {
        return true; // LCOV_EXCL_LINE
      }

      const slang::ast::ValueSymbol* assignedSymbol = nullptr;
      if (!lhsExpr || !tryGetRootValueSymbolReference(*lhsExpr, assignedSymbol) ||
          assignedSymbol != memory.shadowSymbol) {
        return true;
      }

      if (!action.rhs || action.stepDelta != 0 || action.compoundOp) {
        failureReason = "unsupported inferred memory shadow assignment action";
        return false;
      }

      if (isValueSymbolReference(*lhsExpr, *memory.shadowSymbol) &&
          isValueSymbolReference(*action.rhs, *memory.stateSymbol)) {
        if (!topLevel || !guards.empty()) {
          failureReason =
            "unsupported non-top-level whole-array shadow copy in inferred memory block";
          return false;
        }
        sawBaseCopy = true;
        return true;
      }

      const slang::ast::Expression* selectorExpr = nullptr;
      size_t bitOffset = 0;
      size_t bitWidth = 0;
      if (!tryExtractInferredMemoryWriteTarget(
            *lhsExpr,
            memory,
            selectorExpr,
            bitOffset,
            bitWidth,
            failureReason)) {
        return false;
      }

      InferredMemoryWriteAction writeAction;
      writeAction.lhsExpr = lhsExpr;
      writeAction.selectorExpr = selectorExpr;
      writeAction.rhsExpr = action.rhs;
      writeAction.stepDelta = action.stepDelta;
      writeAction.compoundOp = action.compoundOp;
      writeAction.bitOffset = bitOffset;
      writeAction.bitWidth = bitWidth;
      writeAction.guards = guards;
      writeAction.sourceRange = getSourceRange(*lhsExpr);
      actions.push_back(std::move(writeAction));
      return true;
    }

    void inferMemories(SNLDesign* /*design*/, const InstanceBodySymbol& body) {
      inferredMemories_.clear();
      inferredMemoryByStateSymbol_.clear();
      inferredMemoryCombBlocks_.clear();
      inferredMemorySequentialBlocks_.clear();

      std::vector<const slang::ast::ProceduralBlockSymbol*> sequentialBlocks;
      std::vector<const slang::ast::ProceduralBlockSymbol*> combinationalBlocks;
      auto collectProceduralBlock = [&](const Symbol& sym) {
        if (sym.kind != SymbolKind::ProceduralBlock) {
          return;
        }
        const auto& block = sym.as<slang::ast::ProceduralBlockSymbol>();
        if (block.procedureKind == slang::ast::ProceduralBlockKind::AlwaysFF ||
            block.procedureKind == slang::ast::ProceduralBlockKind::Always) {
          sequentialBlocks.push_back(&block);
        }
        if (block.procedureKind == slang::ast::ProceduralBlockKind::AlwaysComb) {
          combinationalBlocks.push_back(&block);
        }
      };
      visitElaboratedNonGenerateMembers(body, collectProceduralBlock);

      for (const auto* blockPtr : combinationalBlocks) {
        const auto& block = *blockPtr;
        const Statement* combStmt = unwrapStatement(block.getBody());
        if (!combStmt) {
          continue; // LCOV_EXCL_LINE
        }
        std::vector<const Statement*> topLevelStatements;
        if (combStmt->kind == slang::ast::StatementKind::List) {
          for (const auto* item : combStmt->as<slang::ast::StatementList>().list) {
            if (item) {
              topLevelStatements.push_back(item);
            }
          }
        } else {
          topLevelStatements.push_back(combStmt);
        }

        std::unordered_set<const slang::ast::ValueSymbol*> localStateCandidates;
        for (const auto* topLevelStmt : topLevelStatements) {
          const Expression* copyLhs = nullptr;
          AssignAction copyAction;
          if (!topLevelStmt || !extractAssignment(*topLevelStmt, copyLhs, copyAction) ||
              !copyAction.rhs || copyAction.stepDelta != 0 || copyAction.compoundOp) {
              continue;
          }

          const slang::ast::ValueSymbol* shadowSymbol = nullptr;
          const slang::ast::ValueSymbol* stateSymbol = nullptr;
          if (!tryGetValueSymbolReference(*copyLhs, shadowSymbol) ||
              !tryGetValueSymbolReference(*copyAction.rhs, stateSymbol) ||
              shadowSymbol == stateSymbol ||
              inferredMemoryByStateSymbol_.contains(stateSymbol) ||
              localStateCandidates.contains(stateSymbol)) {
            continue;
          }

          NLDB0::MemorySignature signature;
          if (!getSupportedMemorySignature(stateSymbol->getType(), signature)) {
            continue;
          }
          NLDB0::MemorySignature shadowSignature;
          if (!getSupportedMemorySignature(shadowSymbol->getType(), shadowSignature) ||
              !(shadowSignature == signature)) {
            continue; // LCOV_EXCL_LINE
          }

          InferredMemory memory;
          memory.stateSymbol = stateSymbol;
          memory.shadowSymbol = shadowSymbol;
          memory.combBlock = &block;
          memory.signature = signature;
          memory.sourceRange = getSourceRange(block);

          bool matchedSequential = false;
          for (const auto* seqBlock : sequentialBlocks) {
            InferredMemory candidate = memory;
            if (tryMatchSequentialMemoryBlock(
                  *seqBlock,
                  *stateSymbol,
                  *shadowSymbol,
                  combinationalBlocks,
                  candidate)) {
              memory = std::move(candidate);
              matchedSequential = true;
              break;
            }
          }
          if (!matchedSequential) {
            continue;
          }

          const size_t memoryIndex = inferredMemories_.size();
          inferredMemories_.push_back(std::move(memory));
          inferredMemoryByStateSymbol_[stateSymbol] = memoryIndex;
          localStateCandidates.insert(stateSymbol);
        }
      }

      for (const auto* seqBlock : sequentialBlocks) {
        InferredMemory memory;
        if (!tryMatchDirectSequentialMemoryBlock(*seqBlock, memory) ||
            !memory.stateSymbol ||
            inferredMemoryByStateSymbol_.contains(memory.stateSymbol)) {
          continue;
        }

        const size_t memoryIndex = inferredMemories_.size();
        const auto* stateSymbol = memory.stateSymbol;
        inferredMemories_.push_back(std::move(memory));
        inferredMemoryByStateSymbol_[stateSymbol] = memoryIndex;
      }

      for (size_t i = 0; i < inferredMemories_.size(); ++i) {
        if (inferredMemories_[i].combBlock) {
          inferredMemoryCombBlocks_[inferredMemories_[i].combBlock] = i;
        }
        if (inferredMemories_[i].seqBlock) {
          inferredMemorySequentialBlocks_[inferredMemories_[i].seqBlock] = i;
        }
      }
    }

    void prepareInferredMemories(SNLDesign* design) {
      for (auto& memory : inferredMemories_) {
        memory.lowered = false;
      }
      for (auto& memory : inferredMemories_) {
        std::string failureReason;
        if ((memory.combBlock &&
             lowerInferredMemoryCombinationalBlock(design, memory, failureReason)) ||
            (!memory.combBlock && !memory.directWriteActions.empty() &&
             lowerInferredMemoryDirectSequentialBlock(design, memory, failureReason))) {
          memory.lowered = true;
        }
      }
    }

    InferredMemoryReadPort* getOrCreateInferredMemoryReadPort(
      SNLDesign* design,
      InferredMemory& memory,
      const Expression& selectorExpr,
      const std::optional<slang::SourceRange>& sourceRange) {
      if (auto it = memory.readPortBySelectorExpr.find(&selectorExpr);
          it != memory.readPortBySelectorExpr.end()) {
        return &memory.readPorts[it->second]; // LCOV_EXCL_LINE
      }

      const auto baseName = std::string(memory.stateSymbol->name);
      const auto portIndex = memory.readPorts.size();
      auto* addrNet = getOrCreateNamedBusNet(
        design,
        joinName(joinName(baseName, "mem_raddr"), std::to_string(portIndex)),
        memory.signature.abits,
        sourceRange);
      auto* dataNet = getOrCreateNamedBusNet(
        design,
        joinName(joinName(baseName, "mem_rdata"), std::to_string(portIndex)),
        memory.signature.width,
        sourceRange);

      std::vector<SNLBitNet*> selectorBits;
      if (!resolveExpressionBits(design, selectorExpr, memory.signature.abits, selectorBits) ||
          selectorBits.size() != memory.signature.abits) {
        return nullptr;
      }
      connectBusNetBits(design, addrNet, selectorBits, sourceRange);

      InferredMemoryReadPort port;
      port.selectorExpr = &selectorExpr;
      port.addrNet = addrNet;
      port.dataNet = dataNet;
      memory.readPorts.push_back(port);
      memory.readPortBySelectorExpr[&selectorExpr] = portIndex;
      return &memory.readPorts.back();
    }

    bool tryResolveInferredMemoryReadBits(
      SNLDesign* design,
      const Expression& expr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      const auto* stripped = stripConversions(expr);
      if (!stripped || stripped->kind != slang::ast::ExpressionKind::ElementSelect) {
        return false;
      }
      const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
      const slang::ast::ValueSymbol* stateSymbol = nullptr;
      if (!tryGetValueSymbolReference(elementExpr.value(), stateSymbol)) {
        return false;
      }
      auto found = inferredMemoryByStateSymbol_.find(stateSymbol);
      if (found == inferredMemoryByStateSymbol_.end()) {
        return false;
      }

      auto& memory = inferredMemories_[found->second];
      if (!memory.lowered) {
        return false;
      }
      if (targetWidth != memory.signature.width) {
        return false; // LCOV_EXCL_LINE
      }
      auto* port = getOrCreateInferredMemoryReadPort(
        design,
        memory,
        elementExpr.selector(),
        getSourceRange(expr));
      if (!port) {
        return false; // LCOV_EXCL_LINE
      }
      bits = collectBits(port->dataNet);
      resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
      return true;
    }

    bool buildInferredMemoryWriteBaseBits(
      SNLDesign* design,
      InferredMemory& memory,
      const InferredMemoryWriteAction& writeAction,
      std::vector<SNLBitNet*>& bits,
      std::vector<SNLBitNet*>* stateBits,
      std::string& failureReason) {
      auto* readPort = getOrCreateInferredMemoryReadPort(
        design,
        memory,
        *writeAction.selectorExpr,
        writeAction.sourceRange);
      if (!readPort) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "unable to resolve inferred memory current entry bits for "
               << std::string(memory.stateSymbol->name);
        failureReason = reason.str();
        return false;
        // LCOV_EXCL_STOP
      }

      auto baseStateBits = collectBits(readPort->dataNet);
      resizeBitsToWidth(
        baseStateBits,
        memory.signature.width,
        static_cast<SNLBitNet*>(getConstNet(design, false)));
      if (stateBits) {
        *stateBits = baseStateBits;
      }
      bits = std::move(baseStateBits);

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      for (const auto& priorPort : memory.writePorts) {
        if (priorPort.dataBits.size() != memory.signature.width) {
          // LCOV_EXCL_START
          failureReason = "internal inferred memory write data width mismatch";
          return false;
          // LCOV_EXCL_STOP
        }
        auto* sameAddr = SNLScalarNet::create(design);
        annotateSourceInfo(sameAddr, writeAction.sourceRange);
        if (!createEqualityAssign(
              design,
              sameAddr,
              *writeAction.selectorExpr,
              *priorPort.selectorExpr,
              writeAction.sourceRange)) {
          // LCOV_EXCL_START
          failureReason = "failed to compare inferred memory write addresses";
          return false;
          // LCOV_EXCL_STOP
        }
        auto* activePrior = combineConditionAnd(
          design,
          priorPort.weNet,
          sameAddr,
          writeAction.sourceRange);
        if (!activePrior || activePrior == const0) {
          continue; // LCOV_EXCL_LINE
        }
        for (size_t bit = 0; bit < bits.size(); ++bit) {
          auto* candidateBit = priorPort.dataBits[bit];
          if (activePrior == const1) {
            bits[bit] = candidateBit; // LCOV_EXCL_LINE
            continue; // LCOV_EXCL_LINE
          }
          if (bits[bit] == candidateBit) {
            continue; // LCOV_EXCL_LINE
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, writeAction.sourceRange);
          createMux2Instance(
            design,
            activePrior,
            bits[bit],
            candidateBit,
            outBit,
            writeAction.sourceRange);
          bits[bit] = outBit;
        }
      }
      return true;
    }

    bool tryExtractInferredMemoryLocalSlice(
      const Expression& expr,
      const slang::ast::ValueSymbol& rootSymbol,
      int32_t selectorIndex,
      size_t entryWidth,
      size_t& bitOffset,
      size_t& bitWidth,
      std::string& failureReason) const {
      const slang::ast::Expression* selectorExpr = nullptr;
      if (!tryExtractInferredMemoryEntryTarget(
            expr,
            rootSymbol,
            entryWidth,
            selectorExpr,
            bitOffset,
            bitWidth,
            failureReason)) {
        return false;
      }
      int32_t constantIndex = 0;
      if (!selectorExpr || !getConstantInt32(*selectorExpr, constantIndex) ||
          constantIndex != selectorIndex) {
        failureReason = "unsupported inferred memory commit selector mismatch";
        return false;
      }
      return true;
    }

    bool resolveInferredMemoryLocalBits(
      SNLDesign* design,
      const Expression& expr,
      int32_t selectorIndex,
      const InferredMemory& memory,
      const std::vector<SNLBitNet*>& stateBits,
      const std::vector<SNLBitNet*>& shadowBits,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits,
      std::string& failureReason) {
      bits.clear();
      if (!targetWidth) {
        return false; // LCOV_EXCL_LINE
      }

      auto resolveLocalSlice = [&](const slang::ast::ValueSymbol& symbol,
                                   const std::vector<SNLBitNet*>& sourceBits) -> bool {
        size_t bitOffset = 0;
        size_t bitWidth = 0;
        std::string localFailureReason;
        if (!tryExtractInferredMemoryLocalSlice(
              expr,
              symbol,
              selectorIndex,
              memory.signature.width,
              bitOffset,
              bitWidth,
              localFailureReason)) {
          return false;
        }
        if (bitOffset + bitWidth > sourceBits.size()) {
          // LCOV_EXCL_START
          failureReason = "unsupported inferred memory local slice out of range";
          return false;
          // LCOV_EXCL_STOP
        }
        bits.assign(
          sourceBits.begin() + static_cast<std::ptrdiff_t>(bitOffset),
          sourceBits.begin() + static_cast<std::ptrdiff_t>(bitOffset + bitWidth));
        resizeBitsToWidth(
          bits,
          targetWidth,
          static_cast<SNLBitNet*>(getConstNet(design, false)));
        return true;
      };

      if (resolveLocalSlice(*memory.shadowSymbol, shadowBits)) {
        return true;
      }
      if (resolveLocalSlice(*memory.stateSymbol, stateBits)) {
        return true;
      }
      if (resolveConstantExpressionBits(design, expr, targetWidth, bits)) {
        return true;
      }

      failureReason = "unsupported inferred memory local expression";
      return false;
    }

    SNLBitNet* buildInferredMemoryLocalConditionBit(
      SNLDesign* design,
      const Expression& expr,
      int32_t selectorIndex,
      const InferredMemory& memory,
      const std::vector<SNLBitNet*>& stateBits,
      const std::vector<SNLBitNet*>& shadowBits,
      const std::optional<slang::SourceRange>& sourceRange,
      std::string& failureReason) {
      bool constantValue = false;
      if (tryEvaluateConstantConditionBit(expr, constantValue)) {
        return static_cast<SNLBitNet*>(getConstNet(design, constantValue));
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        // LCOV_EXCL_START
        failureReason = "unsupported inferred memory null local condition";
        return nullptr;
        // LCOV_EXCL_STOP
      }

      if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
        if (unaryExpr.op == slang::ast::UnaryOperator::LogicalNot ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
          auto* operandBit = buildInferredMemoryLocalConditionBit(
            design,
            unaryExpr.operand(),
            selectorIndex,
            memory,
            stateBits,
            shadowBits,
            sourceRange,
            failureReason);
          return operandBit ? negateCondition(design, operandBit, sourceRange) : nullptr;
        }
      }

      if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
        if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd ||
            binaryExpr.op == slang::ast::BinaryOperator::LogicalOr) {
          auto* lhsBit = buildInferredMemoryLocalConditionBit(
            design,
            binaryExpr.left(),
            selectorIndex,
            memory,
            stateBits,
            shadowBits,
            sourceRange,
            failureReason);
          if (!lhsBit) {
            return nullptr;
          }
          auto* rhsBit = buildInferredMemoryLocalConditionBit(
            design,
            binaryExpr.right(),
            selectorIndex,
            memory,
            stateBits,
            shadowBits,
            sourceRange,
            failureReason);
          if (!rhsBit) {
            return nullptr;
          }
          return binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd
            ? combineConditionAnd(design, lhsBit, rhsBit, sourceRange)
            : combineConditionOr(design, lhsBit, rhsBit, sourceRange);
        }

        if (isEqualityBinaryOp(binaryExpr.op) || isInequalityBinaryOp(binaryExpr.op)) {
          auto leftWidth = getIntegralExpressionBitWidth(binaryExpr.left());
          auto rightWidth = getIntegralExpressionBitWidth(binaryExpr.right());
          if (!leftWidth || !rightWidth) {
            // LCOV_EXCL_START
            failureReason = "unsupported inferred memory local equality width";
            return nullptr;
            // LCOV_EXCL_STOP
          }
          const auto compareWidth = std::max(*leftWidth, *rightWidth);
          std::vector<SNLBitNet*> leftBits;
          std::vector<SNLBitNet*> rightBits;
          if (!resolveInferredMemoryLocalBits(
                design,
                binaryExpr.left(),
                selectorIndex,
                memory,
                stateBits,
                shadowBits,
                compareWidth,
                leftBits,
                failureReason) ||
              !resolveInferredMemoryLocalBits(
                design,
                binaryExpr.right(),
                selectorIndex,
                memory,
                stateBits,
                shadowBits,
                compareWidth,
                rightBits,
                failureReason)) {
            return nullptr;
          }

          std::vector<SNLNet*> xnorBits;
          xnorBits.reserve(compareWidth);
          for (size_t bitIndex = 0; bitIndex < compareWidth; ++bitIndex) {
            auto* xnorBit = SNLScalarNet::create(design);
            annotateSourceInfo(xnorBit, sourceRange);
            auto* xnorOut = createBinaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Xnor),
              leftBits[bitIndex],
              rightBits[bitIndex],
              xnorBit,
              sourceRange);
            if (!xnorOut) {
              // LCOV_EXCL_START
              failureReason = "failed to build inferred memory local equality compare";
              return nullptr;
              // LCOV_EXCL_STOP
            }
            xnorBits.push_back(xnorOut);
          }

          SNLBitNet* eqBit = nullptr;
          if (xnorBits.size() == 1) {
            eqBit = getSingleBitNet(xnorBits.front());
          } else {
            auto* eqNet = SNLScalarNet::create(design);
            annotateSourceInfo(eqNet, sourceRange);
            SNLNet* andOut = eqNet;
            if (!createGateInstance(
                  design,
                  NLDB0::GateType(NLDB0::GateType::And),
                  xnorBits,
                  andOut,
                  sourceRange) ||
                !andOut) {
              // LCOV_EXCL_START
              failureReason = "failed to reduce inferred memory local equality compare";
              return nullptr;
              // LCOV_EXCL_STOP
            }
            eqBit = getSingleBitNet(andOut);
          }
          if (!eqBit) {
            // LCOV_EXCL_START
            failureReason = "failed to finalize inferred memory local equality compare";
            return nullptr;
            // LCOV_EXCL_STOP
          }
          return isInequalityBinaryOp(binaryExpr.op)
            ? negateCondition(design, eqBit, sourceRange)
            : eqBit;
        }
      }

      std::vector<SNLBitNet*> bits;
      if (!resolveInferredMemoryLocalBits(
            design,
            *stripped,
            selectorIndex,
            memory,
            stateBits,
            shadowBits,
            1,
            bits,
            failureReason) ||
          bits.empty()) {
        if (failureReason.empty()) {
          failureReason = "unsupported inferred memory local condition expression"; // LCOV_EXCL_LINE
        }
        return nullptr;
      }
      return bits.front();
    }

    bool applyInferredMemoryCommitActionsForIndex(
      SNLDesign* design,
      const InferredMemory& memory,
      int32_t selectorIndex,
      const std::vector<InferredMemoryCommitAction>& actions,
      const std::vector<SNLBitNet*>& stateBits,
      const std::vector<SNLBitNet*>& shadowBits,
      std::vector<SNLBitNet*>& bits,
      std::string& failureReason) {
      bits = shadowBits;
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

      for (const auto& action : actions) {
        if (action.selectorSymbol) {
          activeForLoopConstants_.emplace_back(action.selectorSymbol, selectorIndex);
        }
        auto popSelectorContext = [&]() {
          if (action.selectorSymbol && !activeForLoopConstants_.empty()) {
            activeForLoopConstants_.pop_back();
          }
        };
        if (action.bitOffset + action.bitWidth > bits.size()) {
          // LCOV_EXCL_START
          failureReason = "invalid inferred memory commit target width";
          popSelectorContext();
          return false;
          // LCOV_EXCL_STOP
        }

        std::vector<SNLBitNet*> assignedBits;
        if (!resolveInferredMemoryLocalBits(
              design,
              *action.rhsExpr,
              selectorIndex,
              memory,
              stateBits,
              shadowBits,
              action.bitWidth,
              assignedBits,
              failureReason) ||
            assignedBits.size() != action.bitWidth) {
          // LCOV_EXCL_START
          if (failureReason.empty()) {
            failureReason = "unable to resolve inferred memory commit RHS bits";
          }
          // LCOV_EXCL_STOP
          popSelectorContext();
          return false;
        }

        SNLBitNet* guardBit = const1;
        for (const auto& guard : action.guards) {
          auto* conditionBit = buildInferredMemoryLocalConditionBit(
            design,
            *guard.expr,
            selectorIndex,
            memory,
            stateBits,
            shadowBits,
            guard.sourceRange,
            failureReason);
          if (!conditionBit) {
            popSelectorContext();
            return false;
          }
          if (!guard.polarity) {
            conditionBit = negateCondition(design, conditionBit, guard.sourceRange);
          }
          guardBit = combineConditionAnd(design, guardBit, conditionBit, guard.sourceRange);
          if (!guardBit) {
            // LCOV_EXCL_START
            failureReason = "failed to build inferred memory commit guard";
            popSelectorContext();
            return false;
            // LCOV_EXCL_STOP
          }
          if (guardBit == const0) {
            break;
          }
        }
        if (guardBit == const0) {
          popSelectorContext();
          continue;
        }

        for (size_t bit = 0; bit < action.bitWidth; ++bit) {
          const size_t targetBit = action.bitOffset + bit;
          if (guardBit == const1) {
            bits[targetBit] = assignedBits[bit];
            continue;
          }
          if (bits[targetBit] == assignedBits[bit]) {
            continue;
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, action.sourceRange);
          createMux2Instance(
            design,
            guardBit,
            bits[targetBit],
            assignedBits[bit],
            outBit,
            action.sourceRange);
          bits[targetBit] = outBit;
        }

        popSelectorContext();
      }
      return true;
    }

    bool applyInferredMemoryCommitPrograms(
      SNLDesign* design,
      const InferredMemory& memory,
      const std::vector<SNLBitNet*>& selectorBits,
      const std::vector<SNLBitNet*>& stateBits,
      const std::vector<SNLBitNet*>& shadowBits,
      const std::optional<slang::SourceRange>& sourceRange,
      std::vector<SNLBitNet*>& bits,
      std::string& failureReason) {
      bits = shadowBits;
      if (memory.commitActionsByIndex.empty()) {
        return true; // LCOV_EXCL_LINE
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      for (const auto& [index, actions] : memory.commitActionsByIndex) {
        std::vector<SNLBitNet*> candidateBits;
        if (!applyInferredMemoryCommitActionsForIndex(
              design,
              memory,
              index,
              actions,
              stateBits,
              shadowBits,
              candidateBits,
              failureReason)) {
          return false;
        }

        auto* selectBit = buildSelectorEqualsIndexBit(design, selectorBits, index, sourceRange);
        if (!selectBit) {
          // LCOV_EXCL_START
          failureReason = "failed to build inferred memory commit selector";
          return false;
          // LCOV_EXCL_STOP
        }
        if (selectBit == const0) {
          continue;
        }
        if (selectBit == const1) {
          bits = std::move(candidateBits);
          return true;
        }

        for (size_t bit = 0; bit < bits.size(); ++bit) {
          if (bits[bit] == candidateBits[bit]) {
            continue;
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, sourceRange);
          createMux2Instance(
            design,
            selectBit,
            bits[bit],
            candidateBits[bit],
            outBit,
            sourceRange);
          bits[bit] = outBit;
        }
      }

      return true;
    }

    bool buildInferredMemoryWriteDataBits(
      SNLDesign* design,
      InferredMemory& memory,
      const InferredMemoryWriteAction& writeAction,
      std::vector<SNLBitNet*>& dataBits,
      std::vector<SNLBitNet*>* stateBits,
      std::string& failureReason) {
      if (writeAction.bitWidth == 0 ||
          writeAction.bitOffset + writeAction.bitWidth > memory.signature.width) {
        // LCOV_EXCL_START
        failureReason = "invalid inferred memory write target width";
        return false;
        // LCOV_EXCL_STOP
      }

      if (writeAction.bitOffset == 0 &&
          writeAction.bitWidth == memory.signature.width &&
          writeAction.stepDelta == 0 &&
          !writeAction.compoundOp) {
        if (stateBits) {
          std::vector<SNLBitNet*> ignoredBaseBits;
          if (!buildInferredMemoryWriteBaseBits(
                design,
                memory,
                writeAction,
                ignoredBaseBits,
                stateBits,
                failureReason)) {
            return false; // LCOV_EXCL_LINE
          }
        }
        if (!resolveExpressionBits(
              design,
              *writeAction.rhsExpr,
              memory.signature.width,
              dataBits) ||
            dataBits.size() != memory.signature.width) {
          std::ostringstream reason;
          reason << "unable to resolve inferred memory write data bits for "
                 << std::string(memory.stateSymbol->name);
          failureReason = reason.str();
          return false;
        }
        return true;
      }

      if (!buildInferredMemoryWriteBaseBits(
            design,
            memory,
            writeAction,
            dataBits,
            stateBits,
            failureReason)) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> currentBits(
        dataBits.begin() + static_cast<std::ptrdiff_t>(writeAction.bitOffset),
        dataBits.begin() + static_cast<std::ptrdiff_t>(writeAction.bitOffset + writeAction.bitWidth));
      AssignAction action;
      action.lhs = writeAction.lhsExpr;
      action.rhs = writeAction.rhsExpr;
      action.stepDelta = writeAction.stepDelta;
      action.compoundOp = writeAction.compoundOp;

      std::vector<SNLBitNet*> assignedBits;
      if (!buildCombinationalAssignBits(
            design,
            action,
            writeAction.bitWidth,
            assignedBits,
            &currentBits,
            failureReason) ||
          assignedBits.size() != writeAction.bitWidth) {
        // LCOV_EXCL_START
        if (failureReason.empty()) {
          failureReason = "unable to resolve inferred memory partial write bits";
        }
        // LCOV_EXCL_STOP
        return false;
      }

      for (size_t bit = 0; bit < writeAction.bitWidth; ++bit) {
        dataBits[writeAction.bitOffset + bit] = assignedBits[bit];
      }
      return true;
    }

    bool lowerInferredMemoryCombinationalBlock(
      SNLDesign* design,
      InferredMemory& memory,
      const std::vector<InferredMemoryWriteAction>& writeActions,
      std::string& failureReason) {
      memory.writePorts.clear();
      const auto baseName = std::string(memory.stateSymbol->name);
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      for (const auto& writeAction : writeActions) {
        SNLBitNet* effectiveWe = const1;
        for (const auto& guard : writeAction.guards) {
          SNLBitNet* guardBit = nullptr;
          if (guard.kind == InferredMemoryGuard::Kind::Condition) {
            std::string conditionFailureReason;
            guardBit = resolveCombinationalConditionNet(
              design,
              *guard.expr,
              &conditionFailureReason);
            if (!guardBit) {
              failureReason = conditionFailureReason.empty()
                ? "unable to resolve inferred memory condition guard"
                : conditionFailureReason;
              return false;
            }
          } else {
            std::string caseFailureReason;
            guardBit = buildCaseItemMatchBit(
              design,
              *guard.caseExpr,
              *guard.caseStmt,
              *guard.caseItem,
              caseFailureReason);
            if (!guardBit) {
              failureReason = caseFailureReason.empty()
                ? "unable to resolve inferred memory case-item guard"
                : caseFailureReason;
              return false;
            }
          }
          if (!guard.polarity) {
            guardBit = negateCondition(design, guardBit, guard.sourceRange);
          }
          effectiveWe = combineConditionAnd(design, effectiveWe, guardBit, guard.sourceRange);
          if (!effectiveWe) {
            failureReason = "failed to build inferred memory write enable";
            return false;
          }
          if (effectiveWe == const0) {
            break;
          }
        }
        if (effectiveWe == const0) {
          continue;
        }

        InferredMemoryWritePort writePort;
        writePort.selectorExpr = writeAction.selectorExpr;
        writePort.rhsExpr = writeAction.rhsExpr;
        writePort.sourceRange = writeAction.sourceRange;
        const auto portIndex = memory.writePorts.size();
        writePort.addrNet = getOrCreateNamedBusNet(
          design,
          joinName(joinName(baseName, "mem_waddr"), std::to_string(portIndex)),
          memory.signature.abits,
          writePort.sourceRange);
        writePort.dataNet = getOrCreateNamedBusNet(
          design,
          joinName(joinName(baseName, "mem_wdata"), std::to_string(portIndex)),
          memory.signature.width,
          writePort.sourceRange);
        writePort.weNet = getOrCreateNamedScalarNet(
          design,
          joinName(joinName(baseName, "mem_we"), std::to_string(portIndex)),
          writePort.sourceRange);

        std::vector<SNLBitNet*> selectorBits;
        if (!resolveExpressionBits(
              design,
              *writePort.selectorExpr,
              memory.signature.abits,
              selectorBits) ||
            selectorBits.size() != memory.signature.abits) {
          std::ostringstream reason;
          reason << "unable to resolve inferred memory write address bits for "
                 << std::string(memory.stateSymbol->name);
          failureReason = reason.str();
          return false;
        }
        connectBusNetBits(design, writePort.addrNet, selectorBits, writePort.sourceRange);

        const bool needsCommitProgram =
          memory.commitBlock && !memory.commitActionsByIndex.empty();
        std::vector<SNLBitNet*> stateBits;
        if (!buildInferredMemoryWriteDataBits(
              design,
              memory,
              writeAction,
              writePort.dataBits,
              needsCommitProgram ? &stateBits : nullptr,
              failureReason)) {
          return false;
        }
        if (needsCommitProgram) {
          std::vector<SNLBitNet*> committedBits;
          if (!applyInferredMemoryCommitPrograms(
                design,
                memory,
                selectorBits,
                stateBits,
                writePort.dataBits,
                writePort.sourceRange,
                committedBits,
                failureReason)) {
            return false;
          }
          writePort.dataBits = std::move(committedBits);
        }
        connectBusNetBits(design, writePort.dataNet, writePort.dataBits, writePort.sourceRange);
        createAssignInstance(design, effectiveWe, writePort.weNet, writePort.sourceRange);
        memory.writePorts.push_back(writePort);
      }
      if (memory.writePorts.empty()) {
        failureReason = "inferred memory did not produce indexed writes";
        return false;
      }

      for (size_t i = 0; i < memory.writePorts.size(); ++i) {
        auto* effectiveWe = static_cast<SNLBitNet*>(memory.writePorts[i].weNet);
        for (size_t later = i + 1; later < memory.writePorts.size(); ++later) {
          auto* sameAddr = SNLScalarNet::create(design);
          annotateSourceInfo(sameAddr, memory.writePorts[i].sourceRange);
          if (!createEqualityAssign(
                design,
                sameAddr,
                *memory.writePorts[i].selectorExpr,
                *memory.writePorts[later].selectorExpr,
                memory.writePorts[i].sourceRange)) {
            failureReason = "failed to compare inferred memory write addresses";
            return false;
          }
          auto* collision = static_cast<SNLBitNet*>(createBinaryGate(
            design,
            NLDB0::GateType(NLDB0::GateType::And),
            sameAddr,
            memory.writePorts[later].weNet,
            nullptr,
            memory.writePorts[i].sourceRange));
          auto* noCollision = static_cast<SNLBitNet*>(createUnaryGate(
            design,
            NLDB0::GateType(NLDB0::GateType::Not),
            collision,
            nullptr,
            memory.writePorts[i].sourceRange));
          effectiveWe = static_cast<SNLBitNet*>(createBinaryGate(
            design,
            NLDB0::GateType(NLDB0::GateType::And),
            effectiveWe,
            noCollision,
            nullptr,
            memory.writePorts[i].sourceRange));
        }
        if (effectiveWe != memory.writePorts[i].weNet) {
          createAssignInstance(design, effectiveWe, memory.writePorts[i].weNet);
        }
      }
      return true;
    }

    bool lowerInferredMemoryCombinationalBlock(
      SNLDesign* design,
      InferredMemory& memory,
      std::string& failureReason) {
      if (!memory.clockExpr) {
        // LCOV_EXCL_START
        failureReason = "inferred memory is missing a clock expression";
        return false;
        // LCOV_EXCL_STOP
      }
      if (!getSingleBitNet(resolveExpressionNet(design, *memory.clockExpr))) {
        failureReason = "unable to resolve inferred memory clock net";
        return false;
      }

      std::vector<InferredMemoryGuard> guards;
      std::vector<InferredMemoryWriteAction> writeActions;
      bool sawBaseCopy = false;
      if (!analyzeInferredMemoryCombinationalStatement(
            memory.combBlock->getBody(),
            memory,
            true,
            guards,
            sawBaseCopy,
            writeActions,
            failureReason)) {
        return false;
      }
      if (!sawBaseCopy) {
        // LCOV_EXCL_START
        failureReason = "inferred memory combinational block is missing top-level shadow copy";
        return false;
        // LCOV_EXCL_STOP
      }
      return lowerInferredMemoryCombinationalBlock(design, memory, writeActions, failureReason);
    }

    bool lowerInferredMemoryDirectSequentialBlock(
      SNLDesign* design,
      InferredMemory& memory,
      std::string& failureReason) {
      if (!memory.clockExpr) {
        failureReason = "inferred memory is missing a clock expression";
        return false;
      }
      if (!getSingleBitNet(resolveExpressionNet(design, *memory.clockExpr))) {
        failureReason = "unable to resolve inferred memory clock net";
        return false;
      }
      if (memory.directWriteActions.empty()) {
        failureReason = "direct inferred memory is missing indexed writes";
        return false;
      }
      return lowerInferredMemoryCombinationalBlock(
        design,
        memory,
        memory.directWriteActions,
        failureReason);
    }

    void finalizeInferredMemories(SNLDesign* design) {
      for (auto& memory : inferredMemories_) {
        if (!memory.stateSymbol || !memory.lowered) {
          continue; // LCOV_EXCL_LINE
        }

        NLDB0::MemorySignature signature = memory.signature;
        signature.readPorts = std::max<size_t>(1, memory.readPorts.size());
        signature.writePorts = std::max<size_t>(1, memory.writePorts.size());
        auto* model = NLDB0::getOrCreateMemory(signature);
        if (!model) {
          throw SNLSVConstructorException("Failed to create inferred memory model"); // LCOV_EXCL_LINE
        }

        auto* inst = SNLInstance::create(
          design,
          model,
          NLName(joinName(std::string(memory.stateSymbol->name), "mem")));
        annotateSourceInfo(inst, memory.sourceRange);

        auto addInstParam = [&](const char* name, const std::string& value) {
          auto* parameter = model->getParameter(NLName(name));
          if (!parameter) {
            throw SNLSVConstructorException("Failed to resolve inferred memory parameter"); // LCOV_EXCL_LINE
          }
          SNLInstParameter::create(inst, parameter, value);
        };

        addInstParam("WIDTH", std::to_string(signature.width));
        addInstParam("DEPTH", std::to_string(signature.depth));
        addInstParam("ABITS", std::to_string(signature.abits));
        addInstParam("RD_PORTS", std::to_string(signature.readPorts));
        addInstParam("WR_PORTS", std::to_string(signature.writePorts));
        addInstParam("RST_ENABLE", signature.resetMode == NLDB0::MemoryResetMode::None ? "0" : "1");
        addInstParam(
          "RST_ASYNC",
          (signature.resetMode == NLDB0::MemoryResetMode::AsyncLow ||
           signature.resetMode == NLDB0::MemoryResetMode::AsyncHigh)
            ? "1"
            : "0");
        addInstParam(
          "RST_ACTIVE_LOW",
          (signature.resetMode == NLDB0::MemoryResetMode::AsyncLow ||
           signature.resetMode == NLDB0::MemoryResetMode::SyncLow)
            ? "1"
            : "0");
        addInstParam("INIT", encodeInitBits(memory.initBits));

        auto* clkTerm = model->getScalarTerm(NLName("CLK"));
        auto* rstTerm = model->getScalarTerm(NLName("RST"));
        if (!clkTerm || !rstTerm) {
          throw SNLSVConstructorException("Failed to resolve inferred memory scalar ports"); // LCOV_EXCL_LINE
        }
        auto* clkNet = getSingleBitNet(resolveExpressionNet(design, *memory.clockExpr));
        if (!clkNet) {
          throw SNLSVConstructorException("Failed to resolve inferred memory clock net"); // LCOV_EXCL_LINE
        }
        inst->setTermNet(clkTerm, clkNet);

        SNLBitNet* rstNet = static_cast<SNLBitNet*>(getConstNet(design, false));
        if (memory.resetSignalExpr) {
          if (auto* resolved = getSingleBitNet(resolveExpressionNet(design, *memory.resetSignalExpr))) {
            rstNet = resolved;
          }
        }
        inst->setTermNet(rstTerm, rstNet);

        auto* raddrTerm = model->getBusTerm(NLName("RADDR"));
        auto* rdataTerm = model->getBusTerm(NLName("RDATA"));
        auto* waddrTerm = model->getBusTerm(NLName("WADDR"));
        auto* wdataTerm = model->getBusTerm(NLName("WDATA"));
        auto* weTerm = model->getBusTerm(NLName("WE"));
        if (!raddrTerm || !rdataTerm || !waddrTerm || !wdataTerm || !weTerm) {
          throw SNLSVConstructorException("Failed to resolve inferred memory bus ports"); // LCOV_EXCL_LINE
        }

        for (size_t i = 0; i < memory.readPorts.size(); ++i) {
          inst->setTermNet(
            raddrTerm,
            static_cast<NLID::Bit>((i + 1) * signature.abits - 1),
            static_cast<NLID::Bit>(i * signature.abits),
            memory.readPorts[i].addrNet,
            static_cast<NLID::Bit>(signature.abits - 1),
            0);
          inst->setTermNet(
            rdataTerm,
            static_cast<NLID::Bit>((i + 1) * signature.width - 1),
            static_cast<NLID::Bit>(i * signature.width),
            memory.readPorts[i].dataNet,
            static_cast<NLID::Bit>(signature.width - 1),
            0);
        }
        if (memory.readPorts.empty()) {
          auto* dummyAddr = getOrCreateNamedBusNet(
            design,
            joinName(std::string(memory.stateSymbol->name), "mem_dummy_raddr"),
            signature.abits,
            memory.sourceRange);
          auto* dummyData = getOrCreateNamedBusNet(
            design,
            joinName(std::string(memory.stateSymbol->name), "mem_dummy_rdata"),
            signature.width,
            memory.sourceRange);
          auto dummyBits = collectBits(dummyAddr);
          for (auto* bit : dummyBits) {
            createAssignInstance(design, getConstNet(design, false), bit);
          }
          inst->setTermNet(raddrTerm, dummyAddr);
          inst->setTermNet(rdataTerm, dummyData);
        }

        for (size_t i = 0; i < memory.writePorts.size(); ++i) {
          inst->setTermNet(
            waddrTerm,
            static_cast<NLID::Bit>((i + 1) * signature.abits - 1),
            static_cast<NLID::Bit>(i * signature.abits),
            memory.writePorts[i].addrNet,
            static_cast<NLID::Bit>(signature.abits - 1),
            0);
          inst->setTermNet(
            wdataTerm,
            static_cast<NLID::Bit>((i + 1) * signature.width - 1),
            static_cast<NLID::Bit>(i * signature.width),
            memory.writePorts[i].dataNet,
            static_cast<NLID::Bit>(signature.width - 1),
            0);
          if (auto* termBit = weTerm->getBitAtPosition(i)) {
            if (auto* instTerm = inst->getInstTerm(termBit)) {
              instTerm->setNet(memory.writePorts[i].weNet);
            }
          }
        }
      }
    }

    void createTerms(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.getPortList()) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.portSymbolsVisited;
#endif
        if (sym->kind != SymbolKind::Port) {
          std::string portName(sym->name);
          if (portName.empty()) {
            portName = "<anonymous>";
          }
          std::ostringstream reason;
          reason << "Unsupported SystemVerilog " << getPortKindLabel(sym->kind)
                 << " declaration for port: " << portName;
          reportUnsupportedElement(reason.str(), getSourceRange(*sym));
          continue;
        }
        const auto& port = sym->as<PortSymbol>();
        std::string portName(port.name);
        auto direction = toSNLDirection(port.direction);
        if (!direction) {
          std::ostringstream reason;
          reason << "Unsupported SystemVerilog port direction";
          if (port.direction == ArgumentDirection::Ref) {
            reason << " 'ref'";
          }
          reason << " for port: " << portName;
          reportUnsupportedElement(reason.str(), getSourceRange(port));
          continue;
        }
        if (auto unsupportedTypeReason = getUnsupportedTypeReason(port.getType())) {
          std::ostringstream reason;
          reason << *unsupportedTypeReason << " for port: " << portName;
          reportUnsupportedElement(reason.str(), getSourceRange(port));
          continue;
        }
        auto range = getRangeFromType(port.getType());
        if (range && range->width() > 1) {
          auto term = SNLBusTerm::create(
            design,
            *direction,
            static_cast<NLID::Bit>(range->left),
            static_cast<NLID::Bit>(range->right),
            NLName(portName));
          annotateSourceInfo(term, getSourceRange(port));
        } else {
          auto term = SNLScalarTerm::create(design, *direction, NLName(portName));
          annotateSourceInfo(term, getSourceRange(port));
        }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.portsCreated;
#endif
      }
    }

    void createNets(SNLDesign* design, const InstanceBodySymbol& body) {
      const auto moduleName = design->getName().getString();
      std::function<void(const slang::ast::Scope&)> visitScope;
      visitScope = [&](const slang::ast::Scope& scope) {
        for (const auto& sym : scope.members()) {
          if (sym.kind == SymbolKind::GenerateBlock) {
            const auto& generateBlock = sym.as<slang::ast::GenerateBlockSymbol>();
            if (!generateBlock.isUninstantiated) {
              visitScope(generateBlock);
            }
            continue;
          }
          if (sym.kind == SymbolKind::GenerateBlockArray) {
            const auto& generateBlockArray =
              sym.as<slang::ast::GenerateBlockArraySymbol>();
            for (const auto* entry : generateBlockArray.entries) {
              if (entry && !entry->isUninstantiated) {
                visitScope(*entry);
              }
            }
            continue;
          }
          if (sym.kind != SymbolKind::Net && sym.kind != SymbolKind::Variable) {
            continue;
          }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          ++svPerfReport_.netOrVariableSymbolsVisited;
#endif
          const auto& valueSym = sym.as<ValueSymbol>();
          auto loweredName = getLoweredSymbolName(valueSym);
          // Port terms are materialized first; let connectTermsToNets own their net creation.
          if (!isInsideGenerateScope(valueSym) && design->getTerm(NLName(loweredName))) {
            continue;
          }
          if (auto* existing = design->getNet(NLName(loweredName))) {
            loweredValueSymbolNets_[&valueSym] = existing;
            continue;
          }
          if (auto unsupportedTypeReason = getUnsupportedTypeReason(valueSym.getType())) {
            const auto& canonical = valueSym.getType().getCanonicalType();
            // Dynamically sized unpacked variables are currently not representable in SNL.
            std::ostringstream reason;
            reason << *unsupportedTypeReason << " for net/variable: "
                   << std::string(valueSym.name);
            if (canonical.isUnpackedArray() && !canonical.hasFixedRange()) {
              reason << " (dynamic unpacked array/queue/associative array)";
            }
            reason << " in module '" << moduleName << "'";
            reportUnsupportedElement(reason.str(), getSourceRange(valueSym));
            continue;
          }
          auto net = getOrCreateNet(design, loweredName, valueSym.getType());
          loweredValueSymbolNets_[&valueSym] = net;
          annotateSourceInfo(net, getSourceRange(valueSym));
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          ++svPerfReport_.netsCreated;
#endif
        }
      };
      visitScope(body);
    }

    SNLInstance* createGateInstance(SNLDesign* design, const NLDB0::GateType& type,
                                    const std::vector<SNLNet*>& inputNets, SNLNet*& outNet,
                                    const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (inputNets.empty()) {
        return nullptr; // LCOV_EXCL_LINE
      }
      if (type.isNInput()) {
        auto gate = NLDB0::getOrCreateNInputGate(type, inputNets.size());
        auto inst = SNLInstance::create(design, gate);
        annotateSourceInfo(inst, sourceRange);
        auto inputs = NLDB0::getGateNTerms(gate);
        auto output = NLDB0::getGateSingleTerm(gate);
        if (!inputs || !output) {
          return nullptr; // LCOV_EXCL_LINE
        }
        for (size_t i = 0; i < inputNets.size(); ++i) {
          auto bit = inputs->getBitAtPosition(i);
          if (!bit) {
            continue; // LCOV_EXCL_LINE
          }
          auto instTerm = inst->getInstTerm(bit);
          if (instTerm) {
            instTerm->setNet(inputNets[i]);
          }
        }
        if (!outNet) {
          // LCOV_EXCL_START
          outNet = SNLScalarNet::create(design);
          annotateSourceInfo(outNet, sourceRange);
          // LCOV_EXCL_STOP
        } 
        if (auto outTerm = inst->getInstTerm(output)) {
          outTerm->setNet(outNet);
        }
        return inst;
      }
      if (type.isNOutput()) {
        if (inputNets.size() != 1) {
          return nullptr; // LCOV_EXCL_LINE
        }
        auto gate = NLDB0::getOrCreateNOutputGate(type, 1);
        auto inst = SNLInstance::create(design, gate);
        annotateSourceInfo(inst, sourceRange);
        auto input = NLDB0::getGateSingleTerm(gate);
        auto outputs = NLDB0::getGateNTerms(gate);
        if (!input || !outputs) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (auto instTerm = inst->getInstTerm(input)) {
          instTerm->setNet(inputNets[0]);
        }
        if (!outNet) {
          // LCOV_EXCL_START
          outNet = SNLScalarNet::create(design);
          annotateSourceInfo(outNet, sourceRange);
          // LCOV_EXCL_STOP
        }
        if (auto outBit = outputs->getBitAtPosition(0)) {
          if (auto outTerm = inst->getInstTerm(outBit)) {
            outTerm->setNet(outNet);
          }
        }
        return inst;
      }
      return nullptr; // LCOV_EXCL_LINE
    }

    SNLInstance* createAssignInstance(
      SNLDesign* design,
      SNLNet* inNet,
      SNLNet* outNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto assignGate = NLDB0::getAssign();
      auto assignInst = SNLInstance::create(design, assignGate);
      annotateSourceInfo(assignInst, sourceRange);
      auto assignInput = NLDB0::getAssignInput();
      auto assignOutput = NLDB0::getAssignOutput();
      if (assignInput) {
        if (auto inTerm = assignInst->getInstTerm(assignInput)) {
          inTerm->setNet(inNet);
        }
      }
      if (assignOutput) {
        if (auto outTerm = assignInst->getInstTerm(assignOutput)) {
          outTerm->setNet(outNet);
        }
      }
      return assignInst;
    }

    bool createDirectAssign(
      SNLDesign* design,
      SNLNet* rhsNet,
      SNLNet* lhsNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (!rhsNet || !lhsNet) {
        return false; // LCOV_EXCL_LINE
      }
      if (auto lhsScalar = dynamic_cast<SNLScalarNet*>(lhsNet)) {
        auto rhsScalar = dynamic_cast<SNLScalarNet*>(rhsNet);
        if (!rhsScalar) {
          return false;
        }
        createAssignInstance(design, rhsScalar, lhsScalar, sourceRange);
        return true;
      }
      auto lhsBus = dynamic_cast<SNLBusNet*>(lhsNet);
      auto rhsBus = dynamic_cast<SNLBusNet*>(rhsNet);
      if (!lhsBus || !rhsBus) {
        return false;
      }
      if (lhsBus->getWidth() != rhsBus->getWidth()) {
        return false;
      }
      auto lhsBit = lhsBus->getMSB();
      auto rhsBit = rhsBus->getMSB();
      auto lhsDesc = lhsBus->getMSB() > lhsBus->getLSB();
      auto rhsDesc = rhsBus->getMSB() > rhsBus->getLSB();
      auto lhsStop = lhsDesc ? -1 : 1;
      auto rhsStop = rhsDesc ? -1 : 1;
      while (lhsBit != lhsBus->getLSB() + lhsStop &&
             rhsBit != rhsBus->getLSB() + rhsStop) {
        auto lhsNetBit = lhsBus->getBit(lhsBit);
        auto rhsNetBit = rhsBus->getBit(rhsBit);
        if (lhsNetBit && rhsNetBit) {
          createAssignInstance(design, rhsNetBit, lhsNetBit, sourceRange);
        }
        lhsDesc ? --lhsBit : ++lhsBit;
        rhsDesc ? --rhsBit : ++rhsBit;
      }
      return true;
    }

    std::optional<int64_t> getActiveForLoopConstant(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped || !slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        return std::nullopt;
      }
      const auto& symbol = stripped->as<slang::ast::ValueExpressionBase>().symbol;
      for (auto it = activeForLoopConstants_.rbegin(); it != activeForLoopConstants_.rend(); ++it) {
        if (it->first == &symbol) {
          return it->second;
        }
      }
      return std::nullopt;
    }

    bool hasActiveForLoopContext() const {
      return !activeForLoopBreaks_.empty();
    }

    bool isCurrentForLoopBreakRequested() const {
      return hasActiveForLoopContext() && activeForLoopBreaks_.back();
    }

    void setCurrentForLoopBreakRequested(bool value) const {
      if (hasActiveForLoopContext()) {
        activeForLoopBreaks_.back() = value;
      }
    }

    void requestCurrentForLoopBreak() const {
      if (!hasActiveForLoopContext()) { // LCOV_EXCL_START
        throw SNLSVConstructorException(
          "Internal error: break statement outside active for-loop context");
      } // LCOV_EXCL_STOP
      activeForLoopBreaks_.back() = true;
    }

    const Symbol* getConstantEvalSymbol(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return nullptr; // LCOV_EXCL_LINE
      }

      if (const Symbol* symbolRef = stripped->getSymbolReference()) {
        return symbolRef;
      }

      return nullptr;
    }

    bool getConstantUnsigned(const Expression& expr, uint64_t& value) const {
      if (auto loopValue = getActiveForLoopConstant(expr)) {
        if (*loopValue < 0) {
          return false;
        }
        value = static_cast<uint64_t>(*loopValue);
        return true;
      }
      const auto* stripped = stripConversions(expr);
      if (!stripped ||
          stripped->kind != slang::ast::ExpressionKind::IntegerLiteral) {
        return false;
      }
      const auto& literal = stripped->as<slang::ast::IntegerLiteral>();
      auto maybeValue = literal.getValue().as<uint64_t>();
      if (!maybeValue) {
        return false;
      }
      value = *maybeValue;
      return true;
    }

    bool tryMultiplyInt64(int64_t lhs, int64_t rhs, int64_t& result) const {
#if defined(__SIZEOF_INT128__)
      const __int128 product = static_cast<__int128>(lhs) * static_cast<__int128>(rhs);
      if (product < static_cast<__int128>(std::numeric_limits<int64_t>::min()) ||
          product > static_cast<__int128>(std::numeric_limits<int64_t>::max())) {
        return false;
      }
      result = static_cast<int64_t>(product);
      return true;
#else
      if (lhs == 0 || rhs == 0) {
        result = 0;
        return true;
      }
      if (lhs == -1) {
        if (rhs == std::numeric_limits<int64_t>::min()) {
          return false;
        }
        result = -rhs;
        return true;
      }
      if (rhs == -1) {
        if (lhs == std::numeric_limits<int64_t>::min()) {
          return false;
        }
        result = -lhs;
        return true;
      }
      if (lhs > 0) {
        if (rhs > 0) {
          if (lhs > std::numeric_limits<int64_t>::max() / rhs) {
            return false;
          }
        } else if (rhs < std::numeric_limits<int64_t>::min() / lhs) {
          return false;
        }
      } else if (rhs > 0) {
        if (lhs < std::numeric_limits<int64_t>::min() / rhs) {
          return false;
        }
      } else if (lhs != 0 && rhs < std::numeric_limits<int64_t>::max() / lhs) {
        return false;
      }
      result = lhs * rhs;
      return true;
#endif
    }

    bool tryPowerInt64(int64_t base, int64_t exponent, int64_t& value) const {
      if (exponent < 0) {
        return false;
      }

      int64_t result = 1;
      int64_t factor = base;
      uint64_t remaining = static_cast<uint64_t>(exponent);
      while (remaining) {
        if (remaining & 1ULL) {
          if (!tryMultiplyInt64(result, factor, result)) {
            return false;
          }
        }
        remaining >>= 1;
        if (remaining && !tryMultiplyInt64(factor, factor, factor)) {
          return false;
        }
      }
      value = result;
      return true;
    }

    bool getConstantInt64(const Expression& expr, int64_t& value) const {
      if (auto loopValue = getActiveForLoopConstant(expr)) {
        value = *loopValue;
        return true;
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }

      if (stripped->kind == slang::ast::ExpressionKind::IntegerLiteral) {
        const auto& literal = stripped->as<slang::ast::IntegerLiteral>();
        return tryGetInt64FromSVInt(literal.getValue(), value);
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      const Symbol* evalSymbol = stripped->getSymbolReference();
      if (!evalSymbol && slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        evalSymbol = &stripped->as<slang::ast::ValueExpressionBase>().symbol;
      }
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        constant = (evaluatedConstant && evaluatedConstant.isInteger()) ? &evaluatedConstant : constant;
      }
      if (!constant || !constant->isInteger()) {
        if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
          const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
          int64_t operandValue = 0;
          if (!getConstantInt64(unaryExpr.operand(), operandValue)) {
            return false;
          }
          switch (unaryExpr.op) {
            case slang::ast::UnaryOperator::Plus:
              value = operandValue;
              return true;
            case slang::ast::UnaryOperator::Minus:
              value = -operandValue;
              return true;
            case slang::ast::UnaryOperator::BitwiseNot:
              value = ~operandValue;
              return true;
            case slang::ast::UnaryOperator::LogicalNot:
              value = operandValue ? 0 : 1;
              return true;
            default:
              return false;
          }
        }

        if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
          int64_t leftValue = 0;
          int64_t rightValue = 0;
          if (!getConstantInt64(binaryExpr.left(), leftValue) ||
              !getConstantInt64(binaryExpr.right(), rightValue)) {
            return false;
          }
          switch (binaryExpr.op) {
            case slang::ast::BinaryOperator::Add:
              value = leftValue + rightValue;
              return true;
            case slang::ast::BinaryOperator::Subtract:
              value = leftValue - rightValue;
              return true;
            case slang::ast::BinaryOperator::Multiply:
              return tryMultiplyInt64(leftValue, rightValue, value);
            case slang::ast::BinaryOperator::Power:
              return tryPowerInt64(leftValue, rightValue, value);
            case slang::ast::BinaryOperator::Divide:
              if (rightValue == 0) {
                return false;
              }
              value = leftValue / rightValue;
              return true;
            case slang::ast::BinaryOperator::Mod:
              if (rightValue == 0) {
                return false;
              }
              value = leftValue % rightValue;
              return true;
            case slang::ast::BinaryOperator::LogicalShiftLeft:
              if (rightValue < 0 ||
                  rightValue >= static_cast<int64_t>(sizeof(int64_t) * 8)) {
                return false;
              }
              value = leftValue << rightValue;
              return true;
            case slang::ast::BinaryOperator::LogicalShiftRight: {
              if (rightValue < 0 ||
                  rightValue >= static_cast<int64_t>(sizeof(int64_t) * 8)) {
                return false;
              }
              const auto unsignedLeft = static_cast<uint64_t>(leftValue);
              value = static_cast<int64_t>(unsignedLeft >> rightValue);
              return true;
            }
            case slang::ast::BinaryOperator::ArithmeticShiftRight:
              if (rightValue < 0 ||
                  rightValue >= static_cast<int64_t>(sizeof(int64_t) * 8)) {
                return false;
              }
              value = leftValue >> rightValue;
              return true;
            case slang::ast::BinaryOperator::BinaryAnd:
              value = leftValue & rightValue;
              return true;
            case slang::ast::BinaryOperator::BinaryOr:
              value = leftValue | rightValue;
              return true;
            case slang::ast::BinaryOperator::BinaryXor:
              value = leftValue ^ rightValue;
              return true;
            case slang::ast::BinaryOperator::LessThan:
              value = leftValue < rightValue ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::LessThanEqual:
              value = leftValue <= rightValue ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::GreaterThan:
              value = leftValue > rightValue ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::GreaterThanEqual:
              value = leftValue >= rightValue ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::Equality:
              value = leftValue == rightValue ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::Inequality:
              value = leftValue != rightValue ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::LogicalAnd:
              value = (leftValue != 0 && rightValue != 0) ? 1 : 0;
              return true;
            case slang::ast::BinaryOperator::LogicalOr:
              value = (leftValue != 0 || rightValue != 0) ? 1 : 0;
              return true;
            // LCOV_EXCL_START
            default:
              return false;
            // LCOV_EXCL_STOP
          }
        }

        return false;
      }
      return tryGetInt64FromSVInt(constant->integer(), value);
    }

    void resizeBitsToWidth(
      std::vector<SNLBitNet*>& bits,
      size_t width,
      SNLBitNet* fillBit) const {
      if (bits.size() > width) {
        bits.resize(width);
      } else if (bits.size() < width) {
        bits.resize(width, fillBit);
      }
    }

    void resizeLogicBitsToWidth(
      std::vector<slang::logic_t>& bits,
      size_t width,
      slang::logic_t fillBit) const {
      if (bits.size() > width) {
        bits.resize(width);
      } else if (bits.size() < width) {
        bits.resize(width, fillBit);
      }
    }

    std::optional<bool> resolveUnbasedOrStructuredPatternBits(
      SNLDesign* design,
      const Expression& stripped,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      if (stripped.kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        const auto value =
          stripped.as<slang::ast::UnbasedUnsizedIntegerLiteral>().getLiteralValue();
        if (value.isUnknown()) {
          return false;
        }
        bits.assign(
          targetWidth,
          static_cast<SNLBitNet*>(getConstNet(design, static_cast<bool>(value))));
        return true;
      }

      if (stripped.kind == slang::ast::ExpressionKind::StructuredAssignmentPattern) {
        const auto& pattern =
          stripped.as<slang::ast::StructuredAssignmentPatternExpression>();
        if (pattern.typeSetters.empty() && pattern.indexSetters.empty()) {
          bool initialized = false;
          bool requiresFullMemberCoverage = false;
          size_t coveredMemberBits = 0;
          std::unordered_set<const Symbol*> coveredMembers;
          if (pattern.defaultSetter) {
            bool defaultBit = false;
            if (getConstantBit(*pattern.defaultSetter, defaultBit)) {
              bits.assign(targetWidth, static_cast<SNLBitNet*>(getConstNet(design, defaultBit)));
              initialized = true;
            } else if (auto defaultWidth = getIntegralExpressionBitWidth(*pattern.defaultSetter)) {
              std::vector<SNLBitNet*> defaultBits;
              if (resolveExpressionBits(
                    design,
                    *pattern.defaultSetter,
                    *defaultWidth,
                    defaultBits) &&
                  defaultBits.size() == *defaultWidth && !defaultBits.empty()) {
                if (defaultBits.size() == targetWidth) {
                  bits = std::move(defaultBits);
                  initialized = true;
                } else if (targetWidth % defaultBits.size() == 0) {
                  bits.clear();
                  bits.reserve(targetWidth);
                  while (bits.size() < targetWidth) {
                    bits.insert(bits.end(), defaultBits.begin(), defaultBits.end());
                  }
                  initialized = true;
                }
              }
            }
          }

          if (!initialized) {
            if (!pattern.defaultSetter && !pattern.memberSetters.empty()) {
              bits.assign(targetWidth, nullptr);
              initialized = true;
              requiresFullMemberCoverage = true;
            } else if (!pattern.defaultSetter || !pattern.memberSetters.empty()) {
              return std::nullopt;
            }
          }

          for (const auto& setter : pattern.memberSetters) {
            if (setter.member->kind != SymbolKind::Field) {
              return std::nullopt; // LCOV_EXCL_LINE
            }
            const auto& field = setter.member->as<slang::ast::FieldSymbol>();
            const auto& fieldType = field.getType().getCanonicalType();
            if (!fieldType.isIntegral()) {
              return std::nullopt; // LCOV_EXCL_LINE
            }
            const auto memberWidthBits = fieldType.getBitWidth();
            if (memberWidthBits <= 0) {
              return std::nullopt; // LCOV_EXCL_LINE
            }
            const auto memberWidth = static_cast<size_t>(memberWidthBits);

            std::vector<SNLBitNet*> memberBits;
            if (!resolveExpressionBits(design, *setter.expr, memberWidth, memberBits) ||
                memberBits.size() != memberWidth) {
              return std::nullopt;
            }

            const auto offset = static_cast<size_t>(field.bitOffset);
            if (offset + memberWidth > bits.size()) {
              return std::nullopt; // LCOV_EXCL_LINE
            }
            if (requiresFullMemberCoverage) {
              if (!coveredMembers.insert(setter.member).second) {
                return std::nullopt; // LCOV_EXCL_LINE
              }
              coveredMemberBits += memberWidth;
              if (coveredMemberBits > targetWidth) {
                return std::nullopt; // LCOV_EXCL_LINE
              }
            }
            std::copy(memberBits.begin(), memberBits.end(), bits.begin() + offset);
          }
          if (requiresFullMemberCoverage && coveredMemberBits != targetWidth) {
            return std::nullopt; // LCOV_EXCL_LINE
          }

          if (initialized || !pattern.memberSetters.empty()) {
            return true;
          }
        }
      }

      return std::nullopt;
    }

    std::optional<size_t> getIntegralExpressionBitWidth(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return std::nullopt; // LCOV_EXCL_LINE
      }
      const auto& canonical = stripped->type->getCanonicalType();
      if (!canonical.isIntegral()) {
        return std::nullopt;
      }
      const auto bitWidth = canonical.getBitWidth();
      if (bitWidth <= 0) {
        return std::nullopt; // LCOV_EXCL_LINE
      }
      return static_cast<size_t>(bitWidth);
    }

    bool getConstantInt32(const Expression& expr, int32_t& value) const {
      if (auto loopValue = getActiveForLoopConstant(expr)) {
        if (*loopValue < static_cast<int64_t>(std::numeric_limits<int32_t>::min()) ||
            *loopValue > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) {
          return false;
        }
        value = static_cast<int32_t>(*loopValue);
        return true;
      }

      uint64_t unsignedValue = 0;
      if (getConstantUnsigned(expr, unsignedValue)) {
        if (unsignedValue > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
          return false;
        }
        value = static_cast<int32_t>(unsignedValue);
        return true;
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }
      if (hasActiveForLoopContext() &&
          (stripped->kind == slang::ast::ExpressionKind::BinaryOp ||
           stripped->kind == slang::ast::ExpressionKind::UnaryOp ||
           slang::ast::ValueExpressionBase::isKind(stripped->kind))) {
        int64_t signedValue = 0;
        if (getConstantInt64(expr, signedValue)) {
          if (signedValue < static_cast<int64_t>(std::numeric_limits<int32_t>::min()) ||
              signedValue > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) {
            return false;
          }
          value = static_cast<int32_t>(signedValue);
          return true;
        }
      }
      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      const Symbol* evalSymbol = stripped->getSymbolReference();
      if (!evalSymbol && slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        evalSymbol = &stripped->as<slang::ast::ValueExpressionBase>().symbol;
      }
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        constant = (evaluatedConstant && evaluatedConstant.isInteger()) ? &evaluatedConstant : constant;
      }
      if (!constant || !constant->isInteger()) {
        return false;
      }
      const auto maybeSigned = constant->integer().as<int64_t>();
      if (!maybeSigned) {
        return false;
      }
      if (*maybeSigned < static_cast<int64_t>(std::numeric_limits<int32_t>::min()) ||
          *maybeSigned > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) {
        return false;
      }
      value = static_cast<int32_t>(*maybeSigned);
      return true;
    }

    bool resolveSelectBits(
      SNLDesign* design,
      const Expression& selectExpr,
      std::vector<SNLBitNet*>& bits) {
      const Expression* valueExpr = nullptr;
      if (selectExpr.kind == slang::ast::ExpressionKind::ElementSelect) {
        valueExpr = &selectExpr.as<slang::ast::ElementSelectExpression>().value();
      } else if (selectExpr.kind == slang::ast::ExpressionKind::RangeSelect) {
        valueExpr = &selectExpr.as<slang::ast::RangeSelectExpression>().value();
      } else {
        return false; // LCOV_EXCL_LINE
      }

      auto valueNet = resolveExpressionNet(design, *valueExpr);
      if (!valueNet) {
        return false;
      }
      auto valueBits = collectBits(valueNet);
      if (valueBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      const auto* symbolRef = selectExpr.getSymbolReference();
      if (!symbolRef) {
        symbolRef = valueExpr->getSymbolReference();
      }
      if (!symbolRef) {
        return false;
      }

      slang::ast::EvalContext evalContext(*symbolRef);
      auto selectedRange = selectExpr.evalSelector(evalContext, true);
      if (!selectedRange) {
        return false;
      }

      auto baseRange = slang::ConstantRange(0, 0);
      if (auto valueBus = dynamic_cast<SNLBusNet*>(valueNet)) {
        baseRange = slang::ConstantRange(
          static_cast<int32_t>(valueBus->getMSB()),
          static_cast<int32_t>(valueBus->getLSB()));
      }

      bits.clear();
      bits.reserve(selectedRange->width());
      int32_t index = selectedRange->right;
      const int32_t end = selectedRange->left;
      const int32_t step = index <= end ? 1 : -1;
      while (index != end + step) {
        const auto translated = baseRange.translateIndex(index);
        if (translated < 0 ||
            translated >= static_cast<int32_t>(valueBits.size())) {
          return false;
        }
        bits.push_back(valueBits[static_cast<size_t>(translated)]);
        index += step;
      }
      return true;
    }

    bool resolveSelectableExpressionBits(
      SNLDesign* design,
      const Expression& expr,
      std::vector<SNLBitNet*>& bits) {
      const Expression* current = stripConversions(expr);
      const Expression* baseExpr = current;
      while (baseExpr) {
        if (baseExpr->kind == slang::ast::ExpressionKind::ElementSelect) {
          baseExpr = stripConversions(baseExpr->as<slang::ast::ElementSelectExpression>().value());
          continue;
        }
        if (baseExpr->kind == slang::ast::ExpressionKind::RangeSelect) {
          baseExpr = stripConversions(baseExpr->as<slang::ast::RangeSelectExpression>().value());
          continue;
        }
        if (baseExpr->kind == slang::ast::ExpressionKind::MemberAccess) {
          baseExpr = stripConversions(baseExpr->as<slang::ast::MemberAccessExpression>().value());
          continue;
        }
        break;
      }
      if (!baseExpr) {
        return false; // LCOV_EXCL_LINE
      }

      auto baseNet = resolveExpressionNet(design, *baseExpr);
      if (!baseNet) {
        return false;
      }
      auto baseBits = collectBits(baseNet);
      if (baseBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      auto baseRange = slang::ConstantRange(0, 0);
      bool hasBaseRange = false;
      if (auto* baseBus = dynamic_cast<SNLBusNet*>(baseNet)) {
        baseRange = slang::ConstantRange(
          static_cast<int32_t>(baseBus->getMSB()),
          static_cast<int32_t>(baseBus->getLSB()));
        hasBaseRange = true;
      } else {
        const auto& baseType = baseExpr->type->getCanonicalType();
        if (baseType.hasFixedRange()) {
          baseRange = baseType.getFixedRange();
          hasBaseRange = true;
        }
      }
      if (!hasBaseRange && !baseBits.empty()) {
        const auto upper = static_cast<int32_t>(baseBits.size() - 1);
        baseRange = slang::ConstantRange(upper, 0);
        hasBaseRange = true;
      }

      const auto tryResolveConstantSelectedRange = [&]() {
        bits.clear();
        if (current->kind == slang::ast::ExpressionKind::ElementSelect) {
          const auto& elementExpr = current->as<slang::ast::ElementSelectExpression>();
          int32_t selectedIndex = 0;
          if (!getConstantInt32(elementExpr.selector(), selectedIndex)) {
            return false;
          }
          const auto translated = baseRange.translateIndex(selectedIndex);
          if (translated < 0 ||
              translated >= static_cast<int32_t>(baseBits.size())) {
            return false;
          }
          bits.push_back(baseBits[static_cast<size_t>(translated)]);
          return true;
        }

        if (current->kind == slang::ast::ExpressionKind::RangeSelect) {
          const auto& rangeExpr = current->as<slang::ast::RangeSelectExpression>();
          switch (rangeExpr.getSelectionKind()) {
            case slang::ast::RangeSelectionKind::Simple: {
              int32_t left = 0;
              int32_t right = 0;
              if (!getConstantInt32(rangeExpr.left(), left) ||
                  !getConstantInt32(rangeExpr.right(), right)) {
                return false;
              }
              int32_t index = right;
              const int32_t end = left;
              const int32_t step = index <= end ? 1 : -1;
              while (index != end + step) {
                const auto translated = baseRange.translateIndex(index);
                if (translated < 0 ||
                    translated >= static_cast<int32_t>(baseBits.size())) {
                  bits.clear();
                  return false;
                }
                bits.push_back(baseBits[static_cast<size_t>(translated)]);
                index += step;
              }
              return !bits.empty();
            }
            case slang::ast::RangeSelectionKind::IndexedUp:
            case slang::ast::RangeSelectionKind::IndexedDown: {
              int32_t startIndex = 0;
              int32_t sliceWidth = 0;
              if (!getConstantInt32(rangeExpr.left(), startIndex) ||
                  !getConstantInt32(rangeExpr.right(), sliceWidth) ||
                  sliceWidth <= 0) {
                return false;
              }
              int64_t lsbIndex = static_cast<int64_t>(startIndex);
              if (rangeExpr.getSelectionKind() ==
                  slang::ast::RangeSelectionKind::IndexedDown) {
                lsbIndex -= static_cast<int64_t>(sliceWidth - 1);
              }
              for (int32_t elem = 0; elem < sliceWidth; ++elem) {
                const int64_t elemIndex = lsbIndex + static_cast<int64_t>(elem);
                if (elemIndex < std::numeric_limits<int32_t>::min() ||
                    elemIndex > std::numeric_limits<int32_t>::max()) {
                  bits.clear();
                  return false;
                }
                const auto translated =
                  baseRange.translateIndex(static_cast<int32_t>(elemIndex));
                if (translated < 0 ||
                    translated >= static_cast<int32_t>(baseBits.size())) {
                  bits.clear();
                  return false;
                }
                bits.push_back(baseBits[static_cast<size_t>(translated)]);
              }
              return !bits.empty();
            }
          }
        }

        return false;
      };

      if (tryResolveConstantSelectedRange()) {
        return true;
      }

      const Symbol* evalSymbol = nullptr;
      if (slang::ast::ValueExpressionBase::isKind(baseExpr->kind)) {
        evalSymbol = &baseExpr->as<slang::ast::ValueExpressionBase>().symbol;
      } else {
        // LCOV_EXCL_START
        evalSymbol = expr.getSymbolReference(false);
        if (!evalSymbol) {
          evalSymbol = expr.getSymbolReference(true);
        }
        // LCOV_EXCL_STOP
      }
      // LCOV_EXCL_START
      if (!evalSymbol) {
        return false;
      }
      // LCOV_EXCL_STOP

      slang::ast::EvalContext evalContext(*evalSymbol);
      auto selectedRange = expr.evalSelector(evalContext, true);
      if (!selectedRange) {
        return false;
      }

      bits.clear();
      bits.reserve(selectedRange->width());
      int32_t index = selectedRange->right;
      const int32_t end = selectedRange->left;
      const int32_t step = index <= end ? 1 : -1;
      while (index != end + step) {
        const auto translated = baseRange.translateIndex(index);
        if (translated < 0 ||
            translated >= static_cast<int32_t>(baseBits.size())) {
          return false; // LCOV_EXCL_LINE
        }
        bits.push_back(baseBits[static_cast<size_t>(translated)]);
        index += step;
      }
      return true;
    }

    bool extractFunctionReturnConstantBit(const Statement& stmt, bool& value) const {
      const Statement* unwrapped = unwrapStatement(stmt);
      if (!unwrapped ||
          unwrapped->kind != slang::ast::StatementKind::Return) {
        return false;
      }
      const auto& returnStmt = unwrapped->as<slang::ast::ReturnStatement>();
      return returnStmt.expr && getConstantBit(*returnStmt.expr, value);
    }

    bool extractFunctionReturnExpression(const Statement& stmt, const Expression*& expr) const {
      expr = nullptr;
      const Statement* unwrapped = unwrapStatement(stmt);
      if (!unwrapped || unwrapped->kind != slang::ast::StatementKind::Return) {
        return false;
      }
      const auto& returnStmt = unwrapped->as<slang::ast::ReturnStatement>();
      if (!returnStmt.expr) {
        return false; // LCOV_EXCL_LINE
      }
      expr = returnStmt.expr;
      return true;
    }

    bool getExpressionConstantValue(
      const Expression& expr,
      slang::ConstantValue& value) const {
      const auto* stripped = stripConversions(expr);
      const Symbol* evalSymbol = stripped->getSymbolReference(false);
      if (!evalSymbol) {
        evalSymbol = stripped->getSymbolReference(true);
      }
      if (!evalSymbol) {
        return false;
      }
      slang::ast::EvalContext evalContext(*evalSymbol);
      value = stripped->eval(evalContext);
      return static_cast<bool>(value);
    }

    bool buildCaseInsideMatchBit(
      SNLDesign* design,
      const Expression& valueExpr,
      const Expression& itemExpr,
      SNLBitNet*& matchBit,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      matchBit = nullptr;

      const auto* strippedItemExpr = stripConversions(itemExpr);
      if (!strippedItemExpr) {
        return false; // LCOV_EXCL_LINE
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

      if (strippedItemExpr->kind == slang::ast::ExpressionKind::ValueRange) {
        const auto& valueRangeExpr = strippedItemExpr->as<slang::ast::ValueRangeExpression>();
        if (valueRangeExpr.rangeKind != slang::ast::ValueRangeKind::Simple) {
          return false; // LCOV_EXCL_LINE
        }

        auto* geNet = SNLScalarNet::create(design);
        annotateSourceInfo(geNet, sourceRange);
        if (!createRelationalAssign(
              design,
              geNet,
              valueExpr,
              valueRangeExpr.left(),
              slang::ast::BinaryOperator::GreaterThanEqual,
              sourceRange)) {
          return false;
        }

        auto* leNet = SNLScalarNet::create(design);
        annotateSourceInfo(leNet, sourceRange);
        if (!createRelationalAssign(
              design,
              leNet,
              valueExpr,
              valueRangeExpr.right(),
              slang::ast::BinaryOperator::LessThanEqual,
              sourceRange)) {
          return false;
        }

        auto* geBit = getSingleBitNet(geNet);
        auto* leBit = getSingleBitNet(leNet);
        if (!geBit || !leBit) {
          return false; // LCOV_EXCL_LINE
        }
        auto* andNet = SNLScalarNet::create(design);
        annotateSourceInfo(andNet, sourceRange);
        matchBit = getSingleBitNet(createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::And),
          geBit,
          leBit,
          andNet,
          sourceRange));
        return matchBit != nullptr;
      }

      auto valueWidth = getIntegralExpressionBitWidth(valueExpr);
      auto itemWidth = getIntegralExpressionBitWidth(*strippedItemExpr);
      if (valueWidth && itemWidth) {
        const auto compareWidth = std::max(*valueWidth, *itemWidth);
        const bool bothSigned =
          valueExpr.type->getCanonicalType().isSigned() &&
          strippedItemExpr->type->getCanonicalType().isSigned();

        std::vector<slang::logic_t> itemPatternBits;
        if (resolveWildcardCaseItemPattern(
              *strippedItemExpr,
              compareWidth,
              bothSigned,
              itemPatternBits) &&
            itemPatternBits.size() == compareWidth) {
          std::vector<SNLBitNet*> valueBits;
          if (!resolveExpressionBits(design, valueExpr, compareWidth, valueBits) ||
              valueBits.size() != compareWidth) {
            return false;
          }

          std::vector<SNLNet*> bitMatches;
          bitMatches.reserve(compareWidth);
          for (size_t bitIndex = 0; bitIndex < compareWidth; ++bitIndex) {
            const auto itemBit = itemPatternBits[bitIndex];
            if (itemBit.isUnknown()) {
              continue;
            }

            auto* currentMatchBit = valueBits[bitIndex];
            if (!static_cast<bool>(itemBit)) {
              if (currentMatchBit == const0) {
                currentMatchBit = const1;
              } else if (currentMatchBit == const1) {
                currentMatchBit = const0;
              } else {
                auto* invertedBit = SNLScalarNet::create(design);
                annotateSourceInfo(invertedBit, sourceRange);
                if (!createUnaryGate(
                      design,
                      NLDB0::GateType(NLDB0::GateType::Not),
                      currentMatchBit,
                      invertedBit,
                      sourceRange)) {
                  return false; // LCOV_EXCL_LINE
                }
                currentMatchBit = invertedBit;
              }
            }
            bitMatches.push_back(currentMatchBit);
          }

          if (bitMatches.empty()) {
            matchBit = const1;
            return true;
          }
          if (bitMatches.size() == 1) {
            matchBit = getSingleBitNet(bitMatches.front());
            return matchBit != nullptr;
          }

          SNLNet* andNet = SNLScalarNet::create(design);
          annotateSourceInfo(andNet, sourceRange);
          if (!createGateInstance(
                design,
                NLDB0::GateType(NLDB0::GateType::And),
                bitMatches,
                andNet,
                sourceRange)) {
            return false; // LCOV_EXCL_LINE
          }
          matchBit = getSingleBitNet(andNet);
          return matchBit != nullptr;
        }
      }

      auto* eqNet = SNLScalarNet::create(design);
      annotateSourceInfo(eqNet, sourceRange);
      if (!createEqualityAssign(
            design,
            eqNet,
            valueExpr,
            *strippedItemExpr,
            sourceRange)) {
        return false;
      }
      matchBit = getSingleBitNet(eqNet);
      return matchBit != nullptr;
    }

    bool resolveSimpleCaseInsideFunctionCallBit(
      SNLDesign* design,
      const slang::ast::CallExpression& callExpr,
      SNLBitNet*& resultBit,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      resultBit = nullptr;
      if (callExpr.isSystemCall() || callExpr.subroutine.index() != 0) {
        return false;
      }

      const auto* subroutine = std::get<0>(callExpr.subroutine);
      if (!subroutine || subroutine->subroutineKind != slang::ast::SubroutineKind::Function ||
          subroutine->hasOutputArgs()) {
        return false;
      }

      auto formalArgs = subroutine->getArguments();
      auto callArgs = callExpr.arguments();
      if (formalArgs.size() != 1 || !formalArgs.front() ||
          callArgs.size() != 1 || !callArgs.front()) {
        return false;
      }
      if (formalArgs.front()->direction != ArgumentDirection::In) {
        return false;
      }

      const Statement* bodyStmt = unwrapStatement(subroutine->getBody());
      if (!bodyStmt || bodyStmt->kind != slang::ast::StatementKind::Case) {
        return false;
      }

      const auto& caseStmt = bodyStmt->as<slang::ast::CaseStatement>();
      if (caseStmt.condition != slang::ast::CaseStatementCondition::Inside) {
        return false;
      }
      const auto* caseExpr = stripConversions(caseStmt.expr);
      if (!caseExpr || !slang::ast::ValueExpressionBase::isKind(caseExpr->kind) ||
          &caseExpr->as<slang::ast::ValueExpressionBase>().symbol != formalArgs.front()) {
        return false;
      }

      if (!caseStmt.defaultCase) {
        return false;
      }
      bool defaultValue = false;
      if (!extractFunctionReturnConstantBit(*caseStmt.defaultCase, defaultValue)) {
        return false;
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

      auto mergeOr = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) return nullptr; // LCOV_EXCL_LINE
        if (leftBit == const0) {
          return rightBit;
        }
        auto* orNet = SNLScalarNet::create(design);
        annotateSourceInfo(orNet, sourceRange);
        return getSingleBitNet(createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::Or),
          leftBit,
          rightBit,
          orNet,
          sourceRange));
      };

      auto makeNot = [&](SNLBitNet* inputBit) -> SNLBitNet* {
        if (!inputBit) return nullptr; // LCOV_EXCL_LINE
        auto* notNet = SNLScalarNet::create(design);
        annotateSourceInfo(notNet, sourceRange);
        if (!createUnaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Not),
              inputBit,
              notNet,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return notNet;
      };

      SNLBitNet* oppositeMatchBit = const0;
      bool sawOppositeCase = false;
      for (const auto& item : caseStmt.items) {
        if (!item.stmt) return false; // LCOV_EXCL_LINE

        bool itemValue = false;
        if (!extractFunctionReturnConstantBit(*item.stmt, itemValue)) {
          return false;
        }
        if (itemValue == defaultValue) {
          continue;
        }

        SNLBitNet* itemMatchBit = const0;
        for (const auto* itemExpr : item.expressions) {
          if (!itemExpr) {
            return false; // LCOV_EXCL_LINE
          }
          SNLBitNet* exprMatchBit = nullptr;
          if (!buildCaseInsideMatchBit(
                design,
                *callArgs.front(),
                *itemExpr,
                exprMatchBit,
                sourceRange)) {
            return false;
          }
          itemMatchBit = mergeOr(itemMatchBit, exprMatchBit);
          if (!itemMatchBit) {
            return false; // LCOV_EXCL_LINE
          }
        }

        oppositeMatchBit = mergeOr(oppositeMatchBit, itemMatchBit);
        if (!oppositeMatchBit) {
          return false; // LCOV_EXCL_LINE
        }
        sawOppositeCase = true;
      }

      if (!sawOppositeCase) {
        resultBit = defaultValue ? const1 : const0;
        return true;
      }

      resultBit = defaultValue ? makeNot(oppositeMatchBit) : oppositeMatchBit;
      return resultBit != nullptr;
    }

    bool resolveRangeCheckFunctionCallBit(
      SNLDesign* design,
      const slang::ast::CallExpression& callExpr,
      SNLBitNet*& resultBit,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      resultBit = nullptr;
      if (callExpr.isSystemCall() || callExpr.subroutine.index() != 0) {
        return false;
      }

      const auto* subroutine = std::get<0>(callExpr.subroutine);
      if (!subroutine || subroutine->subroutineKind != slang::ast::SubroutineKind::Function ||
          subroutine->hasOutputArgs()) {
        return false;
      }

      auto formalArgs = subroutine->getArguments();
      auto callArgs = callExpr.arguments();
      if (formalArgs.size() != 2 || callArgs.size() != 2 ||
          !formalArgs[0] || !formalArgs[1] || !callArgs[0] || !callArgs[1]) {
        return false;
      }
      if (formalArgs[0]->direction != ArgumentDirection::In ||
          formalArgs[1]->direction != ArgumentDirection::In) {
        return false; // LCOV_EXCL_LINE
      }

      const Statement* bodyStmt = unwrapStatement(subroutine->getBody());
      if (!bodyStmt) {
        return false; // LCOV_EXCL_LINE
      }

      bool defaultWhenNoRules = false;
      const Statement* searchStmt = bodyStmt;
      if (searchStmt->kind == slang::ast::StatementKind::List) {
        for (const auto* nested : searchStmt->as<slang::ast::StatementList>().list) {
          const Statement* unwrapped = nested ? unwrapStatement(*nested) : nullptr;
          if (unwrapped && unwrapped->kind == slang::ast::StatementKind::Conditional) {
            searchStmt = unwrapped;
            break;
          }
        }
      }
      if (searchStmt->kind == slang::ast::StatementKind::Conditional) {
        const auto& conditional = searchStmt->as<slang::ast::ConditionalStatement>();
        if (!conditional.ifFalse ||
            conditional.conditions.empty() ||
            !extractFunctionReturnConstantBit(*conditional.ifFalse, defaultWhenNoRules)) {
          return false;
        }
        searchStmt = unwrapStatement(conditional.ifTrue);
        if (!searchStmt) {
          return false; // LCOV_EXCL_LINE
        }
      }

      std::function<const Statement*(const Statement*)> findForLoopStatement =
        [&](const Statement* stmt) -> const Statement* {
          if (!stmt) {
            return nullptr; // LCOV_EXCL_LINE
          }
          const Statement* unwrapped = unwrapStatement(*stmt);
          if (!unwrapped) {
            return nullptr; // LCOV_EXCL_LINE
          }
          if (unwrapped->kind == slang::ast::StatementKind::ForLoop) {
            return unwrapped;
          }
          if (unwrapped->kind == slang::ast::StatementKind::List) {
            for (const auto* nested : unwrapped->as<slang::ast::StatementList>().list) {
              if (auto* forStmt = findForLoopStatement(nested)) {
                return forStmt;
              }
            }
          }
          return nullptr;
        };
      const Statement* forStmt = findForLoopStatement(searchStmt);
      if (!forStmt) {
        return false; // LCOV_EXCL_LINE
      }

      const auto& forLoop = forStmt->as<slang::ast::ForLoopStatement>();
      if (!forLoop.stopExpr ||
          forLoop.stopExpr->kind != slang::ast::ExpressionKind::BinaryOp) {
        return false;
      }
      const auto& stopExpr = forLoop.stopExpr->as<slang::ast::BinaryExpression>();
      if (stopExpr.op != slang::ast::BinaryOperator::LessThan) {
        return false;
      }
      const auto* loopIndexExpr = stripConversions(stopExpr.left());
      if (!loopIndexExpr ||
          !slang::ast::ValueExpressionBase::isKind(loopIndexExpr->kind)) {
        return false;
      }
      const auto* loopSymbol = &loopIndexExpr->as<slang::ast::ValueExpressionBase>().symbol;
      const Expression* ruleCountExpr = &stopExpr.right();

      const Expression* baseExpr = nullptr;
      const Expression* lenExpr = nullptr;
      const Expression* addressExpr = nullptr;
      auto tryExtractRangeCheckCall = [&](const Statement& stmt) -> bool {
        const Statement* unwrapped = unwrapStatement(stmt);
        if (!unwrapped ||
            unwrapped->kind != slang::ast::StatementKind::ExpressionStatement) {
          return false;
        }
        const auto& exprStmt = unwrapped->as<slang::ast::ExpressionStatement>();
        const auto* assignmentExpr = stripConversions(exprStmt.expr);
        if (!assignmentExpr ||
            assignmentExpr->kind != slang::ast::ExpressionKind::Assignment) {
          return false;
        }
        const auto& assignment = assignmentExpr->as<slang::ast::AssignmentExpression>();
        const auto* rhsExpr = stripConversions(assignment.right());
        if (!rhsExpr || rhsExpr->kind != slang::ast::ExpressionKind::Call) {
          return false;
        }
        const auto& rangeCallExpr = rhsExpr->as<slang::ast::CallExpression>();
        const auto rangeCallName = rangeCallExpr.getSubroutineName();
        if (rangeCallExpr.isSystemCall() ||
            !(rangeCallName == "range_check" ||
              rangeCallName.ends_with("::range_check"))) {
          return false;
        }
        auto rangeCallArgs = rangeCallExpr.arguments();
        if (rangeCallArgs.size() != 3 ||
            !rangeCallArgs[0] || !rangeCallArgs[1] || !rangeCallArgs[2]) {
          return false; // LCOV_EXCL_LINE
        }
        baseExpr = rangeCallArgs[0];
        lenExpr = rangeCallArgs[1];
        addressExpr = rangeCallArgs[2];
        return true;
      };

      const Statement* loopBody = unwrapStatement(forLoop.body);
      if (!loopBody) {
        return false; // LCOV_EXCL_LINE
      }
      std::function<bool(const Statement*)> findRangeCheckCallInStatement =
        [&](const Statement* stmt) -> bool {
          if (!stmt) {
            return false; // LCOV_EXCL_LINE
          }
          const Statement* unwrapped = unwrapStatement(*stmt);
          if (!unwrapped) {
            return false; // LCOV_EXCL_LINE
          }
          if (tryExtractRangeCheckCall(*unwrapped)) {
            return true;
          }
          if (unwrapped->kind == slang::ast::StatementKind::List) {
            for (const auto* nested : unwrapped->as<slang::ast::StatementList>().list) {
              if (findRangeCheckCallInStatement(nested)) {
                return true;
              }
            }
          }
          return false;
        };
      const bool foundRangeCheckCall = findRangeCheckCallInStatement(loopBody);
      if (!foundRangeCheckCall || !baseExpr || !lenExpr || !addressExpr) {
        return false;
      }

      slang::ConstantValue cfgValue;
      if (!getExpressionConstantValue(*callArgs[0], cfgValue)) {
        return false;
      }

      slang::ast::EvalContext evalContext(*subroutine);
      evalContext.pushEmptyFrame();
      if (!evalContext.createLocal(formalArgs[0], cfgValue)) {
        return false; // LCOV_EXCL_LINE
      }
      auto* loopVarValue = evalContext.createLocal(
        loopSymbol,
        slang::SVInt(32, 0, false));
      if (!loopVarValue) {
        return false; // LCOV_EXCL_LINE
      }

      slang::ConstantValue ruleCountValue = ruleCountExpr->eval(evalContext);
      if (!ruleCountValue || !ruleCountValue.isInteger() ||
          ruleCountValue.integer().hasUnknown()) {
        return false;
      }
      auto maybeRuleCount = ruleCountValue.integer().as<uint64_t>();
      if (!maybeRuleCount || *maybeRuleCount > 4096) {
        return false;
      }

      std::vector<SNLBitNet*> addressBits64;
      if (!resolveExpressionBits(design, *callArgs[1], 64, addressBits64) ||
          addressBits64.size() != 64) {
        return false;
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      auto makeNot = [&](SNLBitNet* inBit) -> SNLBitNet* {
        if (!inBit) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (inBit == const0) {
          return const1;
        }
        if (inBit == const1) {
          return const0;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createUnaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Not),
              inBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };
      auto makeAnd = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (leftBit == const0 || rightBit == const0) {
          return const0;
        }
        if (leftBit == const1) {
          return rightBit;
        }
        if (rightBit == const1 || leftBit == rightBit) {
          return leftBit;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createBinaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::And),
              leftBit,
              rightBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };
      auto makeOr = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (leftBit == const1 || rightBit == const1) {
          return const1;
        }
        if (leftBit == const0) {
          return rightBit;
        }
        if (rightBit == const0 || leftBit == rightBit) {
          return leftBit;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createBinaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Or),
              leftBit,
              rightBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };
      struct UnsignedConstant65 {
        uint64_t lowValue{0};
        bool highBit{false};
      };
      auto buildUnsignedLessThanConstant = [&](const std::vector<SNLBitNet*>& lhsBits,
                                               const UnsignedConstant65& constantValue) -> SNLBitNet* {
        const auto width = lhsBits.size();
        if (!width || width > 65) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (!constantValue.highBit && constantValue.lowValue == 0) {
          return const0;
        }

        SNLBitNet* ltBit = const0;
        SNLBitNet* eqBit = const1;
        for (size_t bitIndex = width; bitIndex-- > 0;) {
          const bool constantBit =
            (bitIndex == 64) ? constantValue.highBit
                             : ((constantValue.lowValue >> bitIndex) & 1U) != 0;
          auto* lhsBit = lhsBits[bitIndex];
          auto* notLhs = makeNot(lhsBit);
          if (!notLhs) {
            return nullptr; // LCOV_EXCL_LINE
          }
          if (constantBit) {
            auto* ltCandidate = makeAnd(eqBit, notLhs);
            if (!ltCandidate) {
              return nullptr; // LCOV_EXCL_LINE
            }
            ltBit = makeOr(ltBit, ltCandidate);
            eqBit = makeAnd(eqBit, lhsBit);
          } else {
            eqBit = makeAnd(eqBit, notLhs);
          }
          if (!ltBit || !eqBit) {
            return nullptr; // LCOV_EXCL_LINE
          }
        }
        return ltBit;
      };

      SNLBitNet* accumulatedMatch = const0;
      for (uint64_t ruleIndex = 0; ruleIndex < *maybeRuleCount; ++ruleIndex) {
        *loopVarValue = slang::SVInt(32, ruleIndex, false);

        slang::ConstantValue baseValue = baseExpr->eval(evalContext);
        slang::ConstantValue lenValue = lenExpr->eval(evalContext);
        if (!baseValue || !lenValue ||
            !baseValue.isInteger() || !lenValue.isInteger() ||
            baseValue.integer().hasUnknown() || lenValue.integer().hasUnknown()) {
          return false;
        }
        auto maybeBase = baseValue.integer().as<uint64_t>();
        auto maybeLen = lenValue.integer().as<uint64_t>();
        if (!maybeBase || !maybeLen) {
          return false;
        }

        auto* ltBase = buildUnsignedLessThanConstant(
          addressBits64,
          UnsignedConstant65{*maybeBase, false});
        auto* geBase = makeNot(ltBase);
        if (!ltBase || !geBase) {
          return false; // LCOV_EXCL_LINE
        }

        std::vector<SNLBitNet*> addressBits65 = addressBits64;
        addressBits65.push_back(const0);
        const uint64_t limitLowValue = *maybeBase + *maybeLen;
        const bool limitHighBit = limitLowValue < *maybeBase;
        const UnsignedConstant65 limitValue{limitLowValue, limitHighBit};
        auto* ltLimit = buildUnsignedLessThanConstant(addressBits65, limitValue);
        if (!ltLimit) {
          return false; // LCOV_EXCL_LINE
        }

        auto* matchBit = makeAnd(geBase, ltLimit);
        if (!matchBit) {
          return false; // LCOV_EXCL_LINE
        }
        accumulatedMatch = makeOr(accumulatedMatch, matchBit);
        if (!accumulatedMatch) {
          return false; // LCOV_EXCL_LINE
        }
      }

      if (*maybeRuleCount == 0) {
        resultBit = defaultWhenNoRules ? const1 : const0;
      } else {
        resultBit = accumulatedMatch;
      }
      return resultBit != nullptr;
    }

    bool resolveSimpleReturnFunctionCallBits(
      SNLDesign* design,
      const slang::ast::CallExpression& callExpr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      bits.clear();
      if (callExpr.isSystemCall() || callExpr.subroutine.index() != 0) {
        return false;
      }

      const auto* subroutine = std::get<0>(callExpr.subroutine);
      if (!subroutine || subroutine->subroutineKind != slang::ast::SubroutineKind::Function ||
          subroutine->hasOutputArgs()) {
        return false;
      }
      for (auto* activeSubroutine : activeInlinedCallSubroutines_) {
        if (activeSubroutine == subroutine) {
          return false;
        }
      }

      const Statement* bodyStmt = unwrapStatement(subroutine->getBody());
      if (!bodyStmt || bodyStmt->kind != slang::ast::StatementKind::Return) {
        return false;
      }
      const auto& returnStmt = bodyStmt->as<slang::ast::ReturnStatement>();
      if (!returnStmt.expr) {
        return false; // LCOV_EXCL_LINE
      }

      auto formalArgs = subroutine->getArguments();
      auto callArgs = callExpr.arguments();
      if (formalArgs.size() != callArgs.size()) {
        return false; // LCOV_EXCL_LINE
      }

      auto materializeArgumentExpressionNet =
        [&](const slang::ast::FormalArgumentSymbol& formalArg,
            const Expression& callArg,
            SNLNet*& argumentNet) -> bool {
        argumentNet = nullptr;
        const auto& canonicalArgType = formalArg.getType().getCanonicalType();
        if (!canonicalArgType.isIntegral()) {
          return false;
        }
        const auto argBitWidth = canonicalArgType.getBitWidth();
        const auto argWidth = static_cast<size_t>(argBitWidth);

        std::vector<SNLBitNet*> argumentBits;
        if (!resolveExpressionBits(design, callArg, argWidth, argumentBits) ||
            argumentBits.size() != argWidth) {
          return false;
        }

        const auto argumentSourceRange = getSourceRange(callArg);
        if (argWidth == 1) {
          auto* scalarArgumentNet = SNLScalarNet::create(design);
          annotateSourceInfo(scalarArgumentNet, argumentSourceRange);
          argumentNet = scalarArgumentNet;
          if (argumentBits.front() != scalarArgumentNet) {
            createAssignInstance(
              design,
              argumentBits.front(),
              scalarArgumentNet,
              argumentSourceRange);
          }
          return true;
        }

        auto* busArgumentNet =
          SNLBusNet::create(design, static_cast<NLID::Bit>(argWidth - 1), 0);
        annotateSourceInfo(busArgumentNet, argumentSourceRange);
        auto busArgumentBits = collectBits(busArgumentNet);
        if (busArgumentBits.size() != argWidth) {
          return false; // LCOV_EXCL_LINE
        }
        for (size_t i = 0; i < argWidth; ++i) {
          if (argumentBits[i] != busArgumentBits[i]) {
            createAssignInstance(
              design,
              argumentBits[i],
              busArgumentBits[i],
              argumentSourceRange);
          }
        }
        argumentNet = busArgumentNet;
        return true;
      };

      std::unordered_map<const Symbol*, SNLNet*> argumentNets;
      argumentNets.reserve(formalArgs.size());
      for (size_t i = 0; i < formalArgs.size(); ++i) {
        const auto* formalArg = formalArgs[i];
        const auto* callArg = callArgs[i];
        if (!formalArg || !callArg ||
            formalArg->direction != ArgumentDirection::In) {
          return false;
        }
        auto* argumentNet = resolveExpressionNet(design, *callArg);
        if (!argumentNet &&
            !materializeArgumentExpressionNet(*formalArg, *callArg, argumentNet)) {
          return false;
        }
        argumentNets.emplace(formalArg, argumentNet);
      }

      activeFunctionArgumentNets_.push_back(std::move(argumentNets));
      activeInlinedCallSubroutines_.push_back(subroutine);
      const auto guard = slang::ScopeGuard([&]() {
        activeInlinedCallSubroutines_.pop_back();
        activeFunctionArgumentNets_.pop_back();
      });

      return resolveExpressionBits(design, *returnStmt.expr, targetWidth, bits) &&
             bits.size() == targetWidth;
    }

    bool resolveSimpleProceduralReturnFunctionCallBits(
      SNLDesign* design,
      const slang::ast::CallExpression& callExpr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      bits.clear();
      if (callExpr.isSystemCall() || callExpr.subroutine.index() != 0) {
        return false;
      }

      const auto* subroutine = std::get<0>(callExpr.subroutine);
      if (!subroutine || subroutine->subroutineKind != slang::ast::SubroutineKind::Function ||
          subroutine->hasOutputArgs()) {
        return false;
      }
      for (auto* activeSubroutine : activeInlinedCallSubroutines_) {
        if (activeSubroutine == subroutine) {
          return false;
        }
      }

      const Statement* bodyStmt = unwrapStatement(subroutine->getBody());
      if (!bodyStmt) {
        return false; // LCOV_EXCL_LINE
      }
      // unwrapStatement() above already strips nested Block statements.
      // LCOV_EXCL_START
      if (bodyStmt->kind == slang::ast::StatementKind::Block) {
        bodyStmt = unwrapStatement(bodyStmt->as<slang::ast::BlockStatement>().body);
        if (!bodyStmt) {
          return false;
        }
      }
      // LCOV_EXCL_STOP
      if (bodyStmt->kind != slang::ast::StatementKind::List) {
        return false;
      }

      const auto& stmtList = bodyStmt->as<slang::ast::StatementList>().list;
      if (stmtList.empty()) {
        return false;
      }

      size_t returnStmtIndex = stmtList.size();
      const Statement* returnStmt = nullptr;
      while (returnStmtIndex > 0) {
        const auto* candidate = stmtList[returnStmtIndex - 1];
        if (!candidate) {
          // LCOV_EXCL_START
          --returnStmtIndex;
          continue;
          // LCOV_EXCL_STOP
        }
        returnStmt = unwrapStatement(*candidate);
        break;
      }
      if (!returnStmt ||
          returnStmt->kind != slang::ast::StatementKind::Return) {
        return false;
      }
      const auto& returnStatement = returnStmt->as<slang::ast::ReturnStatement>();
      if (!returnStatement.expr) {
        return false; // LCOV_EXCL_LINE
      }
      if (!returnStmtIndex) {
        return false; // LCOV_EXCL_LINE
      }
      --returnStmtIndex;

      const Expression* trackedLhsExpr = nullptr;
      const auto* strippedReturnExpr = stripConversions(*returnStatement.expr);
      if (!strippedReturnExpr) {
        return false; // LCOV_EXCL_LINE
      }
      if (slang::ast::ValueExpressionBase::isKind(strippedReturnExpr->kind) ||
          strippedReturnExpr->kind == slang::ast::ExpressionKind::MemberAccess) {
        trackedLhsExpr = strippedReturnExpr;
      } else if (strippedReturnExpr->kind == slang::ast::ExpressionKind::RangeSelect) {
        trackedLhsExpr = stripConversions(
          strippedReturnExpr->as<slang::ast::RangeSelectExpression>().value());
      } else if (strippedReturnExpr->kind == slang::ast::ExpressionKind::ElementSelect) {
        trackedLhsExpr = stripConversions(
          strippedReturnExpr->as<slang::ast::ElementSelectExpression>().value());
      }
      if (!trackedLhsExpr ||
          (!slang::ast::ValueExpressionBase::isKind(trackedLhsExpr->kind) &&
           trackedLhsExpr->kind != slang::ast::ExpressionKind::MemberAccess)) {
        return false;
      }

      auto formalArgs = subroutine->getArguments();
      auto callArgs = callExpr.arguments();
      if (formalArgs.size() != callArgs.size()) {
        return false; // LCOV_EXCL_LINE
      }

      auto materializeArgumentExpressionNet =
        [&](const slang::ast::FormalArgumentSymbol& formalArg,
            const Expression& callArg,
            SNLNet*& argumentNet) -> bool {
        argumentNet = nullptr;
        const auto& canonicalArgType = formalArg.getType().getCanonicalType();
        if (!canonicalArgType.isIntegral()) {
          return false;
        }
        const auto argBitWidth = canonicalArgType.getBitWidth();
        if (argBitWidth <= 0) {
          return false; // LCOV_EXCL_LINE
        }
        const auto argWidth = static_cast<size_t>(argBitWidth);

        std::vector<SNLBitNet*> argumentBits;
        if (!resolveExpressionBits(design, callArg, argWidth, argumentBits) ||
            argumentBits.size() != argWidth) {
          return false;
        }

        const auto argumentSourceRange = getSourceRange(callArg);
        if (argWidth == 1) {
          auto* scalarArgumentNet = SNLScalarNet::create(design);
          annotateSourceInfo(scalarArgumentNet, argumentSourceRange);
          argumentNet = scalarArgumentNet;
          if (argumentBits.front() != scalarArgumentNet) {
            createAssignInstance(
              design,
              argumentBits.front(),
              scalarArgumentNet,
              argumentSourceRange);
          }
          return true;
        }

        auto* busArgumentNet =
          SNLBusNet::create(design, static_cast<NLID::Bit>(argWidth - 1), 0);
        annotateSourceInfo(busArgumentNet, argumentSourceRange);
        auto busArgumentBits = collectBits(busArgumentNet);
        if (busArgumentBits.size() != argWidth) {
          return false; // LCOV_EXCL_LINE
        }
        for (size_t i = 0; i < argWidth; ++i) {
          if (argumentBits[i] != busArgumentBits[i]) {
            createAssignInstance(
              design,
              argumentBits[i],
              busArgumentBits[i],
              argumentSourceRange);
          }
        }
        argumentNet = busArgumentNet;
        return true;
      };

      std::unordered_map<const Symbol*, SNLNet*> argumentNets;
      argumentNets.reserve(formalArgs.size());
      for (size_t i = 0; i < formalArgs.size(); ++i) {
        const auto* formalArg = formalArgs[i];
        const auto* callArg = callArgs[i];
        if (!formalArg || !callArg ||
            formalArg->direction != ArgumentDirection::In) {
          return false;
        }
        auto* argumentNet = resolveExpressionNet(design, *callArg);
        if (!argumentNet &&
            !materializeArgumentExpressionNet(*formalArg, *callArg, argumentNet)) {
          return false;
        }
        argumentNets.emplace(formalArg, argumentNet);
      }

      activeFunctionArgumentNets_.push_back(std::move(argumentNets));
      activeInlinedCallSubroutines_.push_back(subroutine);
      const auto guard = slang::ScopeGuard([&]() {
        activeInlinedCallSubroutines_.pop_back();
        activeFunctionArgumentNets_.pop_back();
      });

      auto* lhsNet = resolveExpressionNet(design, *trackedLhsExpr);
      if (!lhsNet) {
        return false;
      }
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> dataBits = lhsBits;
      size_t tempIndex = 0;
      CombinationalSubtreeSummaryCache subtreeSummaryCache;
      std::string lowerFailureReason;
      for (size_t i = 0; i < returnStmtIndex; ++i) {
        const auto* item = stmtList[i];
        if (!item) {
          continue; // LCOV_EXCL_LINE
        }
        if (!applyCombinationalStatementForLhs(
              design,
              *item,
              *trackedLhsExpr,
              lhsBits,
              dataBits,
              tempIndex,
              lowerFailureReason,
              nullptr,
              &subtreeSummaryCache)) {
          return false;
        }
      }

      if (dataBits.size() != lhsBits.size()) {
        return false; // LCOV_EXCL_LINE
      }
      auto callSourceRange = getSourceRange(callExpr);
      for (size_t i = 0; i < lhsBits.size(); ++i) {
        if (dataBits[i] == lhsBits[i]) {
          continue;
        }
        createAssignInstance(design, dataBits[i], lhsBits[i], callSourceRange);
      }

      return resolveExpressionBits(design, *returnStatement.expr, targetWidth, bits) &&
             bits.size() == targetWidth;
    }

    bool resolveSimpleCaseReturnFunctionCallBits(
      SNLDesign* design,
      const slang::ast::CallExpression& callExpr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      bits.clear();
      if (callExpr.isSystemCall() || callExpr.subroutine.index() != 0 || !targetWidth) {
        return false;
      }

      const auto* subroutine = std::get<0>(callExpr.subroutine);
      if (!subroutine || subroutine->subroutineKind != slang::ast::SubroutineKind::Function ||
          subroutine->hasOutputArgs()) {
        return false;
      }

      auto formalArgs = subroutine->getArguments();
      auto callArgs = callExpr.arguments();
      if (formalArgs.size() != 1 || !formalArgs.front() ||
          callArgs.size() != 1 || !callArgs.front() ||
          formalArgs.front()->direction != ArgumentDirection::In) {
        return false;
      }

      const Statement* bodyStmt = unwrapStatement(subroutine->getBody());
      if (!bodyStmt || bodyStmt->kind != slang::ast::StatementKind::Case) {
        return false;
      }

      const auto& caseStmt = bodyStmt->as<slang::ast::CaseStatement>();
      if (caseStmt.condition != slang::ast::CaseStatementCondition::Normal) {
        return false;
      }

      const auto* caseExpr = stripConversions(caseStmt.expr);
      if (!caseExpr || !slang::ast::ValueExpressionBase::isKind(caseExpr->kind) ||
          &caseExpr->as<slang::ast::ValueExpressionBase>().symbol != formalArgs.front()) {
        return false;
      }

      if (!caseStmt.defaultCase) {
        return false;
      }
      const Expression* defaultReturnExpr = nullptr;
      if (!extractFunctionReturnExpression(*caseStmt.defaultCase, defaultReturnExpr)) {
        return false;
      }
      if (!resolveExpressionBits(design, *defaultReturnExpr, targetWidth, bits) ||
          bits.size() != targetWidth) {
        return false;
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      auto mergeOr = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (leftBit == const1 || rightBit == const1) {
          return const1; // LCOV_EXCL_LINE
        }
        if (leftBit == const0) {
          return rightBit;
        }
        if (rightBit == const0 || leftBit == rightBit) {
          return leftBit; // LCOV_EXCL_LINE
        }
        auto* orNet = SNLScalarNet::create(design);
        annotateSourceInfo(orNet, getSourceRange(callExpr));
        return getSingleBitNet(createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::Or),
          leftBit,
          rightBit,
          orNet,
          getSourceRange(callExpr)));
      };

      for (const auto& item : caseStmt.items) {
        if (item.expressions.empty() || !item.stmt) {
          return false; // LCOV_EXCL_LINE
        }

        const Expression* itemReturnExpr = nullptr;
        if (!extractFunctionReturnExpression(*item.stmt, itemReturnExpr)) {
          return false;
        }
        std::vector<SNLBitNet*> itemBits;
        if (!resolveExpressionBits(design, *itemReturnExpr, targetWidth, itemBits) ||
            itemBits.size() != targetWidth) {
          return false;
        }

        SNLBitNet* itemMatchBit = const0;
        for (const auto* itemExpr : item.expressions) {
          if (!itemExpr) {
            return false; // LCOV_EXCL_LINE
          }
          auto* eqNet = SNLScalarNet::create(design);
          annotateSourceInfo(eqNet, getSourceRange(*itemExpr));
          if (!createEqualityAssign(
                design,
                eqNet,
                *callArgs.front(),
                *itemExpr,
                getSourceRange(*itemExpr))) {
            return false;
          }
          auto* eqBit = getSingleBitNet(eqNet);
          itemMatchBit = mergeOr(itemMatchBit, eqBit);
          if (!itemMatchBit) {
            return false; // LCOV_EXCL_LINE
          }
        }

        if (itemMatchBit == const0) {
          continue; // LCOV_EXCL_LINE
        }
        for (size_t i = 0; i < targetWidth; ++i) {
          // LCOV_EXCL_START
          if (itemMatchBit == const1) {
            bits[i] = itemBits[i];
            continue;
          }
          // LCOV_EXCL_STOP
          if (bits[i] == itemBits[i]) {
            continue;
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, getSourceRange(callExpr));
          createMux2Instance(
            design,
            itemMatchBit,
            bits[i],
            itemBits[i],
            outBit,
            getSourceRange(callExpr));
          bits[i] = outBit;
        }
      }

      return bits.size() == targetWidth;
    }

    bool resolveExpressionBits(
      SNLDesign* design,
      const Expression& expr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      bits.clear();
      if (!targetWidth) {
        return false; // LCOV_EXCL_LINE
      }

      // Preserve signedness semantics of explicit casts (e.g. $signed(...))
      // when extending to a wider destination width.
      if (expr.kind == slang::ast::ExpressionKind::Conversion) {
        const auto& conversionExpr = expr.as<slang::ast::ConversionExpression>();
        const auto& targetType = expr.type->getCanonicalType();
        if (targetType.isIntegral()) {
          auto operandWidth = getIntegralExpressionBitWidth(conversionExpr.operand());
          if (operandWidth && *operandWidth) {
            std::vector<SNLBitNet*> operandBits;
            if (!resolveExpressionBits(design, conversionExpr.operand(), *operandWidth, operandBits)) {
              return false;
            }
            auto* fillBit = static_cast<SNLBitNet*>(getConstNet(design, false));
            if (targetType.isSigned() && !operandBits.empty()) {
              fillBit = operandBits.back();
            }
            bits = std::move(operandBits);
            resizeBitsToWidth(bits, targetWidth, fillBit);
            return true;
          }
        }
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }

      if (auto resolved = resolveUnbasedOrStructuredPatternBits(design, *stripped, targetWidth, bits)) {
        return *resolved;
      }

      uint64_t constantValue = 0;
      if (getConstantUnsigned(*stripped, constantValue)) {
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          const bool one = i < 64 && ((constantValue >> i) & 1ULL);
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        return true;
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      const Symbol* evalSymbol = getConstantEvalSymbol(*stripped);
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        if (evaluatedConstant && evaluatedConstant.isInteger()) {
          constant = &evaluatedConstant;
        }
      }
      slang::ConstantValue convertedConstant;
      if (constant && !constant->isInteger()) {
        convertedConstant = constant->convertToInt();
        if (convertedConstant && convertedConstant.isInteger()) {
          constant = &convertedConstant;
        }
      }
      if (constant && constant->isInteger()) {
        const auto& intValue = constant->integer();
        if (intValue.hasUnknown()) {
          return false;
        }
        const auto integerWidth = static_cast<size_t>(intValue.getBitWidth());
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          bool one = false;
          if (i < integerWidth) {
            const auto bit = intValue[static_cast<int32_t>(i)];
            if (bit.isUnknown()) {
              return false; // LCOV_EXCL_LINE
            }
            one = static_cast<bool>(bit);
          }
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        return true;
      }

      int64_t signedValue = 0;
      if (getConstantInt64(expr, signedValue)) {
        const auto unsignedValue = static_cast<uint64_t>(signedValue);
        const bool fillBit = signedValue < 0;
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          const bool one = i < 64 ? ((unsignedValue >> i) & 1ULL) : fillBit;
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        return true;
      }

      if (tryResolveInferredMemoryReadBits(design, *stripped, targetWidth, bits)) {
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Call) {
        const auto& callExpr = stripped->as<slang::ast::CallExpression>();
        const auto subroutineName = callExpr.getSubroutineName();
        const bool isSignedCast =
          (subroutineName == "$signed" || subroutineName == "signed");
        const bool isUnsignedCast =
          (subroutineName == "$unsigned" || subroutineName == "unsigned");
        if (isSignedCast || isUnsignedCast) {
          auto args = callExpr.arguments();
          if (args.size() != 1 || !args[0]) {
            return false; // LCOV_EXCL_LINE
          }
          auto argWidth = getIntegralExpressionBitWidth(*args[0]);
          if (!argWidth || !*argWidth) {
            return false; // LCOV_EXCL_LINE
          }
          std::vector<SNLBitNet*> argBits;
          if (!resolveExpressionBits(design, *args[0], *argWidth, argBits)) {
            return false;
          }
          auto* fillBit = static_cast<SNLBitNet*>(getConstNet(design, false));
          if (isSignedCast && !argBits.empty()) {
            fillBit = argBits.back();
          }
          bits = std::move(argBits);
          resizeBitsToWidth(bits, targetWidth, fillBit);
          return true;
        }

        SNLBitNet* callBit = nullptr;
        if (resolveSimpleCaseInsideFunctionCallBit(
              design,
              callExpr,
              callBit,
              getSourceRange(*stripped))) {
          auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
          bits.assign(targetWidth, const0);
          bits.front() = callBit;
          return true;
        }

        if (resolveRangeCheckFunctionCallBit(
              design,
              callExpr,
              callBit,
              getSourceRange(*stripped))) {
          auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
          bits.assign(targetWidth, const0);
          bits.front() = callBit;
          return true;
        }

        if (resolveSimpleReturnFunctionCallBits(
              design,
              callExpr,
              targetWidth,
              bits)) {
          return true;
        }

        if (resolveSimpleProceduralReturnFunctionCallBits(
              design,
              callExpr,
              targetWidth,
              bits)) {
          return true;
        }

        if (resolveSimpleCaseReturnFunctionCallBits(
              design,
              callExpr,
              targetWidth,
              bits)) {
          return true;
        }
      } // LCOV_EXCL_LINE

      if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
        auto unarySourceRange = getSourceRange(*stripped);
        auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
        auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

        if (unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(design, unaryExpr.operand(), targetWidth, operandBits) ||
              operandBits.size() != targetWidth) {
            return false;
          }
          bits.clear();
          bits.reserve(targetWidth);
          for (auto* operandBit : operandBits) {
            if (operandBit == const0) {
              bits.push_back(const1);
              continue;
            }
            if (operandBit == const1) {
              bits.push_back(const0);
              continue;
            }
            auto* notBit = SNLScalarNet::create(design);
            annotateSourceInfo(notBit, unarySourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  operandBit,
                  notBit,
                  unarySourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            bits.push_back(notBit);
          }
          return true;
        }

        if (unaryExpr.op == slang::ast::UnaryOperator::Plus ||
            unaryExpr.op == slang::ast::UnaryOperator::Minus) {
          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(design, unaryExpr.operand(), targetWidth, operandBits) ||
              operandBits.size() != targetWidth) {
            return false;
          }
          if (unaryExpr.op == slang::ast::UnaryOperator::Plus) {
            bits = std::move(operandBits);
            return true;
          }

          std::vector<SNLBitNet*> invertedBits;
          invertedBits.reserve(targetWidth);
          for (auto* operandBit : operandBits) {
            if (operandBit == const0) {
              invertedBits.push_back(const1);
              continue;
            }
            if (operandBit == const1) {
              invertedBits.push_back(const0);
              continue;
            }
            auto* notBit = SNLScalarNet::create(design);
            annotateSourceInfo(notBit, unarySourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  operandBit,
                  notBit,
                  unarySourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            invertedBits.push_back(notBit);
          }

          std::vector<SNLBitNet*> oneBits(targetWidth, const0);
          oneBits.front() = const1;
          return addBitVectors(
            design,
            invertedBits,
            oneBits,
            bits,
            unarySourceRange);
        }

        if (unaryExpr.op == slang::ast::UnaryOperator::BitwiseOr ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseAnd ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseXor ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNor ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNand ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseXnor) {
          auto operandWidth = getIntegralExpressionBitWidth(unaryExpr.operand());
          if (!operandWidth || !*operandWidth) {
            return false; // LCOV_EXCL_LINE
          }
          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(
                design,
                unaryExpr.operand(),
                static_cast<size_t>(*operandWidth),
                operandBits) ||
              operandBits.empty()) {
            return false;
          }

          SNLBitNet* reducedBit = nullptr;
          const bool isOrReduction =
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseOr ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNor;
          const bool isAndReduction =
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseAnd ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNand;

          if (isOrReduction || isAndReduction) {
            reducedBit = isOrReduction ? const0 : const1;
            for (auto* operandBit : operandBits) {
              if (operandBit == const0) {
                if (isOrReduction) {
                  continue;
                }
                reducedBit = const0;
                break;
              }
              if (operandBit == const1) {
                if (isAndReduction) {
                  continue;
                }
                reducedBit = const1;
                break;
              }
              if (reducedBit == const0 && isOrReduction) {
                reducedBit = operandBit;
                continue;
              }
              if (reducedBit == const1 && isAndReduction) {
                reducedBit = operandBit;
                continue;
              }
              auto* nextBit = SNLScalarNet::create(design);
              annotateSourceInfo(nextBit, unarySourceRange);
              if (!createBinaryGate(
                    design,
                    isOrReduction
                      ? NLDB0::GateType(NLDB0::GateType::Or)
                      : NLDB0::GateType(NLDB0::GateType::And),
                    reducedBit,
                    operandBit,
                    nextBit,
                    unarySourceRange)) {
                return false; // LCOV_EXCL_LINE
              }
              reducedBit = nextBit;
            }
          } else {
            reducedBit = const0;
            for (auto* operandBit : operandBits) {
              if (operandBit == const0) {
                continue;
              }
              if (operandBit == const1) {
                if (reducedBit == const0) {
                  reducedBit = const1;
                } else if (reducedBit == const1) {
                  reducedBit = const0;
                } else {
                  auto* notBit = SNLScalarNet::create(design);
                  annotateSourceInfo(notBit, unarySourceRange);
                  if (!createUnaryGate(
                        design,
                        NLDB0::GateType(NLDB0::GateType::Not),
                        reducedBit,
                        notBit,
                        unarySourceRange)) {
                    return false; // LCOV_EXCL_LINE
                  }
                  reducedBit = notBit;
                }
                continue;
              }
              if (reducedBit == const0) {
                reducedBit = operandBit;
                continue;
              }
              if (reducedBit == const1) {
                auto* notBit = SNLScalarNet::create(design);
                annotateSourceInfo(notBit, unarySourceRange);
                if (!createUnaryGate(
                      design,
                      NLDB0::GateType(NLDB0::GateType::Not),
                      operandBit,
                      notBit,
                      unarySourceRange)) {
                  return false; // LCOV_EXCL_LINE
                }
                reducedBit = notBit;
                continue;
              }
              auto* nextBit = SNLScalarNet::create(design);
              annotateSourceInfo(nextBit, unarySourceRange);
              if (!createBinaryGate(
                    design,
                    NLDB0::GateType(NLDB0::GateType::Xor),
                    reducedBit,
                    operandBit,
                    nextBit,
                    unarySourceRange)) {
                return false; // LCOV_EXCL_LINE
              }
              reducedBit = nextBit;
            }
          }

          const bool invertReduction =
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNor ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNand ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseXnor;
          if (invertReduction) {
            if (reducedBit == const0) {
              reducedBit = const1;
            } else if (reducedBit == const1) {
              reducedBit = const0;
            } else {
              auto* notBit = SNLScalarNet::create(design);
              annotateSourceInfo(notBit, unarySourceRange);
              if (!createUnaryGate(
                    design,
                    NLDB0::GateType(NLDB0::GateType::Not),
                    reducedBit,
                    notBit,
                    unarySourceRange)) {
                return false; // LCOV_EXCL_LINE
              }
              reducedBit = notBit;
            }
          }

          bits.clear();
          bits.push_back(reducedBit);
          resizeBitsToWidth(bits, targetWidth, const0);
          return true;
        }

        if (unaryExpr.op == slang::ast::UnaryOperator::LogicalNot) {
          auto operandWidth = getIntegralExpressionBitWidth(unaryExpr.operand());
          if (!operandWidth || !*operandWidth) {
            return false; // LCOV_EXCL_LINE
          }
          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(
                design,
                unaryExpr.operand(),
                static_cast<size_t>(*operandWidth),
                operandBits) ||
              operandBits.empty()) {
            return false;
          }

          SNLBitNet* reductionOr = const0;
          for (auto* operandBit : operandBits) {
            if (operandBit == const1) {
              reductionOr = const1;
              break;
            }
            if (operandBit == const0) {
              continue;
            }
            if (reductionOr == const0) {
              reductionOr = operandBit;
              continue;
            }
            auto* nextOr = SNLScalarNet::create(design);
            annotateSourceInfo(nextOr, unarySourceRange);
            if (!createBinaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Or),
                  reductionOr,
                  operandBit,
                  nextOr,
                  unarySourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            reductionOr = nextOr;
          }

          bits.clear();
          bits.reserve(1);
          if (reductionOr == const0) {
            bits.push_back(const1);
          } else if (reductionOr == const1) {
            bits.push_back(const0);
          } else {
            auto* notBit = SNLScalarNet::create(design);
            annotateSourceInfo(notBit, unarySourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  reductionOr,
                  notBit,
                  unarySourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            bits.push_back(notBit);
          }
          resizeBitsToWidth(bits, targetWidth, const0);
          return true;
        }
      }

      if (stripped->kind == slang::ast::ExpressionKind::ConditionalOp) {
        const auto& conditionalExpr = stripped->as<slang::ast::ConditionalExpression>();
        if (const auto* knownSide = conditionalExpr.knownSide()) {
          return resolveExpressionBits(design, *knownSide, targetWidth, bits);
        }
        if (conditionalExpr.conditions.size() != 1 || conditionalExpr.conditions.front().pattern) {
          return false;
        }

        auto* selectBit = resolveCombinationalConditionNet(
          design,
          *conditionalExpr.conditions.front().expr);
        if (!selectBit) {
          return false;
        }

        auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
        auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
        if (selectBit == const1) {
          return resolveExpressionBits(design, conditionalExpr.left(), targetWidth, bits);
        }
        if (selectBit == const0) {
          return resolveExpressionBits(design, conditionalExpr.right(), targetWidth, bits);
        }

        std::vector<SNLBitNet*> leftBits;
        std::vector<SNLBitNet*> rightBits;
        if (!resolveExpressionBits(design, conditionalExpr.left(), targetWidth, leftBits) ||
            !resolveExpressionBits(design, conditionalExpr.right(), targetWidth, rightBits) ||
            leftBits.size() != targetWidth ||
            rightBits.size() != targetWidth) {
          return false;
        }

        if (leftBits == rightBits) {
          bits = leftBits;
          return true;
        }

        if (targetWidth > 1) {
          return createMux2Instance(
            design,
            selectBit,
            rightBits,
            leftBits,
            bits,
            getSourceRange(*stripped));
        }

        bits.clear();
        bits.reserve(targetWidth);
        auto conditionalSourceRange = getSourceRange(*stripped);
        for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
          if (leftBits[bitIndex] == rightBits[bitIndex]) {
            // LCOV_EXCL_START
            bits.push_back(leftBits[bitIndex]);
            continue;
            // LCOV_EXCL_STOP
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, conditionalSourceRange);
          createMux2Instance(
            design,
            selectBit,
            rightBits[bitIndex],
            leftBits[bitIndex],
            outBit,
            conditionalSourceRange);
          bits.push_back(outBit);
        }
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Inside) {
        const auto& insideExpr = stripped->as<slang::ast::InsideExpression>();
        auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
        auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
        auto insideSourceRange = getSourceRange(*stripped);
        SNLBitNet* insideBit = const0;
        for (const auto* rangeExpr : insideExpr.rangeList()) {
          if (!rangeExpr) {
            continue; // LCOV_EXCL_LINE
          }
          SNLBitNet* eqBit = nullptr;
          if (!buildCaseInsideMatchBit(
                design,
                insideExpr.left(),
                *rangeExpr,
                eqBit,
                insideSourceRange) ||
              !eqBit) {
            return false;
          }

          if (insideBit == const0) {
            insideBit = eqBit;
            continue;
          }

          auto* nextOr = SNLScalarNet::create(design);
          annotateSourceInfo(nextOr, insideSourceRange);
          if (!createBinaryGate(
                design,
                NLDB0::GateType(NLDB0::GateType::Or),
                insideBit,
                eqBit,
                nextOr,
                insideSourceRange)) {
            return false; // LCOV_EXCL_LINE
          }
          insideBit = nextOr;
        }

        bits.assign(targetWidth, const0);
        bits.front() = insideBit;
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
        if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd ||
            binaryExpr.op == slang::ast::BinaryOperator::LogicalOr) {
          auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
          auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
          auto* lhsBit = resolveCombinationalConditionNet(design, binaryExpr.left());
          if (!lhsBit) {
            return false;
          }

          SNLBitNet* logicBit = nullptr;
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd && lhsBit == const0) { logicBit = const0; }
          else if (binaryExpr.op == slang::ast::BinaryOperator::LogicalOr && lhsBit == const1) { logicBit = const1; }
          else {
            auto* rhsBit = resolveCombinationalConditionNet(design, binaryExpr.right());
            if (!rhsBit) {
              return false;
            }

            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd) {
              if (rhsBit == const0) { logicBit = const0; }
              else if (lhsBit == const1) { logicBit = rhsBit; }
              else if (rhsBit == const1 || lhsBit == rhsBit) { logicBit = lhsBit; }
            } else {
              if (rhsBit == const1) { logicBit = const1; }
              else if (lhsBit == const0) { logicBit = rhsBit; }
              else if (rhsBit == const0 || lhsBit == rhsBit) { logicBit = lhsBit; }
            }

            if (!logicBit) {
              auto binarySourceRange = getSourceRange(*stripped);
              auto* outBit = SNLScalarNet::create(design);
              annotateSourceInfo(outBit, binarySourceRange);
              auto gateType = binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd
                ? NLDB0::GateType(NLDB0::GateType::And)
                : NLDB0::GateType(NLDB0::GateType::Or);
              logicBit = getSingleBitNet(createBinaryGate(
                design,
                gateType,
                lhsBit,
                rhsBit,
                outBit,
                binarySourceRange));
              if (!logicBit) {
                return false; // LCOV_EXCL_LINE
              }
            }
          }

          bits.clear();
          bits.push_back(logicBit);
          resizeBitsToWidth(bits, targetWidth, const0);
          return true;
        }
        if (isEqualityBinaryOp(binaryExpr.op) || isInequalityBinaryOp(binaryExpr.op)) {
          auto binarySourceRange = getSourceRange(*stripped);
          reportCaseComparison2StateWarning(binaryExpr.op, binarySourceRange);
          auto* compareBit = SNLScalarNet::create(design);
          annotateSourceInfo(compareBit, binarySourceRange);
          const bool ok = isInequalityBinaryOp(binaryExpr.op)
            ? createInequalityAssign(
                design,
                compareBit,
                binaryExpr.left(),
                binaryExpr.right(),
                binarySourceRange)
            : createEqualityAssign(
                design,
                compareBit,
                binaryExpr.left(),
                binaryExpr.right(),
                binarySourceRange);
          if (!ok) {
            return false;
          }
          bits.clear();
          bits.push_back(compareBit);
          resizeBitsToWidth(
            bits,
            targetWidth,
            static_cast<SNLBitNet*>(getConstNet(design, false)));
          return true;
        }
        if (isRelationalBinaryOp(binaryExpr.op)) {
          auto binarySourceRange = getSourceRange(*stripped);
          auto* compareBit = SNLScalarNet::create(design);
          annotateSourceInfo(compareBit, binarySourceRange);
          if (!createRelationalAssign(
                design,
                compareBit,
                binaryExpr.left(),
                binaryExpr.right(),
                binaryExpr.op,
                binarySourceRange)) {
            return false;
          }
          bits.clear();
          bits.push_back(compareBit);
          resizeBitsToWidth(
            bits,
            targetWidth,
            static_cast<SNLBitNet*>(getConstNet(design, false)));
          return true;
        }
        if (binaryExpr.op == slang::ast::BinaryOperator::Multiply) {
          uint64_t leftConst = 0;
          uint64_t rightConst = 0;
          const bool leftIsConst = getConstantUnsigned(binaryExpr.left(), leftConst);
          const bool rightIsConst = getConstantUnsigned(binaryExpr.right(), rightConst);

          if (leftIsConst && rightIsConst) {
            if (leftConst != 0 &&
                rightConst > std::numeric_limits<uint64_t>::max() / leftConst) {
              return false;
            }
            const uint64_t product = leftConst * rightConst;
            bits.reserve(targetWidth);
            for (size_t i = 0; i < targetWidth; ++i) {
              const bool one = i < 64 && ((product >> i) & 1ULL);
              bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
            }
            return true;
          }

          const bool allowScaledMultiplyLowering =
            targetWidth <= 32 || hasActiveForLoopContext();
          if (!allowScaledMultiplyLowering) {
            // Avoid broad datapath lowering blow-up; keep this fast path for selector-like widths.
          } else {
            uint64_t factor = 0;
            const Expression* scaledExpr = nullptr;
            if (rightIsConst) {
              factor = rightConst;
              scaledExpr = &binaryExpr.left();
            } else if (leftIsConst) {
              factor = leftConst;
              scaledExpr = &binaryExpr.right();
            }

            if (scaledExpr) {
              auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
              if (factor == 0) {
                bits.assign(targetWidth, const0);
                return true;
              }
              if (factor == 1) {
                return resolveExpressionBits(design, *scaledExpr, targetWidth, bits);
              }
              if ((factor & (factor - 1ULL)) == 0ULL) {
                size_t shiftAmount = 0;
                while ((factor >> shiftAmount) > 1ULL) {
                  ++shiftAmount;
                }

                std::vector<SNLBitNet*> scaledBits;
                if (!resolveExpressionBits(design, *scaledExpr, targetWidth, scaledBits) ||
                    scaledBits.size() != targetWidth) {
                  return false;
                }

                bits.assign(targetWidth, const0);
                if (shiftAmount < targetWidth) {
                  for (size_t bit = shiftAmount; bit < targetWidth; ++bit) {
                    bits[bit] = scaledBits[bit - shiftAmount];
                  }
                }
                return true;
              }
            }
          }
        }
        if (binaryExpr.op == slang::ast::BinaryOperator::Divide ||
            binaryExpr.op == slang::ast::BinaryOperator::Mod) {
          int64_t divisorValue = 0;
          if (getConstantInt64(binaryExpr.right(), divisorValue) && divisorValue == 1) {
            if (binaryExpr.op == slang::ast::BinaryOperator::Divide) {
              return resolveExpressionBits(design, binaryExpr.left(), targetWidth, bits);
            }

            bits.assign(
              targetWidth,
              static_cast<SNLBitNet*>(getConstNet(design, false)));
            return true;
          }
        }
        if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
            binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft ||
            binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight ||
            binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftRight) {
          SNLNet* shiftNet = nullptr;
          if (targetWidth == 1) {
            shiftNet = SNLScalarNet::create(design);
          } else {
            shiftNet = SNLBusNet::create(
              design,
              static_cast<NLID::Bit>(targetWidth - 1),
              0);
          }
          annotateSourceInfo(shiftNet, getSourceRange(*stripped));

          bool ok = false;
          std::string shiftFailureReason;
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
              binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft) {
            ok = createLogicalLeftShiftAssign(
              design,
              shiftNet,
              binaryExpr.left(),
              binaryExpr.right(),
              getSourceRange(*stripped));
          } else if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight) {
            ok = createLogicalRightShiftAssign(
              design,
              shiftNet,
              binaryExpr.left(),
              binaryExpr.right(),
              getSourceRange(*stripped),
              &shiftFailureReason);
          } else {
            ok = createArithmeticRightShiftAssign(
              design,
              shiftNet,
              binaryExpr.left(),
              binaryExpr.right(),
              getSourceRange(*stripped),
              &shiftFailureReason);
          }
          if (!ok) {
            return false;
          }

          bits = collectBits(shiftNet);
          return bits.size() == targetWidth;
        }
        if (binaryExpr.op == slang::ast::BinaryOperator::Add ||
            binaryExpr.op == slang::ast::BinaryOperator::Subtract) {
          std::vector<SNLBitNet*> leftBits;
          std::vector<SNLBitNet*> rightBits;
          if (!resolveExpressionBits(design, binaryExpr.left(), targetWidth, leftBits) ||
              !resolveExpressionBits(design, binaryExpr.right(), targetWidth, rightBits)) {
            return false;
          }
          bits.clear();
          bits.reserve(targetWidth);
          auto binarySourceRange = getSourceRange(*stripped);
          if (binaryExpr.op == slang::ast::BinaryOperator::Add) {
            return addBitVectors(design, leftBits, rightBits, bits, binarySourceRange);
          }

          auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
          auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
          std::vector<SNLBitNet*> invertedRightBits;
          invertedRightBits.reserve(rightBits.size());
          for (auto* rightBit : rightBits) {
            if (rightBit == const0) {
              invertedRightBits.push_back(const1);
              continue;
            }
            if (rightBit == const1) {
              invertedRightBits.push_back(const0);
              continue;
            }
            auto* invertedBit = SNLScalarNet::create(design);
            annotateSourceInfo(invertedBit, binarySourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  rightBit,
                  invertedBit,
                  binarySourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            invertedRightBits.push_back(invertedBit);
          }

          auto* carry = const1;
          for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
            auto* diffBit = SNLScalarNet::create(design);
            auto* carryOut = SNLScalarNet::create(design);
            annotateSourceInfo(diffBit, binarySourceRange);
            annotateSourceInfo(carryOut, binarySourceRange);
            if (!createFAInstance(
                  design,
                  leftBits[bitIndex],
                  invertedRightBits[bitIndex],
                  carry,
                  diffBit,
                  carryOut,
                  binarySourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            bits.push_back(diffBit);
            carry = carryOut;
          }
          return true;
        }

        auto gateType = gateTypeFromBinary(binaryExpr.op);
        if (gateType) {
          std::vector<SNLBitNet*> leftBits;
          std::vector<SNLBitNet*> rightBits;
          if (!resolveExpressionBits(design, binaryExpr.left(), targetWidth, leftBits) ||
              !resolveExpressionBits(design, binaryExpr.right(), targetWidth, rightBits)) {
            return false;
          }
          bits.clear();
          bits.reserve(targetWidth);
          auto binarySourceRange = getSourceRange(*stripped);
          for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
            auto* outBit = getSingleBitNet(createBinaryGate(
              design,
              *gateType,
              leftBits[bitIndex],
              rightBits[bitIndex],
              nullptr,
              binarySourceRange));
            if (!outBit) {
              return false; // LCOV_EXCL_LINE
            }
            bits.push_back(outBit);
          }
          return true;
        }
      } // LCOV_EXCL_LINE

      if (stripped->kind == slang::ast::ExpressionKind::Replication) {
        const auto& replicationExpr = stripped->as<slang::ast::ReplicationExpression>();
        int32_t repeatCountSigned = 0;
        if (!getConstantInt32(replicationExpr.count(), repeatCountSigned) || repeatCountSigned < 0) {
          return false;
        }
        const auto repeatCount = static_cast<size_t>(repeatCountSigned);

        size_t concatWidth = 0;
        if (auto concatExprWidth = getIntegralExpressionBitWidth(replicationExpr.concat())) {
          concatWidth = *concatExprWidth;
        }

        if (repeatCount > (std::numeric_limits<size_t>::max() / concatWidth)) {
          return false; // LCOV_EXCL_LINE
        }

        std::vector<SNLBitNet*> concatBits;
        if (!resolveExpressionBits(design, replicationExpr.concat(), concatWidth, concatBits) ||
            concatBits.size() != concatWidth) {
          return false;
        }

        bits.clear();
        bits.reserve(concatWidth * repeatCount);
        for (size_t i = 0; i < repeatCount; ++i) {
          bits.insert(bits.end(), concatBits.begin(), concatBits.end());
        }
        resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Concatenation) {
        const auto& concatExpr = stripped->as<slang::ast::ConcatenationExpression>();
        auto operands = concatExpr.operands();
        if (operands.empty()) {
          return false; // LCOV_EXCL_LINE
        }
        bits.clear();
        for (auto it = operands.rbegin(); it != operands.rend(); ++it) {
          auto* operand = *it;
          if (!operand) {
            return false; // LCOV_EXCL_LINE
          }
          const auto* strippedOperand = stripConversions(*operand);
          size_t operandWidthBits = 0;
          if (auto operandWidth = getIntegralExpressionBitWidth(*operand)) {
            operandWidthBits = *operandWidth;
          } else {
            if (strippedOperand &&
                strippedOperand->kind == slang::ast::ExpressionKind::Replication) {
              const auto& replicationOperand =
                strippedOperand->as<slang::ast::ReplicationExpression>();
              int32_t repeatCountSigned = 0;
              if (getConstantInt32(replicationOperand.count(), repeatCountSigned) &&
                  repeatCountSigned == 0) {
                continue;
              }
            }
            const auto& operandCanonical = operand->type->getCanonicalType();
            if (!operandCanonical.isIntegral()) {
              return false; // LCOV_EXCL_LINE
            }
            const auto bitWidth = operandCanonical.getBitWidth();
            if (bitWidth < 0) {
              return false; // LCOV_EXCL_LINE
            }
            if (bitWidth == 0) {
              continue; // LCOV_EXCL_LINE
            }
            operandWidthBits = static_cast<size_t>(bitWidth);
          }
          if (!operandWidthBits) {
            continue; // LCOV_EXCL_LINE
          }
          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(design, *operand, operandWidthBits, operandBits)) {
            return false;
          }
          bits.insert(bits.end(), operandBits.begin(), operandBits.end());
        }
        resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Streaming) {
        const auto& streamingExpr =
          stripped->as<slang::ast::StreamingConcatenationExpression>();
        if (!streamingExpr.isFixedSize()) {
          return false;
        }

        const auto bitstreamWidth = streamingExpr.getBitstreamWidth();
        if (bitstreamWidth > std::numeric_limits<size_t>::max()) {
          return false; // LCOV_EXCL_LINE
        }

        auto streams = streamingExpr.streams();
        std::vector<SNLBitNet*> streamBits;
        streamBits.reserve(static_cast<size_t>(bitstreamWidth));
        for (size_t streamIndex = streams.size(); streamIndex > 0; --streamIndex) {
          const auto& stream = streams[streamIndex - 1];

          const Expression* operandExpr = stream.operand.get();
          if (stream.withExpr) {
            operandExpr = stream.withExpr;
          }

          size_t operandWidthBits = 0;
          if (stream.constantWithWidth && stream.withExpr) {
            if (*stream.constantWithWidth <= 0) {
              continue; // LCOV_EXCL_LINE
            }
            operandWidthBits = static_cast<size_t>(*stream.constantWithWidth);
          } else if (auto operandWidth = getIntegralExpressionBitWidth(*operandExpr)) {
            operandWidthBits = *operandWidth;
          }
          if (!operandWidthBits) {
            return false;
          }

          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(design, *operandExpr, operandWidthBits, operandBits) ||
              operandBits.size() != operandWidthBits) {
            return false;
          }
          streamBits.insert(streamBits.end(), operandBits.begin(), operandBits.end());
        }
        resizeBitsToWidth(
          streamBits,
          static_cast<size_t>(bitstreamWidth),
          static_cast<SNLBitNet*>(getConstNet(design, false)));

        const auto sliceSize64 = streamingExpr.getSliceSize();
        if (!sliceSize64) {
          bits = std::move(streamBits);
        } else {
          if (sliceSize64 > std::numeric_limits<size_t>::max()) {
            return false; // LCOV_EXCL_LINE
          }
          const auto sliceSize = static_cast<size_t>(sliceSize64);
          std::vector<std::pair<size_t, size_t>> slices;
          slices.reserve((streamBits.size() + sliceSize - 1) / sliceSize);
          for (size_t offset = 0; offset < streamBits.size(); offset += sliceSize) {
            const auto sliceWidth = std::min(sliceSize, streamBits.size() - offset);
            slices.emplace_back(offset, sliceWidth);
          }

          bits.clear();
          bits.reserve(streamBits.size());
          for (auto it = slices.rbegin(); it != slices.rend(); ++it) {
            const auto begin = streamBits.begin() + static_cast<std::ptrdiff_t>(it->first);
            const auto end = begin + static_cast<std::ptrdiff_t>(it->second);
            bits.insert(bits.end(), begin, end);
          }
        }

        resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::ElementSelect) {
        const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
        const auto* valueExpr = stripConversions(elementExpr.value());
        if (valueExpr) {
          const auto& valueType = valueExpr->type->getCanonicalType();
          if (valueType.hasFixedRange()) {
            auto valueNet = resolveExpressionNet(design, *valueExpr);
            auto valueBits = collectBits(valueNet);
            if (valueBits.empty() &&
                valueExpr->kind == slang::ast::ExpressionKind::MemberAccess) {
              if (auto valueWidth = getIntegralExpressionBitWidth(*valueExpr)) {
                if (!resolveExpressionBits(design, *valueExpr, *valueWidth, valueBits) ||
                    valueBits.size() != *valueWidth) {
                  valueBits.clear();
                }
              }
            }
            if (!valueBits.empty()) {
              size_t elementWidth = 0;
              if (auto selectedWidth = getIntegralExpressionBitWidth(*stripped)) {
                elementWidth = *selectedWidth;
              } else if (const auto* elementType = valueType.getArrayElementType()) {
                if (auto elementRange = getRangeFromType(*elementType)) {
                  elementWidth = static_cast<size_t>(elementRange->width());
                }
              }
              if (!elementWidth) {
                const auto arrayWidth = static_cast<size_t>(valueType.getFixedRange().width());
                if (arrayWidth > 0 && (valueBits.size() % arrayWidth) == 0) {
                  elementWidth = valueBits.size() / arrayWidth;
                }
              }

              int32_t selectedIndex = 0;
              const auto indexOk = elementWidth > 0 &&
                getConstantInt32(elementExpr.selector(), selectedIndex);
              if (indexOk) {
                const auto translated = valueType.getFixedRange().translateIndex(selectedIndex);
                if (translated >= 0 &&
                    translated < static_cast<int32_t>(valueType.getFixedRange().width())) {
                  const auto offset = static_cast<size_t>(translated) * elementWidth;
                  if (offset + elementWidth <= valueBits.size()) {
                    bits.assign(
                      valueBits.begin() + static_cast<std::ptrdiff_t>(offset),
                      valueBits.begin() + static_cast<std::ptrdiff_t>(offset + elementWidth));
                    resizeBitsToWidth(
                      bits,
                      targetWidth,
                      static_cast<SNLBitNet*>(getConstNet(design, false)));
                    return true;
                  }
                } // LCOV_EXCL_LINE
              } // LCOV_EXCL_LINE
              if (elementWidth > 0 &&
                  valueBits.size() >=
                    elementWidth * static_cast<size_t>(valueType.getFixedRange().width())) {
                auto selectorWidth = getIntegralExpressionBitWidth(elementExpr.selector());
                if (selectorWidth && *selectorWidth > 0) {
                  std::vector<SNLBitNet*> selectorBits;
                  if (resolveExpressionBits(
                        design,
                        elementExpr.selector(),
                        static_cast<size_t>(*selectorWidth),
                        selectorBits) &&
                      selectorBits.size() == static_cast<size_t>(*selectorWidth)) {
                    auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
                    auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
                    auto elementSourceRange = getSourceRange(*stripped);

                    DynamicElementSelectCacheKey cacheKey;
                    const bool canUseCache = valueNet != nullptr;
                    if (canUseCache) {
                      cacheKey.design = design;
                      cacheKey.valueNet = valueNet;
                      cacheKey.targetWidth = targetWidth;
                      cacheKey.rangeLeft = valueType.getFixedRange().left;
                      cacheKey.rangeRight = valueType.getFixedRange().right;
                      cacheKey.selectorBits = selectorBits;
                      if (auto cacheIt = dynamicElementSelectCache_.find(cacheKey);
                          cacheIt != dynamicElementSelectCache_.end()) {
                        bits = cacheIt->second;
                        return true;
                      }
                    }

                    std::vector<SNLBitNet*> selectedBits(elementWidth, const0);
                    int32_t index = valueType.getFixedRange().right;
                    const int32_t end = valueType.getFixedRange().left;
                    const int32_t step = index <= end ? 1 : -1;
                    while (index != end + step) {
                      const auto translated = valueType.getFixedRange().translateIndex(index);
                      if (translated >= 0 &&
                          translated < static_cast<int32_t>(valueType.getFixedRange().width())) {
                        const auto offset = static_cast<size_t>(translated) * elementWidth;
                        if (offset + elementWidth > valueBits.size()) {
                          return false; // LCOV_EXCL_LINE
                        }

                        auto* equalsIndexBit = buildSelectorEqualsIndexBit(
                          design,
                          selectorBits,
                          index,
                          elementSourceRange);
                        if (!equalsIndexBit) {
                          return false; // LCOV_EXCL_LINE
                        }

                        if (equalsIndexBit == const0) {
                          index += step;
                          continue;
                        }

                        std::vector<SNLBitNet*> candidateBits(
                          valueBits.begin() + static_cast<std::ptrdiff_t>(offset),
                          valueBits.begin() + static_cast<std::ptrdiff_t>(offset + elementWidth));
                        if (elementWidth > 1 && equalsIndexBit != const1) {
                          std::vector<SNLBitNet*> muxedBits;
                          if (!createMux2Instance(
                                design,
                                equalsIndexBit,
                                selectedBits,
                                candidateBits,
                                muxedBits,
                                elementSourceRange)) {
                            return false;
                          }
                          selectedBits = std::move(muxedBits);
                          index += step;
                          continue;
                        }

                        for (size_t elemBit = 0; elemBit < elementWidth; ++elemBit) {
                          auto* candidateBit = candidateBits[elemBit];
                          if (equalsIndexBit == const1) {
                            selectedBits[elemBit] = candidateBit;
                            continue;
                          }
                          if (selectedBits[elemBit] == candidateBit) {
                            continue;
                          }
                          auto* outBit = SNLScalarNet::create(design);
                          annotateSourceInfo(outBit, elementSourceRange);
                          createMux2Instance(
                            design,
                            equalsIndexBit,
                            selectedBits[elemBit],
                            candidateBit,
                            outBit,
                            elementSourceRange);
                          selectedBits[elemBit] = outBit;
                        }
                      }
                      index += step;
                    }

                    bits = std::move(selectedBits);
                    resizeBitsToWidth(bits, targetWidth, const0);
                    if (canUseCache) {
                      dynamicElementSelectCache_.emplace(std::move(cacheKey), bits);
                    }
                    return true;
                  }
                }
              }
            }
          }
        }
      }

      if (stripped->kind == slang::ast::ExpressionKind::RangeSelect) {
        const auto& rangeExpr = stripped->as<slang::ast::RangeSelectExpression>();
        const auto selectionKind = rangeExpr.getSelectionKind();
        if (selectionKind == slang::ast::RangeSelectionKind::IndexedUp ||
            selectionKind == slang::ast::RangeSelectionKind::IndexedDown) {
          const auto* valueExpr = stripConversions(rangeExpr.value());
          if (valueExpr) {
            auto valueNet = resolveExpressionNet(design, *valueExpr);
            auto valueBits = collectBits(valueNet);
            if (valueBits.empty()) {
              if (auto valueWidth = getIntegralExpressionBitWidth(*valueExpr)) {
                if (!resolveExpressionBits(design, *valueExpr, *valueWidth, valueBits) ||
                    valueBits.size() != *valueWidth) {
                  valueBits.clear();
                }
              }
            }

            int32_t constantStartIndex = 0;
            int32_t constantSliceWidth = 0;
            auto selectedWidthBits = getIntegralExpressionBitWidth(*stripped);
            if (!valueBits.empty() &&
                selectedWidthBits &&
                getConstantInt32(rangeExpr.left(), constantStartIndex) &&
                getConstantInt32(rangeExpr.right(), constantSliceWidth) &&
                constantSliceWidth > 0 &&
                *selectedWidthBits == static_cast<size_t>(constantSliceWidth)) {
              slang::ConstantRange baseRange(0, 0);
              bool hasBaseRange = false;
              if (auto* valueBus = dynamic_cast<SNLBusNet*>(valueNet)) {
                baseRange = slang::ConstantRange(
                  static_cast<int32_t>(valueBus->getMSB()),
                  static_cast<int32_t>(valueBus->getLSB()));
                hasBaseRange = true;
              } else {
                const auto& valueType = valueExpr->type->getCanonicalType();
                if (valueType.hasFixedRange()) {
                  baseRange = valueType.getFixedRange();
                  hasBaseRange = true;
                }
              }
              if (!hasBaseRange && !valueBits.empty()) {
                // LCOV_EXCL_START
                const auto upper = static_cast<int32_t>(valueBits.size() - 1);
                baseRange = slang::ConstantRange(upper, 0);
                hasBaseRange = true;
                // LCOV_EXCL_STOP
              }

              if (hasBaseRange) {
                int64_t lsbIndex = static_cast<int64_t>(constantStartIndex);
                if (selectionKind == slang::ast::RangeSelectionKind::IndexedDown) {
                  lsbIndex -= static_cast<int64_t>(constantSliceWidth - 1);
                }

                std::vector<SNLBitNet*> selectedBits;
                selectedBits.reserve(static_cast<size_t>(constantSliceWidth));
                bool valid = true;
                for (int32_t elem = 0; elem < constantSliceWidth; ++elem) {
                  const int64_t elemIndex = lsbIndex + static_cast<int64_t>(elem);
                  if (elemIndex < std::numeric_limits<int32_t>::min() ||
                      elemIndex > std::numeric_limits<int32_t>::max()) {
                    valid = false;
                    break;
                  }
                  const auto translated =
                    baseRange.translateIndex(static_cast<int32_t>(elemIndex));
                  if (translated < 0 ||
                      translated >= static_cast<int32_t>(valueBits.size())) {
                    valid = false;
                    break;
                  }
                  selectedBits.push_back(valueBits[static_cast<size_t>(translated)]);
                }

                if (valid && selectedBits.size() == static_cast<size_t>(constantSliceWidth)) {
                  bits = std::move(selectedBits);
                  resizeBitsToWidth(
                    bits,
                    targetWidth,
                    static_cast<SNLBitNet*>(getConstNet(design, false)));
                  return true;
                }
              }
            }

            const auto& valueType = valueExpr->type->getCanonicalType();
            if (valueType.hasFixedRange()) {
              const auto arrayWidth = static_cast<size_t>(valueType.getFixedRange().width());
              if (!valueBits.empty() && arrayWidth > 0) {
                size_t elementWidth = 0;
                if (valueBits.size() % arrayWidth == 0) {
                  elementWidth = valueBits.size() / arrayWidth;
                }

                size_t selectedWidth = 0;
                if (auto selectedWidthBits = getIntegralExpressionBitWidth(*stripped)) {
                  selectedWidth = *selectedWidthBits;
                }
                if (elementWidth > 0 &&
                    selectedWidth > 0 &&
                    selectedWidth % elementWidth == 0) {
                  size_t sliceElements = selectedWidth / elementWidth;
                  int32_t constantSliceElements = 0;
                  if (getConstantInt32(rangeExpr.right(), constantSliceElements) &&
                      constantSliceElements > 0) {
                    const auto rhsSliceElements = static_cast<size_t>(constantSliceElements);
                    if (rhsSliceElements * elementWidth == selectedWidth) {
                      sliceElements = rhsSliceElements;
                    }
                  }

                  const auto collectCandidateBits =
                    [&](int64_t lsbIndex, std::vector<SNLBitNet*>& candidateBits) {
                      candidateBits.clear();
                      candidateBits.reserve(selectedWidth);
                      for (size_t elem = 0; elem < sliceElements; ++elem) {
                        const int64_t elemIndex = lsbIndex + static_cast<int64_t>(elem);
                        if (elemIndex < std::numeric_limits<int32_t>::min() ||
                            elemIndex > std::numeric_limits<int32_t>::max()) {
                          return false;
                        }
                        const auto translated =
                          valueType.getFixedRange().translateIndex(static_cast<int32_t>(elemIndex));
                        if (translated < 0 || translated >= static_cast<int32_t>(arrayWidth)) {
                          return false;
                        }
                        const auto offset = static_cast<size_t>(translated) * elementWidth;
                        if (offset + elementWidth > valueBits.size()) {
                          return false; // LCOV_EXCL_LINE
                        }
                        for (size_t bit = 0; bit < elementWidth; ++bit) {
                          candidateBits.push_back(valueBits[offset + bit]);
                        }
                      }
                      return candidateBits.size() == selectedWidth;
                    };

                  auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
                  int32_t constantStartIndex = 0;
                  if (getConstantInt32(rangeExpr.left(), constantStartIndex)) {
                    int64_t lsbIndex = static_cast<int64_t>(constantStartIndex);
                    if (selectionKind == slang::ast::RangeSelectionKind::IndexedDown) {
                      lsbIndex -= static_cast<int64_t>(sliceElements - 1);
                    }

                    std::vector<SNLBitNet*> constantCandidateBits;
                    if (collectCandidateBits(lsbIndex, constantCandidateBits)) {
                      bits = std::move(constantCandidateBits);
                      resizeBitsToWidth(bits, targetWidth, const0);
                      return true;
                    }
                  }

                  auto selectorWidth = getIntegralExpressionBitWidth(rangeExpr.left());
                  if (sliceElements > 0 && selectorWidth && *selectorWidth > 0) {
                    std::vector<SNLBitNet*> selectorBits;
                    const auto resolveSelectorBits = [&](const Expression& selectorExpr) {
                      if (resolveExpressionBits(
                            design,
                            selectorExpr,
                            static_cast<size_t>(*selectorWidth),
                            selectorBits) &&
                          selectorBits.size() == static_cast<size_t>(*selectorWidth)) {
                        return true;
                      }

                      const auto* strippedSelector = stripConversions(selectorExpr);
                      if (!strippedSelector ||
                          strippedSelector->kind != slang::ast::ExpressionKind::BinaryOp) {
                        return false;
                      }
                      const auto& selectorBinaryExpr =
                        strippedSelector->as<slang::ast::BinaryExpression>();
                      if (selectorBinaryExpr.op != slang::ast::BinaryOperator::Multiply) {
                        return false;
                      }

                      uint64_t factor = 0;
                      uint64_t leftConst = 0;
                      uint64_t rightConst = 0;
                      const Expression* baseExpr = nullptr;
                      const bool leftIsConst =
                        getConstantUnsigned(selectorBinaryExpr.left(), leftConst);
                      const bool rightIsConst =
                        getConstantUnsigned(selectorBinaryExpr.right(), rightConst);
                      if (rightIsConst) {
                        factor = rightConst;
                        baseExpr = &selectorBinaryExpr.left();
                      } else if (leftIsConst) {
                        factor = leftConst;
                        baseExpr = &selectorBinaryExpr.right();
                      }
                      if (!baseExpr || factor == 0 || (factor & (factor - 1ULL)) != 0ULL) {
                        return false;
                      }

                      size_t shiftAmount = 0;
                      while ((factor >> shiftAmount) > 1ULL) {
                        ++shiftAmount;
                      }

                      std::vector<SNLBitNet*> baseBits;
                      if (!resolveExpressionBits(
                            design,
                            *baseExpr,
                            static_cast<size_t>(*selectorWidth),
                            baseBits) ||
                          baseBits.size() != static_cast<size_t>(*selectorWidth)) {
                        return false;
                      }

                      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
                      selectorBits.assign(static_cast<size_t>(*selectorWidth), const0);
                      if (shiftAmount < static_cast<size_t>(*selectorWidth)) {
                        for (size_t bit = shiftAmount; bit < static_cast<size_t>(*selectorWidth);
                             ++bit) {
                          selectorBits[bit] = baseBits[bit - shiftAmount];
                        }
                      }
                      return true;
                    };

                    if (resolveSelectorBits(rangeExpr.left())) {
                      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
                      auto rangeSourceRange = getSourceRange(*stripped);

                      std::vector<SNLBitNet*> selectedBits(selectedWidth, const0);
                      int32_t startIndex = valueType.getFixedRange().right;
                      const int32_t endIndex = valueType.getFixedRange().left;
                      const int32_t step = startIndex <= endIndex ? 1 : -1;
                      while (startIndex != endIndex + step) {
                        int64_t lsbIndex = static_cast<int64_t>(startIndex);
                        if (selectionKind == slang::ast::RangeSelectionKind::IndexedDown) {
                          lsbIndex -= static_cast<int64_t>(sliceElements - 1);
                        }

                        std::vector<SNLBitNet*> candidateBits;
                        if (collectCandidateBits(lsbIndex, candidateBits)) {
                          auto* equalsIndexBit = buildSelectorEqualsIndexBit(
                            design,
                            selectorBits,
                            startIndex,
                            rangeSourceRange);
                          if (!equalsIndexBit) {
                            return false; // LCOV_EXCL_LINE
                          }
                          if (equalsIndexBit != const0) {
                            if (selectedWidth > 1 && equalsIndexBit != const1) {
                              std::vector<SNLBitNet*> muxedBits;
                              if (!createMux2Instance(
                                    design,
                                    equalsIndexBit,
                                    selectedBits,
                                    candidateBits,
                                    muxedBits,
                                    rangeSourceRange)) {
                                return false;
                              }
                              selectedBits = std::move(muxedBits);
                              startIndex += step;
                              continue;
                            }
                            for (size_t bit = 0; bit < selectedWidth; ++bit) {
                              auto* candidateBit = candidateBits[bit];
                              if (equalsIndexBit == const1) {
                                selectedBits[bit] = candidateBit;
                                continue;
                              }
                              if (selectedBits[bit] == candidateBit) {
                                continue;
                              }
                              auto* outBit = SNLScalarNet::create(design);
                              annotateSourceInfo(outBit, rangeSourceRange);
                              createMux2Instance(
                                design,
                                equalsIndexBit,
                                selectedBits[bit],
                                candidateBit,
                                outBit,
                                rangeSourceRange);
                              selectedBits[bit] = outBit;
                            }
                          }
                        }
                        startIndex += step;
                      }

                      bits = std::move(selectedBits);
                      resizeBitsToWidth(bits, targetWidth, const0);
                      return true;
                    }
                  }
                }
              }
            }
          }
        }
      }

      if (stripped->kind == slang::ast::ExpressionKind::MemberAccess) {
        const auto& memberExpr = stripped->as<slang::ast::MemberAccessExpression>();
        auto memberWidth = getIntegralExpressionBitWidth(*stripped);
        auto valueWidth = getIntegralExpressionBitWidth(memberExpr.value());
        const slang::ast::FieldSymbol* field = nullptr;
        if (memberExpr.member.kind == SymbolKind::Field) {
          field = &memberExpr.member.as<slang::ast::FieldSymbol>();
        }
        if (memberWidth && valueWidth && field) {
          std::vector<SNLBitNet*> valueBits;
          if (resolveExpressionBits(design, memberExpr.value(), *valueWidth, valueBits)) {
            const auto offset = static_cast<size_t>(field->bitOffset);
            if (offset + *memberWidth <= valueBits.size()) {
              bits.assign(
                valueBits.begin() + static_cast<std::ptrdiff_t>(offset),
                valueBits.begin() + static_cast<std::ptrdiff_t>(offset + *memberWidth));
              resizeBitsToWidth(
                bits,
                targetWidth,
                static_cast<SNLBitNet*>(getConstNet(design, false)));
              return true;
            }
          } // LCOV_EXCL_LINE
        }
      } // LCOV_EXCL_LINE

      if (stripped->kind == slang::ast::ExpressionKind::ElementSelect ||
          stripped->kind == slang::ast::ExpressionKind::RangeSelect ||
          stripped->kind == slang::ast::ExpressionKind::MemberAccess) {
        if (!resolveSelectableExpressionBits(design, *stripped, bits)) {
          return false;
        }
        resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
        return true;
      }

      auto net = resolveExpressionNet(design, *stripped);
      if (!net) {
        return false;
      }
      bits = collectBits(net);
      if (bits.empty()) {
        return false; // LCOV_EXCL_LINE
      }
      resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
      return true;
    }

    bool resolveGateOperandBits(
      SNLDesign* design,
      const Expression& expr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt,
      std::string* failureReason = nullptr) {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        // LCOV_EXCL_START
        setFailureReason("stripConversions returned null");
        return false;
        // LCOV_EXCL_STOP
      }
      if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unary = stripped->as<slang::ast::UnaryExpression>();
        if (unary.op == slang::ast::UnaryOperator::BitwiseNot) {
          std::vector<SNLBitNet*> operandBits;
          if (!resolveExpressionBits(design, unary.operand(), targetWidth, operandBits)) {
            std::ostringstream reason;
            reason << "failed to resolve bitwise-not operand bits ("
                   << describeExpression(unary.operand())
                   << ", target_width=" << targetWidth << ")";
            setFailureReason(reason.str());
            return false;
          }

          auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
          auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
          bits.clear();
          bits.reserve(targetWidth);
          for (auto* operandBit : operandBits) {
            if (operandBit == const0) {
              bits.push_back(const1);
              continue;
            }
            if (operandBit == const1) {
              bits.push_back(const0);
              continue;
            }
            auto* notBit = SNLScalarNet::create(design);
            annotateSourceInfo(notBit, sourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  operandBit,
                  notBit,
                  sourceRange)) {
              // LCOV_EXCL_START
              std::ostringstream reason;
              reason << "failed to create NOT gate for bitwise-not operand at bit "
                     << bits.size();
              setFailureReason(reason.str());
              return false;
              // LCOV_EXCL_STOP
            }
            bits.push_back(notBit);
          }
          return true;
        }
      }

      if (!resolveExpressionBits(design, *stripped, targetWidth, bits)) {
        std::ostringstream reason;
        reason << "failed to resolve operand bits (" << describeExpression(*stripped)
               << ", target_width=" << targetWidth << ")";
        setFailureReason(reason.str());
        return false;
      }
      return true;
    }

    bool createBitwiseGateAssign(
      SNLDesign* design,
      const NLDB0::GateType& gateType,
      const std::vector<const Expression*>& operands,
      const std::vector<SNLBitNet*>& lhsBits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt,
      std::string* failureReason = nullptr) {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };
      if (lhsBits.empty() || operands.empty()) {
        // LCOV_EXCL_START
        setFailureReason("empty LHS bits or empty operand list");
        return false; 
        // LCOV_EXCL_STOP
      }
      const auto bitWidth = lhsBits.size();
      std::vector<std::vector<SNLBitNet*>> operandBitsByOperand;
      operandBitsByOperand.reserve(operands.size());
      size_t operandIndex = 0;
      for (const auto* operand : operands) {
        if (!operand) {
          // LCOV_EXCL_START
          std::ostringstream reason;
          reason << "operand#" << operandIndex << " is null";
          setFailureReason(reason.str());
          return false;
          // LCOV_EXCL_STOP
        }
        std::vector<SNLBitNet*> operandBits;
        std::string operandFailureReason;
        if (!resolveGateOperandBits(
              design,
              *operand,
              bitWidth,
              operandBits,
              sourceRange,
              &operandFailureReason)) {
          std::ostringstream reason;
          reason << "operand#" << operandIndex << " (" << describeExpression(*operand) << ")";
          if (!operandFailureReason.empty()) {
            reason << ": " << operandFailureReason;
          }
          setFailureReason(reason.str());
          return false;
        }
        operandBitsByOperand.push_back(std::move(operandBits));
        ++operandIndex;
      }

      for (size_t bitIndex = 0; bitIndex < bitWidth; ++bitIndex) {
        std::vector<SNLNet*> inputs;
        inputs.reserve(operandBitsByOperand.size());
        for (const auto& operandBits : operandBitsByOperand) {
          inputs.push_back(operandBits[bitIndex]);
        }
        SNLNet* gateOut = lhsBits[bitIndex];
        if (!createGateInstance(
              design,
              gateType,
              inputs,
              gateOut,
              sourceRange) ||
            !gateOut) {
          // LCOV_EXCL_START
          std::ostringstream reason;
          reason << "failed to create gate '" << gateType.getString() << "' at bit "
                 << bitIndex << " with " << inputs.size() << " inputs";
          setFailureReason(reason.str());
          return false;
          // LCOV_EXCL_STOP
        }
      }
      return true;
    }

    bool createAddAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& leftExpr,
      const Expression& rightExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> leftBits;
      std::vector<SNLBitNet*> rightBits;
      if (!resolveExpressionBits(design, leftExpr, lhsBits.size(), leftBits) ||
          !resolveExpressionBits(design, rightExpr, lhsBits.size(), rightBits)) {
        return false;
      }

      auto* carry = static_cast<SNLBitNet*>(getConstNet(design, false));
      for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
        auto* carryOut = SNLScalarNet::create(design);
        annotateSourceInfo(carryOut, sourceRange);
        if (!createFAInstance(
              design,
              leftBits[bitIndex],
              rightBits[bitIndex],
              carry,
              lhsBits[bitIndex],
              carryOut,
              sourceRange)) {
          return false; // LCOV_EXCL_LINE
        }
        carry = carryOut;
      }
      return true;
    }

    bool addBitVectors(
      SNLDesign* design,
      const std::vector<SNLBitNet*>& leftBits,
      const std::vector<SNLBitNet*>& rightBits,
      std::vector<SNLBitNet*>& sumBits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (leftBits.size() != rightBits.size()) {
        return false; // LCOV_EXCL_LINE
      }

      sumBits.clear();
      sumBits.reserve(leftBits.size());
      auto* carry = static_cast<SNLBitNet*>(getConstNet(design, false));
      for (size_t bitIndex = 0; bitIndex < leftBits.size(); ++bitIndex) {
        auto* sumBit = SNLScalarNet::create(design);
        auto* carryOut = SNLScalarNet::create(design);
        annotateSourceInfo(sumBit, sourceRange);
        annotateSourceInfo(carryOut, sourceRange);
        if (!createFAInstance(
              design,
              leftBits[bitIndex],
              rightBits[bitIndex],
              carry,
              sumBit,
              carryOut,
              sourceRange)) {
          return false; // LCOV_EXCL_LINE
        }
        sumBits.push_back(sumBit);
        carry = carryOut;
      }
      return true;
    }

    bool createMultiplyAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& leftExpr,
      const Expression& rightExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> leftBits;
      std::vector<SNLBitNet*> rightBits;
      if (!resolveExpressionBits(design, leftExpr, lhsBits.size(), leftBits)) {
        return false;
      }
      if (!resolveExpressionBits(design, rightExpr, lhsBits.size(), rightBits)) {
        return false;
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

      std::vector<SNLBitNet*> accumulatedBits;
      bool hasAccumulated = false;
      for (size_t row = 0; row < lhsBits.size(); ++row) {
        auto* rowEnableBit = rightBits[row];
        if (rowEnableBit == const0) {
          continue;
        }

        std::vector<SNLBitNet*> partialBits(lhsBits.size(), const0);
        for (size_t col = 0; col + row < lhsBits.size(); ++col) {
          auto* multiplicandBit = leftBits[col];
          const auto outIndex = col + row;
          if (rowEnableBit == const1) {
            partialBits[outIndex] = multiplicandBit;
            continue;
          }
          if (multiplicandBit == const0) {
            partialBits[outIndex] = const0;
            continue;
          }
          if (multiplicandBit == const1) {
            partialBits[outIndex] = rowEnableBit;
            continue;
          }

          auto* andBit = SNLScalarNet::create(design);
          annotateSourceInfo(andBit, sourceRange);
          if (!createBinaryGate(
                design,
                NLDB0::GateType(NLDB0::GateType::And),
                multiplicandBit,
                rowEnableBit,
                andBit,
                sourceRange)) {
            return false; // LCOV_EXCL_LINE
          }
          partialBits[outIndex] = andBit;
        }

        bool rowHasSignal = false;
        for (auto* bit : partialBits) {
          if (bit != const0) {
            rowHasSignal = true;
            break;
          }
        }
        if (!rowHasSignal) {
          continue;
        }

        if (!hasAccumulated) {
          accumulatedBits = std::move(partialBits);
          hasAccumulated = true;
          continue;
        }

        std::vector<SNLBitNet*> sumBits;
        if (!addBitVectors(design, accumulatedBits, partialBits, sumBits, sourceRange)) {
          return false; // LCOV_EXCL_LINE
        }
        accumulatedBits = std::move(sumBits);
      }

      if (!hasAccumulated) {
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          createAssignInstance(design, const0, lhsBits[bitIndex], sourceRange);
        }
        return true;
      }

      for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
        createAssignInstance(design, accumulatedBits[bitIndex], lhsBits[bitIndex], sourceRange);
      }
      return true;
    }

    bool createSubAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& leftExpr,
      const Expression& rightExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> leftBits;
      std::vector<SNLBitNet*> rightBits;
      if (!resolveExpressionBits(design, leftExpr, lhsBits.size(), leftBits) ||
          !resolveExpressionBits(design, rightExpr, lhsBits.size(), rightBits)) {
        return false;
      }

      std::vector<SNLBitNet*> invertedRightBits;
      invertedRightBits.reserve(rightBits.size());
      for (auto* rightBit : rightBits) {
        auto* invertedBit = SNLScalarNet::create(design);
        annotateSourceInfo(invertedBit, sourceRange);
        if (!createUnaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Not),
              rightBit,
              invertedBit,
              sourceRange)) {
          return false; // LCOV_EXCL_LINE
        }
        invertedRightBits.push_back(invertedBit);
      }

      // Two's-complement subtraction: A - B == A + (~B) + 1.
      auto* carry = static_cast<SNLBitNet*>(getConstNet(design, true));
      for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
        auto* carryOut = SNLScalarNet::create(design);
        annotateSourceInfo(carryOut, sourceRange);
        if (!createFAInstance(
              design,
              leftBits[bitIndex],
              invertedRightBits[bitIndex],
              carry,
              lhsBits[bitIndex],
              carryOut,
              sourceRange)) {
          return false; // LCOV_EXCL_LINE
        }
        carry = carryOut;
      }
      return true;
    }

    bool createEqualityAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& leftExpr,
      const Expression& rightExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto* lhsBit = getSingleBitNet(lhsNet);
      if (!lhsBit) {
        return false;
      }

      auto leftWidth = getIntegralExpressionBitWidth(leftExpr);
      auto rightWidth = getIntegralExpressionBitWidth(rightExpr);
      if (!leftWidth || !rightWidth) {
        return false;
      }
      const auto compareWidth = std::max(*leftWidth, *rightWidth);
      if (!compareWidth) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> leftBits;
      std::vector<SNLBitNet*> rightBits;
      if (!resolveExpressionBits(design, leftExpr, compareWidth, leftBits) ||
          !resolveExpressionBits(design, rightExpr, compareWidth, rightBits)) {
        return false;
      }

      std::vector<SNLNet*> xnorBits;
      xnorBits.reserve(compareWidth);
      for (size_t bitIndex = 0; bitIndex < compareWidth; ++bitIndex) {
        auto* xnorBit = SNLScalarNet::create(design);
        annotateSourceInfo(xnorBit, sourceRange);
        auto xnorOut = createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::Xnor),
          leftBits[bitIndex],
          rightBits[bitIndex],
          xnorBit,
          sourceRange);
        if (!xnorOut) {
          return false; // LCOV_EXCL_LINE
        }
        xnorBits.push_back(xnorOut);
      }

      if (xnorBits.size() == 1) {
        createAssignInstance(design, xnorBits.front(), lhsBit, sourceRange);
        return true;
      }

      SNLNet* andOut = lhsBit;
      if (!createGateInstance(
            design,
            NLDB0::GateType(NLDB0::GateType::And),
            xnorBits,
            andOut,
            sourceRange) ||
          !andOut) {
        return false; // LCOV_EXCL_LINE
      }
      return true;
    }

    bool createInequalityAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& leftExpr,
      const Expression& rightExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto* lhsBit = getSingleBitNet(lhsNet);
      if (!lhsBit) {
        return false;
      }

      auto* eqBit = SNLScalarNet::create(design);
      annotateSourceInfo(eqBit, sourceRange);
      if (!createEqualityAssign(design, eqBit, leftExpr, rightExpr, sourceRange)) {
        return false;
      }
      if (!createUnaryGate(
            design,
            NLDB0::GateType(NLDB0::GateType::Not),
            eqBit,
            lhsBit,
            sourceRange)) {
        return false; // LCOV_EXCL_LINE
      }
      return true;
    }

    bool createRelationalAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& leftExpr,
      const Expression& rightExpr,
      slang::ast::BinaryOperator op,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto* lhsBit = getSingleBitNet(lhsNet);
      if (!lhsBit || !isRelationalBinaryOp(op)) {
        return false;
      }

      auto leftWidth = getIntegralExpressionBitWidth(leftExpr);
      auto rightWidth = getIntegralExpressionBitWidth(rightExpr);
      if (!leftWidth || !rightWidth) {
        return false;
      }
      const auto compareWidth = std::max(*leftWidth, *rightWidth);
      if (!compareWidth) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> leftBits;
      std::vector<SNLBitNet*> rightBits;
      if (!resolveExpressionBits(design, leftExpr, compareWidth, leftBits) ||
          !resolveExpressionBits(design, rightExpr, compareWidth, rightBits) ||
          leftBits.size() != compareWidth ||
          rightBits.size() != compareWidth) {
        return false;
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      auto makeNot = [&](SNLBitNet* inBit) -> SNLBitNet* {
        if (!inBit) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (inBit == const0) {
          return const1;
        }
        if (inBit == const1) {
          return const0;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createUnaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Not),
              inBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };

      auto makeAnd = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) {
          return nullptr;
        }
        if (leftBit == const0 || rightBit == const0) {
          return const0;
        }
        if (leftBit == const1) {
          return rightBit;
        }
        if (rightBit == const1) {
          return leftBit;
        }
        if (leftBit == rightBit) {
          return leftBit;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createBinaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::And),
              leftBit,
              rightBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };

      auto makeOr = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) {
          return nullptr;
        }
        if (leftBit == const1 || rightBit == const1) {
          return const1;
        }
        if (leftBit == const0) {
          return rightBit;
        }
        if (rightBit == const0) {
          return leftBit;
        }
        if (leftBit == rightBit) {
          return leftBit;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createBinaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Or),
              leftBit,
              rightBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };

      auto makeXnor = [&](SNLBitNet* leftBit, SNLBitNet* rightBit) -> SNLBitNet* {
        if (!leftBit || !rightBit) {
          return nullptr;
        }
        if (leftBit == rightBit) {
          return const1;
        }
        if (leftBit == const0) {
          return makeNot(rightBit);
        }
        if (rightBit == const0) {
          return makeNot(leftBit);
        }
        if (leftBit == const1) {
          return rightBit;
        }
        if (rightBit == const1) {
          return leftBit;
        }
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        if (!createBinaryGate(
              design,
              NLDB0::GateType(NLDB0::GateType::Xnor),
              leftBit,
              rightBit,
              outBit,
              sourceRange)) {
          return nullptr; // LCOV_EXCL_LINE
        }
        return outBit;
      };

      SNLBitNet* eqBit = const1;
      SNLBitNet* gtBit = const0;
      SNLBitNet* ltBit = const0;
      for (size_t bitIndex = compareWidth; bitIndex-- > 0;) {
        auto* leftBit = leftBits[bitIndex];
        auto* rightBit = rightBits[bitIndex];
        auto* notLeft = makeNot(leftBit);
        auto* notRight = makeNot(rightBit);
        if (!notLeft || !notRight) {
          return false; // LCOV_EXCL_LINE
        }
        auto* leftGtRight = makeAnd(leftBit, notRight);
        auto* leftLtRight = makeAnd(notLeft, rightBit);
        if (!leftGtRight || !leftLtRight) {
          return false; // LCOV_EXCL_LINE
        }
        auto* eqAndGt = makeAnd(eqBit, leftGtRight);
        auto* eqAndLt = makeAnd(eqBit, leftLtRight);
        if (!eqAndGt || !eqAndLt) {
          return false; // LCOV_EXCL_LINE
        }
        gtBit = makeOr(gtBit, eqAndGt);
        ltBit = makeOr(ltBit, eqAndLt);
        auto* bitEq = makeXnor(leftBit, rightBit);
        if (!gtBit || !ltBit || !bitEq) {
          return false; // LCOV_EXCL_LINE
        }
        eqBit = makeAnd(eqBit, bitEq);
        if (!eqBit) {
          return false; // LCOV_EXCL_LINE
        }
      }

      const auto& leftType = leftExpr.type->getCanonicalType();
      const auto& rightType = rightExpr.type->getCanonicalType();
      const bool signedCompare =
        leftType.isIntegral() && rightType.isIntegral() &&
        leftType.isSigned() && rightType.isSigned();
      if (signedCompare) {
        auto* signLeft = leftBits.back();
        auto* signRight = rightBits.back();
        auto* sameSign = makeXnor(signLeft, signRight);
        auto* differentSign = makeNot(sameSign);
        auto* notSignLeft = makeNot(signLeft);
        auto* notSignRight = makeNot(signRight);
        if (!sameSign || !differentSign || !notSignLeft || !notSignRight) {
          return false; // LCOV_EXCL_LINE
        }
        auto* leftPositiveRightNegative = makeAnd(notSignLeft, signRight);
        auto* leftNegativeRightPositive = makeAnd(signLeft, notSignRight);
        auto* gtIfSameSign = makeAnd(sameSign, gtBit);
        auto* gtIfDifferentSign = makeAnd(differentSign, leftPositiveRightNegative);
        auto* ltIfSameSign = makeAnd(sameSign, ltBit);
        auto* ltIfDifferentSign = makeAnd(differentSign, leftNegativeRightPositive);
        if (!leftPositiveRightNegative || !leftNegativeRightPositive ||
            !gtIfSameSign || !gtIfDifferentSign || !ltIfSameSign || !ltIfDifferentSign) {
          return false; // LCOV_EXCL_LINE
        }
        gtBit = makeOr(gtIfSameSign, gtIfDifferentSign);
        ltBit = makeOr(ltIfSameSign, ltIfDifferentSign);
        if (!gtBit || !ltBit) {
          return false; // LCOV_EXCL_LINE
        }
      }

      SNLBitNet* relationBit = nullptr;
      switch (op) {
        case slang::ast::BinaryOperator::LessThan:
          relationBit = ltBit;
          break;
        case slang::ast::BinaryOperator::LessThanEqual:
          relationBit = makeOr(ltBit, eqBit);
          break;
        case slang::ast::BinaryOperator::GreaterThan:
          relationBit = gtBit;
          break;
        case slang::ast::BinaryOperator::GreaterThanEqual:
          relationBit = makeOr(gtBit, eqBit);
          break;
        // LCOV_EXCL_START
        default:
          return false;
        // LCOV_EXCL_STOP
      }
      if (!relationBit) {
        return false; // LCOV_EXCL_LINE
      }

      createAssignInstance(design, relationBit, lhsBit, sourceRange);
      return true;
    }

    bool createLogicalRightShiftAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& valueExpr,
      const Expression& shiftAmountExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt,
      std::string* failureReason = nullptr) {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        // LCOV_EXCL_START
        setFailureReason("failed to resolve LHS bits");
        return false;
        // LCOV_EXCL_STOP
      }

      std::vector<SNLBitNet*> valueBits;
      if (!resolveExpressionBits(design, valueExpr, lhsBits.size(), valueBits)) {
        std::ostringstream reason;
        reason << "failed to resolve value bits (" << describeExpression(valueExpr)
               << ", target_width=" << lhsBits.size() << ")";
        setFailureReason(reason.str());
        return false;
      }

      auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));
      uint64_t shiftAmount = 0;
      if (getConstantUnsigned(shiftAmountExpr, shiftAmount)) {
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          const auto srcIndex = static_cast<uint64_t>(bitIndex) + shiftAmount;
          auto* srcBit = srcIndex < lhsBits.size()
            ? valueBits[static_cast<size_t>(srcIndex)]
            : constZero;
          createAssignInstance(design, srcBit, lhsBits[bitIndex], sourceRange);
        }
        return true;
      }

      auto shiftWidth = getIntegralExpressionBitWidth(shiftAmountExpr);
      if (!shiftWidth || !*shiftWidth) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "failed to resolve shift amount width ("
               << describeExpression(shiftAmountExpr) << ")";
        setFailureReason(reason.str());
        return false;
        // LCOV_EXCL_STOP
      }

      std::vector<SNLBitNet*> shiftBits;
      if (!resolveExpressionBits(design, shiftAmountExpr, *shiftWidth, shiftBits) ||
          shiftBits.empty()) {
        std::ostringstream reason;
        reason << "failed to resolve shift amount bits ("
               << describeExpression(shiftAmountExpr)
               << ", target_width=" << *shiftWidth << ")";
        setFailureReason(reason.str());
        return false;
      }

      std::vector<SNLBitNet*> stageBits = valueBits;
      for (size_t stageIndex = 0; stageIndex < shiftBits.size(); ++stageIndex) {
        bool shiftAll = stageIndex >= 63;
        size_t stageShift = 0;
        if (!shiftAll) {
          const auto rawShift = uint64_t {1} << stageIndex;
          if (rawShift >= lhsBits.size()) {
            shiftAll = true;
          } else {
            stageShift = static_cast<size_t>(rawShift);
          }
        }

        std::vector<SNLBitNet*> nextBits;
        nextBits.reserve(lhsBits.size());
        const bool isLastStage = stageIndex + 1 == shiftBits.size();
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          auto* shiftedBit = constZero;
          if (!shiftAll && bitIndex + stageShift < lhsBits.size()) {
            shiftedBit = stageBits[bitIndex + stageShift];
          }

          auto* outBit = isLastStage ? lhsBits[bitIndex] : SNLScalarNet::create(design);
          if (!isLastStage) {
            annotateSourceInfo(outBit, sourceRange);
          }
          createMux2Instance(
            design,
            shiftBits[stageIndex],
            stageBits[bitIndex],
            shiftedBit,
            outBit,
            sourceRange);
          nextBits.push_back(outBit);
        }
        stageBits = std::move(nextBits);
      }
      return true;
    }

    bool createArithmeticRightShiftAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& valueExpr,
      const Expression& shiftAmountExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt,
      std::string* failureReason = nullptr) {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        // LCOV_EXCL_START
        setFailureReason("failed to resolve LHS bits");
        return false;
        // LCOV_EXCL_STOP
      }

      std::vector<SNLBitNet*> valueBits;
      if (!resolveExpressionBits(design, valueExpr, lhsBits.size(), valueBits)) {
        std::ostringstream reason;
        reason << "failed to resolve value bits (" << describeExpression(valueExpr)
               << ", target_width=" << lhsBits.size() << ")";
        setFailureReason(reason.str());
        return false;
      }
      if (valueBits.empty()) {
        // LCOV_EXCL_START
        setFailureReason("failed to resolve non-empty value bits");
        return false;
        // LCOV_EXCL_STOP
      }

      auto* signFillBit = valueBits.back();

      uint64_t shiftAmount = 0;
      if (getConstantUnsigned(shiftAmountExpr, shiftAmount)) {
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          const auto srcIndex = static_cast<uint64_t>(bitIndex) + shiftAmount;
          auto* srcBit = srcIndex < lhsBits.size()
            ? valueBits[static_cast<size_t>(srcIndex)]
            : signFillBit;
          createAssignInstance(design, srcBit, lhsBits[bitIndex], sourceRange);
        }
        return true;
      }

      auto shiftWidth = getIntegralExpressionBitWidth(shiftAmountExpr);
      if (!shiftWidth || !*shiftWidth) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "failed to resolve shift amount width ("
               << describeExpression(shiftAmountExpr) << ")";
        setFailureReason(reason.str());
        return false;
        // LCOV_EXCL_STOP
      }

      std::vector<SNLBitNet*> shiftBits;
      if (!resolveExpressionBits(design, shiftAmountExpr, *shiftWidth, shiftBits) ||
          shiftBits.empty()) {
        std::ostringstream reason;
        reason << "failed to resolve shift amount bits ("
               << describeExpression(shiftAmountExpr)
               << ", target_width=" << *shiftWidth << ")";
        setFailureReason(reason.str());
        return false;
      }

      std::vector<SNLBitNet*> stageBits = valueBits;
      for (size_t stageIndex = 0; stageIndex < shiftBits.size(); ++stageIndex) {
        bool shiftAll = stageIndex >= 63;
        size_t stageShift = 0;
        if (!shiftAll) {
          const auto rawShift = uint64_t {1} << stageIndex;
          if (rawShift >= lhsBits.size()) {
            shiftAll = true;
          } else {
            stageShift = static_cast<size_t>(rawShift);
          }
        }

        std::vector<SNLBitNet*> nextBits;
        nextBits.reserve(lhsBits.size());
        const bool isLastStage = stageIndex + 1 == shiftBits.size();
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          auto* shiftedBit = stageBits.back();
          if (!shiftAll && bitIndex + stageShift < lhsBits.size()) {
            shiftedBit = stageBits[bitIndex + stageShift];
          }

          auto* outBit = isLastStage ? lhsBits[bitIndex] : SNLScalarNet::create(design);
          if (!isLastStage) {
            annotateSourceInfo(outBit, sourceRange);
          }
          createMux2Instance(
            design,
            shiftBits[stageIndex],
            stageBits[bitIndex],
            shiftedBit,
            outBit,
            sourceRange);
          nextBits.push_back(outBit);
        }
        stageBits = std::move(nextBits);
      }
      return true;
    }

    bool createLogicalLeftShiftAssign(
      SNLDesign* design,
      SNLNet* lhsNet,
      const Expression& valueExpr,
      const Expression& shiftAmountExpr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto lhsBits = collectBits(lhsNet);
      if (lhsBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> valueBits;
      if (!resolveExpressionBits(design, valueExpr, lhsBits.size(), valueBits)) {
        return false;
      }

      auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));

      uint64_t shiftAmount = 0;
      if (getConstantUnsigned(shiftAmountExpr, shiftAmount)) {
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          auto* srcBit = constZero;
          if (shiftAmount <= bitIndex) {
            const auto srcIndex = bitIndex - static_cast<size_t>(shiftAmount);
            srcBit = valueBits[srcIndex];
          }
          createAssignInstance(design, srcBit, lhsBits[bitIndex], sourceRange);
        }
        return true;
      }

      auto shiftWidth = getIntegralExpressionBitWidth(shiftAmountExpr);
      if (!shiftWidth || !*shiftWidth) {
        return false; // LCOV_EXCL_LINE
      }

      std::vector<SNLBitNet*> shiftBits;
      if (!resolveExpressionBits(design, shiftAmountExpr, *shiftWidth, shiftBits) ||
          shiftBits.empty()) {
        return false;
      }

      std::vector<SNLBitNet*> stageBits = valueBits;
      for (size_t stageIndex = 0; stageIndex < shiftBits.size(); ++stageIndex) {
        bool shiftAll = stageIndex >= 63;
        size_t stageShift = 0;
        if (!shiftAll) {
          const auto rawShift = uint64_t {1} << stageIndex;
          if (rawShift >= lhsBits.size()) {
            shiftAll = true;
          } else {
            stageShift = static_cast<size_t>(rawShift);
          }
        }

        std::vector<SNLBitNet*> nextBits;
        nextBits.reserve(lhsBits.size());
        const bool isLastStage = stageIndex + 1 == shiftBits.size();
        for (size_t bitIndex = 0; bitIndex < lhsBits.size(); ++bitIndex) {
          auto* shiftedBit = constZero;
          if (!shiftAll && bitIndex >= stageShift) {
            shiftedBit = stageBits[bitIndex - stageShift];
          }

          auto* outBit = isLastStage ? lhsBits[bitIndex] : SNLScalarNet::create(design);
          if (!isLastStage) {
            annotateSourceInfo(outBit, sourceRange);
          }
          createMux2Instance(
            design,
            shiftBits[stageIndex],
            stageBits[bitIndex],
            shiftedBit,
            outBit,
            sourceRange);
          nextBits.push_back(outBit);
        }
        stageBits = std::move(nextBits);
      }
      return true;
    }

    SNLScalarNet* getConstNet(SNLDesign* design, bool one) {
      auto& cache = one ? const1Nets_ : const0Nets_;
      auto it = cache.find(design);
      if (it != cache.end()) {
        return it->second;
      }
      auto net = SNLScalarNet::create(design);
      net->setType(one ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0);
      cache[design] = net;
      return net;
    }

    std::vector<SNLBitNet*> collectBits(SNLNet* net) {
      std::vector<SNLBitNet*> bits;
      if (!net) {
        return bits; // LCOV_EXCL_LINE
      }
      if (auto scalar = dynamic_cast<SNLScalarNet*>(net)) {
        bits.push_back(scalar);
        return bits;
      }
      auto bus = dynamic_cast<SNLBusNet*>(net);
      if (!bus) {
        return bits; // LCOV_EXCL_LINE
      }
      auto msb = bus->getMSB();
      auto lsb = bus->getLSB();
      auto step = lsb <= msb ? 1 : -1;
      for (auto bit = lsb; bit != msb + step; bit += step) {
        if (auto busBit = bus->getBit(bit)) {
          bits.push_back(busBit);
        }
      }
      return bits;
    }

    struct PackedNetRef {
      SNLNet* net {nullptr};
      NLID::Bit msb {0};
      NLID::Bit lsb {0};
    };

    size_t getPackedNetRefWidth(const PackedNetRef& ref) const {
      if (!ref.net) {
        return 0;
      }
      if (dynamic_cast<SNLScalarNet*>(ref.net)) {
        return 1;
      }
      return static_cast<size_t>(ref.msb >= ref.lsb ? ref.msb - ref.lsb + 1 : ref.lsb - ref.msb + 1);
    }

    PackedNetRef getPackedNetRef(SNLNet* net) const {
      if (auto* bus = dynamic_cast<SNLBusNet*>(net)) {
        return PackedNetRef{net, bus->getMSB(), bus->getLSB()};
      }
      return PackedNetRef{net, 0, 0};
    }

    bool tryGetPackedNetRef(const std::vector<SNLBitNet*>& bits, PackedNetRef& ref) const {
      if (bits.empty()) {
        return false;
      }
      if (bits.size() == 1) {
        ref = PackedNetRef{bits.front(), 0, 0};
        return true;
      }

      auto* firstBusBit = dynamic_cast<SNLBusNetBit*>(bits.front());
      if (!firstBusBit) {
        return false;
      }
      auto* bus = firstBusBit->getBus();
      for (size_t i = 1; i < bits.size(); ++i) {
        auto* currentBusBit = dynamic_cast<SNLBusNetBit*>(bits[i]);
        auto* previousBusBit = dynamic_cast<SNLBusNetBit*>(bits[i - 1]);
        if (!currentBusBit || !previousBusBit || currentBusBit->getBus() != bus) {
          return false;
        }
        const auto delta = currentBusBit->getBit() - previousBusBit->getBit();
        if (delta != 1 && delta != -1) {
          return false;
        }
      }
      auto* lastBusBit = dynamic_cast<SNLBusNetBit*>(bits.back());
      if (!lastBusBit) {
        return false;
      }
      ref = PackedNetRef{bus, lastBusBit->getBit(), firstBusBit->getBit()};
      return true;
    }

    std::vector<SNLBitNet*> collectBits(const PackedNetRef& ref) {
      if (!ref.net) {
        return {}; // LCOV_EXCL_LINE
      }
      if (auto* bit = dynamic_cast<SNLBitNet*>(ref.net)) {
        return {bit};
      }
      std::vector<SNLBitNet*> bits;
      auto* bus = static_cast<SNLBusNet*>(ref.net);
      const auto step = ref.lsb <= ref.msb ? 1 : -1;
      for (auto bit = ref.lsb; bit != ref.msb + step; bit += step) {
        if (auto* busBit = bus->getBit(bit)) {
          bits.push_back(busBit);
        }
      }
      return bits;
    }

    SNLNet* materializeBitsAsNet(
      SNLDesign* design,
      const std::vector<SNLBitNet*>& bits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (bits.empty()) {
        return nullptr; // LCOV_EXCL_LINE
      }
      if (bits.size() == 1) {
        return bits.front();
      }
      auto* net = SNLBusNet::create(
        design,
        static_cast<NLID::Bit>(bits.size() - 1),
        0);
      annotateSourceInfo(net, sourceRange);
      connectBusNetBits(design, net, bits, sourceRange);
      return net;
    }

    PackedNetRef getOrMaterializePackedNetRef(
      SNLDesign* design,
      const std::vector<SNLBitNet*>& bits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      PackedNetRef ref;
      if (tryGetPackedNetRef(bits, ref)) {
        return ref;
      }
      return getPackedNetRef(materializeBitsAsNet(design, bits, sourceRange));
    }

    SNLBitNet* getSingleBitNet(SNLNet* net) {
      if (!net) {
        return nullptr;
      }
      if (auto bit = dynamic_cast<SNLBitNet*>(net)) {
        return bit;
      }
      auto bus = dynamic_cast<SNLBusNet*>(net);
      if (!bus || bus->getWidth() != 1) {
        return nullptr;
      }
      return bus->getBit(bus->getMSB()); // LCOV_EXCL_LINE
    }

    SNLBitNet* resolveConditionNet(
      SNLDesign* design,
      const Expression& conditionExpr,
      const std::string& condBaseName,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      const auto* stripped = stripConversions(conditionExpr);
      if (!stripped) {
        return nullptr; // LCOV_EXCL_LINE
      }
      bool constantBit = false;
      if (getConstantBit(*stripped, constantBit)) {
        return static_cast<SNLBitNet*>(getConstNet(design, constantBit));
      }

      auto resolveSingleBitExpression = [&](const Expression& expr) -> SNLBitNet* {
        const auto* strippedExpr = stripConversions(expr);
        if (!strippedExpr) {
          return nullptr; // LCOV_EXCL_LINE
        }
        if (strippedExpr->kind == slang::ast::ExpressionKind::ConditionalOp) {
          return nullptr;
        }
        if (strippedExpr->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = strippedExpr->as<slang::ast::BinaryExpression>();
          const auto logicalOp =
            (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd) ||
            (binaryExpr.op == slang::ast::BinaryOperator::LogicalOr);
          const auto bitwiseSingleBitOp =
            (binaryExpr.op == slang::ast::BinaryOperator::BinaryAnd) ||
            (binaryExpr.op == slang::ast::BinaryOperator::BinaryOr);
          if (logicalOp || bitwiseSingleBitOp) {
            auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
            auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
            auto* lhsBit = resolveConditionNet(
              design,
              binaryExpr.left(),
              joinName("cond_l", condBaseName),
              sourceRange);
            if (!lhsBit) {
              return nullptr;
            }

            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd && lhsBit == const0) {
              return const0;
            }
            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalOr && lhsBit == const1) {
              return const1;
            }

            auto* rhsBit = resolveConditionNet(
              design,
              binaryExpr.right(),
              joinName("cond_r", condBaseName),
              sourceRange);
            if (!rhsBit) {
              return nullptr;
            }

            const bool isAndOp =
              binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd ||
              binaryExpr.op == slang::ast::BinaryOperator::BinaryAnd;
            if (isAndOp) {
              if (rhsBit == const0) {
                return const0;
              }
              if (lhsBit == const1) {
                return rhsBit;
              }
              if (rhsBit == const1) {
                return lhsBit;
              }
              if (lhsBit == rhsBit) {
                return lhsBit;
              }
            } else {
              if (rhsBit == const1) {
                return const1;
              }
              if (lhsBit == const0) {
                return rhsBit;
              }
              if (rhsBit == const0) {
                return lhsBit;
              }
              if (lhsBit == rhsBit) {
                return lhsBit;
              }
            }

            auto logicalSourceRange = sourceRange ? sourceRange : getSourceRange(*strippedExpr);
            auto* outBit = SNLScalarNet::create(design);
            annotateSourceInfo(outBit, logicalSourceRange);
            const auto gateType = isAndOp
              ? NLDB0::GateType(NLDB0::GateType::And)
              : NLDB0::GateType(NLDB0::GateType::Or);
            return getSingleBitNet(createBinaryGate(
              design,
              gateType,
              lhsBit,
              rhsBit,
              outBit,
              logicalSourceRange));
          }

          if (!isEqualityBinaryOp(binaryExpr.op) && !isInequalityBinaryOp(binaryExpr.op)) {
            if (!isRelationalBinaryOp(binaryExpr.op)) {
              return nullptr;
            }

            auto* compareBit = SNLScalarNet::create(design);
            auto compareSourceRange = sourceRange ? sourceRange : getSourceRange(*strippedExpr);
            annotateSourceInfo(compareBit, compareSourceRange);
            if (!createRelationalAssign(
                  design,
                  compareBit,
                  binaryExpr.left(),
                  binaryExpr.right(),
                  binaryExpr.op,
                  compareSourceRange)) {
              return nullptr;
            }
            return compareBit;
          }
          auto* compareBit = SNLScalarNet::create(design);
          auto compareSourceRange = sourceRange ? sourceRange : getSourceRange(*strippedExpr);
          annotateSourceInfo(compareBit, compareSourceRange);
          reportCaseComparison2StateWarning(binaryExpr.op, compareSourceRange);
          const bool ok = isInequalityBinaryOp(binaryExpr.op)
            ? createInequalityAssign(
                design,
                compareBit,
                binaryExpr.left(),
                binaryExpr.right(),
                compareSourceRange)
            : createEqualityAssign(
                design,
                compareBit,
                binaryExpr.left(),
                binaryExpr.right(),
                compareSourceRange);
          if (!ok) {
            return nullptr;
          }
          return compareBit;
        }
        if (auto* bit = getSingleBitNet(resolveExpressionNet(design, *strippedExpr))) {
          return bit;
        }
        const bool isSelectableExpr =
          strippedExpr->kind == slang::ast::ExpressionKind::ElementSelect ||
          strippedExpr->kind == slang::ast::ExpressionKind::RangeSelect ||
          strippedExpr->kind == slang::ast::ExpressionKind::MemberAccess;
        if (!isSelectableExpr) {
          return nullptr;
        }
        auto exprWidth = getIntegralExpressionBitWidth(*strippedExpr);
        if (!exprWidth || *exprWidth != 1) {
          return nullptr;
        }
        std::vector<SNLBitNet*> exprBits;
        if (!resolveExpressionBits(design, *strippedExpr, 1, exprBits) || exprBits.size() != 1) {
          return nullptr;
        }
        return exprBits.front();
      };

      if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
        if (unaryExpr.op == slang::ast::UnaryOperator::LogicalNot ||
            unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
          auto operandNet = resolveSingleBitExpression(unaryExpr.operand());
          if (!operandNet) {
            return nullptr;
          }

          auto condNet = getOrCreateNamedNet(
            design,
            joinName("cond_not", condBaseName),
            nullptr,
            sourceRange);
          auto condBit = getSingleBitNet(condNet);
          if (!condBit) {
            return nullptr; // LCOV_EXCL_LINE
          }
          createUnaryGate(
            design,
            NLDB0::GateType(NLDB0::GateType::Not),
            operandNet,
            condBit,
            sourceRange);
          return condBit;
        }
      }

      return resolveSingleBitExpression(conditionExpr);
    }

    std::string getExpressionBaseName(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (stripped && slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        const auto& valueExpr = stripped->as<slang::ast::ValueExpressionBase>();
        return std::string(valueExpr.symbol.name);
      }
      return {}; // LCOV_EXCL_LINE
    }

    // LCOV_EXCL_START
    std::string describeExpressionKind(slang::ast::ExpressionKind kind) const {
      if (kind == slang::ast::ExpressionKind::UnaryOp) {
        return "UnaryOp";
      }
      if (kind == slang::ast::ExpressionKind::BinaryOp) {
        return "BinaryOp";
      }
      if (kind == slang::ast::ExpressionKind::ElementSelect) {
        return "ElementSelect";
      }
      if (kind == slang::ast::ExpressionKind::RangeSelect) {
        return "RangeSelect";
      }
      if (kind == slang::ast::ExpressionKind::MemberAccess) {
        return "MemberAccess";
      }
      if (kind == slang::ast::ExpressionKind::Concatenation) {
        return "Concatenation";
      }
      if (kind == slang::ast::ExpressionKind::Streaming) {
        return "Streaming";
      }
      if (kind == slang::ast::ExpressionKind::Replication) {
        return "Replication";
      }
      if (kind == slang::ast::ExpressionKind::ConditionalOp) {
        return "ConditionalOp";
      }
      if (kind == slang::ast::ExpressionKind::Inside) {
        return "Inside";
      }
      if (kind == slang::ast::ExpressionKind::IntegerLiteral) {
        return "IntegerLiteral";
      }
      if (kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        return "UnbasedUnsizedIntegerLiteral";
      }
      if (kind == slang::ast::ExpressionKind::ValueRange) {
        return "ValueRange";
      }
      if (kind == slang::ast::ExpressionKind::Conversion) {
        return "Conversion";
      }
      if (kind == slang::ast::ExpressionKind::SimpleAssignmentPattern) {
        return "SimpleAssignmentPattern";
      }
      if (kind == slang::ast::ExpressionKind::StructuredAssignmentPattern) {
        return "StructuredAssignmentPattern";
      }
      if (kind == slang::ast::ExpressionKind::ReplicatedAssignmentPattern) {
        return "ReplicatedAssignmentPattern";
      }
      std::ostringstream fallback;
      fallback << "kind#" << static_cast<int>(kind);
      return fallback.str();
    }
    // LCOV_EXCL_STOP

    // LCOV_EXCL_START
    std::string describeUnaryOperator(slang::ast::UnaryOperator op) const {
      if (op == slang::ast::UnaryOperator::BitwiseNot) {
        return "~";
      }
      if (op == slang::ast::UnaryOperator::LogicalNot) {
        return "!";
      }
      std::ostringstream fallback;
      fallback << "op#" << static_cast<int>(op);
      return fallback.str();
    }
    // LCOV_EXCL_STOP

    // LCOV_EXCL_START
    std::string describeRangeSelectionKind(slang::ast::RangeSelectionKind kind) const {
      switch (kind) {
        case slang::ast::RangeSelectionKind::Simple:
          return "Simple";
        case slang::ast::RangeSelectionKind::IndexedUp:
          return "IndexedUp";
        case slang::ast::RangeSelectionKind::IndexedDown:
          return "IndexedDown";
      }
      std::ostringstream fallback;
      fallback << "kind#" << static_cast<int>(kind);
      return fallback.str();
    }
    // LCOV_EXCL_STOP

    // LCOV_EXCL_START
    std::string describeExpression(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return "expr=<null>";
      }
      std::ostringstream description;
      description << describeExpressionKind(stripped->kind);
      if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
        description << " op=" << slang::ast::OpInfo::getText(binaryExpr.op);
      } else if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
        description << " op=" << describeUnaryOperator(unaryExpr.op);
      } else if (stripped->kind == slang::ast::ExpressionKind::MemberAccess) {
        const auto& memberExpr = stripped->as<slang::ast::MemberAccessExpression>();
        description << " member=" << std::string(memberExpr.member.name);
        const auto* memberBase = stripConversions(memberExpr.value());
        if (memberBase && slang::ast::ValueExpressionBase::isKind(memberBase->kind)) {
          description << " owner="
                      << std::string(
                           memberBase->as<slang::ast::ValueExpressionBase>().symbol.name);
        }
      } else if (stripped->kind == slang::ast::ExpressionKind::RangeSelect) {
        const auto& rangeExpr = stripped->as<slang::ast::RangeSelectExpression>();
        description << " sel=" << describeRangeSelectionKind(rangeExpr.getSelectionKind());
        const auto* valueExpr = stripConversions(rangeExpr.value());
        const auto* leftExpr = stripConversions(rangeExpr.left());
        const auto* rightExpr = stripConversions(rangeExpr.right());
        if (valueExpr) {
          description << " value=" << describeExpressionKind(valueExpr->kind);
          auto valueBaseName = getExpressionBaseName(*valueExpr);
          if (!valueBaseName.empty()) {
            description << "(" << valueBaseName << ")";
          }
        }
        if (leftExpr) {
          description << " left=" << describeExpressionKind(leftExpr->kind);
          int32_t leftConst = 0;
          if (getConstantInt32(*leftExpr, leftConst)) {
            description << "(" << leftConst << ")";
          }
        }
        if (rightExpr) {
          description << " right=" << describeExpressionKind(rightExpr->kind);
          int32_t rightConst = 0;
          if (getConstantInt32(*rightExpr, rightConst)) {
            description << "(" << rightConst << ")";
          }
        }
      }
      if (auto width = getIntegralExpressionBitWidth(*stripped)) {
        description << " width=" << *width;
      }
      auto baseName = getExpressionBaseName(*stripped);
      if (!baseName.empty()) {
        description << " base=" << baseName;
      }
      return description.str();
    }
    // LCOV_EXCL_STOP

    // LCOV_EXCL_START
    std::string joinName(const std::string& prefix, const std::string& base) const {
      if (prefix.empty()) {
        return base;
      }
      if (base.empty()) {
        return prefix;
      }
      return prefix + "_" + base;
    }
    // LCOV_EXCL_STOP

    std::string sanitizeName(const std::string& text) const {
      std::string sanitized;
      sanitized.reserve(text.size());
      bool previousWasSeparator = false;
      for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
          sanitized.push_back(static_cast<char>(ch));
          previousWasSeparator = false;
          continue;
        }
        if (!sanitized.empty() && !previousWasSeparator) {
          sanitized.push_back('_');
          previousWasSeparator = true;
        }
      }
      if (!sanitized.empty() && sanitized.back() == '_') {
        sanitized.pop_back();
      }
      return sanitized.empty() ? std::string("unnamed") : sanitized;
    }

    bool isInsideGenerateScope(const Symbol& symbol) const {
      for (const auto* scope = symbol.getParentScope(); scope;
           scope = scope->asSymbol().getParentScope()) {
        const auto kind = scope->asSymbol().kind;
        if (kind == SymbolKind::GenerateBlock || kind == SymbolKind::GenerateBlockArray) {
          return true;
        }
      }
      return false;
    }

    std::string getLoweredSymbolName(const Symbol& symbol) const {
      if (!isInsideGenerateScope(symbol)) {
        return std::string(symbol.name);
      }
      auto hierarchicalPath = symbol.getHierarchicalPath();
      if (hierarchicalPath.empty()) {
        return sanitizeName(std::string(symbol.name));
      }
      return sanitizeName(hierarchicalPath);
    }

    bool isCompatibleNet(const SNLNet* net, const SNLNet* like) const {
      if (!net || !like) {
        return false;
      }
      if (auto likeBus = dynamic_cast<const SNLBusNet*>(like)) {
        auto bus = dynamic_cast<const SNLBusNet*>(net);
        if (!bus) {
          return false;
        }
        return bus->getMSB() == likeBus->getMSB() && bus->getLSB() == likeBus->getLSB();
      }
      return dynamic_cast<const SNLScalarNet*>(net) != nullptr;
    }

    SNLNet* getOrCreateNamedNet(
      SNLDesign* design,
      const std::string& baseName,
      const SNLNet* like,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto name = baseName.empty() ? std::string("tmp") : baseName;
      int suffix = 0;
      while (true) {
        if (auto existing = design->getNet(NLName(name))) {
          if (isCompatibleNet(existing, like)) {
            return existing;
          }
        } else {
          if (auto likeBus = dynamic_cast<const SNLBusNet*>(like)) {
            auto net = SNLBusNet::create(
              design,
              likeBus->getMSB(),
              likeBus->getLSB(),
              NLName(name));
            annotateSourceInfo(net, sourceRange);
            return net;
          }
          auto net = SNLScalarNet::create(design, NLName(name));
          annotateSourceInfo(net, sourceRange);
          return net;
        }
        name = baseName + "_" + std::to_string(suffix++);
      }
    }


    SNLNet* createBinaryGate(
      SNLDesign* design,
      const NLDB0::GateType& type,
      SNLNet* in0,
      SNLNet* in1,
      SNLBitNet* outNet = nullptr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      std::vector<SNLNet*> inputs { in0, in1 };
      SNLNet* gateOutNet = outNet;
      createGateInstance(design, type, inputs, gateOutNet, sourceRange);
      return gateOutNet;
    }

    SNLNet* createUnaryGate(
      SNLDesign* design,
      const NLDB0::GateType& type,
      SNLNet* in0,
      SNLBitNet* outNet = nullptr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      std::vector<SNLNet*> inputs { in0 };
      SNLNet* gateOutNet = outNet;
      createGateInstance(design, type, inputs, gateOutNet, sourceRange);
      return gateOutNet;
    }

    void createMux2Instance(
      SNLDesign* design,
      SNLBitNet* select,
      const PackedNetRef& inA,
      const PackedNetRef& inB,
      SNLNet* outNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (!select || !inA.net || !inB.net || !outNet) {
        return; // LCOV_EXCL_LINE
      }
      const auto width = outNet->getWidth();
      if (width != getPackedNetRefWidth(inA) || width != getPackedNetRefWidth(inB)) {
        throw SNLSVConstructorException("Internal error: mux width mismatch");
      }

      auto* mux2 = NLDB0::getOrCreateMux2(width);
      auto* inst = SNLInstance::create(design, mux2);
      annotateSourceInfo(inst, sourceRange);
      auto* aTerm = NLDB0::getMux2InputA(mux2);
      auto* bTerm = NLDB0::getMux2InputB(mux2);
      auto* sTerm = NLDB0::getMux2Select(mux2);
      auto* yTerm = NLDB0::getMux2Output(mux2);
      inst->setTermNet(aTerm, inA.net, inA.msb, inA.lsb);
      inst->setTermNet(bTerm, inB.net, inB.msb, inB.lsb);
      inst->setTermNet(sTerm, select);
      inst->setTermNet(yTerm, outNet);
    }

    void createMux2Instance(
      SNLDesign* design,
      SNLBitNet* select,
      SNLBitNet* inA,
      SNLBitNet* inB,
      SNLBitNet* outNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      createMux2Instance(
        design,
        select,
        PackedNetRef{inA, 0, 0},
        PackedNetRef{inB, 0, 0},
        outNet,
        sourceRange);
    }

    bool createMux2Instance(
      SNLDesign* design,
      SNLBitNet* select,
      const std::vector<SNLBitNet*>& inA,
      const std::vector<SNLBitNet*>& inB,
      std::vector<SNLBitNet*>& outBits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt,
      SNLNet* explicitOutNet = nullptr) {
      if (inA.size() != inB.size() || inA.empty()) {
        return false;
      }
      auto inARef = getOrMaterializePackedNetRef(design, inA, sourceRange);
      auto inBRef = getOrMaterializePackedNetRef(design, inB, sourceRange);
      SNLNet* outNet = explicitOutNet;
      if (outNet) {
        if (outNet->getWidth() != inA.size()) {
          return false;
        }
      } else if (inA.size() == 1) {
        auto* outBit = SNLScalarNet::create(design);
        annotateSourceInfo(outBit, sourceRange);
        outNet = outBit;
      } else {
        auto* outBus = SNLBusNet::create(
          design,
          static_cast<NLID::Bit>(inA.size() - 1),
          0);
        annotateSourceInfo(outBus, sourceRange);
        outNet = outBus;
      }
      createMux2Instance(design, select, inARef, inBRef, outNet, sourceRange);
      outBits = collectBits(outNet);
      return true;
    }

    bool createFAInstance(
      SNLDesign* design,
      SNLBitNet* inA,
      SNLBitNet* inB,
      SNLBitNet* inCI,
      SNLBitNet* outS,
      SNLBitNet* outCO,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto fa = NLDB0::getFA();
      if (!fa) { return false; }
      if (!outS || !outCO) {
        return false; // LCOV_EXCL_LINE
      }
      auto inst = SNLInstance::create(design, fa);
      annotateSourceInfo(inst, sourceRange);
      if (auto t = NLDB0::getFAInputA())  { if (auto it = inst->getInstTerm(t)) it->setNet(inA); }
      if (auto t = NLDB0::getFAInputB())  { if (auto it = inst->getInstTerm(t)) it->setNet(inB); }
      if (auto t = NLDB0::getFAInputCI()) { if (auto it = inst->getInstTerm(t)) it->setNet(inCI); }
      if (auto t = NLDB0::getFAOutputS())  { if (auto it = inst->getInstTerm(t)) it->setNet(outS); }
      if (auto t = NLDB0::getFAOutputCO()) { if (auto it = inst->getInstTerm(t)) it->setNet(outCO); }
      return true;
    }

    std::vector<SNLBitNet*> buildIncrementer(
      SNLDesign* design,
      const std::vector<SNLBitNet*>& inBits,
      const std::vector<SNLBitNet*>& sumOutBits,
      const std::vector<SNLBitNet*>& carryOutBits,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (sumOutBits.size() != inBits.size() || carryOutBits.size() != inBits.size()) {
        throw SNLSVConstructorException("Internal error: invalid incrementer scratch nets"); // LCOV_EXCL_LINE
      }
      std::vector<SNLBitNet*> sumBits;
      sumBits.reserve(inBits.size());
      // Incrementing by 1: FA(A=bit, B=0, CI=carry), CI_lsb=1
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* carry   = static_cast<SNLBitNet*>(getConstNet(design, true));
      for (size_t i = 0; i < inBits.size(); ++i) {
        auto* sumNet = sumOutBits[i];
        auto* carryNet = carryOutBits[i];
        createFAInstance(design, inBits[i], const0, carry, sumNet, carryNet, sourceRange);
        sumBits.push_back(sumNet);
        carry = carryNet;
      }
      return sumBits;
    }

    struct AssignAction {
      const Expression* lhs {nullptr};
      const Expression* rhs {nullptr};
      int8_t stepDelta {0};
      std::optional<slang::ast::BinaryOperator> compoundOp {};
    };

    struct AlwaysFFChain {
      const Expression* lhs {nullptr};
      const Expression* resetCond {nullptr};
      AssignAction resetAction {};
      const Expression* enableCond {nullptr};
      AssignAction enableAction {};
      AssignAction defaultAction {};
      bool hasDefault {false};
    };

    struct AlwaysLatchPattern {
      const Expression* lhs {nullptr};
      const Expression* enableCond {nullptr};
      AssignAction dataAction {};
      AssignAction defaultAction {};
      bool hasDefault {false};
    };

    const Statement* unwrapStatement(const Statement& stmt) const {
      const Statement* current = &stmt;
      while (current) {
        if (current->kind == slang::ast::StatementKind::Block) {
          current = &current->as<slang::ast::BlockStatement>().body;
          continue;
        }
        if (current->kind == slang::ast::StatementKind::List) {
          const auto& list = current->as<slang::ast::StatementList>().list;
          if (list.size() == 1) {
            // LCOV_EXCL_START
            current = list[0];
            continue;
          } // LCOV_EXCL_STOP
        }
        break;
      }
      return current;
    }

    bool isIgnorableSequentialTimingStatement(const Statement& stmt) const {
      switch (stmt.kind) {
        case slang::ast::StatementKind::ConcurrentAssertion:
        case slang::ast::StatementKind::ImmediateAssertion:
          return true;
        case slang::ast::StatementKind::ExpressionStatement: {
          const auto& expr = stmt.as<slang::ast::ExpressionStatement>().expr;
          if (expr.kind != slang::ast::ExpressionKind::Call) {
            return false;
          }
          return expr.as<slang::ast::CallExpression>().isSystemCall();
        }
        default:
          return false;
      }
    }

    bool isIgnorableSequentialStatementTree(const Statement& stmt) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!isIgnorableSequentialStatementTree(*item)) {
            return false;
          }
        }
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& conditional = current->as<slang::ast::ConditionalStatement>();
        if (!isIgnorableSequentialStatementTree(conditional.ifTrue)) {
          return false;
        }
        return !conditional.ifFalse ||
               isIgnorableSequentialStatementTree(*conditional.ifFalse);
      }
      if (current->kind == slang::ast::StatementKind::Case) {
        const auto& caseStmt = current->as<slang::ast::CaseStatement>();
        for (const auto& item : caseStmt.items) {
          if (!isIgnorableSequentialStatementTree(*item.stmt)) {
            return false;
          }
        }
        return !caseStmt.defaultCase ||
               isIgnorableSequentialStatementTree(*caseStmt.defaultCase);
      }
      return false;
    }

    struct CombinationalSubtreeSummary {
      std::unordered_set<const slang::ast::ValueSymbol*> affectedSymbols {};
      bool mayBreak {false};
      bool hasUnknownEffects {false};
      bool computed {false};
    };

    using CombinationalSubtreeSummaryCache =
      std::unordered_map<const Statement*, CombinationalSubtreeSummary>;

    const CombinationalSubtreeSummary& getOrComputeCombinationalSubtreeSummary(
      const Statement& stmt,
      CombinationalSubtreeSummaryCache& cache,
      const std::unordered_set<const slang::ast::ValueSymbol*>* ignoredSymbols = nullptr) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        current = &stmt; // LCOV_EXCL_LINE
      }

      auto [it, inserted] = cache.try_emplace(current);
      auto& summary = it->second;
      if (!inserted && summary.computed) {
        return summary;
      }

      summary = {};
      summary.computed = true;

      auto mergeSummary = [&](const CombinationalSubtreeSummary& child) {
        summary.mayBreak = summary.mayBreak || child.mayBreak;
        summary.hasUnknownEffects = summary.hasUnknownEffects || child.hasUnknownEffects;
        summary.affectedSymbols.insert(child.affectedSymbols.begin(), child.affectedSymbols.end());
      };

      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty) {
        return summary;
      }

      if (current->kind == slang::ast::StatementKind::VariableDeclaration) {
        const auto& declStmt = current->as<slang::ast::VariableDeclStatement>();
        if (!declStmt.symbol.getInitializer()) {
          return summary;
        }
        if (!ignoredSymbols || !ignoredSymbols->contains(&declStmt.symbol)) {
          summary.affectedSymbols.insert(&declStmt.symbol);
        }
        return summary;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          mergeSummary(getOrComputeCombinationalSubtreeSummary(*item, cache, ignoredSymbols));
        }
        return summary;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        mergeSummary(
          getOrComputeCombinationalSubtreeSummary(forStmt.body, cache, ignoredSymbols));
        return summary;
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& conditional = current->as<slang::ast::ConditionalStatement>();
        mergeSummary(
          getOrComputeCombinationalSubtreeSummary(
            conditional.ifTrue,
            cache,
            ignoredSymbols));
        if (conditional.ifFalse) {
          mergeSummary(
            getOrComputeCombinationalSubtreeSummary(
              *conditional.ifFalse,
              cache,
              ignoredSymbols));
        }
        return summary;
      }

      if (current->kind == slang::ast::StatementKind::Case) {
        const auto& caseStmt = current->as<slang::ast::CaseStatement>();
        for (const auto& item : caseStmt.items) {
          mergeSummary(
            getOrComputeCombinationalSubtreeSummary(
              *item.stmt,
              cache,
              ignoredSymbols));
        }
        if (caseStmt.defaultCase) {
          mergeSummary(
            getOrComputeCombinationalSubtreeSummary(
              *caseStmt.defaultCase,
              cache,
              ignoredSymbols));
        }
        return summary;
      }

      if (current->kind == slang::ast::StatementKind::Break) {
        summary.mayBreak = true;
        return summary;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (extractAssignment(*current, lhsExpr, action)) {
        lhsExpr = getTrackedAlwaysCombLHS(lhsExpr);
        if (!shouldIgnoreTrackedLHS(lhsExpr, ignoredSymbols)) {
          const slang::ast::ValueSymbol* symbol = nullptr;
          if (lhsExpr && tryGetRootValueSymbolReference(*lhsExpr, symbol)) {
            summary.affectedSymbols.insert(symbol);
          } else {
            summary.hasUnknownEffects = true; // LCOV_EXCL_LINE
          }
        }
        return summary;
      }

      summary.hasUnknownEffects = true; // LCOV_EXCL_LINE
      return summary;
    }

    const slang::ast::TimedStatement* findTimedStatement(const Statement& stmt) {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return nullptr; // LCOV_EXCL_LINE
      }
      if (current->kind == slang::ast::StatementKind::Timed) {
        return &current->as<slang::ast::TimedStatement>();
      }
      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        const slang::ast::TimedStatement* timedStatement = nullptr;
        const Statement* unsupportedStatement = nullptr;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          const Statement* unwrapped = unwrapStatement(*item);
          if (!unwrapped) {
            continue; // LCOV_EXCL_LINE
          }
          if (unwrapped->kind == slang::ast::StatementKind::Timed) {
            if (timedStatement) {
              unsupportedStatement = unwrapped;
              break;
            }
            timedStatement = &unwrapped->as<slang::ast::TimedStatement>();
            continue;
          }
          if (isIgnorableSequentialTimingStatement(*unwrapped)) {
            continue;
          }
          unsupportedStatement = unwrapped;
          break;
        }
        if (!unsupportedStatement) {
          return timedStatement;
        }
        std::ostringstream reason;
        reason << "Unsupported statement list while extracting sequential timing control"
               << " (size=" << list.size();
        if (!list.empty() && list[0]) {
          reason << ", first kind=" << list[0]->kind;
        }
        reason << ", unsupported kind=" << unsupportedStatement->kind;
        reason << ")";
        reportUnsupportedElement(
          reason.str(),
          getSourceRange(*current));
        return nullptr;
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return nullptr;
      }
      std::ostringstream reason;
      reason << "Unsupported statement while extracting sequential timing control"
             << " (kind=" << current->kind
             << ", expected timed '@(...)' statement)";
      reportUnsupportedElement(
        reason.str(),
        getSourceRange(*current));
      return nullptr;
    }

    bool extractAssignment(const Statement& stmt, const Expression*& lhs, AssignAction& action) const {
      auto current = unwrapStatement(stmt);
      if (!current) {
        return false; // LCOV_EXCL_LINE
      }
      if (current->kind != slang::ast::StatementKind::ExpressionStatement) {
        return false;
      }
      const auto& expr = current->as<slang::ast::ExpressionStatement>().expr;
      if (expr.kind == slang::ast::ExpressionKind::Assignment) {
        const auto& assign = expr.as<slang::ast::AssignmentExpression>();
        lhs = &assign.left();
        action.lhs = lhs;
        action.rhs = &assign.right();
        action.stepDelta = 0;
        action.compoundOp = assign.op;
        return true;
      }
      if (expr.kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unary = expr.as<slang::ast::UnaryExpression>();
        if (unary.op == slang::ast::UnaryOperator::Postincrement ||
            unary.op == slang::ast::UnaryOperator::Preincrement ||
            unary.op == slang::ast::UnaryOperator::Postdecrement ||
            unary.op == slang::ast::UnaryOperator::Predecrement) {
          lhs = &unary.operand();
          action.lhs = lhs;
          action.rhs = &unary.operand();
          action.stepDelta =
            (unary.op == slang::ast::UnaryOperator::Postdecrement ||
             unary.op == slang::ast::UnaryOperator::Predecrement)
            ? static_cast<int8_t>(-1)
            : static_cast<int8_t>(1);
          action.compoundOp = std::nullopt;
          return true;
        }
      }
      return false;
    }

    bool sameLhs(const Expression* left, const Expression* right) const {
      if (!left || !right) {
        return false; // LCOV_EXCL_LINE
      }
      const auto* leftExpr = stripConversions(*left);
      const auto* rightExpr = stripConversions(*right);
      if (!leftExpr || !rightExpr) {
        return false; // LCOV_EXCL_LINE
      }
      if (leftExpr && rightExpr &&
          slang::ast::ValueExpressionBase::isKind(leftExpr->kind) &&
          slang::ast::ValueExpressionBase::isKind(rightExpr->kind)) {
        const auto& leftSym = leftExpr->as<slang::ast::ValueExpressionBase>().symbol;
        const auto& rightSym = rightExpr->as<slang::ast::ValueExpressionBase>().symbol;
        return &leftSym == &rightSym || leftSym.name == rightSym.name;
      }
      if (leftExpr->kind == slang::ast::ExpressionKind::MemberAccess &&
          rightExpr->kind == slang::ast::ExpressionKind::MemberAccess) {
        const auto& leftMember = leftExpr->as<slang::ast::MemberAccessExpression>();
        const auto& rightMember = rightExpr->as<slang::ast::MemberAccessExpression>();
        if (&leftMember.member != &rightMember.member &&
            leftMember.member.name != rightMember.member.name) {
          return false;
        }
        return sameLhs(&leftMember.value(), &rightMember.value());
      }
      return left == right;
    }

    bool isForLoopControlSymbolRef(const Expression& expr, const Symbol& symbol) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped || !slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        return false;
      }
      const auto& exprSymbol = stripped->as<slang::ast::ValueExpressionBase>().symbol;
      return &exprSymbol == &symbol;
    }

    bool isActiveForLoopVariableExpr(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }
      if (getActiveForLoopConstant(*stripped).has_value()) {
        return true;
      }
      if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        return isActiveForLoopVariableExpr(
          stripped->as<slang::ast::UnaryExpression>().operand());
      }
      if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
        return isActiveForLoopVariableExpr(binaryExpr.left()) ||
               isActiveForLoopVariableExpr(binaryExpr.right());
      }
      return false;
    }

    bool extractForLoopInitializerValue(
      const Expression& initializerExpr,
      const Symbol* expectedLoopSymbol,
      const Symbol*& resolvedLoopSymbol,
      int64_t& loopValue) const {
      resolvedLoopSymbol = expectedLoopSymbol;
      if (getConstantInt64(initializerExpr, loopValue)) {
        return resolvedLoopSymbol != nullptr;
      }

      const auto* strippedInit = stripConversions(initializerExpr);
      if (!strippedInit || strippedInit->kind != slang::ast::ExpressionKind::Assignment) {
        return false;
      }

      const auto& assignExpr = strippedInit->as<slang::ast::AssignmentExpression>();
      const auto* lhsExpr = stripConversions(assignExpr.left());
      if (!lhsExpr || !slang::ast::ValueExpressionBase::isKind(lhsExpr->kind)) {
        return false;
      }

      const auto& lhsSymbol = lhsExpr->as<slang::ast::ValueExpressionBase>().symbol;
      if (expectedLoopSymbol && &lhsSymbol != expectedLoopSymbol) {
        return false;
      }

      if (!getConstantInt64(assignExpr.right(), loopValue)) {
        return false;
      }

      resolvedLoopSymbol = &lhsSymbol;
      return true;
    }

    bool extractForLoopControl(
      const slang::ast::ForLoopStatement& forStmt,
      const Symbol*& loopSymbol,
      int64_t& loopValue,
      std::string& failureReason) const {
      loopSymbol = nullptr;
      loopValue = 0;

      if (forStmt.loopVars.size() == 1 && forStmt.initializers.size() <= 1) {
        const auto* loopVar = forStmt.loopVars.front();
        if (!loopVar) {
          // LCOV_EXCL_START
          failureReason = "unsupported for-loop with null control variable";
          return false;
          // LCOV_EXCL_STOP
        }

        const Expression* initializer = loopVar->getInitializer();
        // Slang materializes a non-null initializer expression for declared
        // for-loop variables in current versions, so this fallback and
        // missing-initializer diagnostic are defensive and not reachable.
        // LCOV_EXCL_START
        if (!initializer && forStmt.initializers.size() == 1) {
          initializer = forStmt.initializers.front();
        }
        if (!initializer) {
          std::ostringstream reason;
          reason << "unsupported for-loop control variable without initializer"
                 << " (name=" << std::string(loopVar->name) << ")";
          failureReason = reason.str();
          return false;
        }
        // LCOV_EXCL_STOP
        const Symbol* resolvedLoopSymbol = loopVar;
        if (!extractForLoopInitializerValue(*initializer, loopVar, resolvedLoopSymbol, loopValue)) {
          std::ostringstream reason;
          reason << "unsupported non-constant for-loop initializer for control variable '"
                 << std::string(loopVar->name) << "'";
          failureReason = reason.str();
          return false;
        }
        loopSymbol = resolvedLoopSymbol;
        return true;
      }

      if (forStmt.loopVars.empty() && forStmt.initializers.size() == 1) {
        const auto* initializer = forStmt.initializers.front();
        const Symbol* resolvedLoopSymbol = nullptr;
        if (!extractForLoopInitializerValue(*initializer, nullptr, resolvedLoopSymbol, loopValue)) {
          failureReason = "unsupported non-constant for-loop initializer RHS expression";
          return false;
        }
        loopSymbol = resolvedLoopSymbol;
        return true;
      }

      std::ostringstream reason;
      reason << "unsupported for-loop initializer structure"
             << " (loop_vars=" << forStmt.loopVars.size()
             << ", initializers=" << forStmt.initializers.size() << ")";
      failureReason = reason.str();
      return false;
    }

    bool evaluateForLoopStopCondition(
      const Expression& stopExpr,
      const Symbol& loopSymbol,
      int64_t loopValue,
      bool& shouldExecuteBody,
      std::string& failureReason) const {
      const auto* strippedStopExpr = stripConversions(stopExpr);
      if (!strippedStopExpr ||
          strippedStopExpr->kind != slang::ast::ExpressionKind::BinaryOp) {
        failureReason = "unsupported for-loop stop expression (expected binary comparison)";
        return false;
      }

      const auto& binaryExpr = strippedStopExpr->as<slang::ast::BinaryExpression>();
      const bool leftIsLoopVar = isForLoopControlSymbolRef(binaryExpr.left(), loopSymbol);
      const bool rightIsLoopVar = isForLoopControlSymbolRef(binaryExpr.right(), loopSymbol);
      if (leftIsLoopVar == rightIsLoopVar) {
        failureReason = "unsupported for-loop stop expression (control variable not isolated)";
        return false;
      }

      int64_t otherValue = 0;
      if (!getConstantInt64(leftIsLoopVar ? binaryExpr.right() : binaryExpr.left(), otherValue)) {
        failureReason = "unsupported non-constant for-loop bound expression";
        return false;
      }

      const int64_t lhsValue = leftIsLoopVar ? loopValue : otherValue;
      const int64_t rhsValue = leftIsLoopVar ? otherValue : loopValue;
      switch (binaryExpr.op) {
        case slang::ast::BinaryOperator::LessThan:
          shouldExecuteBody = lhsValue < rhsValue;
          return true;
        case slang::ast::BinaryOperator::LessThanEqual:
          shouldExecuteBody = lhsValue <= rhsValue;
          return true;
        case slang::ast::BinaryOperator::GreaterThan:
          shouldExecuteBody = lhsValue > rhsValue;
          return true;
        case slang::ast::BinaryOperator::GreaterThanEqual:
          shouldExecuteBody = lhsValue >= rhsValue;
          return true;
        case slang::ast::BinaryOperator::Equality:
          shouldExecuteBody = lhsValue == rhsValue;
          return true;
        case slang::ast::BinaryOperator::Inequality:
          shouldExecuteBody = lhsValue != rhsValue;
          return true;
        default:
          break;
      }

      std::ostringstream reason;
      reason << "unsupported for-loop comparison operator in stop expression: "
             << slang::ast::OpInfo::getText(binaryExpr.op);
      failureReason = reason.str();
      return false;
    }

    bool evaluateForLoopStepRHS(
      const Expression& rhsExpr,
      const Symbol& loopSymbol,
      int64_t loopValue,
      int64_t& nextLoopValue) const {
      if (isForLoopControlSymbolRef(rhsExpr, loopSymbol)) {
        nextLoopValue = loopValue;
        return true;
      }

      if (getConstantInt64(rhsExpr, nextLoopValue)) {
        return true;
      }

      const auto* strippedRHSExpr = stripConversions(rhsExpr);
      if (!strippedRHSExpr ||
          strippedRHSExpr->kind != slang::ast::ExpressionKind::BinaryOp) {
        return false;
      }

      const auto& rhsBinaryExpr = strippedRHSExpr->as<slang::ast::BinaryExpression>();
      int64_t constantOperand = 0;
      switch (rhsBinaryExpr.op) {
        case slang::ast::BinaryOperator::Add:
          if (isForLoopControlSymbolRef(rhsBinaryExpr.left(), loopSymbol) &&
              getConstantInt64(rhsBinaryExpr.right(), constantOperand)) {
            nextLoopValue = loopValue + constantOperand;
            return true;
          }
          if (isForLoopControlSymbolRef(rhsBinaryExpr.right(), loopSymbol) &&
              getConstantInt64(rhsBinaryExpr.left(), constantOperand)) {
            nextLoopValue = constantOperand + loopValue;
            return true;
          }
          return false;
        case slang::ast::BinaryOperator::Subtract:
          if (isForLoopControlSymbolRef(rhsBinaryExpr.left(), loopSymbol) &&
              getConstantInt64(rhsBinaryExpr.right(), constantOperand)) {
            nextLoopValue = loopValue - constantOperand;
            return true;
          }
          if (isForLoopControlSymbolRef(rhsBinaryExpr.right(), loopSymbol) &&
              getConstantInt64(rhsBinaryExpr.left(), constantOperand)) {
            nextLoopValue = constantOperand - loopValue;
            return true;
          }
          return false;
        default:
          return false;
      }
    }

    bool applyForLoopStepExpression(
      const Expression& stepExpr,
      const Symbol& loopSymbol,
      int64_t& loopValue,
      std::string& failureReason) const {
      const auto* strippedStepExpr = stripConversions(stepExpr);
      if (!strippedStepExpr) {
        // LCOV_EXCL_START
        failureReason = "unsupported null for-loop step expression";
        return false;
        // LCOV_EXCL_STOP
      }

      if (strippedStepExpr->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = strippedStepExpr->as<slang::ast::UnaryExpression>();
        if (!isForLoopControlSymbolRef(unaryExpr.operand(), loopSymbol)) {
          failureReason = "unsupported for-loop unary step on non-control variable";
          return false;
        }
        if (unaryExpr.op == slang::ast::UnaryOperator::Preincrement ||
            unaryExpr.op == slang::ast::UnaryOperator::Postincrement) {
          ++loopValue;
          return true;
        }
        if (unaryExpr.op == slang::ast::UnaryOperator::Predecrement ||
            unaryExpr.op == slang::ast::UnaryOperator::Postdecrement) {
          --loopValue;
          return true;
        }
      }

      if (strippedStepExpr->kind == slang::ast::ExpressionKind::Assignment) {
        const auto& assignExpr = strippedStepExpr->as<slang::ast::AssignmentExpression>();
        if (!isForLoopControlSymbolRef(assignExpr.left(), loopSymbol)) {
          failureReason = "unsupported for-loop assignment step on non-control variable";
          return false;
        }

        if (assignExpr.isCompound()) {
          int64_t rhsValue = 0;
          if (!getConstantInt64(assignExpr.right(), rhsValue)) {
            failureReason = "unsupported non-constant compound for-loop assignment step";
            return false;
          }
        }

        int64_t nextLoopValue = 0;
        if (evaluateForLoopStepRHS(assignExpr.right(), loopSymbol, loopValue, nextLoopValue)) {
          loopValue = nextLoopValue;
          return true;
        }
      }

      failureReason = "unsupported for-loop step expression";
      return false;
    }

    bool unrollForLoopStatement(
      const slang::ast::ForLoopStatement& forStmt,
      const std::function<bool()>& bodyCallback,
      std::string& failureReason) const {
      const Symbol* loopSymbol = nullptr;
      int64_t loopValue = 0;
      if (!extractForLoopControl(forStmt, loopSymbol, loopValue, failureReason)) {
        return false;
      }

      if (!forStmt.stopExpr) {
        failureReason = "unsupported for-loop without stop condition";
        return false;
      }
      if (forStmt.steps.size() != 1 || !forStmt.steps.front()) {
        failureReason = "unsupported for-loop step count (only one step expression is supported)";
        return false;
      }

      activeForLoopBreaks_.push_back(false);
      auto popBreakContext = [&]() {
        if (!activeForLoopBreaks_.empty()) {
          activeForLoopBreaks_.pop_back();
        }
      };

      constexpr size_t kMaxForLoopUnrollIterations = 4096;
      size_t iterationCount = 0;
      while (true) {
        setCurrentForLoopBreakRequested(false);

        bool shouldExecuteBody = false;
        if (!evaluateForLoopStopCondition(
              *forStmt.stopExpr,
              *loopSymbol,
              loopValue,
              shouldExecuteBody,
              failureReason)) {
          popBreakContext();
          return false;
        }

        if (!shouldExecuteBody) {
          popBreakContext();
          return true;
        }

        if (iterationCount++ >= kMaxForLoopUnrollIterations) {
          std::ostringstream reason;
          reason << "for-loop unroll iteration limit exceeded (" << kMaxForLoopUnrollIterations
                 << ")";
          failureReason = reason.str();
          popBreakContext();
          return false;
        }

        activeForLoopConstants_.emplace_back(loopSymbol, loopValue);
        const bool bodyOk = bodyCallback();
        activeForLoopConstants_.pop_back();
        if (!bodyOk) {
          popBreakContext();
          return false;
        }
        if (isCurrentForLoopBreakRequested()) {
          popBreakContext();
          return true;
        }

        if (!applyForLoopStepExpression(
              *forStmt.steps.front(),
              *loopSymbol,
              loopValue,
              failureReason)) {
          popBreakContext();
          return false;
        }
      }
    }

    const Expression* getTrackedAlwaysCombLHS(const Expression* lhsExpr) const {
      if (!lhsExpr) {
        return nullptr; // LCOV_EXCL_LINE
      }
      const auto* stripped = stripConversions(*lhsExpr);
      if (!stripped) {
        return lhsExpr;
      }

      const Expression* baseExpr = nullptr;
      if (stripped->kind == slang::ast::ExpressionKind::ElementSelect) {
        const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
        baseExpr = stripConversions(elementExpr.value());
        if (isActiveForLoopVariableExpr(elementExpr.selector())) {
          if (baseExpr &&
              (slang::ast::ValueExpressionBase::isKind(baseExpr->kind) ||
               baseExpr->kind == slang::ast::ExpressionKind::MemberAccess)) {
            return baseExpr;
          }
        }
        int32_t selectedIndex = 0;
        if (getConstantInt32(elementExpr.selector(), selectedIndex)) {
          return lhsExpr;
        }
      } else if (stripped->kind == slang::ast::ExpressionKind::RangeSelect) {
        const auto& rangeExpr = stripped->as<slang::ast::RangeSelectExpression>();
        baseExpr = stripConversions(rangeExpr.value());
        if (isActiveForLoopVariableExpr(rangeExpr.left()) ||
            isActiveForLoopVariableExpr(rangeExpr.right())) {
          if (baseExpr &&
              (slang::ast::ValueExpressionBase::isKind(baseExpr->kind) ||
               baseExpr->kind == slang::ast::ExpressionKind::MemberAccess)) {
            return baseExpr;
          }
        }
        int32_t left = 0;
        int32_t right = 0;
        if (getConstantInt32(rangeExpr.left(), left) &&
            getConstantInt32(rangeExpr.right(), right)) {
          return lhsExpr;
        }
      } else {
        return lhsExpr;
      }

      if (!baseExpr) {
        return lhsExpr; // LCOV_EXCL_LINE
      }
      return baseExpr;
    }

    static size_t hashCombine(size_t seed, size_t value) {
      return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    struct DynamicElementSelectCacheKey {
      SNLDesign* design {nullptr};
      SNLNet* valueNet {nullptr};
      size_t targetWidth {0};
      int32_t rangeLeft {0};
      int32_t rangeRight {0};
      std::vector<SNLBitNet*> selectorBits {};

      bool operator==(const DynamicElementSelectCacheKey& other) const {
        return design == other.design &&
               valueNet == other.valueNet &&
               targetWidth == other.targetWidth &&
               rangeLeft == other.rangeLeft &&
               rangeRight == other.rangeRight &&
               selectorBits == other.selectorBits;
      }
    };

    struct DynamicElementSelectCacheKeyHash {
      size_t operator()(const DynamicElementSelectCacheKey& key) const noexcept {
        size_t hashValue = std::hash<SNLDesign*>{}(key.design);
        hashValue =
          SNLSVConstructorImpl::hashCombine(hashValue, std::hash<SNLNet*>{}(key.valueNet));
        hashValue =
          SNLSVConstructorImpl::hashCombine(hashValue, std::hash<size_t>{}(key.targetWidth));
        hashValue =
          SNLSVConstructorImpl::hashCombine(hashValue, std::hash<int32_t>{}(key.rangeLeft));
        hashValue =
          SNLSVConstructorImpl::hashCombine(hashValue, std::hash<int32_t>{}(key.rangeRight));
        for (auto* bit : key.selectorBits) {
          hashValue =
            SNLSVConstructorImpl::hashCombine(hashValue, std::hash<SNLBitNet*>{}(bit));
        }
        return hashValue;
      }
    };

    SNLBitNet* buildSelectorEqualsIndexBit(
      SNLDesign* design,
      const std::vector<SNLBitNet*>& selectorBits,
      int32_t index,
      const std::optional<slang::SourceRange>& sourceRange) {
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

      SNLBitNet* equalsIndexBit = const1;
      for (size_t bitIndex = 0; bitIndex < selectorBits.size(); ++bitIndex) {
        const bool indexOne =
          ((static_cast<uint64_t>(static_cast<uint32_t>(index)) >> bitIndex) & 1ULL) != 0ULL;
        auto* selectorBit = selectorBits[bitIndex];
        auto* expectedBit = indexOne ? const1 : const0;

        SNLBitNet* bitEquals = nullptr;
        if (selectorBit == expectedBit) {
          bitEquals = const1;
        } else if (selectorBit == (indexOne ? const0 : const1)) {
          bitEquals = const0;
        } else {
          auto* xnorBit = SNLScalarNet::create(design);
          annotateSourceInfo(xnorBit, sourceRange);
          bitEquals = getSingleBitNet(createBinaryGate(
            design,
            NLDB0::GateType(NLDB0::GateType::Xnor),
            selectorBit,
            expectedBit,
            xnorBit,
            sourceRange));
          if (!bitEquals) {
            return nullptr; // LCOV_EXCL_LINE
          }
        }

        if (equalsIndexBit == const0 || bitEquals == const0) {
          equalsIndexBit = const0;
          break;
        }
        if (equalsIndexBit == const1) {
          equalsIndexBit = bitEquals;
          continue;
        }
        if (bitEquals == const1) {
          continue;
        }

        auto* andBit = SNLScalarNet::create(design);
        annotateSourceInfo(andBit, sourceRange);
        equalsIndexBit = getSingleBitNet(createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::And),
          equalsIndexBit,
          bitEquals,
          andBit,
          sourceRange));
        if (!equalsIndexBit) {
          return nullptr; // LCOV_EXCL_LINE
        }
      }

      return equalsIndexBit;
    }

    bool extractAlwaysFFChain(const Statement& stmt, AlwaysFFChain& chain) const {
      auto current = unwrapStatement(stmt);
      if (!current) {
        return false;
      }
      if (current->kind != slang::ast::StatementKind::Conditional) {
        const Expression* lhs = nullptr;
        AssignAction action;
        if (!extractAssignment(*current, lhs, action)) {
          return false;
        }
        const auto* strippedLHS = lhs ? stripConversions(*lhs) : nullptr;
        if (!strippedLHS ||
            strippedLHS->kind != slang::ast::ExpressionKind::NamedValue) {
          return false;
        }
        if (!getIntegralExpressionBitWidth(*strippedLHS)) {
          return false;
        }
        chain.lhs = lhs;
        chain.defaultAction = action;
        chain.hasDefault = true;
        return true;
      }
      const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
      if (condStmt.conditions.size() != 1) {
        return false; // LCOV_EXCL_LINE
      }
      const Expression* lhs = nullptr;
      AssignAction resetAction;
      if (!extractAssignment(condStmt.ifTrue, lhs, resetAction)) {
        return false;
      }
      chain.lhs = lhs;
      chain.resetCond = condStmt.conditions[0].expr;
      chain.resetAction = resetAction;

      if (!condStmt.ifFalse) {
        return true;
      }

      const Statement* falseStmt = unwrapStatement(*condStmt.ifFalse);
      if (!falseStmt) {
        return true; // LCOV_EXCL_LINE
      }

      if (falseStmt->kind == slang::ast::StatementKind::Conditional) {
        const auto& enableStmt = falseStmt->as<slang::ast::ConditionalStatement>();
        if (enableStmt.conditions.size() != 1) {
          return false; // LCOV_EXCL_LINE
        }
        const Expression* enableLhs = nullptr;
        AssignAction enableAction;
        if (!extractAssignment(enableStmt.ifTrue, enableLhs, enableAction)) {
          return false;
        }
        if (!sameLhs(enableLhs, chain.lhs)) {
          return false;
        }
        chain.enableCond = enableStmt.conditions[0].expr;
        chain.enableAction = enableAction;

        if (enableStmt.ifFalse) {
          const Expression* defaultLhs = nullptr;
          AssignAction defaultAction;
          if (!extractAssignment(*enableStmt.ifFalse, defaultLhs, defaultAction)) {
            return false;
          }
          if (!sameLhs(defaultLhs, chain.lhs)) {
            return false;
          }
          chain.defaultAction = defaultAction;
          chain.hasDefault = true;
        }
        return true;
      }

      const Expression* defaultLhs = nullptr;
      AssignAction defaultAction;
      if (!extractAssignment(*falseStmt, defaultLhs, defaultAction)) {
        return false;
      }
      if (!sameLhs(defaultLhs, chain.lhs)) {
        return false;
      }
      chain.defaultAction = defaultAction;
      chain.hasDefault = true;
      return true;
    }

    bool extractAlwaysLatchPattern(
      const Statement& stmt,
      AlwaysLatchPattern& pattern) const {
      auto current = unwrapStatement(stmt);
      if (!current || current->kind != slang::ast::StatementKind::Conditional) {
        return false;
      }
      const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
      if (condStmt.conditions.size() != 1 || condStmt.conditions.front().pattern) {
        return false;
      }

      const Expression* lhs = nullptr;
      AssignAction dataAction;
      if (!extractAssignment(condStmt.ifTrue, lhs, dataAction)) {
        return false;
      }
      pattern.lhs = lhs;
      pattern.enableCond = condStmt.conditions.front().expr;
      pattern.dataAction = dataAction;

      if (!condStmt.ifFalse) {
        return true;
      }

      const Expression* defaultLhs = nullptr;
      AssignAction defaultAction;
      if (!extractAssignment(*condStmt.ifFalse, defaultLhs, defaultAction)) {
        return false;
      }
      if (!sameLhs(defaultLhs, pattern.lhs)) {
        return false;
      }
      pattern.defaultAction = defaultAction;
      pattern.hasDefault = true;
      return true;
    }

    bool needsIncrementerForAction(
      SNLDesign* design,
      SNLNet* lhsNet,
      const AssignAction& action) {
      if (action.stepDelta > 0) {
        return true;
      }
      if (!action.rhs) {
        return false; // LCOV_EXCL_LINE
      }
      const auto* rhsExpr = stripConversions(*action.rhs);
      if (!rhsExpr || rhsExpr->kind != slang::ast::ExpressionKind::BinaryOp) {
        return false;
      }
      const auto& bin = rhsExpr->as<slang::ast::BinaryExpression>();
      if (bin.op != slang::ast::BinaryOperator::Add) {
        return false;
      }
      auto leftNet = resolveExpressionNet(design, bin.left());
      auto rightNet = resolveExpressionNet(design, bin.right());
      const auto* leftExpr = stripConversions(bin.left());
      const auto* rightExpr = stripConversions(bin.right());
      bool constOne = false;
      if (leftNet == lhsNet && rightExpr && getConstantBit(*rightExpr, constOne) && constOne) {
        return true;
      }
      if (rightNet == lhsNet && leftExpr && getConstantBit(*leftExpr, constOne) && constOne) {
        return true;
      }
      return false;
    }

    bool collectDirectAssignments(
      const Statement& stmt,
      std::vector<std::pair<const Expression*, AssignAction>>& assignments,
      std::string* failureReason = nullptr) const {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };

      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Block) {
        return collectDirectAssignments(
          current->as<slang::ast::BlockStatement>().body,
          assignments,
          failureReason);
      }
      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!collectDirectAssignments(*item, assignments, failureReason)) {
            return false;
          }
        }
        return true;
      }

      const Expression* lhs = nullptr;
      AssignAction action;
      if (extractAssignment(*current, lhs, action)) {
        assignments.emplace_back(lhs, action);
        return true;
      }

      std::ostringstream reason;
      reason << "expected direct assignment statement in reset branch (kind="
             << current->kind << ")";
      setFailureReason(reason.str());
      return false;
    }

    bool getSingleLHSFallbackPathAssignmentMax(
      const Statement& stmt,
      const Expression& trackedLhs,
      size_t& maxAssignments,
      std::string* failureReason = nullptr) const {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };

      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        maxAssignments = 0;
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current) ||
          current->kind == slang::ast::StatementKind::Empty ||
          current->kind == slang::ast::StatementKind::VariableDeclaration) {
        maxAssignments = 0;
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Block) {
        return getSingleLHSFallbackPathAssignmentMax(
          current->as<slang::ast::BlockStatement>().body,
          trackedLhs,
          maxAssignments,
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::List) {
        size_t totalAssignments = 0;
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          size_t itemAssignments = 0;
          if (!getSingleLHSFallbackPathAssignmentMax(
                *item,
                trackedLhs,
                itemAssignments,
                failureReason)) {
            return false;
          }
          totalAssignments += itemAssignments;
          if (totalAssignments > 1) {
            setFailureReason(
              "single-LHS fallback path contains multiple direct assignments");
            return false;
          }
        }
        maxAssignments = totalAssignments;
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        size_t trueAssignments = 0;
        if (!getSingleLHSFallbackPathAssignmentMax(
              condStmt.ifTrue,
              trackedLhs,
              trueAssignments,
              failureReason)) {
          return false;
        }
        size_t falseAssignments = 0;
        if (condStmt.ifFalse &&
            !getSingleLHSFallbackPathAssignmentMax(
              *condStmt.ifFalse,
              trackedLhs,
              falseAssignments,
              failureReason)) {
          return false;
        }
        maxAssignments = std::max(trueAssignments, falseAssignments);
        return true;
      }

      const Expression* lhs = nullptr;
      AssignAction action;
      if (extractAssignment(*current, lhs, action)) {
        maxAssignments = sameLhs(lhs, &trackedLhs) ? 1 : 0;
        return true;
      }

      std::ostringstream reason;
      reason << "single-LHS fallback encountered unsupported statement kind "
             << "(kind=" << current->kind << ")";
      setFailureReason(reason.str());
      return false;
    }

    bool applySequentialStatementForLhs(
      SNLDesign* design,
      const Statement& stmt,
      const Expression& lhsExpr,
      SNLNet* lhsNet,
      const std::vector<SNLBitNet*>& lhsBits,
      const std::string& baseName,
      std::vector<SNLBitNet*>& dataBits,
      std::vector<SNLBitNet*>& incrementerBits,
      size_t& tempIndex,
      std::string& failureReason) {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::VariableDeclaration) {
        // Local variable declarations inside always_ff blocks are bookkeeping
        // statements and do not directly drive tracked sequential LHS targets.
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!applySequentialStatementForLhs(
                design,
                *item,
                lhsExpr,
                lhsNet,
                lhsBits,
                baseName,
                dataBits,
                incrementerBits,
                tempIndex,
                failureReason)) {
            return false;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        return unrollForLoopStatement(
          forStmt,
          [&]() {
            return applySequentialStatementForLhs(
              design,
              forStmt.body,
              lhsExpr,
              lhsNet,
              lhsBits,
              baseName,
              dataBits,
              incrementerBits,
              tempIndex,
              failureReason);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        const auto& conditionExpr = *condStmt.conditions[0].expr;
        int64_t constantConditionValue = 0;
        if (getConstantInt64(conditionExpr, constantConditionValue)) {
          const Statement* selectedStmt = constantConditionValue ? &condStmt.ifTrue : condStmt.ifFalse;
          if (!selectedStmt) {
            return true;
          }
          return applySequentialStatementForLhs(
            design,
            *selectedStmt,
            lhsExpr,
            lhsNet,
            lhsBits,
            baseName,
            dataBits,
            incrementerBits,
            tempIndex,
            failureReason);
        }

        std::vector<SNLBitNet*> trueBits = dataBits;
        if (!applySequentialStatementForLhs(
              design,
              condStmt.ifTrue,
              lhsExpr,
              lhsNet,
              lhsBits,
              baseName,
              trueBits,
              incrementerBits,
              tempIndex,
              failureReason)) {
          return false;
        }

        std::vector<SNLBitNet*> falseBits = dataBits;
        if (condStmt.ifFalse) {
          if (!applySequentialStatementForLhs(
                design,
                *condStmt.ifFalse,
                lhsExpr,
                lhsNet,
                lhsBits,
                baseName,
                falseBits,
                incrementerBits,
                tempIndex,
                failureReason)) {
            return false;
          }
        }

        auto condSourceRange = getSourceRange(condStmt);
        auto condBaseName = joinName("cond" + std::to_string(tempIndex++), baseName);
        auto* condNet = resolveConditionNet(
          design,
          conditionExpr,
          condBaseName,
          condSourceRange);
        if (!condNet) {
          std::ostringstream reason;
          reason << "unable to resolve single-bit condition net while lowering conditional"
                 << " (" << describeExpression(conditionExpr) << ")";
          failureReason = reason.str();
          return false;
        }

        std::vector<SNLBitNet*> mergedBits;
        if (!createMux2Instance(
              design,
              condNet,
              falseBits,
              trueBits,
              mergedBits,
              condSourceRange)) {
          return false;
        }
        dataBits = std::move(mergedBits);
        return true;
      }

      const Expression* assignedLHS = nullptr;
      AssignAction action;
      if (extractAssignment(*current, assignedLHS, action)) {
        if (!sameLhs(assignedLHS, &lhsExpr)) {
          const auto* strippedAssignedLHS = stripConversions(*assignedLHS);
          if (strippedAssignedLHS &&
              strippedAssignedLHS->kind == slang::ast::ExpressionKind::ElementSelect) {
            const auto& elementExpr =
              strippedAssignedLHS->as<slang::ast::ElementSelectExpression>();
            const auto* baseExpr = stripConversions(elementExpr.value());
            if (baseExpr && sameLhs(baseExpr, &lhsExpr)) {
              const auto& baseType = baseExpr->type->getCanonicalType();
              if (!baseType.hasFixedRange()) { // LCOV_EXCL_START
                std::ostringstream reason;
                reason
                  << "unsupported sequential element-select assignment base without fixed range: "
                  << describeExpression(*assignedLHS);
                failureReason = reason.str();
                return false;
              } // LCOV_EXCL_STOP

              auto elementWidth = getIntegralExpressionBitWidth(*assignedLHS);
              if (!elementWidth || !*elementWidth) { // LCOV_EXCL_START
                std::ostringstream reason;
                reason << "unable to resolve sequential element-select assignment width for "
                       << describeExpression(*assignedLHS);
                failureReason = reason.str();
                return false;
              } // LCOV_EXCL_STOP

              const auto arrayWidth = static_cast<size_t>(baseType.getFixedRange().width());
              const auto totalSelectedWidth = static_cast<size_t>(*elementWidth) * arrayWidth;
              static_cast<void>(totalSelectedWidth);

              if (action.compoundOp || action.stepDelta != 0 || !action.rhs) {
                std::ostringstream reason;
                reason << "unsupported sequential assignment action for element-select LHS "
                       << describeExpression(*assignedLHS);
                failureReason = reason.str();
                return false;
              }

              std::vector<SNLBitNet*> assignedBits;
              if (!resolveExpressionBits(
                    design,
                    *action.rhs,
                    static_cast<size_t>(*elementWidth),
                    assignedBits) ||
                  assignedBits.size() != static_cast<size_t>(*elementWidth)) {
                std::ostringstream reason;
                reason << "unable to resolve sequential element-select RHS bits for "
                       << describeExpression(*action.rhs)
                       << " (target_width=" << *elementWidth << ")";
                failureReason = reason.str();
                return false;
              }

              auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
              auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
              auto elementSourceRange = getSourceRange(*assignedLHS);
              auto updateSlice = [&](size_t offset, SNLBitNet* selectBit) {
                std::vector<SNLBitNet*> currentBits(
                  dataBits.begin() + static_cast<std::ptrdiff_t>(offset),
                  dataBits.begin() + static_cast<std::ptrdiff_t>(offset + static_cast<size_t>(*elementWidth)));
                if (static_cast<size_t>(*elementWidth) > 1 && selectBit != const0 && selectBit != const1) {
                  std::vector<SNLBitNet*> updatedBits;
                  if (!createMux2Instance(
                        design,
                        selectBit,
                        currentBits,
                        assignedBits,
                        updatedBits,
                        elementSourceRange)) {
                    throw SNLSVConstructorException(
                      "Internal error: failed to build wide mux for sequential element-select assignment");
                  }
                  std::copy(
                    updatedBits.begin(),
                    updatedBits.end(),
                    dataBits.begin() + static_cast<std::ptrdiff_t>(offset));
                  return;
                }
                for (size_t elemBit = 0; elemBit < static_cast<size_t>(*elementWidth); ++elemBit) {
                  auto* candidateBit = assignedBits[elemBit];
                  if (selectBit == const1) {
                    dataBits[offset + elemBit] = candidateBit;
                    continue;
                  }
                  if (selectBit == const0 || dataBits[offset + elemBit] == candidateBit) {
                    continue;
                  }
                  // LCOV_EXCL_START
                  auto* outBit = SNLScalarNet::create(design);
                  annotateSourceInfo(outBit, elementSourceRange);
                  createMux2Instance(
                    design,
                    selectBit,
                    dataBits[offset + elemBit],
                    candidateBit,
                    outBit,
                    elementSourceRange);
                  dataBits[offset + elemBit] = outBit;
                  // LCOV_EXCL_STOP
                }
              };

              int32_t selectedIndex = 0;
              if (getConstantInt32(elementExpr.selector(), selectedIndex)) {
                const auto translated = baseType.getFixedRange().translateIndex(selectedIndex);
                if (translated < 0 ||
                    translated >= static_cast<int32_t>(baseType.getFixedRange().width())) {
                  std::ostringstream reason;
                  reason << "constant element-select index out of range in sequential assignment: "
                         << describeExpression(*assignedLHS);
                  failureReason = reason.str();
                  return false;
                }
                updateSlice(
                  static_cast<size_t>(translated) * static_cast<size_t>(*elementWidth),
                  const1);
                return true;
              }

              auto selectorWidth = getIntegralExpressionBitWidth(elementExpr.selector());
              if (!selectorWidth || !*selectorWidth) {
                // LCOV_EXCL_START
                std::ostringstream reason;
                reason << "unable to resolve dynamic index width in sequential assignment LHS: "
                       << describeExpression(*assignedLHS);
                failureReason = reason.str();
                return false;
                // LCOV_EXCL_STOP
              }

              std::vector<SNLBitNet*> selectorBits;
              if (!resolveExpressionBits(
                    design,
                    elementExpr.selector(),
                    static_cast<size_t>(*selectorWidth),
                    selectorBits) ||
                  selectorBits.size() != static_cast<size_t>(*selectorWidth)) {
                std::ostringstream reason;
                reason << "unable to resolve dynamic index bits in sequential assignment LHS: "
                       << describeExpression(*assignedLHS);
                failureReason = reason.str();
                return false;
              }

              int32_t index = baseType.getFixedRange().right;
              const int32_t end = baseType.getFixedRange().left;
              const int32_t step = index <= end ? 1 : -1;
              while (index != end + step) {
                const auto translated = baseType.getFixedRange().translateIndex(index);
                if (translated >= 0 &&
                    translated < static_cast<int32_t>(baseType.getFixedRange().width())) {
                  auto* equalsIndexBit = buildSelectorEqualsIndexBit(
                    design,
                    selectorBits,
                    index,
                    elementSourceRange);
                  if (!equalsIndexBit) { // LCOV_EXCL_START
                    failureReason =
                      "failed to build selector decode while lowering sequential "
                      "element-select assignment";
                    return false;
                  } // LCOV_EXCL_STOP
                  updateSlice(
                    static_cast<size_t>(translated) * static_cast<size_t>(*elementWidth),
                    equalsIndexBit);
                }
                index += step;
              }
              return true;
            }
          }
          return true;
        }
        auto statementSourceRange = getSourceRange(*current);
        if (needsIncrementerForAction(design, lhsNet, action) && incrementerBits.empty()) {
          auto incNet = getOrCreateNamedNet(
            design,
            joinName("inc", baseName),
            lhsNet,
            statementSourceRange);
          auto incBits = collectBits(incNet);
          auto incCarryNet = getOrCreateNamedNet(
            design,
            joinName("inc_carry", baseName),
            lhsNet,
            statementSourceRange);
          auto carryBits = collectBits(incCarryNet);
          incrementerBits = buildIncrementer(
            design,
            lhsBits,
            incBits,
            carryBits,
            statementSourceRange);
        }

        auto actionSourceRange = action.rhs
          ? getSourceRange(*action.rhs)
          : statementSourceRange;
        auto assignedBits = buildAssignBits(
          design,
          action,
          lhsNet,
          lhsBits,
          incrementerBits.empty() ? nullptr : &incrementerBits,
          actionSourceRange);
        if (assignedBits.empty()) {
          std::ostringstream reason;
          reason << "failed to lower assignment action for LHS '"
                 << getExpressionBaseName(lhsExpr) << "'";
          failureReason = reason.str();
          return false;
        }
        dataBits = std::move(assignedBits);
        return true;
      }

      std::ostringstream reason;
      reason << "unsupported statement kind while lowering sequential block"
             << " (kind=" << current->kind << ")";
      failureReason = reason.str();
      return false;
    }

    bool collectAssignedLHSExpressions(
      const Statement& stmt,
      std::vector<const Expression*>& lhsExpressions,
      std::string* failureReason = nullptr,
      bool trackAlwaysCombDynamicLHS = false,
      const std::unordered_set<const slang::ast::ValueSymbol*>* ignoredSymbols = nullptr) const {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };

      const Statement* current = unwrapStatement(stmt);
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!collectAssignedLHSExpressions(
                *item,
                lhsExpressions,
                failureReason,
                trackAlwaysCombDynamicLHS,
                ignoredSymbols)) {
            return false;
          }
          if (isCurrentForLoopBreakRequested()) {
            break;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        std::string forLoopFailureReason;
        if (!unrollForLoopStatement(
              forStmt,
              [&]() {
                return collectAssignedLHSExpressions(
                  forStmt.body,
                  lhsExpressions,
                  failureReason,
                  trackAlwaysCombDynamicLHS,
                  ignoredSymbols);
              },
              forLoopFailureReason)) {
          setFailureReason(forLoopFailureReason);
          return false;
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& conditional = current->as<slang::ast::ConditionalStatement>();
        const bool hasLoopContext = hasActiveForLoopContext();
        const bool incomingBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
        if (hasLoopContext) {
          setCurrentForLoopBreakRequested(false);
        }
        if (!collectAssignedLHSExpressions(
              conditional.ifTrue,
              lhsExpressions,
              failureReason,
              trackAlwaysCombDynamicLHS,
              ignoredSymbols)) {
          return false;
        }
        const bool trueBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
        if (hasLoopContext) {
          setCurrentForLoopBreakRequested(false);
        }
        bool falseBreak = false;
        if (conditional.ifFalse &&
            !collectAssignedLHSExpressions(
              *conditional.ifFalse,
              lhsExpressions,
              failureReason,
              trackAlwaysCombDynamicLHS,
              ignoredSymbols)) {
          return false;
        }
        if (hasLoopContext) {
          falseBreak = isCurrentForLoopBreakRequested();
          // A break that appears only in one branch remains conditional and does
          // not unconditionally terminate the enclosing loop.
          setCurrentForLoopBreakRequested(incomingBreak || (trueBreak && falseBreak));
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Case) {
        const auto& caseStmt = current->as<slang::ast::CaseStatement>();
        for (const auto& item : caseStmt.items) {
          if (!collectAssignedLHSExpressions(
                *item.stmt,
                lhsExpressions,
                failureReason,
                trackAlwaysCombDynamicLHS,
                ignoredSymbols)) {
            return false;
          }
        }
        if (caseStmt.defaultCase &&
            !collectAssignedLHSExpressions(
              *caseStmt.defaultCase,
              lhsExpressions,
              failureReason,
              trackAlwaysCombDynamicLHS,
              ignoredSymbols)) {
          return false;
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::VariableDeclaration) {
        // Local variable declarations in always_comb (including initializer forms
        // like "int i = 0" used by for-loop indices) do not directly write tracked
        // design LHS targets and can be ignored by assignment collection.
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Break) {
        requestCurrentForLoopBreak();
        return true;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (extractAssignment(*current, lhsExpr, action)) {
        if (trackAlwaysCombDynamicLHS) {
          lhsExpr = getTrackedAlwaysCombLHS(lhsExpr);
        }
        if (shouldIgnoreTrackedLHS(lhsExpr, ignoredSymbols)) {
          return true;
        }
        bool alreadyPresent = false;
        for (const auto* existing : lhsExpressions) {
          if (sameLhs(existing, lhsExpr)) {
            alreadyPresent = true;
            break;
          }
        }
        if (!alreadyPresent) {
          lhsExpressions.push_back(lhsExpr);
        }
        return true;
      }

      std::ostringstream reason;
      reason << "unsupported statement kind while collecting assignments"
             << " (kind=" << current->kind << ")";
      setFailureReason(reason.str());
      return false;
    }

    bool resolveFixedUnpackedArraySelectionBits(
      SNLDesign* design,
      const Expression& expr,
      std::vector<SNLBitNet*>& bits) {
      bits.clear();

      const auto* stripped = stripConversions(expr);
      if (!stripped ||
          (stripped->kind != slang::ast::ExpressionKind::ElementSelect &&
           stripped->kind != slang::ast::ExpressionKind::RangeSelect)) {
        return false;
      }

      const Expression* baseExpr = getSelectionBaseExpression(*stripped);
      if (!baseExpr) {
        return false; // LCOV_EXCL_LINE
      }
      const auto& baseType = baseExpr->type->getCanonicalType();
      if (!baseType.isUnpackedArray() || !baseType.hasFixedRange()) {
        return false;
      }

      auto* baseNet = resolveExpressionNet(design, *baseExpr);
      if (!baseNet) {
        return false;
      }
      auto baseBits = collectBits(baseNet);
      if (baseBits.empty()) {
        return false; // LCOV_EXCL_LINE
      }

      const auto arrayWidth = static_cast<size_t>(baseType.getFixedRange().width());
      if (!arrayWidth || baseBits.size() % arrayWidth != 0) {
        return false;
      }
      const auto elementWidth = baseBits.size() / arrayWidth;
      if (!elementWidth) {
        return false; // LCOV_EXCL_LINE
      }

      const auto appendElementBits = [&](int32_t selectedIndex) {
        const auto translated = baseType.getFixedRange().translateIndex(selectedIndex);
        if (translated < 0 ||
            translated >= static_cast<int32_t>(arrayWidth)) {
          return false;
        }
        const auto offset = static_cast<size_t>(translated) * elementWidth;
        if (offset + elementWidth > baseBits.size()) {
          return false; // LCOV_EXCL_LINE
        }
        bits.insert(
          bits.end(),
          baseBits.begin() + static_cast<std::ptrdiff_t>(offset),
          baseBits.begin() + static_cast<std::ptrdiff_t>(offset + elementWidth));
        return true;
      };

      if (stripped->kind == slang::ast::ExpressionKind::ElementSelect) {
        const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
        int32_t selectedIndex = 0;
        if (!getConstantInt32(elementExpr.selector(), selectedIndex)) {
          return false;
        }
        return appendElementBits(selectedIndex);
      }

      const auto& rangeExpr = stripped->as<slang::ast::RangeSelectExpression>();
      switch (rangeExpr.getSelectionKind()) {
        case slang::ast::RangeSelectionKind::Simple: {
          int32_t left = 0;
          int32_t right = 0;
          if (!getConstantInt32(rangeExpr.left(), left) ||
              !getConstantInt32(rangeExpr.right(), right)) {
            return false;
          }
          int32_t index = right;
          const int32_t end = left;
          const int32_t step = index <= end ? 1 : -1;
          while (index != end + step) {
            if (!appendElementBits(index)) {
              bits.clear();
              return false;
            }
            index += step;
          }
          return !bits.empty();
        }
        case slang::ast::RangeSelectionKind::IndexedUp:
        case slang::ast::RangeSelectionKind::IndexedDown: {
          int32_t startIndex = 0;
          int32_t sliceWidth = 0;
          if (!getConstantInt32(rangeExpr.left(), startIndex) ||
              !getConstantInt32(rangeExpr.right(), sliceWidth) ||
              sliceWidth <= 0) {
            return false;
          }
          int64_t lsbIndex = static_cast<int64_t>(startIndex);
          if (rangeExpr.getSelectionKind() ==
              slang::ast::RangeSelectionKind::IndexedDown) {
            lsbIndex -= static_cast<int64_t>(sliceWidth - 1);
          }
          for (int32_t elem = 0; elem < sliceWidth; ++elem) {
            const int64_t elemIndex = lsbIndex + static_cast<int64_t>(elem);
            if (elemIndex < std::numeric_limits<int32_t>::min() ||
                elemIndex > std::numeric_limits<int32_t>::max()) {
              bits.clear();
              return false;
            }
            if (!appendElementBits(static_cast<int32_t>(elemIndex))) {
              bits.clear();
              return false;
            }
          }
          return !bits.empty();
        }
      }

      return false; // LCOV_EXCL_LINE
    }

    bool resolveAssignmentLHSBits(
      SNLDesign* design,
      const Expression& lhsExpr,
      std::vector<SNLBitNet*>& lhsBits,
      std::string* failureReason = nullptr) {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };

      lhsBits.clear();
      if (auto* lhsNet = resolveExpressionNet(design, lhsExpr)) {
        lhsBits = collectBits(lhsNet);
        if (!lhsBits.empty()) {
          return true;
        }
      }

      const auto* strippedLHS = stripConversions(lhsExpr);
      const bool lhsIsSupportedSlice =
        strippedLHS &&
        (strippedLHS->kind == slang::ast::ExpressionKind::ElementSelect ||
         strippedLHS->kind == slang::ast::ExpressionKind::RangeSelect ||
         strippedLHS->kind == slang::ast::ExpressionKind::MemberAccess);
      if (!lhsIsSupportedSlice) {
        std::ostringstream reason;
        reason << "unsupported always_comb assignment LHS: "
               << describeExpression(lhsExpr);
        setFailureReason(reason.str());
        return false;
      }

      auto lhsWidth = getIntegralExpressionBitWidth(lhsExpr);
      if ((!lhsWidth || !*lhsWidth) &&
          resolveFixedUnpackedArraySelectionBits(design, lhsExpr, lhsBits) &&
          !lhsBits.empty()) {
        return true;
      }
      if (!lhsWidth || !*lhsWidth) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb assignment LHS width for "
               << describeExpression(lhsExpr);
        setFailureReason(reason.str());
        return false;
      }

      const auto lhsWidthBits = static_cast<size_t>(*lhsWidth);
      if (!resolveExpressionBits(design, lhsExpr, lhsWidthBits, lhsBits) ||
          lhsBits.size() != lhsWidthBits ||
          lhsBits.empty()) {
        std::ostringstream reason;
        reason << "failed to resolve always_comb assignment LHS bits for "
               << describeExpression(lhsExpr)
               << " (target_width=" << lhsWidthBits << ")";
        setFailureReason(reason.str());
        return false;
      }
      return true;
    }

    SNLNet* resolveAssignmentBaseNet(
      SNLDesign* design,
      const Expression& lhsExpr) {
      if (auto* lhsNet = resolveExpressionNet(design, lhsExpr)) {
        return lhsNet;
      }
      const auto* baseExpr = getSelectionBaseExpression(lhsExpr);
      if (!baseExpr || sameLhs(baseExpr, &lhsExpr)) {
        return nullptr;
      }
      return resolveExpressionNet(design, *baseExpr);
    }

    SNLBitNet* resolveCombinationalConditionNet(
      SNLDesign* design,
      const Expression& conditionExpr,
      std::string* failureReason = nullptr) {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };

      bool constantBit = false;
      if (getConstantBit(conditionExpr, constantBit)) {
        return static_cast<SNLBitNet*>(getConstNet(design, constantBit));
      }

      const auto* stripped = stripConversions(conditionExpr);
      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      if (stripped) {
        if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
          const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
          if (unaryExpr.op == slang::ast::UnaryOperator::LogicalNot ||
              unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
            auto* operandBit =
              resolveCombinationalConditionNet(design, unaryExpr.operand(), failureReason);
            if (!operandBit) {
              return nullptr;
            }
            if (operandBit == const0) {
              return const1;
            }
            if (operandBit == const1) {
              return const0;
            }
            auto unarySourceRange = getSourceRange(*stripped);
            auto* outBit = SNLScalarNet::create(design);
            annotateSourceInfo(outBit, unarySourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  operandBit,
                  outBit,
                  unarySourceRange)) {
              return nullptr; // LCOV_EXCL_LINE
            }
            return outBit;
          }
        } else if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd ||
              binaryExpr.op == slang::ast::BinaryOperator::LogicalOr) {
            auto* lhsBit =
              resolveCombinationalConditionNet(design, binaryExpr.left(), failureReason);
            if (!lhsBit) {
              return nullptr;
            }
            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd && lhsBit == const0) {
              return const0;
            }
            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalOr && lhsBit == const1) {
              return const1;
            }

            auto* rhsBit =
              resolveCombinationalConditionNet(design, binaryExpr.right(), failureReason);
            if (!rhsBit) {
              return nullptr;
            }
            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd) {
              if (rhsBit == const0) {
                return const0;
              }
              if (lhsBit == const1) {
                return rhsBit;
              }
              if (rhsBit == const1 || lhsBit == rhsBit) {
                return lhsBit;
              }
            } else {
              if (rhsBit == const1) {
                return const1;
              }
              if (lhsBit == const0) {
                return rhsBit;
              }
              if (rhsBit == const0 || lhsBit == rhsBit) {
                return lhsBit;
              }
            }

            auto binarySourceRange = getSourceRange(*stripped);
            auto* outBit = SNLScalarNet::create(design);
            annotateSourceInfo(outBit, binarySourceRange);
            auto gateType = binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd
              ? NLDB0::GateType(NLDB0::GateType::And)
              : NLDB0::GateType(NLDB0::GateType::Or);
            return getSingleBitNet(createBinaryGate(
              design,
              gateType,
              lhsBit,
              rhsBit,
              outBit,
              binarySourceRange));
          }
        }
      }

      std::vector<SNLBitNet*> conditionBits;
      if (!resolveExpressionBits(design, conditionExpr, 1, conditionBits) ||
          conditionBits.size() != 1) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb condition bit for "
               << describeExpression(conditionExpr);
        setFailureReason(reason.str());
        return nullptr;
      }
      return conditionBits.front();
    }

    SNLBitNet* buildCaseItemMatchBit(
      SNLDesign* design,
      const Expression& caseExpr,
      const slang::ast::CaseStatement& caseStmt,
      const slang::ast::CaseStatement::ItemGroup& item,
      std::string& failureReason) {
      if (caseStmt.condition == slang::ast::CaseStatementCondition::Inside) {
        std::ostringstream reason;
        reason << "unsupported always_comb case condition kind "
               << static_cast<int>(caseStmt.condition);
        failureReason = reason.str();
        return nullptr;
      }

      auto caseSourceRange = getSourceRange(caseStmt);
      const bool wildcardCase =
        caseStmt.condition == slang::ast::CaseStatementCondition::WildcardJustZ ||
        caseStmt.condition == slang::ast::CaseStatementCondition::WildcardXOrZ;
      if (!wildcardCase && !shouldSuppressCaseComparison2StateWarning(caseStmt)) {
        reportCaseComparison2StateWarning(
          slang::ast::BinaryOperator::CaseEquality,
          caseSourceRange);
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      SNLBitNet* itemMatchBit = const0;
      for (const auto* itemExpr : item.expressions) {
        if (!itemExpr) {
          // LCOV_EXCL_START
          failureReason = "unsupported null always_comb case item expression";
          return nullptr;
          // LCOV_EXCL_STOP
        }

        SNLBitNet* exprMatchBit = nullptr;
        if (!wildcardCase) {
          exprMatchBit = SNLScalarNet::create(design);
          annotateSourceInfo(exprMatchBit, caseSourceRange);
          if (!createEqualityAssign(
                design,
                exprMatchBit,
                caseExpr,
                *itemExpr,
                caseSourceRange)) {
            std::ostringstream reason;
            reason << "unable to resolve always_comb case item match for "
                   << describeExpression(*itemExpr);
            failureReason = reason.str();
            return nullptr;
          }
        } else {
          auto leftWidth = getIntegralExpressionBitWidth(caseExpr);
          auto rightWidth = getIntegralExpressionBitWidth(*itemExpr);
          if (!leftWidth || !rightWidth) {
            std::ostringstream reason;
            reason << "unable to determine always_comb wildcard case item width for "
                   << describeExpression(*itemExpr);
            failureReason = reason.str();
            return nullptr;
          }

          const auto compareWidth = std::max(*leftWidth, *rightWidth);
          std::vector<SNLBitNet*> caseBits;
          if (!resolveExpressionBits(design, caseExpr, compareWidth, caseBits) ||
              caseBits.size() != compareWidth) {
            std::ostringstream reason;
            reason << "unable to resolve always_comb case expression bits for "
                   << describeExpression(caseExpr);
            failureReason = reason.str();
            return nullptr;
          }

          const auto* strippedItemExpr = stripConversions(*itemExpr);
          if (!strippedItemExpr) {
            failureReason = "unable to inspect always_comb wildcard case item"; // LCOV_EXCL_LINE
            return nullptr;                                                     // LCOV_EXCL_LINE
          }

          const bool bothSigned =
            caseExpr.type->getCanonicalType().isSigned() &&
            itemExpr->type->getCanonicalType().isSigned();
          std::vector<slang::logic_t> itemPatternBits;
          if (!resolveWildcardCaseItemPattern(
                *itemExpr,
                compareWidth,
                bothSigned,
                itemPatternBits) ||
              itemPatternBits.size() != compareWidth) {
            std::ostringstream reason;
            reason << "unable to resolve always_comb wildcard case item constant for "
                   << describeExpression(*itemExpr);
            failureReason = reason.str();
            return nullptr;
          }

          std::vector<SNLNet*> bitMatches;
          bitMatches.reserve(compareWidth);
          for (size_t bitIndex = 0; bitIndex < compareWidth; ++bitIndex) {
            const auto itemBit = itemPatternBits[bitIndex];
            const bool wildcard =
              caseStmt.condition == slang::ast::CaseStatementCondition::WildcardXOrZ
              ? itemBit.isUnknown()
              : exactlyEqual(itemBit, slang::logic_t::z);
            if (wildcard) {
              continue;
            }
            if (itemBit.isUnknown()) {
              std::ostringstream reason;
              reason << "unable to resolve always_comb wildcard case item match for "
                     << describeExpression(*itemExpr);
              failureReason = reason.str();
              return nullptr;
            }

            auto* matchBit = caseBits[bitIndex];
            if (!static_cast<bool>(itemBit)) {
              if (matchBit == const0) {
                matchBit = const1;
              } else if (matchBit == const1) {
                matchBit = const0;
              } else {
                auto* invertedBit = SNLScalarNet::create(design);
                annotateSourceInfo(invertedBit, caseSourceRange);
                if (!createUnaryGate(
                      design,
                      NLDB0::GateType(NLDB0::GateType::Not),
                      matchBit,
                      invertedBit,
                      caseSourceRange)) {
                  return nullptr; // LCOV_EXCL_LINE
                }
                matchBit = invertedBit;
              }
            }
            bitMatches.push_back(matchBit);
          }

          if (bitMatches.empty()) {
            exprMatchBit = const1;
          } else if (bitMatches.size() == 1) {
            exprMatchBit = getSingleBitNet(bitMatches.front());
          } else {
            SNLNet* andBit = SNLScalarNet::create(design);
            annotateSourceInfo(andBit, caseSourceRange);
            if (!createGateInstance(
                  design,
                  NLDB0::GateType(NLDB0::GateType::And),
                  bitMatches,
                  andBit,
                  caseSourceRange)) {
              return nullptr; // LCOV_EXCL_LINE
            }
            exprMatchBit = getSingleBitNet(andBit);
          }
        }

        if (itemMatchBit == const0) {
          itemMatchBit = exprMatchBit;
          continue;
        }

        auto* orBit = SNLScalarNet::create(design);
        annotateSourceInfo(orBit, caseSourceRange);
        itemMatchBit = getSingleBitNet(createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::Or),
          itemMatchBit,
          exprMatchBit,
          orBit,
          caseSourceRange));
        if (!itemMatchBit) {
          // LCOV_EXCL_START
          failureReason = "failed to build always_comb case item match";
          return nullptr;
          // LCOV_EXCL_STOP
        }
      }
      return itemMatchBit;
    }

    bool resolveWildcardCaseItemPattern(
      const Expression& expr,
      size_t targetWidth,
      bool signExtend,
      std::vector<slang::logic_t>& bits) const {
      bits.clear();
      if (!targetWidth) {
        return false; // LCOV_EXCL_LINE
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }

      auto appendIntegerBits = [&](const slang::SVInt& value) {
        const auto integerWidth = static_cast<size_t>(value.getBitWidth());
        bits.reserve(std::min(targetWidth, integerWidth));
        for (size_t i = 0; i < std::min(targetWidth, integerWidth); ++i) {
          bits.push_back(value[static_cast<int32_t>(i)]);
        }
        const auto fillBit =
          signExtend && integerWidth
            ? value[static_cast<int32_t>(integerWidth - 1)]
            : slang::logic_t(0);
        resizeLogicBitsToWidth(bits, targetWidth, fillBit);
        return true;
      };

      if (stripped->kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        const auto value =
          stripped->as<slang::ast::UnbasedUnsizedIntegerLiteral>().getLiteralValue();
        bits.assign(targetWidth, value);
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::IntegerLiteral) {
        return appendIntegerBits(
          stripped->as<slang::ast::IntegerLiteral>().getValue());
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      const Symbol* evalSymbol = getConstantEvalSymbol(*stripped);
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        if (evaluatedConstant && evaluatedConstant.isInteger()) {
          constant = &evaluatedConstant;
        }
      }
      slang::ConstantValue convertedConstant;
      if (constant && !constant->isInteger()) {
        convertedConstant = constant->convertToInt();
        if (convertedConstant && convertedConstant.isInteger()) {
          constant = &convertedConstant;
        }
      }
      if (constant && constant->isInteger()) {
        return appendIntegerBits(constant->integer());
      }

      if (stripped->kind == slang::ast::ExpressionKind::Replication) {
        const auto& replicationExpr = stripped->as<slang::ast::ReplicationExpression>();
        int32_t repeatCountSigned = 0;
        if (!getConstantInt32(replicationExpr.count(), repeatCountSigned) || repeatCountSigned < 0) {
          return false;
        }
        const auto repeatCount = static_cast<size_t>(repeatCountSigned);

        size_t concatWidth = 0;
        if (auto concatExprWidth = getIntegralExpressionBitWidth(replicationExpr.concat())) {
          concatWidth = *concatExprWidth;
        } else {
          const auto& concatCanonical = replicationExpr.concat().type->getCanonicalType();
          if (!concatCanonical.isIntegral()) {
            return false;
          }
          const auto bitWidth = concatCanonical.getBitWidth();
          if (bitWidth < 0) {
            return false;
          }
          concatWidth = static_cast<size_t>(bitWidth);
        }

        if (concatWidth && repeatCount > (std::numeric_limits<size_t>::max() / concatWidth)) {
          return false; // LCOV_EXCL_LINE
        }

        std::vector<slang::logic_t> concatBits;
        if (!resolveWildcardCaseItemPattern(
              replicationExpr.concat(),
              concatWidth,
              false,
              concatBits) ||
            concatBits.size() != concatWidth) {
          return false;
        }

        bits.reserve(concatWidth * repeatCount);
        for (size_t i = 0; i < repeatCount; ++i) {
          bits.insert(bits.end(), concatBits.begin(), concatBits.end());
        }
        const auto fillBit =
          signExtend && !bits.empty() ? bits.back() : slang::logic_t(0);
        resizeLogicBitsToWidth(bits, targetWidth, fillBit);
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Concatenation) {
        const auto& concatExpr = stripped->as<slang::ast::ConcatenationExpression>();
        auto operands = concatExpr.operands();
        if (operands.empty()) {
          return false; // LCOV_EXCL_LINE
        }

        bits.clear();
        for (auto it = operands.rbegin(); it != operands.rend(); ++it) {
          auto* operand = *it;
          if (!operand) {
            return false; // LCOV_EXCL_LINE
          }

          size_t operandWidthBits = 0;
          if (auto operandWidth = getIntegralExpressionBitWidth(*operand)) {
            operandWidthBits = *operandWidth;
          } else {
            const auto* strippedOperand = stripConversions(*operand);
            if (strippedOperand &&
                strippedOperand->kind == slang::ast::ExpressionKind::Replication) {
              const auto& replicationOperand =
                strippedOperand->as<slang::ast::ReplicationExpression>();
              int32_t repeatCountSigned = 0;
              if (getConstantInt32(replicationOperand.count(), repeatCountSigned) &&
                  repeatCountSigned == 0) {
                continue;
              }
            }
            const auto& operandCanonical = operand->type->getCanonicalType();
            if (!operandCanonical.isIntegral()) {
              return false; // LCOV_EXCL_LINE
            }
            const auto bitWidth = operandCanonical.getBitWidth();
            if (bitWidth < 0) {
              return false; // LCOV_EXCL_LINE
            }
            if (bitWidth == 0) {
              continue; // LCOV_EXCL_LINE
            }
            operandWidthBits = static_cast<size_t>(bitWidth);
          }
          if (!operandWidthBits) {
            continue; // LCOV_EXCL_LINE
          }

          std::vector<slang::logic_t> operandBits;
          if (!resolveWildcardCaseItemPattern(
                *operand,
                operandWidthBits,
                false,
                operandBits) ||
              operandBits.size() != operandWidthBits) {
            return false;
          }
          bits.insert(bits.end(), operandBits.begin(), operandBits.end());
        }

        const auto fillBit =
          signExtend && !bits.empty() ? bits.back() : slang::logic_t(0);
        resizeLogicBitsToWidth(bits, targetWidth, fillBit);
        return true;
      }

      return false;
    }

    bool resolveUnknownLiteralBitsAsZero(
      SNLDesign* design,
      const Expression& expr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits,
      bool& usedUnknownFallback) {
      bits.clear();
      usedUnknownFallback = false;
      if (!targetWidth) {
        return false; // LCOV_EXCL_LINE
      }

      if (resolveExpressionBits(design, expr, targetWidth, bits) && bits.size() == targetWidth) {
        return true;
      }

      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));

      if (stripped->kind == slang::ast::ExpressionKind::ConditionalOp) {
        const auto& conditionalExpr = stripped->as<slang::ast::ConditionalExpression>();
        if (const auto* knownSide = conditionalExpr.knownSide()) {
          return resolveUnknownLiteralBitsAsZero(
            design,
            *knownSide,
            targetWidth,
            bits,
            usedUnknownFallback);
        }
        if (conditionalExpr.conditions.size() != 1 || conditionalExpr.conditions.front().pattern) {
          return false;
        }
        auto* selectBit = resolveCombinationalConditionNet(
          design,
          *conditionalExpr.conditions.front().expr);
        if (!selectBit) {
          return false;
        }
        if (selectBit == const1) {
          return resolveUnknownLiteralBitsAsZero(
            design,
            conditionalExpr.left(),
            targetWidth,
            bits,
            usedUnknownFallback);
        }
        if (selectBit == const0) {
          return resolveUnknownLiteralBitsAsZero(
            design,
            conditionalExpr.right(),
            targetWidth,
            bits,
            usedUnknownFallback);
        }

        std::vector<SNLBitNet*> leftBits;
        std::vector<SNLBitNet*> rightBits;
        bool leftUsedUnknownFallback = false;
        bool rightUsedUnknownFallback = false;
        if (!resolveUnknownLiteralBitsAsZero(
              design,
              conditionalExpr.left(),
              targetWidth,
              leftBits,
              leftUsedUnknownFallback) ||
            !resolveUnknownLiteralBitsAsZero(
              design,
              conditionalExpr.right(),
              targetWidth,
              rightBits,
              rightUsedUnknownFallback) ||
            leftBits.size() != targetWidth ||
            rightBits.size() != targetWidth ||
            (!leftUsedUnknownFallback && !rightUsedUnknownFallback)) {
          return false;
        }

        usedUnknownFallback = leftUsedUnknownFallback || rightUsedUnknownFallback;
        if (leftBits == rightBits) {
          bits = std::move(leftBits);
          return true;
        }

        auto conditionalSourceRange = getSourceRange(*stripped);
        if (targetWidth > 1) {
          return createMux2Instance(
            design,
            selectBit,
            rightBits,
            leftBits,
            bits,
            conditionalSourceRange);
        }

        bits.clear();
        bits.reserve(targetWidth);
        for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, conditionalSourceRange);
          createMux2Instance(
            design,
            selectBit,
            rightBits[bitIndex],
            leftBits[bitIndex],
            outBit,
            conditionalSourceRange);
          bits.push_back(outBit);
        }
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        const auto value =
          stripped->as<slang::ast::UnbasedUnsizedIntegerLiteral>().getLiteralValue();
        if (!value.isUnknown()) {
          return false;
        }
        bits.assign(targetWidth, const0);
        usedUnknownFallback = true;
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::IntegerLiteral) {
        const auto& literal = stripped->as<slang::ast::IntegerLiteral>();
        const auto& literalValue = literal.getValue();
        if (!literalValue.hasUnknown()) {
          return false;
        }
        const auto integerWidth = static_cast<size_t>(literalValue.getBitWidth());
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          bool one = false;
          if (i < integerWidth) {
            const auto bit = literalValue[static_cast<int32_t>(i)];
            if (!bit.isUnknown()) {
              one = static_cast<bool>(bit);
            }
          }
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        usedUnknownFallback = true;
        return true;
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      const Symbol* evalSymbol = getConstantEvalSymbol(*stripped);
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        if (evaluatedConstant && evaluatedConstant.isInteger()) {
          constant = &evaluatedConstant;
        }
      }
      slang::ConstantValue convertedConstant;
      if (constant && !constant->isInteger()) {
        convertedConstant = constant->convertToInt();
        if (convertedConstant && convertedConstant.isInteger()) {
          constant = &convertedConstant;
        }
      }
      if (constant && constant->isInteger()) {
        const auto& intValue = constant->integer();
        if (!intValue.hasUnknown()) {
          return false;
        }
        const auto integerWidth = static_cast<size_t>(intValue.getBitWidth());
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          bool one = false;
          if (i < integerWidth) {
            const auto bit = intValue[static_cast<int32_t>(i)];
            if (!bit.isUnknown()) {
              one = static_cast<bool>(bit);
            }
          }
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        usedUnknownFallback = true;
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Replication) {
        const auto& replicationExpr = stripped->as<slang::ast::ReplicationExpression>();
        int32_t repeatCountSigned = 0;
        if (!getConstantInt32(replicationExpr.count(), repeatCountSigned) || repeatCountSigned < 0) {
          return false;
        }
        const auto repeatCount = static_cast<size_t>(repeatCountSigned);

        size_t concatWidth = 0;
        if (auto concatExprWidth = getIntegralExpressionBitWidth(replicationExpr.concat())) {
          concatWidth = *concatExprWidth;
        } else {
          const auto& concatCanonical = replicationExpr.concat().type->getCanonicalType();
          if (!concatCanonical.isIntegral()) {
            return false;
          }
          const auto bitWidth = concatCanonical.getBitWidth();
          if (bitWidth < 0) {
            return false;
          }
          concatWidth = static_cast<size_t>(bitWidth);
        }

        if (concatWidth && repeatCount > (std::numeric_limits<size_t>::max() / concatWidth)) {
          return false; // LCOV_EXCL_LINE
        }

        std::vector<SNLBitNet*> concatBits;
        bool concatUsedUnknownFallback = false;
        if (!resolveUnknownLiteralBitsAsZero(
              design,
              replicationExpr.concat(),
              concatWidth,
              concatBits,
              concatUsedUnknownFallback) ||
            concatBits.size() != concatWidth || !concatUsedUnknownFallback) {
          return false;
        }

        bits.clear();
        bits.reserve(concatWidth * repeatCount);
        for (size_t i = 0; i < repeatCount; ++i) {
          bits.insert(bits.end(), concatBits.begin(), concatBits.end());
        }
        resizeBitsToWidth(bits, targetWidth, const0);
        usedUnknownFallback = true;
        return true;
      }

      if (stripped->kind == slang::ast::ExpressionKind::Concatenation) {
        const auto& concatExpr = stripped->as<slang::ast::ConcatenationExpression>();
        auto operands = concatExpr.operands();
        if (operands.empty()) {
          return false; // LCOV_EXCL_LINE
        }

        bool sawUnknownFallback = false;
        bits.clear();
        for (auto it = operands.rbegin(); it != operands.rend(); ++it) {
          auto* operand = *it;
          if (!operand) {
            return false; // LCOV_EXCL_LINE
          }

          const auto* strippedOperand = stripConversions(*operand);
          size_t operandWidthBits = 0;
          if (auto operandWidth = getIntegralExpressionBitWidth(*operand)) {
            operandWidthBits = *operandWidth;
          } else {
            if (strippedOperand &&
                strippedOperand->kind == slang::ast::ExpressionKind::Replication) {
              const auto& replicationOperand =
                strippedOperand->as<slang::ast::ReplicationExpression>();
              int32_t repeatCountSigned = 0;
              if (getConstantInt32(replicationOperand.count(), repeatCountSigned) &&
                  repeatCountSigned == 0) {
                continue;
              }
            }
            const auto& operandCanonical = operand->type->getCanonicalType();
            if (!operandCanonical.isIntegral()) {
              return false; // LCOV_EXCL_LINE
            }
            const auto bitWidth = operandCanonical.getBitWidth();
            if (bitWidth < 0) {
              return false; // LCOV_EXCL_LINE
            }
            if (bitWidth == 0) {
              continue; // LCOV_EXCL_LINE
            }
            operandWidthBits = static_cast<size_t>(bitWidth);
          }
          if (!operandWidthBits) {
            continue; // LCOV_EXCL_LINE
          }

          std::vector<SNLBitNet*> operandBits;
          bool operandUsedUnknownFallback = false;
          if (!resolveUnknownLiteralBitsAsZero(
                design,
                *operand,
                operandWidthBits,
                operandBits,
                operandUsedUnknownFallback) ||
              operandBits.size() != operandWidthBits) {
            return false;
          }
          sawUnknownFallback = sawUnknownFallback || operandUsedUnknownFallback;
          bits.insert(bits.end(), operandBits.begin(), operandBits.end());
        }
        if (!sawUnknownFallback) {
          return false;
        }
        resizeBitsToWidth(bits, targetWidth, const0);
        usedUnknownFallback = true;
        return true;
      }

      return false;
    }

    bool buildCombinationalAssignBits(
      SNLDesign* design,
      const AssignAction& action,
      size_t targetWidth,
      std::vector<SNLBitNet*>& assignedBits,
      const std::vector<SNLBitNet*>* currentBits,
      std::string& failureReason) {
      if (action.stepDelta != 0) {
        if (!currentBits || currentBits->size() != targetWidth) {
          failureReason =
            "unsupported increment/decrement assignment in always_comb without current LHS bits";
          return false;
        }

        auto sourceRange = action.rhs ? getSourceRange(*action.rhs) : std::optional<slang::SourceRange> {};
        auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
        auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
        std::vector<SNLBitNet*> oneBits(targetWidth, const0);
        oneBits.front() = const1;

        if (action.stepDelta > 0) {
          return addBitVectors(
            design,
            *currentBits,
            oneBits,
            assignedBits,
            sourceRange);
        }

        std::vector<SNLBitNet*> invertedOneBits;
        invertedOneBits.reserve(oneBits.size());
        for (auto* oneBit : oneBits) {
          if (oneBit == const0) {
            invertedOneBits.push_back(const1);
            continue;
          }
          invertedOneBits.push_back(const0);
        }

        assignedBits.clear();
        assignedBits.reserve(targetWidth);
        auto* carry = const1;
        for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
          auto* diffBit = SNLScalarNet::create(design);
          auto* carryOut = SNLScalarNet::create(design);
          annotateSourceInfo(diffBit, sourceRange);
          annotateSourceInfo(carryOut, sourceRange);
          if (!createFAInstance(
                design,
                (*currentBits)[bitIndex],
                invertedOneBits[bitIndex],
                carry,
                diffBit,
                carryOut,
                sourceRange)) {
            return false; // LCOV_EXCL_LINE
          }
          assignedBits.push_back(diffBit);
          carry = carryOut;
        }
        return true;
      }
      if (!action.rhs) { // LCOV_EXCL_START
        failureReason = "missing RHS expression in always_comb assignment";
        return false;
      } // LCOV_EXCL_STOP
      if (action.compoundOp) {
        if (!currentBits || currentBits->size() != targetWidth) {
          failureReason =
            "unsupported compound assignment in always_comb without current LHS bits";
          return false;
        }

        const Expression* compoundRhsExpr = action.rhs;
        std::vector<SNLBitNet*> rhsBits;
        const auto resolveCompoundRhs = [&](const Expression& rhsExpr) {
          rhsBits.clear();
          return resolveExpressionBits(design, rhsExpr, targetWidth, rhsBits) &&
                 rhsBits.size() == targetWidth;
        };
        if (!resolveCompoundRhs(*compoundRhsExpr)) {
          const auto* strippedCompoundRhs = stripConversions(*action.rhs);
          if (action.lhs &&
              strippedCompoundRhs &&
              strippedCompoundRhs->kind == slang::ast::ExpressionKind::BinaryOp) {
            const auto& binaryExpr = strippedCompoundRhs->as<slang::ast::BinaryExpression>();
            if (binaryExpr.op == *action.compoundOp) {
              const Expression* recoveredCompoundRhs = nullptr;
              if (sameLhs(action.lhs, &binaryExpr.left())) {
                recoveredCompoundRhs = &binaryExpr.right(); // LCOV_EXCL_LINE
              } else if (sameLhs(action.lhs, &binaryExpr.right())) {
                recoveredCompoundRhs = &binaryExpr.left(); // LCOV_EXCL_LINE
              }
              if (!recoveredCompoundRhs) {
                std::vector<SNLBitNet*> probeBits;
                const auto canResolveSide = [&](const Expression& expr) {
                  probeBits.clear();
                  return resolveExpressionBits(design, expr, targetWidth, probeBits) &&
                         probeBits.size() == targetWidth;
                };
                const bool leftResolves = canResolveSide(binaryExpr.left());
                const bool rightResolves = canResolveSide(binaryExpr.right());
                if (!leftResolves && rightResolves) {
                  recoveredCompoundRhs = &binaryExpr.right();
                } else if (leftResolves && !rightResolves) {
                  recoveredCompoundRhs = &binaryExpr.left(); // LCOV_EXCL_LINE
                }
              }
              if (recoveredCompoundRhs && resolveCompoundRhs(*recoveredCompoundRhs)) {
                compoundRhsExpr = recoveredCompoundRhs;
              }
            }
          }
        }
        if (rhsBits.size() != targetWidth) {
          std::ostringstream reason;
          reason << "unable to resolve always_comb compound assignment RHS bits for "
                 << describeExpression(*compoundRhsExpr)
                 << " (target_width=" << targetWidth << ")";
          failureReason = reason.str();
          return false;
        }

        auto sourceRange = getSourceRange(*compoundRhsExpr);
        if (*action.compoundOp == slang::ast::BinaryOperator::Add) {
          return addBitVectors(
            design,
            *currentBits,
            rhsBits,
            assignedBits,
            sourceRange);
        }
        if (*action.compoundOp == slang::ast::BinaryOperator::Subtract) {
          auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
          auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
          std::vector<SNLBitNet*> invertedRightBits;
          invertedRightBits.reserve(rhsBits.size());
          for (auto* rightBit : rhsBits) {
            if (rightBit == const0) {
              invertedRightBits.push_back(const1);
              continue;
            }
            if (rightBit == const1) {
              invertedRightBits.push_back(const0);
              continue;
            }
            auto* invertedBit = SNLScalarNet::create(design);
            annotateSourceInfo(invertedBit, sourceRange);
            if (!createUnaryGate(
                  design,
                  NLDB0::GateType(NLDB0::GateType::Not),
                  rightBit,
                  invertedBit,
                  sourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            invertedRightBits.push_back(invertedBit);
          }

          assignedBits.clear();
          assignedBits.reserve(targetWidth);
          auto* carry = const1;
          for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
            auto* diffBit = SNLScalarNet::create(design);
            auto* carryOut = SNLScalarNet::create(design);
            annotateSourceInfo(diffBit, sourceRange);
            annotateSourceInfo(carryOut, sourceRange);
            if (!createFAInstance(
                  design,
                  (*currentBits)[bitIndex],
                  invertedRightBits[bitIndex],
                  carry,
                  diffBit,
                  carryOut,
                  sourceRange)) {
              return false; // LCOV_EXCL_LINE
            }
            assignedBits.push_back(diffBit);
            carry = carryOut;
          }
          return true;
        }

        auto gateType = gateTypeFromBinary(*action.compoundOp);
        if (!gateType) {
          std::ostringstream reason;
          reason << "unsupported compound assignment operator in always_comb: "
                 << slang::ast::OpInfo::getText(*action.compoundOp);
          failureReason = reason.str();
          return false;
        }

        assignedBits.clear();
        assignedBits.reserve(targetWidth);
        for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
          auto* outBit = getSingleBitNet(createBinaryGate(
            design,
            *gateType,
            (*currentBits)[bitIndex],
            rhsBits[bitIndex],
            nullptr,
            sourceRange));
          if (!outBit) {
            return false; // LCOV_EXCL_LINE
          }
          assignedBits.push_back(outBit);
        }
        return true;
      }

      bool usedUnknownLiteralFallback = false;
      if ((!resolveExpressionBits(design, *action.rhs, targetWidth, assignedBits) ||
           assignedBits.size() != targetWidth) &&
          (!resolveUnknownLiteralBitsAsZero(
             design,
             *action.rhs,
             targetWidth,
             assignedBits,
             usedUnknownLiteralFallback) ||
           assignedBits.size() != targetWidth || !usedUnknownLiteralFallback)) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb RHS bits for "
               << describeExpression(*action.rhs)
               << " (target_width=" << targetWidth << ")";
        failureReason = reason.str();
        return false;
      }
      if (usedUnknownLiteralFallback) {
        reportWarning(
          "Unknown literal bits in always_comb assignment RHS lowered as 0 in SNL "
          "(X/Z distinction is not preserved)",
          getSourceRange(*action.rhs));
      }
      return true;
    }

    bool tryApplyCombinationalElementSelectAssignmentForLhs(
      SNLDesign* design,
      const Expression& assignedLHS,
      const Expression& trackedLhs,
      const AssignAction& action,
      const std::vector<SNLBitNet*>& lhsBits,
      std::vector<SNLBitNet*>& dataBits,
      std::string& failureReason,
      bool& handled) {
      handled = false;

      const auto* strippedAssignedLHS = stripConversions(assignedLHS);
      if (!strippedAssignedLHS ||
          strippedAssignedLHS->kind != slang::ast::ExpressionKind::ElementSelect) {
        return true;
      }

      const auto& elementExpr =
        strippedAssignedLHS->as<slang::ast::ElementSelectExpression>();
      const auto* baseExpr = stripConversions(elementExpr.value());
      if (!baseExpr || !sameLhs(baseExpr, &trackedLhs)) {
        return true;
      }
      handled = true;

      const auto& baseType = baseExpr->type->getCanonicalType();
      if (!baseType.hasFixedRange()) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "unsupported always_comb element-select assignment base without fixed range: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
        // LCOV_EXCL_STOP
      }

      auto elementWidth = getIntegralExpressionBitWidth(assignedLHS);
      if (!elementWidth || !*elementWidth) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb element-select assignment width for "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }

      const auto arrayWidth = static_cast<size_t>(baseType.getFixedRange().width());
      const auto totalSelectedWidth = static_cast<size_t>(*elementWidth) * arrayWidth;
      if (lhsBits.size() < totalSelectedWidth || dataBits.size() != lhsBits.size()) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "width mismatch while lowering always_comb element-select assignment for "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
        // LCOV_EXCL_STOP
      }

      std::vector<SNLBitNet*> assignedBits;
      if (!buildCombinationalAssignBits(
            design,
            action,
            static_cast<size_t>(*elementWidth),
            assignedBits,
            nullptr,
            failureReason)) {
        return false;
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
      auto elementSourceRange = getSourceRange(assignedLHS);

      auto updateSlice = [&](size_t offset, SNLBitNet* selectBit) {
        std::vector<SNLBitNet*> currentBits(
          dataBits.begin() + static_cast<std::ptrdiff_t>(offset),
          dataBits.begin() + static_cast<std::ptrdiff_t>(offset + static_cast<size_t>(*elementWidth)));
        if (static_cast<size_t>(*elementWidth) > 1 && selectBit != const0 && selectBit != const1) {
          std::vector<SNLBitNet*> updatedBits;
          if (!createMux2Instance(
                design,
                selectBit,
                currentBits,
                assignedBits,
                updatedBits,
                elementSourceRange)) {
            // LCOV_EXCL_START
            throw SNLSVConstructorException(
              "Internal error: failed to build wide mux for always_comb element-select assignment");
            // LCOV_EXCL_STOP
          }
          std::copy(
            updatedBits.begin(),
            updatedBits.end(),
            dataBits.begin() + static_cast<std::ptrdiff_t>(offset));
          return;
        }
        for (size_t elemBit = 0; elemBit < static_cast<size_t>(*elementWidth); ++elemBit) {
          auto* candidateBit = assignedBits[elemBit];
          if (selectBit == const1) {
            dataBits[offset + elemBit] = candidateBit;
            continue;
          }
          if (selectBit == const0 || dataBits[offset + elemBit] == candidateBit) {
            continue;
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, elementSourceRange);
          createMux2Instance(
            design,
            selectBit,
            dataBits[offset + elemBit],
            candidateBit,
            outBit,
            elementSourceRange);
          dataBits[offset + elemBit] = outBit;
        }
      };

      int32_t selectedIndex = 0;
      if (getConstantInt32(elementExpr.selector(), selectedIndex)) {
        const auto translated = baseType.getFixedRange().translateIndex(selectedIndex);
        if (translated < 0 || translated >= static_cast<int32_t>(baseType.getFixedRange().width())) {
          std::ostringstream reason;
          reason << "constant element-select index out of range in always_comb assignment: "
                 << describeExpression(assignedLHS);
          failureReason = reason.str();
          return false;
        }
        updateSlice(static_cast<size_t>(translated) * static_cast<size_t>(*elementWidth), const1);
        return true;
      }

      auto selectorWidth = getIntegralExpressionBitWidth(elementExpr.selector());
      if (!selectorWidth || !*selectorWidth) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "unable to resolve dynamic index width in always_comb assignment LHS: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
        // LCOV_EXCL_STOP
      }

      std::vector<SNLBitNet*> selectorBits;
      if (!resolveExpressionBits(
            design,
            elementExpr.selector(),
            static_cast<size_t>(*selectorWidth),
            selectorBits) ||
          selectorBits.size() != static_cast<size_t>(*selectorWidth)) {
        std::ostringstream reason;
        reason << "unable to resolve dynamic index bits in always_comb assignment LHS: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }

      int32_t index = baseType.getFixedRange().right;
      const int32_t end = baseType.getFixedRange().left;
      const int32_t step = index <= end ? 1 : -1;
      while (index != end + step) {
        const auto translated = baseType.getFixedRange().translateIndex(index);
        if (translated >= 0 && translated < static_cast<int32_t>(baseType.getFixedRange().width())) {
          auto* equalsIndexBit = buildSelectorEqualsIndexBit(
            design,
            selectorBits,
            index,
            elementSourceRange);
          if (!equalsIndexBit) {
            // LCOV_EXCL_START
            failureReason =
              "failed to build selector decode while lowering always_comb element-select assignment";
            return false;
            // LCOV_EXCL_STOP
          }
          updateSlice(static_cast<size_t>(translated) * static_cast<size_t>(*elementWidth),
                      equalsIndexBit);
        }
        index += step;
      }

      return true;
    }

    bool tryApplyCombinationalRangeSelectAssignmentForLhs(
      SNLDesign* design,
      const Expression& assignedLHS,
      const Expression& trackedLhs,
      const AssignAction& action,
      const std::vector<SNLBitNet*>& lhsBits,
      std::vector<SNLBitNet*>& dataBits,
      std::string& failureReason,
      bool& handled) {
      handled = false;

      const auto* strippedAssignedLHS = stripConversions(assignedLHS);
      if (!strippedAssignedLHS ||
          strippedAssignedLHS->kind != slang::ast::ExpressionKind::RangeSelect) {
        return true;
      }

      const auto& rangeExpr =
        strippedAssignedLHS->as<slang::ast::RangeSelectExpression>();
      const auto* baseExpr = stripConversions(rangeExpr.value());
      if (!baseExpr || !sameLhs(baseExpr, &trackedLhs)) {
        return true;
      }
      handled = true;

      const auto& baseType = baseExpr->type->getCanonicalType();
      if (!baseType.hasFixedRange()) {
        std::ostringstream reason;
        reason << "unsupported always_comb range-select assignment base without fixed range: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }

      auto selectedWidth = getIntegralExpressionBitWidth(assignedLHS);
      if (!selectedWidth || !*selectedWidth) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb range-select assignment width for "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }

      const auto arrayWidth = static_cast<size_t>(baseType.getFixedRange().width());
      if (arrayWidth == 0 || lhsBits.size() != dataBits.size() || lhsBits.size() % arrayWidth != 0) {
        std::ostringstream reason;
        reason << "width mismatch while lowering always_comb range-select assignment for "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }
      const auto elementWidth = lhsBits.size() / arrayWidth;
      if (elementWidth == 0 || *selectedWidth % elementWidth != 0) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb range-select element width for "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }

      std::vector<SNLBitNet*> assignedBits;
      if (!buildCombinationalAssignBits(
            design,
            action,
            *selectedWidth,
            assignedBits,
            nullptr,
            failureReason)) {
        return false;
      }

      const auto sliceElements = *selectedWidth / elementWidth;
      const auto collectSliceOffsets =
        [&](int64_t lsbIndex, std::vector<size_t>& offsets) {
          offsets.clear();
          offsets.reserve(*selectedWidth);
          for (size_t elem = 0; elem < sliceElements; ++elem) {
            const int64_t elemIndex = lsbIndex + static_cast<int64_t>(elem);
            if (elemIndex < std::numeric_limits<int32_t>::min() ||
                elemIndex > std::numeric_limits<int32_t>::max()) {
              return false;
            }
            const auto translated =
              baseType.getFixedRange().translateIndex(static_cast<int32_t>(elemIndex));
            if (translated < 0 || translated >= static_cast<int32_t>(arrayWidth)) {
              return false;
            }
            const auto offset = static_cast<size_t>(translated) * elementWidth;
            if (offset + elementWidth > dataBits.size()) {
              return false; // LCOV_EXCL_LINE
            }
            for (size_t bit = 0; bit < elementWidth; ++bit) {
              offsets.push_back(offset + bit);
            }
          }
          return offsets.size() == *selectedWidth;
        };

      int32_t left = 0;
      int32_t right = 0;
      int64_t lsbIndex = 0;
      switch (rangeExpr.getSelectionKind()) {
        case slang::ast::RangeSelectionKind::Simple: {
          if (!getConstantInt32(rangeExpr.left(), left) ||
              !getConstantInt32(rangeExpr.right(), right)) {
            std::ostringstream reason;
            reason << "unable to resolve constant range-select bounds in always_comb assignment: "
                   << describeExpression(assignedLHS);
            failureReason = reason.str();
            return false;
          }
          lsbIndex = std::min(left, right);
          break;
        }
        case slang::ast::RangeSelectionKind::IndexedUp:
        case slang::ast::RangeSelectionKind::IndexedDown: {
          if (!getConstantInt32(rangeExpr.left(), left) ||
              !getConstantInt32(rangeExpr.right(), right) ||
              right <= 0) {
            std::ostringstream reason;
            reason << "unable to resolve constant indexed range-select bounds in always_comb assignment: "
                   << describeExpression(assignedLHS);
            failureReason = reason.str();
            return false;
          }
          lsbIndex = static_cast<int64_t>(left);
          if (rangeExpr.getSelectionKind() ==
              slang::ast::RangeSelectionKind::IndexedDown) {
            lsbIndex -= static_cast<int64_t>(sliceElements - 1);
          }
          break;
        }
      }

      std::vector<size_t> sliceOffsets;
      if (!collectSliceOffsets(lsbIndex, sliceOffsets)) {
        std::ostringstream reason;
        reason << "constant range-select out of range in always_comb assignment: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
      }

      for (size_t bit = 0; bit < sliceOffsets.size(); ++bit) {
        dataBits[sliceOffsets[bit]] = assignedBits[bit];
      }
      return true;
    }

    bool applyCombinationalStatementForLhs(
      SNLDesign* design,
      const Statement& stmt,
      const Expression& lhsExpr,
      const std::vector<SNLBitNet*>& lhsBits,
      std::vector<SNLBitNet*>& dataBits,
      size_t& tempIndex,
      std::string& failureReason,
      const std::unordered_set<const slang::ast::ValueSymbol*>* ignoredSymbols = nullptr,
      CombinationalSubtreeSummaryCache* subtreeSummaryCache = nullptr) {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true; // LCOV_EXCL_LINE
      }
      if (subtreeSummaryCache) {
        const slang::ast::ValueSymbol* trackedSymbol = nullptr;
        if (tryGetRootValueSymbolReference(lhsExpr, trackedSymbol)) {
          const auto& subtreeSummary = getOrComputeCombinationalSubtreeSummary(
            *current,
            *subtreeSummaryCache,
            ignoredSymbols);
          if (!subtreeSummary.hasUnknownEffects &&
              !subtreeSummary.mayBreak &&
              !subtreeSummary.affectedSymbols.contains(trackedSymbol)) {
            return true;
          }
        }
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!applyCombinationalStatementForLhs(
                design,
                *item,
                lhsExpr,
                lhsBits,
                dataBits,
                tempIndex,
                failureReason,
                ignoredSymbols,
                subtreeSummaryCache)) {
            return false;
          }
          if (isCurrentForLoopBreakRequested()) {
            break;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::ForLoop) {
        const auto& forStmt = current->as<slang::ast::ForLoopStatement>();
        return unrollForLoopStatement(
          forStmt,
          [&]() {
            return applyCombinationalStatementForLhs(
              design,
              forStmt.body,
              lhsExpr,
              lhsBits,
              dataBits,
              tempIndex,
              failureReason,
              ignoredSymbols,
              subtreeSummaryCache);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        std::string conditionFailureReason;
        auto* conditionBit = resolveCombinationalConditionNet(
          design,
          *condStmt.conditions[0].expr,
          &conditionFailureReason);
        if (!conditionBit) {
          failureReason = conditionFailureReason;
          return false;
        }

        auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
        auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
        if (conditionBit == const1 || conditionBit == const0) {
          const bool hasLoopContext = hasActiveForLoopContext();
          const bool incomingBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
          if (hasLoopContext) {
            setCurrentForLoopBreakRequested(false);
          }

          const Statement* selectedStmt =
            (conditionBit == const1) ? &condStmt.ifTrue : condStmt.ifFalse;
          if (!selectedStmt) {
            if (hasLoopContext) {
              setCurrentForLoopBreakRequested(incomingBreak);
            }
            return true;
          }

          std::vector<SNLBitNet*> selectedBits = dataBits;
          if (!applyCombinationalStatementForLhs(
                design,
                *selectedStmt,
                lhsExpr,
                lhsBits,
                selectedBits,
                tempIndex,
                failureReason,
                ignoredSymbols,
                subtreeSummaryCache)) {
            return false;
          }

          const bool selectedBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
          dataBits = std::move(selectedBits);
          if (hasLoopContext) {
            setCurrentForLoopBreakRequested(incomingBreak || selectedBreak);
          }
          return true;
        }

        const bool hasLoopContext = hasActiveForLoopContext();
        const bool incomingBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
        if (hasLoopContext) {
          setCurrentForLoopBreakRequested(false);
        }

        std::vector<SNLBitNet*> trueBits = dataBits;
        if (!applyCombinationalStatementForLhs(
              design,
              condStmt.ifTrue,
              lhsExpr,
              lhsBits,
              trueBits,
              tempIndex,
              failureReason,
              ignoredSymbols,
              subtreeSummaryCache)) {
          return false;
        }
        const bool trueBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;
        if (hasLoopContext) {
          setCurrentForLoopBreakRequested(false);
        }

        std::vector<SNLBitNet*> falseBits = dataBits;
        if (condStmt.ifFalse) {
          if (!applyCombinationalStatementForLhs(
                design,
                *condStmt.ifFalse,
                lhsExpr,
                lhsBits,
                falseBits,
                tempIndex,
                failureReason,
                ignoredSymbols,
                subtreeSummaryCache)) {
            return false;
          }
        }
        const bool falseBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;

        auto condSourceRange = getSourceRange(condStmt);
        if (trueBits == falseBits) {
          dataBits = std::move(trueBits);
          ++tempIndex;
          if (hasLoopContext) {
            // Only propagate breaks that are unconditional across branches.
            setCurrentForLoopBreakRequested(incomingBreak || (trueBreak && falseBreak));
          }
          return true;
        }
        std::vector<SNLBitNet*> mergedBits;
        if (!createMux2Instance(
              design,
              conditionBit,
              falseBits,
              trueBits,
              mergedBits,
              condSourceRange)) {
          // LCOV_EXCL_START
          failureReason = "unable to merge always_comb conditional branches";
          return false;
          // LCOV_EXCL_STOP
        }
        dataBits = std::move(mergedBits);
        ++tempIndex;
        if (hasLoopContext) {
          // Only propagate breaks that are unconditional across branches.
          setCurrentForLoopBreakRequested(incomingBreak || (trueBreak && falseBreak));
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Case) {
        const auto& caseStmt = current->as<slang::ast::CaseStatement>();

        std::vector<SNLBitNet*> mergedBits = dataBits;
        if (caseStmt.defaultCase) {
          std::vector<SNLBitNet*> defaultBits = dataBits;
          if (!applyCombinationalStatementForLhs(
                design,
                *caseStmt.defaultCase,
                lhsExpr,
                lhsBits,
                defaultBits,
                tempIndex,
                failureReason,
                ignoredSymbols,
                subtreeSummaryCache)) {
            return false;
          }
          mergedBits = std::move(defaultBits);
        }

        auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
        auto* const1 = static_cast<SNLBitNet*>(getConstNet(design, true));
        for (auto itemIt = caseStmt.items.rbegin(); itemIt != caseStmt.items.rend(); ++itemIt) {
          std::vector<SNLBitNet*> itemBits = dataBits;
          if (!applyCombinationalStatementForLhs(
                design,
                *itemIt->stmt,
                lhsExpr,
                lhsBits,
                itemBits,
                tempIndex,
                failureReason,
                ignoredSymbols,
                subtreeSummaryCache)) {
            return false;
          }

          auto* itemMatchBit = buildCaseItemMatchBit(
            design,
            caseStmt.expr,
            caseStmt,
            *itemIt,
            failureReason);
          if (!itemMatchBit) {
            return false;
          }

          auto itemSourceRange = getSourceRange(*itemIt->stmt);
          if (itemBits == mergedBits) {
            mergedBits = std::move(itemBits);
            ++tempIndex;
            continue;
          }
          std::vector<SNLBitNet*> selectedBits;
          if (!createMux2Instance(
                design,
                itemMatchBit,
                mergedBits,
                itemBits,
                selectedBits,
                itemSourceRange)) {
            return false; // LCOV_EXCL_LINE
          }
          mergedBits = std::move(selectedBits);
          ++tempIndex;
        }

        dataBits = std::move(mergedBits);
        return true;
      }

      if (current->kind == slang::ast::StatementKind::VariableDeclaration) {
        const auto& declStmt = current->as<slang::ast::VariableDeclStatement>();
        const auto* initializer = declStmt.symbol.getInitializer();
        const auto* strippedTrackedLHS = stripConversions(lhsExpr);
        if (!initializer || !strippedTrackedLHS ||
            !slang::ast::ValueExpressionBase::isKind(strippedTrackedLHS->kind) ||
            &strippedTrackedLHS->as<slang::ast::ValueExpressionBase>().symbol != &declStmt.symbol) {
          // Local variable declarations in always_comb are usually bookkeeping
          // only from the point of view of tracked LHS rewriting.
          return true;
        }
        std::vector<SNLBitNet*> initBits;
        if (!resolveExpressionBits(design, *initializer, lhsBits.size(), initBits) ||
            initBits.size() != lhsBits.size()) {
          std::ostringstream reason;
          reason << "unable to resolve always_comb initializer bits for local '"
                 << std::string(declStmt.symbol.name) << "'";
          failureReason = reason.str();
          return false;
        }
        dataBits = std::move(initBits);
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Break) {
        requestCurrentForLoopBreak();
        return true;
      }

      const Expression* assignedLHS = nullptr;
      AssignAction action;
      if (extractAssignment(*current, assignedLHS, action)) {
        if (!sameLhs(assignedLHS, &lhsExpr)) {
          bool handled = false;
          if (!tryApplyCombinationalElementSelectAssignmentForLhs(
                design,
                *assignedLHS,
                lhsExpr,
                action,
                lhsBits,
                dataBits,
                failureReason,
                handled)) {
            return false;
          }
          if (handled) {
            return true;
          }

          if (!tryApplyCombinationalRangeSelectAssignmentForLhs(
                design,
                *assignedLHS,
                lhsExpr,
                action,
                lhsBits,
                dataBits,
                failureReason,
                handled)) {
            return false;
          }
          if (handled) {
            return true;
          }

          const auto* strippedAssignedLHS = stripConversions(*assignedLHS);
          if (strippedAssignedLHS &&
              strippedAssignedLHS->kind == slang::ast::ExpressionKind::RangeSelect) {
            const auto& rangeExpr =
              strippedAssignedLHS->as<slang::ast::RangeSelectExpression>();
            const auto* rangeBaseExpr = stripConversions(rangeExpr.value());
            if (rangeBaseExpr && sameLhs(rangeBaseExpr, &lhsExpr)) {
              std::vector<SNLBitNet*> assignedLHSBits;
              if (resolveAssignmentLHSBits(design, *assignedLHS, assignedLHSBits) &&
                  assignedLHSBits.size() == lhsBits.size() &&
                  assignedLHSBits == lhsBits) {
                std::vector<SNLBitNet*> assignedBits;
                if (!buildCombinationalAssignBits(
                      design,
                      action,
                      lhsBits.size(),
                      assignedBits,
                      &dataBits,
                      failureReason)) {
                  return false;
                }
                dataBits = std::move(assignedBits);
                return true;
              }
            }
          }
          return true;
        }
        std::vector<SNLBitNet*> assignedBits;
        if (!buildCombinationalAssignBits(
              design,
              action,
              lhsBits.size(),
              assignedBits,
              &dataBits,
              failureReason)) {
          return false;
        }
        dataBits = std::move(assignedBits);
        return true;
      }

      std::ostringstream reason;
      reason << "unsupported statement kind while lowering always_comb block"
             << " (kind=" << current->kind << ")";
      failureReason = reason.str();
      return false;
    }

    bool lowerCombinationalProceduralBlock(
      SNLDesign* design,
      const Statement& stmt,
      const std::optional<slang::SourceRange>& sourceRange,
      std::string& failureReason,
      const std::unordered_set<const slang::ast::ValueSymbol*>* ignoredSymbols = nullptr) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      const auto designName = design->getName().getString();
      const auto makeCombinationalLHSScopeName =
        [&](const Expression& lhsExpr, size_t bitWidth) {
          std::ostringstream name;
          name << "SNLSVConstructorImpl::lowerCombinationalProceduralBlock.lhs("
               << designName;
          auto baseName = getExpressionBaseName(lhsExpr);
          if (!baseName.empty()) {
            name << ":" << baseName;
          }
          if (auto lhsSourceInfo = getSourceInfo(getSourceRange(lhsExpr))) {
            name << "@" << lhsSourceInfo->line;
          }
          name << ":w" << bitWidth << ")";
          return name.str();
        };
      const auto makeCombinationalLHSStepScopeName =
        [&](const char* phase,
            const Expression& lhsExpr,
            std::optional<size_t> bitWidth = std::nullopt,
            std::optional<size_t> tempIndex = std::nullopt,
            std::optional<size_t> changedBits = std::nullopt) {
          std::ostringstream name;
          name << "SNLSVConstructorImpl::lowerCombinationalProceduralBlock." << phase << "("
               << designName;
          auto baseName = getExpressionBaseName(lhsExpr);
          if (!baseName.empty()) {
            name << ":" << baseName;
          }
          if (auto lhsSourceInfo = getSourceInfo(getSourceRange(lhsExpr))) {
            name << "@" << lhsSourceInfo->line;
          }
          if (bitWidth) {
            name << ":w" << *bitWidth;
          }
          if (tempIndex) {
            name << ":t" << *tempIndex;
          }
          if (changedBits) {
            name << ":c" << *changedBits;
          }
          name << ")";
          return name.str();
        };
#endif
      std::vector<const Expression*> lhsExpressions;
      CombinationalSubtreeSummaryCache subtreeSummaryCache;
      {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        NajaPerf::Scope scope(
          std::string("SNLSVConstructorImpl::lowerCombinationalProceduralBlock.collectAssignedLHS(") +
          designName + ")");
#endif
        if (!collectAssignedLHSExpressions(
              stmt,
              lhsExpressions,
              &failureReason,
              true,
              ignoredSymbols)) {
          return false;
        }
      }

      for (const auto* rawLhsExpr : lhsExpressions) {
        if (!rawLhsExpr) {
          continue; // LCOV_EXCL_LINE
        }
        const auto* lhsExpr = getTrackedAlwaysCombLHS(rawLhsExpr);
        if (!lhsExpr) {
          continue; // LCOV_EXCL_LINE
        }
        std::vector<SNLBitNet*> lhsBits;
        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          NajaPerf::Scope scope(
            makeCombinationalLHSStepScopeName("resolveLHSBits", *lhsExpr));
#endif
          if (!resolveAssignmentLHSBits(design, *lhsExpr, lhsBits, &failureReason)) {
            return false;
          }
        }
        if (lhsBits.empty()) {
          continue; // LCOV_EXCL_LINE
        }

        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          NajaPerf::Scope scope(makeCombinationalLHSScopeName(*lhsExpr, lhsBits.size()));
#endif
          std::vector<SNLBitNet*> dataBits = lhsBits;
          size_t tempIndex = 0;
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(
              makeCombinationalLHSStepScopeName(
                "replayStatements",
                *lhsExpr,
                lhsBits.size()));
#endif
            if (!applyCombinationalStatementForLhs(
                  design,
                  stmt,
                  *lhsExpr,
                  lhsBits,
                  dataBits,
                  tempIndex,
                  failureReason,
                  ignoredSymbols,
                  &subtreeSummaryCache)) {
              return false;
            }
          }
          size_t changedBits = 0;
          for (size_t i = 0; i < lhsBits.size(); ++i) {
            if (dataBits[i] != lhsBits[i]) {
              ++changedBits;
            }
          }
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(
              makeCombinationalLHSStepScopeName(
                "emitAssigns",
                *lhsExpr,
                lhsBits.size(),
                tempIndex,
                changedBits));
#endif
            for (size_t i = 0; i < lhsBits.size(); ++i) {
              if (dataBits[i] == lhsBits[i]) {
                continue;
              }
              createAssignInstance(design, dataBits[i], lhsBits[i], sourceRange);
            }
          }
        }
      }
      return true;
    }

    bool lowerSequentialMultiAssignmentConditional(
      SNLDesign* design,
      const Statement& stmt,
      SNLBitNet* clkNet,
      slang::ast::EdgeKind clockEdge,
      const Expression* asyncResetEventExpr,
      const std::optional<slang::ast::EdgeKind>& asyncResetEventEdge,
      const std::optional<slang::SourceRange>& blockSourceRange,
      std::string& failureReason) {
      const Statement* current = unwrapStatement(stmt);
      if (!current || current->kind != slang::ast::StatementKind::Conditional) {
        failureReason = "top-level statement is not a conditional";
        return false;
      }

      const auto& topCond = current->as<slang::ast::ConditionalStatement>();
      std::vector<const Expression*> resetLHSExpressions;
      if (!collectAssignedLHSExpressions(
            topCond.ifTrue,
            resetLHSExpressions,
            &failureReason,
            true)) {
        return false;
      }
      if (resetLHSExpressions.empty()) {
        failureReason = "reset branch does not contain assignments";
        return false;
      }

      if (resetLHSExpressions.size() < 2) {
        std::vector<const Expression*> conditionalLHSExpressions;
        if (!collectAssignedLHSExpressions(*current, conditionalLHSExpressions)) {
          failureReason = "fallback currently supports only multi-LHS reset branches";
          return false;
        }
        if (conditionalLHSExpressions.size() != 1 ||
            !sameLhs(conditionalLHSExpressions.front(), resetLHSExpressions.front())) {
          failureReason = "fallback currently supports only multi-LHS reset branches";
          return false;
        }
        size_t maxAssignments = 0;
        if (!getSingleLHSFallbackPathAssignmentMax(
              *current,
              *resetLHSExpressions.front(),
              maxAssignments,
              &failureReason)) {
          return false;
        }
        if (maxAssignments > 1) {
          failureReason = "single-LHS fallback path contains multiple direct assignments";
          return false;
        }
      }

      for (const auto* lhsExpr : resetLHSExpressions) {
        auto* lhsNet = resolveExpressionNet(design, *lhsExpr);
        if (!lhsNet) {
          std::ostringstream reason;
          reason << "unable to resolve assignment LHS net for '"
                 << getExpressionBaseName(*lhsExpr) << "'";
          failureReason = reason.str();
          return false;
        }
        auto lhsBits = collectBits(lhsNet);

        auto baseName = getExpressionBaseName(*lhsExpr);
        // LCOV_EXCL_START
        if (baseName.empty() && !lhsNet->isUnnamed()) {
          baseName = lhsNet->getName().getString();
        }
        // LCOV_EXCL_STOP

        std::vector<SNLBitNet*> dataBits = lhsBits;
        std::vector<SNLBitNet*> incrementerBits;
        size_t tempIndex = 0;
        if (topCond.ifFalse) {
          if (!applySequentialStatementForLhs(
                design,
                *topCond.ifFalse,
                *lhsExpr,
                lhsNet,
                lhsBits,
                baseName,
                dataBits,
                incrementerBits,
                tempIndex,
                failureReason)) {
            return false;
          }
        }

        auto resetSourceRange = getSourceRange(*topCond.conditions[0].expr);
        std::vector<SNLBitNet*> resetBits = lhsBits;
        std::vector<SNLBitNet*> resetIncrementerBits;
        size_t resetTempIndex = 0;
        if (!applySequentialStatementForLhs(
              design,
              topCond.ifTrue,
              *lhsExpr,
              lhsNet,
              lhsBits,
              baseName,
              resetBits,
              resetIncrementerBits,
              resetTempIndex,
              failureReason)) {
          return false;
        }

        bool useAsyncResetDFFRN = false;
        bool useAsyncResetDFFRE = false;
        bool useAsyncResetDFFSE = false;
        SNLBitNet* asyncResetControlNet = nullptr;
        if (asyncResetEventExpr && asyncResetEventEdge && !resetBits.empty()) {
          auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));
          auto* constOne = static_cast<SNLBitNet*>(getConstNet(design, true));
          bool resetToZero = true;
          bool resetToOne = true;
          for (auto* bit : resetBits) {
            if (bit != constZero) {
              resetToZero = false;
            }
            if (bit != constOne) {
              resetToOne = false;
            }
            if (!resetToZero && !resetToOne) {
              break;
            }
          }
          auto* candidateResetNet =
            getSingleBitNet(resolveExpressionNet(design, *asyncResetEventExpr));
          if (candidateResetNet) {
            if (*asyncResetEventEdge == slang::ast::EdgeKind::NegEdge &&
                resetToZero &&
                NLDB0::getDFFRN() &&
                isActiveLowResetConditionForSignal(
                  *topCond.conditions[0].expr,
                  *asyncResetEventExpr)) {
              asyncResetControlNet = candidateResetNet;
              useAsyncResetDFFRN = true;
            } else if (
              *asyncResetEventEdge == slang::ast::EdgeKind::PosEdge &&
              isActiveHighResetConditionForSignal(
                *topCond.conditions[0].expr,
                *asyncResetEventExpr)) {
              if (resetToZero && NLDB0::getDFFRE()) {
                asyncResetControlNet = candidateResetNet;
                useAsyncResetDFFRE = true;
              } else if (resetToOne && NLDB0::getDFFSE()) {
                asyncResetControlNet = candidateResetNet;
                useAsyncResetDFFSE = true;
              }
            }
          }
        }

        if (!useAsyncResetDFFRN && !useAsyncResetDFFRE && !useAsyncResetDFFSE) {
          auto* resetNet = resolveConditionNet(
            design,
            *topCond.conditions[0].expr,
            joinName("rst", baseName),
            resetSourceRange);
          if (!resetNet) {
            std::ostringstream reason;
            reason << "unable to resolve reset condition net for '"
                   << getExpressionBaseName(*lhsExpr) << "'";
            failureReason = reason.str();
            return false;
          }
          auto rstNet = getOrCreateNamedNet(
            design,
            joinName("rst", baseName),
            lhsNet,
            resetSourceRange);
          auto rstBits = collectBits(rstNet);
          if (rstBits.size() != dataBits.size()) {
            // LCOV_EXCL_START
            std::ostringstream reason;
            reason << "reset mux width mismatch for '" << getExpressionBaseName(*lhsExpr) << "'";
            failureReason = reason.str();
            return false;
            // LCOV_EXCL_STOP
          }
          for (size_t i = 0; i < dataBits.size(); ++i) {
            createMux2Instance(
              design,
              resetNet,
              dataBits[i],
              resetBits[i],
              rstBits[i],
              resetSourceRange);
          }
          dataBits = std::move(rstBits);
        }

        auto* constEnableOne = static_cast<SNLBitNet*>(getConstNet(design, true));
        for (size_t i = 0; i < lhsBits.size(); ++i) {
          if (useAsyncResetDFFRN) {
            createDFFRNInstance(
              design,
              clkNet,
              dataBits[i],
              asyncResetControlNet,
              lhsBits[i],
              blockSourceRange);
          } else if (useAsyncResetDFFRE) {
            createDFFREInstance(
              design,
              clkNet,
              dataBits[i],
              constEnableOne,
              asyncResetControlNet,
              lhsBits[i],
              blockSourceRange);
          } else if (useAsyncResetDFFSE) {
            createDFFSEInstance(
              design,
              clkNet,
              dataBits[i],
              constEnableOne,
              asyncResetControlNet,
              lhsBits[i],
              blockSourceRange);
          } else {
            if (clockEdge == slang::ast::EdgeKind::NegEdge) {
              createDFFNInstance(
                design,
                clkNet,
                dataBits[i],
                lhsBits[i],
                blockSourceRange);
            } else {
              createDFFInstance(
                design,
                clkNet,
                dataBits[i],
                lhsBits[i],
                blockSourceRange);
            }
          }
        }
      }

      return true;
    }

    bool lowerSequentialDirectMultiAssignment(
      SNLDesign* design,
      const Statement& stmt,
      SNLBitNet* clkNet,
      slang::ast::EdgeKind clockEdge,
      const std::optional<slang::SourceRange>& blockSourceRange,
      std::string& failureReason) {
      const Statement* current = unwrapStatement(stmt);
      if (!current || current->kind == slang::ast::StatementKind::Conditional) {
        failureReason = "top-level statement is not a direct assignment block";
        return false;
      }

      std::vector<const Expression*> lhsExpressions;
      if (!collectAssignedLHSExpressions(*current, lhsExpressions, &failureReason)) {
        return false;
      }
      if (lhsExpressions.empty()) {
        failureReason = "statement does not contain assignments";
        return false;
      }

      for (const auto* lhsExpr : lhsExpressions) {
        const auto* strippedLhs = stripConversions(*lhsExpr);
        if (!strippedLhs) {
          continue; // LCOV_EXCL_LINE
        }
        switch (strippedLhs->kind) {
          case slang::ast::ExpressionKind::NamedValue:
            if (!getIntegralExpressionBitWidth(*strippedLhs)) {
              std::ostringstream reason;
              reason << "direct multi-assignment LHS is not an integral signal: "
                     << describeExpression(*lhsExpr);
              failureReason = reason.str();
              return false;
            }
            break;
          case slang::ast::ExpressionKind::ElementSelect:
          case slang::ast::ExpressionKind::RangeSelect:
          case slang::ast::ExpressionKind::MemberAccess:
            if (!getIntegralExpressionBitWidth(*strippedLhs)) {
              std::vector<SNLBitNet*> selectionBits;
              if (!resolveFixedUnpackedArraySelectionBits(design, *lhsExpr, selectionBits) ||
                  selectionBits.empty()) {
                std::ostringstream reason;
                reason << "direct multi-assignment LHS is not a supported integral selection: "
                       << describeExpression(*lhsExpr);
                failureReason = reason.str();
                return false;
              }
            }
            break;
          default: {
            std::ostringstream reason;
            reason << "direct multi-assignment LHS is not a supported integral target: "
                   << describeExpression(*lhsExpr);
            failureReason = reason.str();
            return false;
          }
        }
      }

      for (const auto* lhsExpr : lhsExpressions) {
        auto* lhsNet = resolveAssignmentBaseNet(design, *lhsExpr);
        std::vector<SNLBitNet*> lhsBits;
        if (!resolveAssignmentLHSBits(design, *lhsExpr, lhsBits, &failureReason) ||
            lhsBits.empty()) {
          if (failureReason.empty()) {
            std::ostringstream reason;
            reason << "unable to resolve assignment LHS bits for '"
                   << getExpressionBaseName(*lhsExpr) << "'";
            failureReason = reason.str();
          }
          return false;
        }

        auto baseName = getExpressionBaseName(*lhsExpr);
        if (baseName.empty() && lhsNet && !lhsNet->isUnnamed()) {
          baseName = lhsNet->getName().getString(); // LCOV_EXCL_LINE
        }

        std::vector<SNLBitNet*> dataBits = lhsBits;
        std::vector<SNLBitNet*> incrementerBits;
        size_t tempIndex = 0;
        if (!applySequentialStatementForLhs(
              design,
              *current,
              *lhsExpr,
              lhsNet,
              lhsBits,
              baseName,
              dataBits,
              incrementerBits,
              tempIndex,
              failureReason)) {
          return false;
        }

        for (size_t i = 0; i < lhsBits.size(); ++i) {
          if (clockEdge == slang::ast::EdgeKind::NegEdge) {
            createDFFNInstance(
              design,
              clkNet,
              dataBits[i],
              lhsBits[i],
              blockSourceRange);
          } else {
            createDFFInstance(
              design,
              clkNet,
              dataBits[i],
              lhsBits[i],
              blockSourceRange);
          }
        }
      }

      return true;
    }

    bool getConstantBit(const Expression& expr, bool& value) const {
      if (auto loopValue = getActiveForLoopConstant(expr)) {
        if (*loopValue == 0) {
          value = false;
          return true;
        }
        if (*loopValue == 1) {
          value = true;
          return true;
        }
      }
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }
      if (stripped->kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        const auto bitValue =
          stripped->as<slang::ast::UnbasedUnsizedIntegerLiteral>().getLiteralValue();
        if (bitValue.isUnknown()) {
          return false;
        }
        value = static_cast<bool>(bitValue);
        return true;
      }
      if (stripped->kind == slang::ast::ExpressionKind::IntegerLiteral) {
        const auto& literal = stripped->as<slang::ast::IntegerLiteral>();
        auto maybeValue = literal.getValue().as<uint64_t>();
        if (!maybeValue) {
          return false;
        }
        if (*maybeValue == 0) {
          value = false;
          return true;
        }
        if (*maybeValue == 1) {
          value = true;
          return true;
        }
        return false;
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      const Symbol* evalSymbol = getConstantEvalSymbol(*stripped);
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        if (evaluatedConstant && evaluatedConstant.isInteger()) {
          constant = &evaluatedConstant;
        }
      }
      slang::ConstantValue convertedConstant;
      if (constant && !constant->isInteger()) {
        convertedConstant = constant->convertToInt();
        if (convertedConstant && convertedConstant.isInteger()) {
          constant = &convertedConstant;
        }
      }
      if (!constant || !constant->isInteger()) {
        return false;
      }
      auto maybeValue = constant->integer().as<uint64_t>();
      if (!maybeValue) {
        return false;
      }
      if (*maybeValue == 0) {
        value = false;
        return true;
      }
      if (*maybeValue == 1) {
        value = true;
        return true;
      }
      return false;
    }

    bool resolveConstantExpressionBits(
      SNLDesign* design,
      const Expression& expr,
      size_t targetWidth,
      std::vector<SNLBitNet*>& bits) {
      bits.clear();
      if (!targetWidth) {
        return false; // LCOV_EXCL_LINE
      }
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false; // LCOV_EXCL_LINE
      }

      if (auto resolved = resolveUnbasedOrStructuredPatternBits(design, *stripped, targetWidth, bits)) {
        return *resolved;
      }

      uint64_t constantValue = 0;
      if (getConstantUnsigned(*stripped, constantValue)) {
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          const bool one = i < 64 && ((constantValue >> i) & 1ULL);
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        return true;
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      if ((!constant || !constant->isInteger()) && stripped->getSymbolReference()) {
        slang::ast::EvalContext evalContext(*stripped->getSymbolReference());
        evaluatedConstant = stripped->eval(evalContext);
        if (evaluatedConstant && evaluatedConstant.isInteger()) {
          constant = &evaluatedConstant;
        }
      }
      slang::ConstantValue convertedConstant;
      if (constant && !constant->isInteger()) {
        convertedConstant = constant->convertToInt();
        if (convertedConstant && convertedConstant.isInteger()) {
          constant = &convertedConstant;
        }
      }
      if (!constant || !constant->isInteger()) {
        int64_t signedValue = 0;
        if (!getConstantInt64(expr, signedValue)) {
          return false;
        }
        const auto unsignedValue = static_cast<uint64_t>(signedValue);
        const bool fillBit = signedValue < 0;
        bits.reserve(targetWidth);
        for (size_t i = 0; i < targetWidth; ++i) {
          const bool one = i < 64 ? ((unsignedValue >> i) & 1ULL) : fillBit;
          bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
        }
        return true;
      }

      const auto& intValue = constant->integer();
      if (intValue.hasUnknown()) {
        return false;
      }
      const auto integerWidth = static_cast<size_t>(intValue.getBitWidth());
      bits.reserve(targetWidth);
      for (size_t i = 0; i < targetWidth; ++i) {
        bool one = false;
        if (i < integerWidth) {
          const auto bit = intValue[static_cast<int32_t>(i)];
          if (bit.isUnknown()) {
            return false; // LCOV_EXCL_LINE
          }
          one = static_cast<bool>(bit);
        }
        bits.push_back(static_cast<SNLBitNet*>(getConstNet(design, one)));
      }
      return true;
    }

    std::vector<SNLBitNet*> buildAssignBits(
      SNLDesign* design,
      const AssignAction& action,
      SNLNet* lhsNet,
      const std::vector<SNLBitNet*>& lhsBits,
      const std::vector<SNLBitNet*>* incrementerBits = nullptr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto getIncrementerBits = [&]() -> const std::vector<SNLBitNet*>& {
        if (!incrementerBits || incrementerBits->empty()) {
          throw SNLSVConstructorException("Internal error: missing precomputed incrementer bits in sequential assignment"); // LCOV_EXCL_LINE
        }
        return *incrementerBits;
      }; // LCOV_EXCL_LINE
      if (action.stepDelta > 0) {
        return getIncrementerBits();
      }
      if (action.stepDelta < 0) {
        reportUnsupportedElement(
          "Unsupported decrement assignment in sequential block",
          sourceRange);
        return {};
      }
      if (!action.rhs) {
        throw SNLSVConstructorException("Internal error: missing RHS expression in sequential assignment"); // LCOV_EXCL_LINE
      }
      const auto* rhsExpr = stripConversions(*action.rhs);
      bool constValue = false;
      if (rhsExpr && getConstantBit(*rhsExpr, constValue)) {
        auto constantNet = getConstNet(design, constValue);
        return std::vector<SNLBitNet*>(lhsBits.size(), constantNet);
      }
      if (rhsExpr &&
          rhsExpr->kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        const auto value =
          rhsExpr->as<slang::ast::UnbasedUnsizedIntegerLiteral>().getLiteralValue();
        if (value.isUnknown()) {
          reportWarning(
            "Unbased unsized unknown literal in sequential assignment lowered as 0 in SNL "
            "(X/Z distinction is not preserved)",
            sourceRange);
          auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));
          return std::vector<SNLBitNet*>(lhsBits.size(), constZero);
        }
      }
      std::vector<SNLBitNet*> constantBits;
      if (rhsExpr &&
          resolveConstantExpressionBits(design, *rhsExpr, lhsBits.size(), constantBits)) {
        return constantBits;
      }
      if (rhsExpr && rhsExpr->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& bin = rhsExpr->as<slang::ast::BinaryExpression>();
        if (bin.op == slang::ast::BinaryOperator::Add) {
          auto leftNet = resolveExpressionNet(design, bin.left());
          auto rightNet = resolveExpressionNet(design, bin.right());
          const auto* leftExpr = stripConversions(bin.left());
          const auto* rightExpr = stripConversions(bin.right());
          bool constOne = false;
          if (leftNet == lhsNet && rightExpr && getConstantBit(*rightExpr, constOne) && constOne) {
            return getIncrementerBits();
          }
          if (rightNet == lhsNet && leftExpr && getConstantBit(*leftExpr, constOne) && constOne) {
            return getIncrementerBits();
          }
        }
        if (!gateTypeFromBinary(bin.op)) {
          const bool supportedArithmeticOp =
            bin.op == slang::ast::BinaryOperator::Add ||
            bin.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
            bin.op == slang::ast::BinaryOperator::ArithmeticShiftLeft ||
            bin.op == slang::ast::BinaryOperator::LogicalShiftRight ||
            bin.op == slang::ast::BinaryOperator::ArithmeticShiftRight;
          if (!supportedArithmeticOp) {
            std::ostringstream reason;
            reason << "Unsupported binary operator in sequential assignment: "
                   << slang::ast::OpInfo::getText(bin.op);
            reportUnsupportedElement(reason.str(), sourceRange);
            return {};
          }
        }
      }
      if (!rhsExpr) {
        throw SNLSVConstructorException("Internal error: null RHS expression in sequential assignment"); // LCOV_EXCL_LINE
      }
      auto rhsNet = resolveExpressionNet(design, *rhsExpr);
      std::vector<SNLBitNet*> rhsBits;
      if (rhsNet) {
        rhsBits = collectBits(rhsNet);
        if (rhsBits.size() == lhsBits.size()) {
          return rhsBits;
        }
      }

      std::vector<SNLBitNet*> resolvedBits;
      if (resolveExpressionBits(design, *rhsExpr, lhsBits.size(), resolvedBits) &&
          resolvedBits.size() == lhsBits.size()) {
        return resolvedBits;
      }

      resolvedBits.clear();
      if (resolveFixedUnpackedArraySelectionBits(design, *rhsExpr, resolvedBits) &&
          resolvedBits.size() == lhsBits.size()) {
        return resolvedBits;
      }

      if (!rhsNet) {
        reportUnsupportedElement("Unsupported RHS in sequential assignment", sourceRange);
        return {};
      }
      return {}; // LCOV_EXCL_LINE
    }

    struct ClockEventInfo {
      const Expression* expr {nullptr};
      slang::ast::EdgeKind edge {slang::ast::EdgeKind::PosEdge};
    };

    const Expression* getClockExpression(const TimingControl& timing) {
      if (timing.kind == slang::ast::TimingControlKind::SignalEvent) {
        const auto& event = timing.as<slang::ast::SignalEventControl>();
        if (event.edge == slang::ast::EdgeKind::PosEdge) {
          return &event.expr;
        }
        reportUnsupportedElement(
          "Unsupported sequential timing edge; only posedge is supported",
          getSourceRange(timing));
        return nullptr;
      } else if (timing.kind == slang::ast::TimingControlKind::EventList) {
        const auto& eventList = timing.as<slang::ast::EventListControl>();
        const Expression* clockExpr = nullptr;
        for (const auto* eventCtrl : eventList.events) {
          if (!eventCtrl ||
              eventCtrl->kind != slang::ast::TimingControlKind::SignalEvent) {
            // LCOV_EXCL_START
            reportUnsupportedElement(
              "Unsupported sequential event list; only signal events are supported",
              getSourceRange(timing));
            return nullptr;
            // LCOV_EXCL_STOP
          }

          const auto& event = eventCtrl->as<slang::ast::SignalEventControl>();
          if (event.edge == slang::ast::EdgeKind::PosEdge) {
            // Keep the first posedge event as clock. Additional posedge/negedge
            // events are handled separately as potential async controls.
            if (!clockExpr) {
              clockExpr = &event.expr;
            }
            continue;
          }

          if (event.edge == slang::ast::EdgeKind::NegEdge) {
            // Allow common async-reset sensitivity lists:
            //   @(posedge clk or negedge rst_n)
            // while still using only the posedge event as DFF clock.
            continue;
          }

          reportUnsupportedElement(
            "Unsupported sequential timing edge in event list; only posedge/negedge are supported",
            getSourceRange(*eventCtrl));
          return nullptr;
        }
        if (clockExpr) {
          return clockExpr;
        }

        reportUnsupportedElement(
          "Unsupported sequential event list; missing posedge clock event",
          getSourceRange(timing));
        return nullptr;
      }
      reportUnsupportedElement(
        "Unsupported sequential timing control",
        getSourceRange(timing));
      return nullptr;
    }

    std::optional<ClockEventInfo> getSequentialClockEventInfo(const TimingControl& timing) {
      if (timing.kind == slang::ast::TimingControlKind::SignalEvent) {
        const auto& event = timing.as<slang::ast::SignalEventControl>();
        if (event.edge == slang::ast::EdgeKind::PosEdge ||
            event.edge == slang::ast::EdgeKind::NegEdge) {
          return ClockEventInfo{&event.expr, event.edge};
        }
        reportUnsupportedElement(
          "Unsupported sequential timing edge; only posedge/negedge are supported",
          getSourceRange(timing));
        return std::nullopt;
      }

      const auto* clockExpr = getClockExpression(timing);
      if (!clockExpr) {
        return std::nullopt;
      }
      return ClockEventInfo{clockExpr, slang::ast::EdgeKind::PosEdge};
    }

    bool isImplicitCombinationalTimingControl(const TimingControl& timing) const {
      return timing.kind == slang::ast::TimingControlKind::ImplicitEvent;
    }

    struct AsyncEventInfo {
      const Expression* expr {nullptr};
      std::optional<slang::ast::EdgeKind> edge;
      bool multiple {false};
    };

    AsyncEventInfo getSingleAsyncEventExpression(
      const TimingControl& timing,
      const Expression& clockExpr) const {
      AsyncEventInfo info;
      if (timing.kind != slang::ast::TimingControlKind::EventList) {
        return info;
      }
      const auto& eventList = timing.as<slang::ast::EventListControl>();
      bool skippedClock = false;
      for (const auto* eventCtrl : eventList.events) {
        if (!eventCtrl ||
            eventCtrl->kind != slang::ast::TimingControlKind::SignalEvent) {
          continue; // LCOV_EXCL_LINE
        }
        const auto& event = eventCtrl->as<slang::ast::SignalEventControl>();
        if (!skippedClock &&
            event.edge == slang::ast::EdgeKind::PosEdge &&
            sameLhs(&event.expr, &clockExpr)) {
          skippedClock = true;
          continue;
        }
        if (!info.expr) {
          info.expr = &event.expr;
          info.edge = event.edge;
          continue;
        }
        info.multiple = true;
        info.expr = nullptr;
        info.edge.reset();
        return info;
      }
      return info;
    }

    bool isActiveLowResetConditionForSignal(
      const Expression& resetConditionExpr,
      const Expression& resetSignalExpr) const {
      const auto* condition = stripConversions(resetConditionExpr);
      if (!condition || condition->kind != slang::ast::ExpressionKind::UnaryOp) {
        return false;
      }
      const auto& unaryExpr = condition->as<slang::ast::UnaryExpression>();
      if (unaryExpr.op != slang::ast::UnaryOperator::LogicalNot &&
          unaryExpr.op != slang::ast::UnaryOperator::BitwiseNot) {
        return false;
      }
      return sameLhs(&unaryExpr.operand(), &resetSignalExpr);
    }

    bool isActiveHighResetConditionForSignal(
      const Expression& resetConditionExpr,
      const Expression& resetSignalExpr) const {
      const auto* condition = stripConversions(resetConditionExpr);
      if (!condition) {
        return false; // LCOV_EXCL_LINE
      }
      return sameLhs(condition, &resetSignalExpr);
    }

    void createDFFInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dff = NLDB0::getDFF();
      auto inst = SNLInstance::create(design, dff);
      annotateSourceInfo(inst, sourceRange);
      auto cTerm = NLDB0::getDFFClock();
      auto dTerm = NLDB0::getDFFData();
      auto qTerm = NLDB0::getDFFOutput();
      if (cTerm) {
        if (auto instTerm = inst->getInstTerm(cTerm)) {
          instTerm->setNet(clkNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createDLatchInstance(
      SNLDesign* design,
      SNLNet* enableNet,
      SNLNet* dNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dlatch = NLDB0::getDLatch();
      auto inst = SNLInstance::create(design, dlatch);
      annotateSourceInfo(inst, sourceRange);
      auto eTerm = NLDB0::getDLatchEnable();
      auto dTerm = NLDB0::getDLatchData();
      auto qTerm = NLDB0::getDLatchOutput();
      if (eTerm) {
        if (auto instTerm = inst->getInstTerm(eTerm)) {
          instTerm->setNet(enableNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createDFFNInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dffn = NLDB0::getDFFN();
      auto inst = SNLInstance::create(design, dffn);
      annotateSourceInfo(inst, sourceRange);
      auto cTerm = NLDB0::getDFFNClock();
      auto dTerm = NLDB0::getDFFNData();
      auto qTerm = NLDB0::getDFFNOutput();
      if (cTerm) {
        if (auto instTerm = inst->getInstTerm(cTerm)) {
          instTerm->setNet(clkNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createDFFRNInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* resetNNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dffrn = NLDB0::getDFFRN();
      auto inst = SNLInstance::create(design, dffrn);
      annotateSourceInfo(inst, sourceRange);
      auto cTerm = NLDB0::getDFFRNClock();
      auto dTerm = NLDB0::getDFFRNData();
      auto rnTerm = NLDB0::getDFFRNResetN();
      auto qTerm = NLDB0::getDFFRNOutput();
      if (cTerm) {
        if (auto instTerm = inst->getInstTerm(cTerm)) {
          instTerm->setNet(clkNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (rnTerm) {
        if (auto instTerm = inst->getInstTerm(rnTerm)) {
          instTerm->setNet(resetNNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createDFFEInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* enableNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dffe = NLDB0::getDFFE();
      auto inst = SNLInstance::create(design, dffe);
      annotateSourceInfo(inst, sourceRange);
      auto cTerm = NLDB0::getDFFEClock();
      auto dTerm = NLDB0::getDFFEData();
      auto eTerm = NLDB0::getDFFEEnable();
      auto qTerm = NLDB0::getDFFEOutput();
      if (cTerm) {
        if (auto instTerm = inst->getInstTerm(cTerm)) {
          instTerm->setNet(clkNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (eTerm) {
        if (auto instTerm = inst->getInstTerm(eTerm)) {
          instTerm->setNet(enableNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createDFFREInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* enableNet,
      SNLNet* resetNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dffre = NLDB0::getDFFRE();
      auto inst = SNLInstance::create(design, dffre);
      annotateSourceInfo(inst, sourceRange);
      auto cTerm = NLDB0::getDFFREClock();
      auto dTerm = NLDB0::getDFFREData();
      auto eTerm = NLDB0::getDFFREEnable();
      auto rTerm = NLDB0::getDFFREReset();
      auto qTerm = NLDB0::getDFFREOutput();
      if (cTerm) {
        if (auto instTerm = inst->getInstTerm(cTerm)) {
          instTerm->setNet(clkNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (eTerm) {
        if (auto instTerm = inst->getInstTerm(eTerm)) {
          instTerm->setNet(enableNet);
        }
      }
      if (rTerm) {
        if (auto instTerm = inst->getInstTerm(rTerm)) {
          instTerm->setNet(resetNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createDFFSEInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* enableNet,
      SNLNet* setNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dffse = NLDB0::getDFFSE();
      auto inst = SNLInstance::create(design, dffse);
      annotateSourceInfo(inst, sourceRange);
      auto cTerm = NLDB0::getDFFSEClock();
      auto dTerm = NLDB0::getDFFSEData();
      auto eTerm = NLDB0::getDFFSEEnable();
      auto sTerm = NLDB0::getDFFSESet();
      auto qTerm = NLDB0::getDFFSEOutput();
      if (cTerm) {
        if (auto instTerm = inst->getInstTerm(cTerm)) {
          instTerm->setNet(clkNet);
        }
      }
      if (dTerm) {
        if (auto instTerm = inst->getInstTerm(dTerm)) {
          instTerm->setNet(dNet);
        }
      }
      if (eTerm) {
        if (auto instTerm = inst->getInstTerm(eTerm)) {
          instTerm->setNet(enableNet);
        }
      }
      if (sTerm) {
        if (auto instTerm = inst->getInstTerm(sTerm)) {
          instTerm->setNet(setNet);
        }
      }
      if (qTerm) {
        if (auto instTerm = inst->getInstTerm(qTerm)) {
          instTerm->setNet(qNet);
        }
      }
    }

    void createSequentialLogic(SNLDesign* design, const InstanceBodySymbol& body) {
      const auto moduleName = design->getName().getString();
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
      const auto makeSequentialPerfScopeName = [&](const char* phase) {
        return std::string("SNLSVConstructorImpl::createSequentialLogic.") + phase + "(" +
               moduleName + ")";
      };
      const auto makeProceduralBlockPerfScopeName =
        [&](const char* phase, const slang::ast::ProceduralBlockSymbol& block) {
          std::ostringstream name;
          name << "SNLSVConstructorImpl::createSequentialLogic." << phase << "(" << moduleName;
          if (auto sourceInfo = getSourceInfo(getSourceRange(block))) {
            name << ":" << sourceInfo->line;
          }
          name << ")";
          return name.str();
        };
#endif
      std::vector<const slang::ast::ProceduralBlockSymbol*> proceduralBlocks;
      auto collectProceduralBlock = [&](const Symbol& sym) {
        if (sym.kind == SymbolKind::ProceduralBlock) {
          proceduralBlocks.push_back(&sym.as<slang::ast::ProceduralBlockSymbol>());
        }
      };
      visitElaboratedNonGenerateMembers(body, collectProceduralBlock);
      for (const auto* blockPtr : proceduralBlocks) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.proceduralBlocksVisited;
#endif
        const auto& block = *blockPtr;
        auto blockSourceRange = getSourceRange(block);
        auto lowerCombinationalBlock = [&](const Statement& combinationalStmt) {
          std::unordered_set<const slang::ast::ValueSymbol*> ignoredSymbols;
          const std::unordered_set<const slang::ast::ValueSymbol*>* ignoredSymbolsPtr = nullptr;
          for (const auto& memory : inferredMemories_) {
            if (!memory.lowered) {
              continue;
            }
            if (memory.combBlock == &block && memory.shadowSymbol) {
              ignoredSymbols.insert(memory.shadowSymbol);
            }
            if (memory.commitBlock == &block && memory.commitSymbol) {
              ignoredSymbols.insert(memory.commitSymbol);
            }
          }
          if (!ignoredSymbols.empty()) {
            ignoredSymbolsPtr = &ignoredSymbols;
          }
          std::string combFailureReason;
          if (!lowerCombinationalProceduralBlock(
                design,
                combinationalStmt,
                blockSourceRange,
                combFailureReason,
                ignoredSymbolsPtr)) {
            std::ostringstream reason;
            reason << "Unsupported combinational block in module '" << moduleName << "'";
            if (!combFailureReason.empty()) {
              reason << ": " << combFailureReason;
            }
            reportUnsupportedElement(reason.str(), blockSourceRange);
            return false;
          }
          return true;
        };
        auto lowerAlwaysLatchBlock =
          [&](const Statement& latchStmt,
              const std::optional<slang::SourceRange>& latchSourceRange,
              std::string& latchFailureReason) -> bool {
          AlwaysLatchPattern pattern;
          if (!extractAlwaysLatchPattern(latchStmt, pattern)) {
            latchFailureReason = "unsupported statement pattern for always_latch lowering";
            return false;
          }

          auto* lhsNet = resolveExpressionNet(design, *pattern.lhs);
          if (!lhsNet) {
            latchFailureReason = "unable to resolve latch assignment LHS net";
            return false;
          }
          auto lhsBits = collectBits(lhsNet);
          if (lhsBits.empty()) {
            latchFailureReason = "unable to collect latch assignment LHS bits";
            return false;
          }

          auto baseName = getExpressionBaseName(*pattern.lhs);
          if (baseName.empty() && !lhsNet->isUnnamed()) {
            baseName = lhsNet->getName().getString();
          }

          auto getActionSourceRange = [&](const AssignAction& action) {
            if (action.rhs) {
              return getSourceRange(*action.rhs);
            }
            return latchSourceRange; // LCOV_EXCL_LINE
          };

          auto* enableNet = resolveConditionNet(
            design,
            *pattern.enableCond,
            joinName("latch_en", baseName),
            getSourceRange(*pattern.enableCond));
          if (!enableNet) {
            latchFailureReason = "unable to resolve latch enable condition net";
            return false;
          }

          std::vector<SNLBitNet*> incrementerBits;
          if (needsIncrementerForAction(design, lhsNet, pattern.dataAction) ||
              (pattern.hasDefault &&
               needsIncrementerForAction(design, lhsNet, pattern.defaultAction))) {
            auto* incNet = getOrCreateNamedNet(
              design,
              joinName("inc", baseName),
              lhsNet,
              latchSourceRange);
            auto incBits = collectBits(incNet);
            auto* incCarryNet = getOrCreateNamedNet(
              design,
              joinName("inc_carry", baseName),
              lhsNet,
              latchSourceRange);
            auto carryBits = collectBits(incCarryNet);
            incrementerBits = buildIncrementer(
              design,
              lhsBits,
              incBits,
              carryBits,
              latchSourceRange);
          }

          auto dataBits = buildAssignBits(
            design,
            pattern.dataAction,
            lhsNet,
            lhsBits,
            &incrementerBits,
            getActionSourceRange(pattern.dataAction));
          if (dataBits.empty()) {
            return false;
          }

          bool defaultIsHold = !pattern.hasDefault;
          if (pattern.hasDefault) {
            auto defaultBits = buildAssignBits(
              design,
              pattern.defaultAction,
              lhsNet,
              lhsBits,
              &incrementerBits,
              getActionSourceRange(pattern.defaultAction));
            if (defaultBits.empty()) {
              return false;
            }
            const bool explicitSelfAssignment =
              pattern.defaultAction.stepDelta == 0 &&
              pattern.defaultAction.rhs &&
              sameLhs(pattern.defaultAction.rhs, pattern.lhs);
            const bool bitwiseSelfAssignment =
              defaultBits.size() == lhsBits.size() &&
              std::equal(defaultBits.begin(), defaultBits.end(), lhsBits.begin());
            defaultIsHold = explicitSelfAssignment || bitwiseSelfAssignment;
          }

          if (!defaultIsHold) {
            latchFailureReason =
              "always_latch currently supports only implicit or explicit hold default branches";
            return false;
          }

          for (size_t i = 0; i < lhsBits.size(); ++i) {
            createDLatchInstance(
              design,
              enableNet,
              dataBits[i],
              lhsBits[i],
              latchSourceRange);
          }
          return true;
        };
        if (block.procedureKind == slang::ast::ProceduralBlockKind::AlwaysComb) {
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(makeProceduralBlockPerfScopeName("lowerAlwaysCombBlock", block));
            ++svPerfReport_.alwaysCombBlocksVisited;
#endif
            if (lowerCombinationalBlock(block.getBody())) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
              ++svPerfReport_.alwaysCombBlocksLowered;
#endif
            }
          }
          continue;
        }

        if (block.procedureKind == slang::ast::ProceduralBlockKind::Initial &&
            isIgnorableSequentialStatementTree(block.getBody())) {
          continue;
        }

        if (block.procedureKind == slang::ast::ProceduralBlockKind::AlwaysLatch) {
          const Statement* latchStmt = unwrapStatement(block.getBody());
          auto latchSourceRange = latchStmt ? getSourceRange(*latchStmt) : blockSourceRange;
          if (latchStmt && isIgnorableSequentialStatementTree(*latchStmt)) {
            continue;
          }
          std::string latchFailureReason;
          if (!latchStmt ||
              !lowerAlwaysLatchBlock(*latchStmt, latchSourceRange, latchFailureReason)) {
            if (!latchFailureReason.empty()) {
              std::ostringstream reason;
              reason << "Unsupported latch block in module '" << moduleName << "'";
              reason << ": " << latchFailureReason;
              reportUnsupportedElement(reason.str(), latchSourceRange);
            }
          }
          continue;
        }

        if (block.procedureKind != slang::ast::ProceduralBlockKind::AlwaysFF &&
            block.procedureKind != slang::ast::ProceduralBlockKind::Always) {
          std::ostringstream reason;
          reason << "Unsupported procedural block in module '" << moduleName
                 << "': unsupported procedure kind " << block.procedureKind
                 << " (only always/always_ff/always_comb/always_latch are currently lowered)";
          reportUnsupportedElement(reason.str(), blockSourceRange);
          continue;
        }
        if (auto inferred = inferredMemorySequentialBlocks_.find(&block);
            inferred != inferredMemorySequentialBlocks_.end() &&
            inferredMemories_[inferred->second].lowered) {
          continue;
        }

        const Statement* stmt = &block.getBody();
        const TimingControl* timing = nullptr;
        if (stmt) {
          if (const auto* timed = findTimedStatement(*stmt)) {
            timing = &timed->timing;
            stmt = &timed->stmt;
          }
        }
        stmt = stmt ? unwrapStatement(*stmt) : nullptr;

        if (block.procedureKind == slang::ast::ProceduralBlockKind::Always &&
            timing &&
            stmt &&
            isImplicitCombinationalTimingControl(*timing)) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          NajaPerf::Scope scope(makeProceduralBlockPerfScopeName("lowerAlwaysStarBlock", block));
          ++svPerfReport_.alwaysCombBlocksVisited;
#endif
          if (lowerCombinationalBlock(*stmt)) {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            ++svPerfReport_.alwaysCombBlocksLowered;
#endif
          }
          continue;
        }

#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.sequentialBlocksVisited;
#endif

        auto statementSourceRange = stmt ? getSourceRange(*stmt) : blockSourceRange;
        if (stmt && isIgnorableSequentialStatementTree(*stmt)) {
          continue;
        }

        SNLBitNet* clkNet = nullptr;
        auto clockEdge = slang::ast::EdgeKind::PosEdge;
        const Expression* asyncResetEventExpr = nullptr;
        std::optional<slang::ast::EdgeKind> asyncResetEventEdge;
        if (timing) {
          auto clockEvent = getSequentialClockEventInfo(*timing);
          if (clockEvent) {
            clockEdge = clockEvent->edge;
            clkNet = getSingleBitNet(resolveExpressionNet(design, *clockEvent->expr));
            auto asyncEvent = getSingleAsyncEventExpression(*timing, *clockEvent->expr);
            if (asyncEvent.multiple) {
              std::ostringstream reason;
              reason << "Unsupported sequential block in module '" << moduleName
                     << "': unsupported event list with multiple async events";
              reportUnsupportedElement(reason.str(), blockSourceRange);
              continue;
            }
            asyncResetEventExpr = asyncEvent.expr;
            asyncResetEventEdge = asyncEvent.edge;
          }
        }
        if (!clkNet) {
          clkNet = getSingleBitNet(design->getNet(NLName("clk")));
        }
        if (!clkNet) {
          std::ostringstream reason;
          reason << "Unsupported sequential block in module '" << moduleName
                 << "': unable to resolve a single-bit clock net";
          reportUnsupportedElement(reason.str(), blockSourceRange);
          continue;
        }

        AlwaysFFChain chain;
        bool extractedAlwaysFFChain = false;
        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          NajaPerf::Scope scope(makeSequentialPerfScopeName("extractAlwaysFFChain"));
#endif
          extractedAlwaysFFChain = stmt && extractAlwaysFFChain(*stmt, chain);
        }
        if (!extractedAlwaysFFChain) {
          std::string multiAssignFailureReason;
          if (stmt) {
            const Statement* current = unwrapStatement(*stmt);
            if (current &&
                current->kind == slang::ast::StatementKind::Conditional) {
              if (lowerSequentialMultiAssignmentConditional(
                    design,
                    *stmt,
                    clkNet,
                    clockEdge,
                    asyncResetEventExpr,
                    asyncResetEventEdge,
                    statementSourceRange,
                    multiAssignFailureReason)) {
                continue;
              }
            } else if (
              lowerSequentialDirectMultiAssignment(
                design,
                *stmt,
                clkNet,
                clockEdge,
                statementSourceRange,
                multiAssignFailureReason)) {
              continue;
            }
          }
          std::ostringstream reason;
          reason << "Unsupported sequential block in module '" << moduleName
                 << "': unsupported statement pattern for sequential lowering";
          if (!multiAssignFailureReason.empty()) {
            reason << " (" << multiAssignFailureReason << ")";
          }
          reportUnsupportedElement(reason.str(), statementSourceRange);
          continue;
        }
        const bool explicitHoldDefault =
          chain.hasDefault &&
          chain.defaultAction.stepDelta == 0 &&
          chain.defaultAction.rhs &&
          sameLhs(chain.defaultAction.rhs, chain.lhs);
        if (!chain.enableCond &&
            !asyncResetEventExpr &&
            chain.resetCond &&
            explicitHoldDefault) {
          // Pattern: if (cond) q <= d; else q <= q;
          // Lower as clock-enable instead of reset+mux.
          chain.enableCond = chain.resetCond;
          chain.enableAction = chain.resetAction;
          chain.resetCond = nullptr;
          chain.resetAction = AssignAction{};
          chain.defaultAction = AssignAction{};
          chain.hasDefault = false;
        }
        SNLNet* lhsNet = nullptr;
        std::vector<SNLBitNet*> lhsBits;
        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          NajaPerf::Scope scope(makeSequentialPerfScopeName("resolveLHS"));
#endif
          lhsNet = resolveExpressionNet(design, *chain.lhs);
          if (!lhsNet) {
            std::ostringstream reason;
            reason << "Unsupported sequential block in module '" << moduleName
                   << "': unable to resolve assignment LHS net";
            reportUnsupportedElement(reason.str(), statementSourceRange);
            continue;
          }
          lhsBits = collectBits(lhsNet);
          if (lhsBits.empty()) {
            // LCOV_EXCL_START
            std::ostringstream reason;
            reason << "Unsupported sequential block in module '" << moduleName
                   << "': unable to collect LHS bits";
            reportUnsupportedElement(reason.str(), statementSourceRange);
            continue;
            // LCOV_EXCL_STOP
          }
        }

        auto baseName = getExpressionBaseName(*chain.lhs);
        if (baseName.empty() && !lhsNet->isUnnamed()) { 
          baseName = lhsNet->getName().getString(); // LCOV_EXCL_LINE
        }

        auto getActionSourceRange = [&](const AssignAction& action) {
          if (action.rhs) {
            return getSourceRange(*action.rhs);
          }
          return statementSourceRange; // LCOV_EXCL_LINE
        };
        auto defaultSourceRange = chain.hasDefault
          ? getActionSourceRange(chain.defaultAction)
          : statementSourceRange;
        auto enableSourceRange = chain.enableCond
          ? getSourceRange(*chain.enableCond)
          : statementSourceRange;
        auto resetSourceRange = chain.resetCond
          ? getSourceRange(*chain.resetCond)
          : statementSourceRange; // LCOV_EXCL_LINE

        auto needsIncrementer = [&](const AssignAction& action) -> bool {
          if (action.stepDelta > 0) {
            return true;
          }
          if (!action.rhs) {
            return false;
          }
          const auto* rhsExpr = stripConversions(*action.rhs);
          if (!rhsExpr || rhsExpr->kind != slang::ast::ExpressionKind::BinaryOp) {
            return false;
          }
          const auto& bin = rhsExpr->as<slang::ast::BinaryExpression>();
          if (bin.op != slang::ast::BinaryOperator::Add) {
            return false;
          }
          auto leftNet = resolveExpressionNet(design, bin.left());
          auto rightNet = resolveExpressionNet(design, bin.right());
          const auto* leftExpr = stripConversions(bin.left());
          const auto* rightExpr = stripConversions(bin.right());
          bool constOne = false;
          if (leftNet == lhsNet && rightExpr && getConstantBit(*rightExpr, constOne) && constOne) {
            return true;
          }
          if (rightNet == lhsNet && leftExpr && getConstantBit(*leftExpr, constOne) && constOne) {
            return true;
          }
          return false;
        };

        std::vector<SNLBitNet*> incrementerBits;
        if (needsIncrementer(chain.resetAction) || needsIncrementer(chain.enableAction) ||
            (chain.hasDefault && needsIncrementer(chain.defaultAction))) {
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(makeSequentialPerfScopeName("buildIncrementer"));
#endif
            auto incNet = getOrCreateNamedNet(
              design,
              joinName("inc", baseName),
              lhsNet,
              statementSourceRange);
            auto incBits = collectBits(incNet);
            auto incCarryNet = getOrCreateNamedNet(
              design,
              joinName("inc_carry", baseName),
              lhsNet,
              statementSourceRange);
            auto carryBits = collectBits(incCarryNet);
            incrementerBits = buildIncrementer(
              design,
              lhsBits,
              incBits,
              carryBits,
              statementSourceRange);
          }
        }

        std::vector<SNLBitNet*> defaultBits;
        if (chain.hasDefault) {
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(makeSequentialPerfScopeName("buildDefaultBits"));
#endif
            defaultBits = buildAssignBits(
              design,
              chain.defaultAction,
              lhsNet,
              lhsBits,
              &incrementerBits,
              defaultSourceRange);
          }
          if (defaultBits.empty()) {
            continue;
          }
        } else {
          defaultBits = lhsBits;
        }

        std::vector<SNLBitNet*> enableBits;
        if (chain.enableCond) {
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(makeSequentialPerfScopeName("buildEnableBits"));
#endif
            enableBits = buildAssignBits(
              design,
              chain.enableAction,
              lhsNet,
              lhsBits,
              &incrementerBits,
              getActionSourceRange(chain.enableAction));
          }
          if (enableBits.empty()) {
            continue;
          }
        }

        std::vector<SNLBitNet*> resetBits;
        if (chain.resetCond) {
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(makeSequentialPerfScopeName("buildResetBits"));
#endif
            resetBits = buildAssignBits(
              design,
              chain.resetAction,
              lhsNet,
              lhsBits,
              &incrementerBits,
              getActionSourceRange(chain.resetAction));
          }
          if (resetBits.empty()) {
            continue;
          }
        }

        std::vector<SNLBitNet*> dataBits = defaultBits;
        SNLBitNet* enableNet = nullptr;
        bool canUseEnablePrimitive = false;
        bool defaultIsHold = !chain.hasDefault;
        if (chain.hasDefault) {
          const bool explicitSelfAssignment =
            chain.defaultAction.stepDelta == 0 &&
            chain.defaultAction.rhs &&
            sameLhs(chain.defaultAction.rhs, chain.lhs);
          const bool bitwiseSelfAssignment =
            defaultBits.size() == lhsBits.size() &&
            std::equal(defaultBits.begin(), defaultBits.end(), lhsBits.begin());
          defaultIsHold = explicitSelfAssignment || bitwiseSelfAssignment;
        }
        if (chain.enableCond) {
          enableNet = resolveConditionNet(
            design,
            *chain.enableCond,
            joinName("en", baseName),
            enableSourceRange);
          if (!enableNet) {
            std::ostringstream reason;
            reason << "Unsupported sequential block in module '" << moduleName
                   << "': unable to resolve enable condition net";
            reportUnsupportedElement(reason.str(), enableSourceRange);
            continue;
          }
          canUseEnablePrimitive =
            clockEdge == slang::ast::EdgeKind::PosEdge &&
            NLDB0::getDFFE() &&
            defaultIsHold;
        }

        bool useAsyncResetDFFRN = false;
        bool useAsyncResetDFFRE = false;
        bool useAsyncResetDFFSE = false;
        SNLBitNet* asyncResetControlNet = nullptr;
        if (chain.resetCond && asyncResetEventExpr && asyncResetEventEdge && !resetBits.empty()) {
          auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));
          auto* constOne = static_cast<SNLBitNet*>(getConstNet(design, true));
          bool resetToZero = true;
          bool resetToOne = true;
          for (auto* bit : resetBits) {
            if (bit != constZero) {
              resetToZero = false;
            }
            if (bit != constOne) {
              resetToOne = false;
            }
            if (!resetToZero && !resetToOne) {
              break;
            }
          }
          auto* candidateResetNet =
            getSingleBitNet(resolveExpressionNet(design, *asyncResetEventExpr));
          if (candidateResetNet) {
            if (*asyncResetEventEdge == slang::ast::EdgeKind::NegEdge &&
                resetToZero &&
                NLDB0::getDFFRN() &&
                isActiveLowResetConditionForSignal(*chain.resetCond, *asyncResetEventExpr)) {
              asyncResetControlNet = candidateResetNet;
              useAsyncResetDFFRN = true;
            } else if (
              *asyncResetEventEdge == slang::ast::EdgeKind::PosEdge &&
              isActiveHighResetConditionForSignal(*chain.resetCond, *asyncResetEventExpr)) {
              if (resetToZero && NLDB0::getDFFRE()) {
                asyncResetControlNet = candidateResetNet;
                useAsyncResetDFFRE = true;
              } else if (resetToOne && NLDB0::getDFFSE()) {
                asyncResetControlNet = candidateResetNet;
                useAsyncResetDFFSE = true;
              }
            }
          }
        }

        bool useClockEnablePrimitive = false;
        if (chain.enableCond && canUseEnablePrimitive) {
          if (!chain.resetCond || useAsyncResetDFFRE || useAsyncResetDFFSE) {
            useClockEnablePrimitive = true;
          }
        }

        if (chain.enableCond) {
          if (useClockEnablePrimitive) {
            dataBits = enableBits;
          } else {
            {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
              NajaPerf::Scope scope(makeSequentialPerfScopeName("buildEnableMux"));
#endif
              auto enNet = getOrCreateNamedNet(
                design,
                joinName("en", baseName),
                lhsNet,
                enableSourceRange);
              auto enBits = collectBits(enNet);
              if (enBits.size() != dataBits.size()) {
                // LCOV_EXCL_START
                std::ostringstream reason;
                reason << "Unsupported sequential block in module '" << moduleName
                       << "': enable mux width mismatch";
                reportUnsupportedElement(reason.str(), enableSourceRange);
                continue;
                // LCOV_EXCL_STOP
              }
              if (!createMux2Instance(
                    design,
                    enableNet,
                    dataBits,
                    enableBits,
                    enBits,
                    enableSourceRange,
                    enNet)) {
                // LCOV_EXCL_START
                std::ostringstream reason;
                reason << "Unsupported sequential block in module '" << moduleName
                       << "': unable to build enable mux";
                reportUnsupportedElement(reason.str(), enableSourceRange);
                continue;
                // LCOV_EXCL_STOP
              }
              dataBits = std::move(enBits);
            }
          }
        }

        if (chain.resetCond &&
            !useAsyncResetDFFRN &&
            !useAsyncResetDFFRE &&
            !useAsyncResetDFFSE) {
          {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
            NajaPerf::Scope scope(makeSequentialPerfScopeName("buildResetMux"));
#endif
            auto resetNet = resolveConditionNet(
              design,
              *chain.resetCond,
              joinName("rst", baseName),
              resetSourceRange);
            if (!resetNet) {
              std::ostringstream reason;
              reason << "Unsupported sequential block in module '" << moduleName
                     << "': unable to resolve reset condition net";
              reportUnsupportedElement(reason.str(), resetSourceRange);
              continue;
            }
            auto rstNet = getOrCreateNamedNet(
              design,
              joinName("rst", baseName),
              lhsNet,
              resetSourceRange);
            auto rstBits = collectBits(rstNet);
            if (rstBits.size() != dataBits.size()) {
              // LCOV_EXCL_START
              std::ostringstream reason;
              reason << "Unsupported sequential block in module '" << moduleName
                     << "': reset mux width mismatch";
              reportUnsupportedElement(reason.str(), resetSourceRange);
              continue;
              // LCOV_EXCL_STOP
            }
            if (!createMux2Instance(
                  design,
                  resetNet,
                  dataBits,
                  resetBits,
                  rstBits,
                  resetSourceRange,
                  rstNet)) {
              // LCOV_EXCL_START
              std::ostringstream reason;
              reason << "Unsupported sequential block in module '" << moduleName
                     << "': unable to build reset mux";
              reportUnsupportedElement(reason.str(), resetSourceRange);
              continue;
              // LCOV_EXCL_STOP
            }
            dataBits = std::move(rstBits);
          }
        }

        auto* constEnableOne = static_cast<SNLBitNet*>(getConstNet(design, true));
        {
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          NajaPerf::Scope scope(makeSequentialPerfScopeName("emitSequentialPrimitives"));
#endif
          for (size_t i = 0; i < lhsBits.size(); ++i) {
            if (useAsyncResetDFFRN) {
              createDFFRNInstance(
                design,
                clkNet,
                dataBits[i],
                asyncResetControlNet,
                lhsBits[i],
                statementSourceRange);
            } else if (useAsyncResetDFFRE) {
              createDFFREInstance(
                design,
                clkNet,
                dataBits[i],
                useClockEnablePrimitive ? enableNet : constEnableOne,
                asyncResetControlNet,
                lhsBits[i],
                statementSourceRange);
            } else if (useAsyncResetDFFSE) {
              createDFFSEInstance(
                design,
                clkNet,
                dataBits[i],
                useClockEnablePrimitive ? enableNet : constEnableOne,
                asyncResetControlNet,
                lhsBits[i],
                statementSourceRange);
            } else if (useClockEnablePrimitive) {
              createDFFEInstance(
                design,
                clkNet,
                dataBits[i],
                enableNet,
                lhsBits[i],
                statementSourceRange);
            } else {
              if (clockEdge == slang::ast::EdgeKind::NegEdge) {
                createDFFNInstance(
                  design,
                  clkNet,
                  dataBits[i],
                  lhsBits[i],
                  statementSourceRange);
              } else {
                createDFFInstance(
                  design,
                  clkNet,
                  dataBits[i],
                  lhsBits[i],
                  statementSourceRange);
              }
            }
          }
        }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.sequentialBlocksLowered;
#endif
      }
    }

    void createContinuousAssigns(SNLDesign* design, const InstanceBodySymbol& body) {
      const auto moduleName = design->getName().getString();
      std::function<void(const slang::ast::Scope&)> visitScope;
      visitScope = [&](const slang::ast::Scope& scope) {
        for (const auto& sym : scope.members()) {
          if (sym.kind == SymbolKind::GenerateBlock) {
            const auto& generateBlock = sym.as<slang::ast::GenerateBlockSymbol>();
            if (!generateBlock.isUninstantiated) {
              visitScope(generateBlock);
            }
            continue;
          }
          if (sym.kind == SymbolKind::GenerateBlockArray) {
            const auto& generateBlockArray =
              sym.as<slang::ast::GenerateBlockArraySymbol>();
            for (const auto* entry : generateBlockArray.entries) {
              if (entry && !entry->isUninstantiated) {
                visitScope(*entry);
              }
            }
            continue;
          }
          if (sym.kind != SymbolKind::ContinuousAssign) {
            continue;
          }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
        ++svPerfReport_.continuousAssignsVisited;
#endif
        const auto& continuousAssign = sym.as<slang::ast::ContinuousAssignSymbol>();
        const auto& assignment = continuousAssign.getAssignment();
        const auto& assignExpr = assignment.as<slang::ast::AssignmentExpression>();
        auto assignSourceRange = getSourceRange(assignExpr);
        auto lhsNet = resolveExpressionNet(design, assignExpr.left());
        std::vector<SNLBitNet*> lhsBits;
        bool lhsResolvedAsBitSlice = false;
        bool lhsResolvedAsPackedMemberSlice = false;
        if (!lhsNet) {
          const auto* lhsExpr = stripConversions(assignExpr.left());
          const bool lhsIsSupportedSlice =
            lhsExpr &&
            (lhsExpr->kind == slang::ast::ExpressionKind::ElementSelect ||
             lhsExpr->kind == slang::ast::ExpressionKind::RangeSelect ||
             lhsExpr->kind == slang::ast::ExpressionKind::MemberAccess ||
             lhsExpr->kind == slang::ast::ExpressionKind::Concatenation);
          if (lhsIsSupportedSlice) {
            auto lhsWidth = getIntegralExpressionBitWidth(assignExpr.left());
            if (lhsWidth && *lhsWidth) {
              const auto lhsWidthBits = static_cast<size_t>(*lhsWidth);
              if (resolveExpressionBits(design, assignExpr.left(), lhsWidthBits, lhsBits) &&
                  lhsBits.size() == lhsWidthBits &&
                  !lhsBits.empty()) {
                lhsResolvedAsBitSlice = true;
                lhsResolvedAsPackedMemberSlice =
                  lhsExpr->kind == slang::ast::ExpressionKind::MemberAccess;
                if (lhsBits.size() == 1) {
                  lhsNet = lhsBits.front();
                }
              }
            }
          }
          if (!lhsResolvedAsBitSlice) {
            std::ostringstream reason;
            reason << "Unsupported LHS in continuous assign in module '" << moduleName << "'";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue;
          }
        }

        const auto* rhs = stripConversions(assignExpr.right());

        auto unwrapSignedUnsignedCastCall = [&](const Expression* exprPtr) -> const Expression* {
          const Expression* current = exprPtr;
          while (current && current->kind == slang::ast::ExpressionKind::Call) {
            const auto& callExpr = current->as<slang::ast::CallExpression>();
            const auto subroutineName = callExpr.getSubroutineName();
            const bool isSignedCast =
              (subroutineName == "$signed" || subroutineName == "signed");
            const bool isUnsignedCast =
              (subroutineName == "$unsigned" || subroutineName == "unsigned");
            if (!isSignedCast && !isUnsignedCast) {
              break;
            }
            auto args = callExpr.arguments();
            if (args.size() != 1 || !args[0]) {
              break; // LCOV_EXCL_LINE
            }
            current = stripConversions(*args[0]);
          }
          return current;
        };

        const auto* rhsCastUnwrapped = unwrapSignedUnsignedCastCall(rhs);
        if (rhsCastUnwrapped && rhsCastUnwrapped->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = rhsCastUnwrapped->as<slang::ast::BinaryExpression>();
          const bool useBitSliceFallbackForShift =
            lhsResolvedAsBitSlice &&
            (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
             binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft ||
             binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight ||
             binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftRight);
          if (!useBitSliceFallbackForShift &&
              binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight) {
            std::string shiftFailureReason;
            if (!createLogicalRightShiftAssign(
                  design,
                  lhsNet,
                  binaryExpr.left(),
                  binaryExpr.right(),
                  assignSourceRange,
                  &shiftFailureReason)) {
              std::ostringstream reason;
              reason << "Unsupported binary expression in continuous assign: >>";
              if (!shiftFailureReason.empty()) {
                reason << " (" << shiftFailureReason << ")";
              }
              reportUnsupportedElement(reason.str(), assignSourceRange);
            }
            continue;
          }
          if (!useBitSliceFallbackForShift &&
              binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftRight) {
            std::string shiftFailureReason;
            if (!createArithmeticRightShiftAssign(
                  design,
                  lhsNet,
                  binaryExpr.left(),
                  binaryExpr.right(),
                  assignSourceRange,
                  &shiftFailureReason)) {
              std::ostringstream reason;
              reason << "Unsupported binary expression in continuous assign: >>>";
              if (!shiftFailureReason.empty()) {
                reason << " (" << shiftFailureReason << ")";
              }
              reportUnsupportedElement(reason.str(), assignSourceRange);
            }
            continue;
          }
          if (!useBitSliceFallbackForShift &&
              (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
               binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft)) {
            if (!createLogicalLeftShiftAssign(
                  design,
                  lhsNet,
                  binaryExpr.left(),
                  binaryExpr.right(),
                  assignSourceRange)) {
              std::ostringstream reason;
              reason << "Unsupported binary expression in continuous assign: "
                     << slang::ast::OpInfo::getText(binaryExpr.op);
              reportUnsupportedElement(reason.str(), assignSourceRange);
            }
            continue;
          }
        }

        std::optional<NLDB0::GateType> gateType;
        std::vector<const Expression*> operands;
        if (rhs->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = rhs->as<slang::ast::BinaryExpression>();
          const bool useBitSliceFallbackForBinary =
            lhsResolvedAsBitSlice &&
            (binaryExpr.op == slang::ast::BinaryOperator::Add ||
             binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
             binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft ||
             binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight ||
             binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftRight);
          if (!useBitSliceFallbackForBinary) {
            if (binaryExpr.op == slang::ast::BinaryOperator::Add) {
              if (!createAddAssign(
                    design,
                    lhsNet,
                    binaryExpr.left(),
                    binaryExpr.right(),
                    assignSourceRange)) {
                reportUnsupportedElement(
                  "Unsupported binary expression in continuous assign: +",
                  assignSourceRange);
              }
              continue;
            }
            if (binaryExpr.op == slang::ast::BinaryOperator::Multiply) {
              if (!createMultiplyAssign(
                    design,
                    lhsNet,
                    binaryExpr.left(),
                    binaryExpr.right(),
                    assignSourceRange)) {
                reportUnsupportedElement(
                  "Unsupported binary expression in continuous assign: *",
                  assignSourceRange);
              }
              continue;
            }
            if (binaryExpr.op == slang::ast::BinaryOperator::Subtract) {
              if (!createSubAssign(
                    design,
                    lhsNet,
                    binaryExpr.left(),
                    binaryExpr.right(),
                    assignSourceRange)) {
                reportUnsupportedElement(
                  "Unsupported binary expression in continuous assign: -",
                  assignSourceRange);
              }
              continue;
            }
            if (isEqualityBinaryOp(binaryExpr.op)) {
              reportCaseComparison2StateWarning(binaryExpr.op, assignSourceRange);
              if (!createEqualityAssign(
                    design,
                    lhsNet,
                    binaryExpr.left(),
                    binaryExpr.right(),
                    assignSourceRange)) {
                std::ostringstream reason;
                reason << "Unsupported binary expression in continuous assign: "
                       << slang::ast::OpInfo::getText(binaryExpr.op);
                reportUnsupportedElement(reason.str(), assignSourceRange);
              }
              continue;
            }
            if (isInequalityBinaryOp(binaryExpr.op)) {
              reportCaseComparison2StateWarning(binaryExpr.op, assignSourceRange);
              if (!createInequalityAssign(
                    design,
                    lhsNet,
                    binaryExpr.left(),
                    binaryExpr.right(),
                    assignSourceRange)) {
                std::ostringstream reason;
                reason << "Unsupported binary expression in continuous assign: "
                       << slang::ast::OpInfo::getText(binaryExpr.op);
                reportUnsupportedElement(reason.str(), assignSourceRange);
              }
              continue;
            }
            if (isRelationalBinaryOp(binaryExpr.op)) {
              if (!createRelationalAssign(
                    design,
                    lhsNet,
                    binaryExpr.left(),
                    binaryExpr.right(),
                    binaryExpr.op,
                    assignSourceRange)) {
                std::ostringstream reason;
                reason << "Unsupported binary expression in continuous assign: "
                       << slang::ast::OpInfo::getText(binaryExpr.op);
                reportUnsupportedElement(reason.str(), assignSourceRange);
              }
              continue;
            }
            gateType = gateTypeFromBinary(binaryExpr.op);
            if (!gateType) {
              std::ostringstream reason;
              reason << "Unsupported binary operator in continuous assign: "
                     << slang::ast::OpInfo::getText(binaryExpr.op);
              reportUnsupportedElement(reason.str(), assignSourceRange);
              continue;
            }
            collectBinaryOperands(*rhs, binaryExpr.op, operands);
          }
        } else if (rhs->kind == slang::ast::ExpressionKind::UnaryOp) {
          const auto& unaryExpr = rhs->as<slang::ast::UnaryExpression>();
          const auto* operandExpr = stripConversions(unaryExpr.operand());
          if (unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot && operandExpr &&
              operandExpr->kind == slang::ast::ExpressionKind::BinaryOp) {
            const auto& binaryExpr = operandExpr->as<slang::ast::BinaryExpression>();
            switch (binaryExpr.op) {
              case slang::ast::BinaryOperator::BinaryAnd:
                gateType = NLDB0::GateType(NLDB0::GateType::Nand);
                break;
              case slang::ast::BinaryOperator::BinaryOr:
                gateType = NLDB0::GateType(NLDB0::GateType::Nor);
                break;
              case slang::ast::BinaryOperator::BinaryXor:
                gateType = NLDB0::GateType(NLDB0::GateType::Xnor);
                break;
              default:
                std::ostringstream reason;
                reason << "Unsupported binary operator under bitwise not in continuous assign: "
                       << slang::ast::OpInfo::getText(binaryExpr.op);
                reportUnsupportedElement(reason.str(), assignSourceRange);
                continue;
            }
            if (gateType) {
              collectBinaryOperands(*operandExpr, binaryExpr.op, operands);
            }
          } else if (unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
            gateType = NLDB0::GateType(NLDB0::GateType::Not);
            if (operandExpr) {
              operands.push_back(operandExpr);
            }
          }
        }

        if (gateType && !operands.empty()) {
          std::vector<SNLBitNet*> lhsGateBits;
          if (lhsResolvedAsBitSlice) {
            lhsGateBits = lhsBits;
          } else if (lhsNet) {
            lhsGateBits = collectBits(lhsNet);
          }

          if (lhsResolvedAsBitSlice || lhsGateBits.size() > 1) {
            std::string gateFailureReason;
            if (!createBitwiseGateAssign(
                  design,
                  *gateType,
                  operands,
                  lhsGateBits,
                  assignSourceRange,
                  &gateFailureReason)) {
              std::ostringstream reason;
              reason << "Unsupported gate construction in continuous assign"
                     << " (gate=" << gateType->getString()
                     << ", lhs_width=" << lhsGateBits.size();
              if (!gateFailureReason.empty()) {
                reason << ", " << gateFailureReason;
              }
              reason << ")";
              reportUnsupportedElement(reason.str(), assignSourceRange);
            }
            continue;
          }

          std::vector<SNLNet*> inputNets;
          inputNets.reserve(operands.size());
          bool ok = true;
          size_t failedOperandIndex = 0;
          std::string failedOperandReason;
          for (size_t operandIndex = 0; operandIndex < operands.size(); ++operandIndex) {
            const auto* operand = operands[operandIndex];
            std::vector<SNLBitNet*> operandBits;
            std::string operandFailureReason;
              if (!resolveGateOperandBits(
                    design,
                    *operand,
                    1,
                    operandBits,
                    assignSourceRange,
                    &operandFailureReason)) {
                failedOperandIndex = operandIndex;
                failedOperandReason = operandFailureReason;
                ok = false;
                break;
              }
            inputNets.push_back(operandBits.front());
          }
          if (!ok) {
            std::ostringstream reason;
            reason << "Unsupported operand in continuous gate assign"
                   << " (gate=" << gateType->getString()
                   << ", operand#" << failedOperandIndex;
            if (!failedOperandReason.empty()) {
              reason << ", " << failedOperandReason;
            }
            reason << ")";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue;
          }

          SNLNet* gateOutNet = nullptr;
          std::string baseName = getExpressionBaseName(assignExpr.left());
          std::string gateOutName = joinName(gateType->getString(), baseName);
          if (gateType->isNOutput()) {
            gateOutNet = getOrCreateNamedNet(
              design,
              gateOutName,
              nullptr,
              assignSourceRange);
          } else {
            gateOutNet = getOrCreateNamedNet(
              design,
              gateOutName,
              lhsNet,
              assignSourceRange);
          }
          if (!createGateInstance(
                design,
                *gateType,
                inputNets,
                gateOutNet,
                assignSourceRange) ||
              !gateOutNet) {
            // LCOV_EXCL_START
            std::ostringstream reason;
            reason << "Unsupported gate construction in continuous assign"
                   << " (gate=" << gateType->getString()
                   << ", input_count=" << inputNets.size()
                   << ", output_name=" << gateOutName
                   << ", reason=createGateInstance failed)";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue;
            // LCOV_EXCL_STOP
          }
          createAssignInstance(design, gateOutNet, lhsNet, assignSourceRange);
          continue;
        }

        if (rhs->kind == slang::ast::ExpressionKind::ConditionalOp) {
          std::vector<SNLBitNet*> lhsAssignBits;
          if (lhsResolvedAsBitSlice) {
            lhsAssignBits = lhsBits;
          } else if (lhsNet) {
            lhsAssignBits = collectBits(lhsNet);
          }

          std::vector<SNLBitNet*> rhsBits;
          bool usedUnknownLiteralFallback = false;
          if ((!resolveExpressionBits(design, *rhs, lhsAssignBits.size(), rhsBits) ||
               rhsBits.size() != lhsAssignBits.size()) &&
              (!resolveUnknownLiteralBitsAsZero(
                 design,
                 *rhs,
                 lhsAssignBits.size(),
                 rhsBits,
                 usedUnknownLiteralFallback) ||
               rhsBits.size() != lhsAssignBits.size() || !usedUnknownLiteralFallback)) {
            reportUnsupportedElement(
              "Unsupported RHS in continuous assign in module '" + moduleName + "'",
              assignSourceRange);
            continue;
          }
          if (usedUnknownLiteralFallback) {
            reportWarning(
              "Unknown literal bits in continuous assign RHS lowered as 0 in SNL "
              "(X/Z distinction is not preserved)",
              assignSourceRange);
          }
          for (size_t i = 0; i < lhsAssignBits.size(); ++i) {
            createAssignInstance(design, rhsBits[i], lhsAssignBits[i], assignSourceRange);
          }
          continue;
        }

        if (lhsResolvedAsBitSlice) {
          const bool rhsIsFixedStreaming =
            rhs->kind == slang::ast::ExpressionKind::Streaming &&
            rhs->as<slang::ast::StreamingConcatenationExpression>().isFixedSize();
          const auto* rhsStripped = stripConversions(*rhs);
          const bool rhsIsResizableLiteral =
            rhsStripped &&
            (rhsStripped->kind == slang::ast::ExpressionKind::IntegerLiteral ||
             rhsStripped->kind ==
               slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral);
          size_t rhsWidthBits = lhsBits.size();
          if (!rhsIsFixedStreaming && !lhsResolvedAsPackedMemberSlice &&
              !rhsIsResizableLiteral) {
            auto rhsWidth = getIntegralExpressionBitWidth(*rhs);
            if (!rhsWidth || !*rhsWidth ||
                static_cast<size_t>(*rhsWidth) < lhsBits.size()) {
              reportUnsupportedElement(
                "Unsupported net compatibility in continuous assign",
                assignSourceRange);
              continue;
            }
          }
          std::vector<SNLBitNet*> rhsBits;
          if (!resolveExpressionBits(design, *rhs, rhsWidthBits, rhsBits) ||
              rhsBits.size() != lhsBits.size()) {
            std::ostringstream reason;
            reason << "Unsupported RHS in continuous assign in module '" << moduleName << "'";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue;
          }
          for (size_t i = 0; i < lhsBits.size(); ++i) {
            createAssignInstance(design, rhsBits[i], lhsBits[i], assignSourceRange);
          }
          continue;
        }

        auto rhsNet = resolveExpressionNet(design, *rhs);
        auto lhsAssignBits = collectBits(lhsNet);
        if (!rhsNet) {
          std::vector<SNLBitNet*> rhsBits;
          if (!lhsAssignBits.empty() &&
              resolveExpressionBits(design, *rhs, lhsAssignBits.size(), rhsBits) &&
              rhsBits.size() == lhsAssignBits.size()) {
            for (size_t i = 0; i < lhsAssignBits.size(); ++i) {
              createAssignInstance(design, rhsBits[i], lhsAssignBits[i], assignSourceRange);
            }
            continue;
          }
          std::ostringstream reason;
          reason << "Unsupported RHS in continuous assign in module '" << moduleName << "'";
          reportUnsupportedElement(reason.str(), assignSourceRange);
          continue;
        }
        if (!createDirectAssign(design, rhsNet, lhsNet, assignSourceRange)) {
          std::vector<SNLBitNet*> rhsBits;
          if (!lhsAssignBits.empty() &&
              resolveExpressionBits(design, *rhs, lhsAssignBits.size(), rhsBits) &&
              rhsBits.size() == lhsAssignBits.size()) {
            for (size_t i = 0; i < lhsAssignBits.size(); ++i) {
              createAssignInstance(design, rhsBits[i], lhsAssignBits[i], assignSourceRange);
            }
            continue;
          }
          reportUnsupportedElement(
            "Unsupported net compatibility in continuous assign",
            assignSourceRange);
        }
        }
      };
      visitScope(body);
    }

    void connectTermsToNets(SNLDesign* design) {
      for (auto term : design->getTerms()) {
        auto name = term->getName().getString();
        SNLNet* net = design->getNet(NLName(name));
        if (!net) {
          if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
            net = SNLBusNet::create(
              design,
              busTerm->getMSB(),
              busTerm->getLSB(),
              NLName(name));
          } else {
            net = SNLScalarNet::create(design, NLName(name));
          }
          SNLAttributes::cloneAttributes(term, net);
          cloneRTLInfos(term, net);
        }
        term->setNet(net);
      }
    }

    void createInstances(SNLDesign* design, const InstanceBodySymbol& body) {
      std::function<void(const slang::ast::Scope&)> visitScope;
      visitScope = [&](const slang::ast::Scope& scope) {
        for (const auto& sym : scope.members()) {
          if (sym.kind == SymbolKind::GenerateBlock) {
            const auto& generateBlock = sym.as<slang::ast::GenerateBlockSymbol>();
            if (!generateBlock.isUninstantiated) {
              visitScope(generateBlock);
            }
            continue;
          }
          if (sym.kind == SymbolKind::GenerateBlockArray) {
            const auto& generateBlockArray =
              sym.as<slang::ast::GenerateBlockArraySymbol>();
            for (const auto* entry : generateBlockArray.entries) {
              if (entry && !entry->isUninstantiated) {
                visitScope(*entry);
              }
            }
            continue;
          }
          if (sym.kind != SymbolKind::Instance) {
            continue;
          }
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          ++svPerfReport_.instanceSymbolsVisited;
#endif
          const auto& instance = sym.as<InstanceSymbol>();
          const auto* canonicalBody = instance.getCanonicalBody();
          auto modelDesign = buildDesign(canonicalBody ? *canonicalBody : instance.body);
          auto inst = SNLInstance::create(
            design,
            modelDesign,
            NLName(getLoweredSymbolName(instance)));
          annotateSourceInfo(inst, getSourceRange(instance));
          connectInstance(inst, instance);
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
          ++svPerfReport_.instancesCreated;
#endif
        }
      };
      visitScope(body);
    }

    void connectInstance(SNLInstance* inst, const InstanceSymbol& instance) {
      auto model = inst->getModel();
      for (const auto* conn : instance.getPortConnections()) {
        if (!conn) {
          continue; // LCOV_EXCL_LINE
        }
        std::string portName(conn->port.name);
        if (portName.empty()) {
          continue; // LCOV_EXCL_LINE
        }
        auto term = model->getTerm(NLName(portName));
        if (!term) {
          std::ostringstream reason;
          reason << "Unsupported instance connection: missing term '" << portName
                 << "' on model '" << model->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), getSourceRange(instance));
          continue; // LCOV_EXCL_LINE
        }
        const Expression* expr = conn->getExpression();
        if (!expr) {
          continue;
        }
        auto exprSourceRange = getSourceRange(*expr);
        const Expression* connectionExpr = stripConnectionLValueArgConversions(*expr);
        auto net = resolveExpressionNet(inst->getDesign(), *connectionExpr);

        auto reportInstanceConnectionFailure =
          [&](const std::string& failureReason) {
          std::ostringstream reason;
          reason << "Unsupported instance connection for port '" << portName
                 << "' on instance '" << inst->getName().getString()
                 << "': " << failureReason;
          reportUnsupportedElement(
            formatReasonWithSourceExcerpt(reason.str(), exprSourceRange),
            exprSourceRange);
        };

        auto resolveSelectableConnectionBits =
          [&](size_t targetWidth, std::vector<SNLBitNet*>& bits) -> bool {
          const bool isSelectable =
            connectionExpr->kind == slang::ast::ExpressionKind::ElementSelect ||
            connectionExpr->kind == slang::ast::ExpressionKind::RangeSelect ||
            connectionExpr->kind == slang::ast::ExpressionKind::MemberAccess;
          if (!isSelectable) {
            return false;
          }
          if (!resolveExpressionBits(inst->getDesign(), *connectionExpr, targetWidth, bits)) {
            return false;
          }
          return bits.size() == targetWidth;
        };
        auto resolveExactWidthConnectionBits =
          [&](size_t targetWidth, std::vector<SNLBitNet*>& bits) -> bool {
          auto exprWidth = getIntegralExpressionBitWidth(*connectionExpr);
          if (!exprWidth || *exprWidth <= 0 ||
              static_cast<size_t>(*exprWidth) != targetWidth) {
            return false;
          }
          if (!resolveExpressionBits(inst->getDesign(), *connectionExpr, targetWidth, bits)) {
            return false;
          }
          return bits.size() == targetWidth;
        };

        auto connectBusBits =
          [&](SNLBusTerm* busTerm,
              const std::vector<SNLBitNet*>& bits,
              std::string* failureReason = nullptr) -> bool {
          if (!busTerm || bits.size() != static_cast<size_t>(busTerm->getWidth())) {
            return false;
          }
          SNLInstance::Terms bitTerms;
          bitTerms.reserve(bits.size());
          auto termMSB = busTerm->getMSB();
          auto termLSB = busTerm->getLSB();
          auto termStep = termLSB <= termMSB ? 1 : -1;
          for (auto termBit = termLSB; termBit != termMSB + termStep; termBit += termStep) {
            auto* bitTerm = busTerm->getBit(termBit);
            if (!bitTerm) {
              return false; // LCOV_EXCL_LINE
            }
            bitTerms.push_back(bitTerm);
          }
          try {
            inst->setTermsNets(bitTerms, bits);
          } catch (const NLException& e) {
            if (failureReason) {
              *failureReason = e.what();
            }
            return false;
          }
          return true;
        };

        if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
          if (net) {
            auto instTerm = inst->getInstTerm(scalarTerm);
            if (instTerm) {
              try {
                instTerm->setNet(net);
              } catch (const NLException& e) {
                reportInstanceConnectionFailure(e.what());
                continue;
              }
            }
            continue;
          }
          std::vector<SNLBitNet*> connectionBits;
          if (resolveSelectableConnectionBits(1, connectionBits) ||
              resolveExactWidthConnectionBits(1, connectionBits)) {
            auto instTerm = inst->getInstTerm(scalarTerm);
            if (instTerm) {
              try {
                instTerm->setNet(connectionBits.front());
              } catch (const NLException& e) {
                reportInstanceConnectionFailure(e.what());
                continue;
              }
            }
            continue;
          }
          std::ostringstream reason;
          reason << "Unsupported instance connection expression for port '" << portName
                 << "' on instance '" << inst->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), exprSourceRange);
          continue;
        }
        auto busTerm = dynamic_cast<SNLBusTerm*>(term);
        auto busNet = dynamic_cast<SNLBusNet*>(net);
        if (busTerm && !busNet) {
          std::vector<SNLBitNet*> connectionBits;
          std::string failureReason;
          if (resolveSelectableConnectionBits(busTerm->getWidth(), connectionBits) ||
              resolveExactWidthConnectionBits(busTerm->getWidth(), connectionBits)) {
            if (connectBusBits(busTerm, connectionBits, &failureReason)) {
              continue;
            }
            if (!failureReason.empty()) {
              reportInstanceConnectionFailure(failureReason);
              continue;
            }
          }
        }
        if (!busTerm || !busNet) {
          std::ostringstream reason;
          reason << "Unsupported instance connection net/term compatibility for port '"
                 << portName << "' on instance '" << inst->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), exprSourceRange);
          continue;
        }
        std::string failureReason;
        if (!connectBusBits(busTerm, collectBits(busNet), &failureReason)) {
          if (!failureReason.empty()) {
            reportInstanceConnectionFailure(failureReason);
            continue;
          }
          std::ostringstream reason;
          reason << "Unsupported instance connection net/term compatibility for port '"
                 << portName << "' on instance '" << inst->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), exprSourceRange);
        }
      }
    }

  private:
    NLLibrary* library_ {nullptr};
    SNLSVConstructor::Config config_ {};
    SNLSVConstructor::ConstructOptions options_ {};
    std::unordered_map<SNLDesign*, SNLScalarNet*> const0Nets_ {};
    std::unordered_map<SNLDesign*, SNLScalarNet*> const1Nets_ {};
    std::vector<InferredMemory> inferredMemories_ {};
    std::unordered_map<const slang::ast::ValueSymbol*, size_t> inferredMemoryByStateSymbol_ {};
    std::unordered_map<const slang::ast::ProceduralBlockSymbol*, size_t>
      inferredMemoryCombBlocks_ {};
    std::unordered_map<const slang::ast::ProceduralBlockSymbol*, size_t>
      inferredMemorySequentialBlocks_ {};
    mutable std::vector<std::pair<const Symbol*, int64_t>> activeForLoopConstants_ {};
    mutable std::vector<bool> activeForLoopBreaks_ {};
    mutable std::vector<std::unordered_map<const Symbol*, SNLNet*>> activeFunctionArgumentNets_ {};
    mutable std::vector<const slang::ast::SubroutineSymbol*> activeInlinedCallSubroutines_ {};
    std::unordered_map<const slang::ast::ValueSymbol*, SNLNet*> loweredValueSymbolNets_ {};
    std::unordered_map<
      DynamicElementSelectCacheKey,
      std::vector<SNLBitNet*>,
      DynamicElementSelectCacheKeyHash>
      dynamicElementSelectCache_ {};
    std::unique_ptr<slang::driver::Driver> driver_;
    std::unique_ptr<slang::ast::Compilation> compilation_;
    std::vector<std::shared_ptr<slang::syntax::SyntaxTree>> syntaxTrees_;
    std::unordered_map<const InstanceBodySymbol*, SNLDesign*> bodyToDesign_;
    std::unordered_map<const slang::ast::DefinitionSymbol*, std::vector<const InstanceBodySymbol*>>
      representativeBodiesByDefinition_;
    std::unordered_map<std::string, size_t> elaboratedDesignOrdinals_;
    std::vector<std::string> warnings_;
    std::unordered_set<std::string> emittedWarnings_;
    std::vector<std::string> unsupportedElements_;
#ifdef NAJA_ENABLE_SV_CONSTRUCTOR_PERF_REPORT
    mutable SVPerfReport svPerfReport_ {};
#endif
};

SNLSVConstructor::SNLSVConstructor(NLLibrary* library):
  library_(library)
{}

void SNLSVConstructor::construct(const Paths& paths) {
  construct(paths, ConstructOptions{});
}

void SNLSVConstructor::construct(const std::filesystem::path& path) {
  construct(path, ConstructOptions{});
}

void SNLSVConstructor::construct(const Paths& paths, const ConstructOptions& options) {
  NajaPerf::Scope scope("SNLSVConstructor::construct");
  SNLSVConstructorImpl impl(library_, config_, options);
  impl.construct(paths);
}

void SNLSVConstructor::construct(const std::filesystem::path& path, const ConstructOptions& options) {
  construct(Paths{path}, options);
}

}  // namespace naja::NL
