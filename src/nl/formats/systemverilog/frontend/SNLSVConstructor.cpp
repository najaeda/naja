// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSVConstructor.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <limits>
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
#include "NLName.h"
#include "NLLibrary.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTermBit.h"
#include "SNLBusTerm.h"
#include "SNLAttributes.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

#include "SNLSVConstructorException.h"

#include "slang/ast/Compilation.h"
#include "slang/ast/ASTSerializer.h"
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

  slang::Diagnostics parseDiags;
  for (const auto& tree : driver.syntaxTrees) {
    parseDiags.append_range(tree->diagnostics()); // LCOV_EXCL_LINE
  }
  if (!parseDiags.empty()) {
    parseDiags.sort(driver.sourceManager); // LCOV_EXCL_LINE
    const auto parseDetails =
      slang::DiagnosticEngine::reportAll(driver.sourceManager, parseDiags); // LCOV_EXCL_LINE
    if (!parseDetails.empty()) { // LCOV_EXCL_LINE
      details << parseDetails; // LCOV_EXCL_LINE
      hasDetails = true; // LCOV_EXCL_LINE
    } // LCOV_EXCL_LINE
  } // LCOV_EXCL_LINE

  if (!hasDetails) {
    return std::nullopt;
  }

  std::ostringstream reason;
  reason << "SystemVerilog compilation failed:\n" << details.str();
  return reason.str();
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
    return; // LCOV_EXCL_LINE
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
    return false; // LCOV_EXCL_LINE
  }
  if (const auto* symbol = stripped->getSymbolReference()) {
    if (symbol->kind == SymbolKind::EnumValue) {
      return true;
    }
  }
  const auto* constant = stripped->getConstant();
  if (!constant || !constant->isInteger()) {
    return false;
  }
  return !constant->integer().hasUnknown();
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
      return "multi-port"; // LCOV_EXCL_LINE
    default:
      // createTerms only calls this helper for non-Port entries from getPortList.
      return "non-port"; // LCOV_EXCL_LINE
  }
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

}  // namespace

class SNLSVConstructorImpl {
  public:
    explicit SNLSVConstructorImpl(
      NLLibrary* library,
      const SNLSVConstructor::ConstructOptions& options):
      library_(library),
      options_(options)
    {}

    void construct(const SNLSVConstructor::Paths& paths) {
      if (!library_) {
        throw SNLSVConstructorException("SNLSVConstructor requires a valid NLLibrary");
      }
      driver_.reset();
      syntaxTrees_.clear();
      warnings_.clear();
      emittedWarnings_.clear();
      unsupportedElements_.clear();
      constructWithSlangDriver(paths);

      dumpDiagnosticsReport(getCompilationDiagnosticsReport(*compilation_));
      if (auto failure = getCompilationFailureDetails(*compilation_)) {
        throw SNLSVConstructorException(*failure);
      }

      const auto& root = compilation_->getRoot();
      for (const auto* top : root.topInstances) {
        buildDesign(top->body);
      }

      dumpElaboratedASTJson(root);
      throwIfUnsupportedElements();
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
        if (ec) { // LCOV_EXCL_LINE
          std::ostringstream reason; // LCOV_EXCL_LINE
          reason << "Failed to create diagnostics report directory: " << parent.string(); // LCOV_EXCL_LINE
          throw SNLSVConstructorException(reason.str()); // LCOV_EXCL_LINE
        } // LCOV_EXCL_LINE
        // LCOV_EXCL_STOP
      }

      std::ofstream output(reportPath, std::ios::out | std::ios::trunc);
      // LCOV_EXCL_START
      if (!output) { // LCOV_EXCL_LINE
        std::ostringstream reason; // LCOV_EXCL_LINE
        reason << "Failed to create diagnostics report file: " << reportPath.string(); // LCOV_EXCL_LINE
        throw SNLSVConstructorException(reason.str()); // LCOV_EXCL_LINE
      } // LCOV_EXCL_LINE
      // LCOV_EXCL_STOP

      if (!report.empty()) {
        output << report;
      } else {
        output << "No SystemVerilog diagnostics.\n";
      }

      // LCOV_EXCL_START
      if (!output.good()) { // LCOV_EXCL_LINE
        std::ostringstream reason; // LCOV_EXCL_LINE
        reason << "Failed to write diagnostics report file: " << reportPath.string(); // LCOV_EXCL_LINE
        throw SNLSVConstructorException(reason.str()); // LCOV_EXCL_LINE
      } // LCOV_EXCL_LINE
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

    SNLDesign* buildDesign(const InstanceBodySymbol& body) {
      const auto& definition = body.getDefinition();
      std::string defName(definition.name);
      auto existingIt = nameToDesign_.find(defName);
      if (existingIt != nameToDesign_.end()) {
        return existingIt->second;
      }

      auto design = SNLDesign::create(library_, NLName(defName));
      nameToDesign_[defName] = design;
      bodyToDesign_[&body] = design;
      annotateSourceInfo(design, getSourceRange(definition));

      createTerms(design, body);
      createNets(design, body);
      connectTermsToNets(design);
      createContinuousAssigns(design, body);
      createInstances(design, body);
      createSequentialLogic(design, body);

      return design;
    }

    struct SourceInfo {
      std::string file;
      size_t line {0};
      size_t column {0};
      size_t endLine {0};
      size_t endColumn {0};
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

    void reportUnsupportedElement(
      const std::string& reason,
      const std::optional<slang::SourceRange>& maybeRange = std::nullopt) {
      std::ostringstream message;
      if (auto sourceInfo = getSourceInfo(maybeRange)) {
        message << sourceInfo->file << ":" << sourceInfo->line << ":" << sourceInfo->column
                << ": ";
      }
      message << reason;
      unsupportedElements_.push_back(message.str());
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
      if (caseStmt.condition != slang::ast::CaseStatementCondition::Normal) {
        return false;
      }
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

    void addAttribute(
      NLObject* object,
      const char* name,
      SNLAttributeValue::Type type,
      const std::string& value) const {
      if (!object) {
        return; // LCOV_EXCL_LINE
      }
      SNLAttribute attribute(
        NLName(name),
        SNLAttributeValue(type, value));
      if (auto design = dynamic_cast<SNLDesign*>(object)) {
        SNLAttributes::addAttribute(design, attribute);
        return;
      }
      if (auto designObject = dynamic_cast<SNLDesignObject*>(object)) {
        SNLAttributes::addAttribute(designObject, attribute);
      }
    }

    void annotateSourceInfo(
      NLObject* object,
      const std::optional<slang::SourceRange>& maybeRange) const {
      auto sourceInfo = getSourceInfo(maybeRange);
      if (!sourceInfo) {
        return; // LCOV_EXCL_LINE
      }
      addAttribute(
        object,
        "sv_src_file",
        SNLAttributeValue::Type::STRING,
        sourceInfo->file);
      addAttribute(
        object,
        "sv_src_line",
        SNLAttributeValue::Type::NUMBER,
        std::to_string(sourceInfo->line));
      addAttribute(
        object,
        "sv_src_column",
        SNLAttributeValue::Type::NUMBER,
        std::to_string(sourceInfo->column));
      addAttribute(
        object,
        "sv_src_end_line",
        SNLAttributeValue::Type::NUMBER,
        std::to_string(sourceInfo->endLine));
      addAttribute(
        object,
        "sv_src_end_column",
        SNLAttributeValue::Type::NUMBER,
        std::to_string(sourceInfo->endColumn));
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

    void createTerms(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.getPortList()) {
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
      }
    }

    void createNets(SNLDesign* design, const InstanceBodySymbol& body) {
      const auto moduleName = design->getName().getString();
      for (const auto& sym : body.members()) {
        if (sym.kind == SymbolKind::Net || sym.kind == SymbolKind::Variable) {
          const auto& valueSym = sym.as<ValueSymbol>();
          std::string name(valueSym.name);
          // Port terms are materialized first; let connectTermsToNets own their net creation.
          if (design->getTerm(NLName(name))) {
            continue;
          }
          if (design->getNet(NLName(name))) {
            continue; // LCOV_EXCL_LINE
          }
          if (auto unsupportedTypeReason = getUnsupportedTypeReason(valueSym.getType())) {
            const auto& canonical = valueSym.getType().getCanonicalType();
            // Dynamically sized unpacked variables are currently not representable in SNL.
            std::ostringstream reason;
            reason << *unsupportedTypeReason << " for net/variable: " << name;
            if (canonical.isUnpackedArray() && !canonical.hasFixedRange()) {
              reason << " (dynamic unpacked array/queue/associative array)";
            }
            reason << " in module '" << moduleName << "'";
            reportUnsupportedElement(reason.str(), getSourceRange(valueSym));
            continue;
          }
          auto net = getOrCreateNet(design, name, valueSym.getType());
          annotateSourceInfo(net, getSourceRange(valueSym));
        }
      }
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
        } // LCOV_EXCL_LINE
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
          outNet = SNLScalarNet::create(design); // LCOV_EXCL_LINE
          annotateSourceInfo(outNet, sourceRange); // LCOV_EXCL_LINE
        } // LCOV_EXCL_LINE
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

    bool requestCurrentForLoopBreak(std::string* failureReason = nullptr) const {
      if (!hasActiveForLoopContext()) {
        if (failureReason) {
          *failureReason = "unsupported break statement outside for-loop";
        }
        return false;
      }
      activeForLoopBreaks_.back() = true;
      return true;
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
        if (auto maybeSigned = literal.getValue().as<int64_t>()) {
          value = *maybeSigned;
          return true;
        }
        if (auto maybeUnsigned = literal.getValue().as<uint64_t>()) {
          if (*maybeUnsigned > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            return false;
          }
          value = static_cast<int64_t>(*maybeUnsigned);
          return true;
        }
        return false;
      }

      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      if ((!constant || !constant->isInteger()) && stripped->getSymbolReference()) {
        slang::ast::EvalContext evalContext(*stripped->getSymbolReference());
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
              value = leftValue * rightValue;
              return true;
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
            default:
              return false;
          }
        }

        return false;
      }
      const auto maybeSigned = constant->integer().as<int64_t>();
      if (maybeSigned) {
        value = *maybeSigned;
        return true;
      }
      const auto maybeUnsigned = constant->integer().as<uint64_t>();
      if (!maybeUnsigned ||
          *maybeUnsigned > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        return false;
      }
      value = static_cast<int64_t>(*maybeUnsigned);
      return true;
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
        if (pattern.defaultSetter && pattern.memberSetters.empty() &&
            pattern.typeSetters.empty() && pattern.indexSetters.empty()) {
          bool defaultBit = false;
          if (getConstantBit(*pattern.defaultSetter, defaultBit)) {
            bits.assign(targetWidth, static_cast<SNLBitNet*>(getConstNet(design, defaultBit)));
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
      const slang::ConstantValue* constant = stripped->getConstant();
      slang::ConstantValue evaluatedConstant;
      if ((!constant || !constant->isInteger()) && stripped->getSymbolReference()) {
        slang::ast::EvalContext evalContext(*stripped->getSymbolReference());
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
      // LCOV_EXCL_STOPs

      slang::ast::EvalContext evalContext(*evalSymbol);
      auto selectedRange = expr.evalSelector(evalContext, true);
      if (!selectedRange) {
        return false;
      }

      auto baseRange = slang::ConstantRange(0, 0);
      if (auto baseBus = dynamic_cast<SNLBusNet*>(baseNet)) {
        baseRange = slang::ConstantRange(
          static_cast<int32_t>(baseBus->getMSB()),
          static_cast<int32_t>(baseBus->getLSB()));
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
        return false;
      }
      expr = returnStmt.expr;
      return true;
    }

    bool getExpressionConstantValue(
      const Expression& expr,
      slang::ConstantValue& value) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return false;
      }
      if (const auto* constant = stripped->getConstant()) {
        value = *constant;
        return static_cast<bool>(value);
      }

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
          return false;
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

        if (geBit == const0 || leBit == const0) {
          matchBit = const0;
          return true;
        }
        if (geBit == const1) {
          matchBit = leBit;
          return true;
        }
        if (leBit == const1) {
          matchBit = geBit;
          return true;
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
        if (!leftBit || !rightBit) {
          return nullptr;
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
        if (!inputBit) {
          return nullptr;
        }
        if (inputBit == const0) {
          return const1;
        }
        if (inputBit == const1) {
          return const0;
        }
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
        if (item.expressions.empty() || !item.stmt) {
          return false;
        }

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
        return false;
      }

      const Statement* bodyStmt = unwrapStatement(subroutine->getBody());
      if (!bodyStmt) {
        return false;
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
          return false;
        }
      }

      std::function<const Statement*(const Statement*)> findForLoopStatement =
        [&](const Statement* stmt) -> const Statement* {
          if (!stmt) {
            return nullptr;
          }
          const Statement* unwrapped = unwrapStatement(*stmt);
          if (!unwrapped) {
            return nullptr;
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
          } else if (unwrapped->kind == slang::ast::StatementKind::Block) {
            return findForLoopStatement(&unwrapped->as<slang::ast::BlockStatement>().body);
          }
          return nullptr;
        };
      const Statement* forStmt = findForLoopStatement(searchStmt);
      if (!forStmt) {
        return false;
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
          return false;
        }
        baseExpr = rangeCallArgs[0];
        lenExpr = rangeCallArgs[1];
        addressExpr = rangeCallArgs[2];
        return true;
      };

      const Statement* loopBody = unwrapStatement(forLoop.body);
      if (!loopBody) {
        return false;
      }
      std::function<bool(const Statement*)> findRangeCheckCallInStatement =
        [&](const Statement* stmt) -> bool {
          if (!stmt) {
            return false;
          }
          const Statement* unwrapped = unwrapStatement(*stmt);
          if (!unwrapped) {
            return false;
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
          } else if (unwrapped->kind == slang::ast::StatementKind::Block) {
            return findRangeCheckCallInStatement(&unwrapped->as<slang::ast::BlockStatement>().body);
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
        return false;
      }
      auto* loopVarValue = evalContext.createLocal(
        loopSymbol,
        slang::SVInt(32, 0, false));
      if (!loopVarValue) {
        return false;
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
          return nullptr;
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
          return nullptr;
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
      auto buildUnsignedLessThanConstant = [&](const std::vector<SNLBitNet*>& lhsBits,
                                               unsigned __int128 constantValue) -> SNLBitNet* {
        const auto width = lhsBits.size();
        if (!width) {
          return nullptr; // LCOV_EXCL_LINE
        }

        const auto maxValue = (static_cast<unsigned __int128>(1) << width);
        if (constantValue >= maxValue) {
          return const1;
        }
        if (constantValue == 0) {
          return const0;
        }

        SNLBitNet* ltBit = const0;
        SNLBitNet* eqBit = const1;
        for (size_t bitIndex = width; bitIndex-- > 0;) {
          const bool constantBit = ((constantValue >> bitIndex) & 1) != 0;
          auto* lhsBit = lhsBits[bitIndex];
          auto* notLhs = makeNot(lhsBit);
          if (!notLhs) {
            return nullptr;
          }
          if (constantBit) {
            auto* ltCandidate = makeAnd(eqBit, notLhs);
            if (!ltCandidate) {
              return nullptr;
            }
            ltBit = makeOr(ltBit, ltCandidate);
            eqBit = makeAnd(eqBit, lhsBit);
          } else {
            eqBit = makeAnd(eqBit, notLhs);
          }
          if (!ltBit || !eqBit) {
            return nullptr;
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
          static_cast<unsigned __int128>(*maybeBase));
        auto* geBase = makeNot(ltBase);
        if (!ltBase || !geBase) {
          return false;
        }

        std::vector<SNLBitNet*> addressBits65 = addressBits64;
        addressBits65.push_back(const0);
        const unsigned __int128 limitValue =
          static_cast<unsigned __int128>(*maybeBase) +
          static_cast<unsigned __int128>(*maybeLen);
        auto* ltLimit = buildUnsignedLessThanConstant(addressBits65, limitValue);
        if (!ltLimit) {
          return false;
        }

        auto* matchBit = makeAnd(geBase, ltLimit);
        if (!matchBit) {
          return false;
        }
        accumulatedMatch = makeOr(accumulatedMatch, matchBit);
        if (!accumulatedMatch) {
          return false;
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
        return false;
      }

      auto formalArgs = subroutine->getArguments();
      auto callArgs = callExpr.arguments();
      if (formalArgs.size() != callArgs.size()) {
        return false;
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
          return false;
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

      return resolveExpressionBits(design, *returnStmt.expr, targetWidth, bits) &&
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
          return nullptr;
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
          return false;
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
          continue;
        }
        for (size_t i = 0; i < targetWidth; ++i) {
          if (itemMatchBit == const1) {
            bits[i] = itemBits[i];
            continue;
          }
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
      const Symbol* evalSymbol = stripped->getSymbolReference();
      if ((!constant || !constant->isInteger()) && evalSymbol) {
        slang::ast::EvalContext evalContext(*evalSymbol);
        evaluatedConstant = stripped->eval(evalContext);
        if (evaluatedConstant && evaluatedConstant.isInteger()) {
          constant = &evaluatedConstant;
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
            return false;
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
            return false;
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

        bits.clear();
        bits.reserve(targetWidth);
        auto conditionalSourceRange = getSourceRange(*stripped);
        for (size_t bitIndex = 0; bitIndex < targetWidth; ++bitIndex) {
          if (leftBits[bitIndex] == rightBits[bitIndex]) {
            bits.push_back(leftBits[bitIndex]);
            continue;
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
          const auto* strippedRangeExpr = stripConversions(*rangeExpr);
          if (!strippedRangeExpr ||
              strippedRangeExpr->kind == slang::ast::ExpressionKind::ValueRange) {
            return false;
          }

          SNLNet* eqNet = SNLScalarNet::create(design);
          annotateSourceInfo(eqNet, insideSourceRange);
          if (!createEqualityAssign(
                design,
                eqNet,
                insideExpr.left(),
                *strippedRangeExpr,
                insideSourceRange)) {
            return false;
          }
          auto* eqBit = getSingleBitNet(eqNet);
          if (!eqBit) {
            return false; // LCOV_EXCL_LINE
          }

          if (insideBit == const1 || eqBit == const1) {
            insideBit = const1;
            continue;
          }
          if (eqBit == const0) {
            continue;
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
        } else {
          const auto& concatCanonical = replicationExpr.concat().type->getCanonicalType();
          if (!concatCanonical.isIntegral()) {
            return false; // LCOV_EXCL_LINE
          }
          const auto bitWidth = concatCanonical.getBitWidth();
          if (bitWidth < 0) {
            return false; // LCOV_EXCL_LINE
          }
          concatWidth = static_cast<size_t>(bitWidth);
        }

        if (concatWidth == 0 || repeatCount == 0) {
          bits.clear();
          resizeBitsToWidth(bits, targetWidth, static_cast<SNLBitNet*>(getConstNet(design, false)));
          return true;
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
              continue;
            }
            operandWidthBits = static_cast<size_t>(bitWidth);
          }
          if (!operandWidthBits) {
            continue;
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
              continue;
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

                        for (size_t elemBit = 0; elemBit < elementWidth; ++elemBit) {
                          auto* candidateBit = valueBits[offset + elemBit];
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
            const auto& valueType = valueExpr->type->getCanonicalType();
            if (valueType.hasFixedRange()) {
              auto valueNet = resolveExpressionNet(design, *valueExpr);
              auto valueBits = collectBits(valueNet);
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
                      const Expression* baseExpr = nullptr;
                      if (getConstantUnsigned(selectorBinaryExpr.left(), factor)) {
                        baseExpr = &selectorBinaryExpr.right();
                      } else if (getConstantUnsigned(selectorBinaryExpr.right(), factor)) {
                        baseExpr = &selectorBinaryExpr.left();
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
                      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
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
                        candidateBits.reserve(selectedWidth);
                        bool validCandidate = true;
                        for (size_t elem = 0; elem < sliceElements; ++elem) {
                          const int64_t elemIndex = lsbIndex + static_cast<int64_t>(elem);
                          if (elemIndex < std::numeric_limits<int32_t>::min() ||
                              elemIndex > std::numeric_limits<int32_t>::max()) {
                            validCandidate = false;
                            break;
                          }
                          const auto translated =
                            valueType.getFixedRange().translateIndex(static_cast<int32_t>(elemIndex));
                          if (translated < 0 || translated >= static_cast<int32_t>(arrayWidth)) {
                            validCandidate = false;
                            break;
                          }
                          const auto offset = static_cast<size_t>(translated) * elementWidth;
                          if (offset + elementWidth > valueBits.size()) {
                            validCandidate = false;
                            break; // LCOV_EXCL_LINE
                          }
                          for (size_t bit = 0; bit < elementWidth; ++bit) {
                            candidateBits.push_back(valueBits[offset + bit]);
                          }
                        }

                        if (validCandidate && candidateBits.size() == selectedWidth) {
                          auto* equalsIndexBit = buildSelectorEqualsIndexBit(
                            design,
                            selectorBits,
                            startIndex,
                            rangeSourceRange);
                          if (!equalsIndexBit) {
                            return false; // LCOV_EXCL_LINE
                          }
                          if (equalsIndexBit != const0) {
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
        setFailureReason("stripConversions returned null");
        return false; // LCOV_EXCL_LINE
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
          if (operandBits.size() != targetWidth) {
            std::ostringstream reason;
            reason << "bitwise-not operand width mismatch: expected " << targetWidth
                   << ", got " << operandBits.size()
                   << " (" << describeExpression(unary.operand()) << ")";
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
              std::ostringstream reason;
              reason << "failed to create NOT gate for bitwise-not operand at bit "
                     << bits.size();
              setFailureReason(reason.str());
              return false; // LCOV_EXCL_LINE
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
        setFailureReason("empty LHS bits or empty operand list");
        return false; // LCOV_EXCL_LINE
      }
      const auto bitWidth = lhsBits.size();
      std::vector<std::vector<SNLBitNet*>> operandBitsByOperand;
      operandBitsByOperand.reserve(operands.size());
      size_t operandIndex = 0;
      for (const auto* operand : operands) {
        if (!operand) {
          std::ostringstream reason;
          reason << "operand#" << operandIndex << " is null";
          setFailureReason(reason.str());
          return false; // LCOV_EXCL_LINE
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
        if (operandBits.size() != bitWidth) {
          std::ostringstream reason;
          reason << "operand#" << operandIndex << " width mismatch: expected " << bitWidth
                 << ", got " << operandBits.size()
                 << " (" << describeExpression(*operand) << ")";
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
          std::ostringstream reason;
          reason << "failed to create gate '" << gateType.getString() << "' at bit "
                 << bitIndex << " with " << inputs.size() << " inputs";
          setFailureReason(reason.str());
          return false; // LCOV_EXCL_LINE
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
            return false;
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
          return false;
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
          return nullptr;
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
          return false;
        }
        auto* leftGtRight = makeAnd(leftBit, notRight);
        auto* leftLtRight = makeAnd(notLeft, rightBit);
        if (!leftGtRight || !leftLtRight) {
          return false;
        }
        auto* eqAndGt = makeAnd(eqBit, leftGtRight);
        auto* eqAndLt = makeAnd(eqBit, leftLtRight);
        if (!eqAndGt || !eqAndLt) {
          return false;
        }
        gtBit = makeOr(gtBit, eqAndGt);
        ltBit = makeOr(ltBit, eqAndLt);
        auto* bitEq = makeXnor(leftBit, rightBit);
        if (!gtBit || !ltBit || !bitEq) {
          return false;
        }
        eqBit = makeAnd(eqBit, bitEq);
        if (!eqBit) {
          return false;
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
          return false;
        }
        auto* leftPositiveRightNegative = makeAnd(notSignLeft, signRight);
        auto* leftNegativeRightPositive = makeAnd(signLeft, notSignRight);
        auto* gtIfSameSign = makeAnd(sameSign, gtBit);
        auto* gtIfDifferentSign = makeAnd(differentSign, leftPositiveRightNegative);
        auto* ltIfSameSign = makeAnd(sameSign, ltBit);
        auto* ltIfDifferentSign = makeAnd(differentSign, leftNegativeRightPositive);
        if (!leftPositiveRightNegative || !leftNegativeRightPositive ||
            !gtIfSameSign || !gtIfDifferentSign || !ltIfSameSign || !ltIfDifferentSign) {
          return false;
        }
        gtBit = makeOr(gtIfSameSign, gtIfDifferentSign);
        ltBit = makeOr(ltIfSameSign, ltIfDifferentSign);
        if (!gtBit || !ltBit) {
          return false;
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
        default:
          return false; // LCOV_EXCL_LINE
      }
      if (!relationBit) {
        return false;
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
        setFailureReason("failed to resolve LHS bits");
        return false; // LCOV_EXCL_LINE
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
        std::ostringstream reason;
        reason << "failed to resolve shift amount width ("
               << describeExpression(shiftAmountExpr) << ")";
        setFailureReason(reason.str());
        return false; // LCOV_EXCL_LINE
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
        setFailureReason("failed to resolve LHS bits");
        return false; // LCOV_EXCL_LINE
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
        setFailureReason("failed to resolve non-empty value bits");
        return false; // LCOV_EXCL_LINE
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
        std::ostringstream reason;
        reason << "failed to resolve shift amount width ("
               << describeExpression(shiftAmountExpr) << ")";
        setFailureReason(reason.str());
        return false; // LCOV_EXCL_LINE
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
          if (logicalOp) {
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

            if (binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd) {
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
            const auto gateType = binaryExpr.op == slang::ast::BinaryOperator::LogicalAnd
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
            return nullptr;
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
      std::ostringstream fallback;
      fallback << "kind#" << static_cast<int>(kind);
      return fallback.str();
    }

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

    std::string describeExpression(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (!stripped) {
        return "expr=<null>"; // LCOV_EXCL_LINE
      }
      std::ostringstream description;
      description << describeExpressionKind(stripped->kind);
      if (stripped->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& binaryExpr = stripped->as<slang::ast::BinaryExpression>();
        description << " op=" << slang::ast::OpInfo::getText(binaryExpr.op);
      } else if (stripped->kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unaryExpr = stripped->as<slang::ast::UnaryExpression>();
        description << " op=" << describeUnaryOperator(unaryExpr.op);
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

    std::string joinName(const std::string& prefix, const std::string& base) const {
      if (prefix.empty()) {
        return base; // LCOV_EXCL_LINE
      }
      if (base.empty()) {
        return prefix; // LCOV_EXCL_LINE
      }
      return prefix + "_" + base;
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
      SNLBitNet* inA,
      SNLBitNet* inB,
      SNLBitNet* outNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto mux2 = NLDB0::getMux2();
      auto inst = SNLInstance::create(design, mux2);
      annotateSourceInfo(inst, sourceRange);
      auto aTerm = NLDB0::getMux2InputA();
      auto bTerm = NLDB0::getMux2InputB();
      auto sTerm = NLDB0::getMux2Select();
      auto yTerm = NLDB0::getMux2Output();
      if (auto instTerm = inst->getInstTerm(aTerm)) {
        instTerm->setNet(inA);
      }
      if (auto instTerm = inst->getInstTerm(bTerm)) {
        instTerm->setNet(inB);
      }
      if (auto instTerm = inst->getInstTerm(sTerm)) {
        instTerm->setNet(select);
      }
      if (auto instTerm = inst->getInstTerm(yTerm)) {
        instTerm->setNet(outNet);
      }
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

    const Statement* unwrapStatement(const Statement& stmt) const {
      const Statement* current = &stmt;
      while (current) {
        if (current->kind == slang::ast::StatementKind::Block) {
          current = &current->as<slang::ast::BlockStatement>().body;
          continue;
        }
        if (current->kind == slang::ast::StatementKind::List) {
          const auto& list = current->as<slang::ast::StatementList>().list;
          if (list.size() == 1) { // LCOV_EXCL_START
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
        default:
          return false;
      }
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
      return getActiveForLoopConstant(expr).has_value();
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
          failureReason = "unsupported for-loop with null control variable";
          return false; // LCOV_EXCL_LINE
        }

        const Expression* initializer = loopVar->getInitializer();
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
        if (!initializer) {
          failureReason = "unsupported for-loop initializer expression";
          return false;
        }
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
        failureReason = "unsupported null for-loop step expression";
        return false; // LCOV_EXCL_LINE
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
          if (*assignExpr.op == slang::ast::BinaryOperator::Add) {
            loopValue += rhsValue;
            return true;
          }
          if (*assignExpr.op == slang::ast::BinaryOperator::Subtract) {
            loopValue -= rhsValue;
            return true;
          }
          std::ostringstream reason;
          reason << "unsupported compound for-loop assignment operator: "
                 << slang::ast::OpInfo::getText(*assignExpr.op);
          failureReason = reason.str();
          return false;
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
      if (!stripped || stripped->kind != slang::ast::ExpressionKind::ElementSelect) {
        return lhsExpr;
      }

      const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
      const auto* baseExpr = stripConversions(elementExpr.value());
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

      if (!baseExpr) {
        return lhsExpr; // LCOV_EXCL_LINE
      }
      if (!slang::ast::ValueExpressionBase::isKind(baseExpr->kind) &&
          baseExpr->kind != slang::ast::ExpressionKind::MemberAccess) {
        return lhsExpr;
      }
      return baseExpr;
    }

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
      if (!current || current->kind != slang::ast::StatementKind::Conditional) {
        return false;
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

      if (condStmt.ifFalse->kind == slang::ast::StatementKind::Conditional) {
        const auto& enableStmt = condStmt.ifFalse->as<slang::ast::ConditionalStatement>();
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
      if (!extractAssignment(*condStmt.ifFalse, defaultLhs, defaultAction)) {
        return false;
      }
      if (!sameLhs(defaultLhs, chain.lhs)) {
        return false;
      }
      chain.defaultAction = defaultAction;
      chain.hasDefault = true;
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
        return true;
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
        return true;
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Block) {
        return applySequentialStatementForLhs(
          design,
          current->as<slang::ast::BlockStatement>().body,
          lhsExpr,
          lhsNet,
          lhsBits,
          baseName,
          dataBits,
          incrementerBits,
          tempIndex,
          failureReason);
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

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        if (condStmt.conditions.size() != 1) {
          std::ostringstream reason;
          reason << "unsupported conditional statement while lowering sequential block"
                 << " (conditions=" << condStmt.conditions.size() << ")";
          failureReason = reason.str();
          return false;
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

        if (trueBits.size() != lhsBits.size() || falseBits.size() != lhsBits.size()) {
          failureReason = "width mismatch while lowering conditional statement";
          return false;
        }
        auto condSourceRange = getSourceRange(condStmt);
        auto condBaseName = joinName("cond" + std::to_string(tempIndex++), baseName);
        auto* condNet = resolveConditionNet(
          design,
          *condStmt.conditions[0].expr,
          condBaseName,
          condSourceRange);
        if (!condNet) {
          std::ostringstream reason;
          reason << "unable to resolve single-bit condition net while lowering conditional"
                 << " (" << describeExpression(*condStmt.conditions[0].expr) << ")";
          failureReason = reason.str();
          return false;
        }

        std::vector<SNLBitNet*> mergedBits;
        mergedBits.reserve(lhsBits.size());
        for (size_t i = 0; i < lhsBits.size(); ++i) {
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, condSourceRange);
          createMux2Instance(
            design,
            condNet,
            falseBits[i],
            trueBits[i],
            outBit,
            condSourceRange);
          mergedBits.push_back(outBit);
        }
        dataBits = std::move(mergedBits);
        return true;
      }

      const Expression* assignedLHS = nullptr;
      AssignAction action;
      if (extractAssignment(*current, assignedLHS, action)) {
        if (!sameLhs(assignedLHS, &lhsExpr)) {
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
      bool trackAlwaysCombDynamicLHS = false) const {
      auto setFailureReason = [&](std::string reason) {
        if (failureReason) {
          *failureReason = std::move(reason);
        }
      };

      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true;
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Block) {
        return collectAssignedLHSExpressions(
          current->as<slang::ast::BlockStatement>().body,
          lhsExpressions,
          failureReason,
          trackAlwaysCombDynamicLHS);
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
                trackAlwaysCombDynamicLHS)) {
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
                  trackAlwaysCombDynamicLHS);
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
              trackAlwaysCombDynamicLHS)) {
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
              trackAlwaysCombDynamicLHS)) {
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
                trackAlwaysCombDynamicLHS)) {
            return false;
          }
        }
        if (caseStmt.defaultCase &&
            !collectAssignedLHSExpressions(
              *caseStmt.defaultCase,
              lhsExpressions,
              failureReason,
              trackAlwaysCombDynamicLHS)) {
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
        std::string breakFailureReason;
        if (!requestCurrentForLoopBreak(&breakFailureReason)) {
          setFailureReason(std::move(breakFailureReason));
          return false;
        }
        return true;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (extractAssignment(*current, lhsExpr, action)) {
        if (trackAlwaysCombDynamicLHS) {
          lhsExpr = getTrackedAlwaysCombLHS(lhsExpr);
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

      if (strippedLHS->kind == slang::ast::ExpressionKind::ElementSelect) {
        const auto& lhsElementExpr = strippedLHS->as<slang::ast::ElementSelectExpression>();
        int32_t selectedIndex = 0;
        if (!getConstantInt32(lhsElementExpr.selector(), selectedIndex)) {
          std::ostringstream reason;
          reason << "unsupported dynamic index in always_comb assignment LHS: "
                 << describeExpression(lhsExpr);
          setFailureReason(reason.str());
          return false;
        }
      }

      auto lhsWidth = getIntegralExpressionBitWidth(lhsExpr);
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
      if (caseStmt.condition != slang::ast::CaseStatementCondition::Normal) {
        std::ostringstream reason;
        reason << "unsupported always_comb case condition kind "
               << static_cast<int>(caseStmt.condition);
        failureReason = reason.str();
        return nullptr;
      }
      if (item.expressions.empty()) {
        failureReason = "unsupported empty always_comb case item";
        return nullptr;
      }

      auto caseSourceRange = getSourceRange(caseStmt);
      if (!shouldSuppressCaseComparison2StateWarning(caseStmt)) {
        reportCaseComparison2StateWarning(
          slang::ast::BinaryOperator::CaseEquality,
          caseSourceRange);
      }

      auto* const0 = static_cast<SNLBitNet*>(getConstNet(design, false));
      SNLBitNet* itemMatchBit = const0;
      for (const auto* itemExpr : item.expressions) {
        if (!itemExpr) {
          failureReason = "unsupported null always_comb case item expression";
          return nullptr; // LCOV_EXCL_LINE
        }

        auto* exprMatchBit = SNLScalarNet::create(design);
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
          failureReason = "failed to build always_comb case item match";
          return nullptr; // LCOV_EXCL_LINE
        }
      }
      return itemMatchBit;
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
      if (!action.rhs) {
        failureReason = "missing RHS expression in always_comb assignment";
        return false;
      }
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
                recoveredCompoundRhs = &binaryExpr.right();
              } else if (sameLhs(action.lhs, &binaryExpr.right())) {
                recoveredCompoundRhs = &binaryExpr.left();
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
                  recoveredCompoundRhs = &binaryExpr.left();
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

      if (!resolveExpressionBits(design, *action.rhs, targetWidth, assignedBits) ||
          assignedBits.size() != targetWidth) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb RHS bits for "
               << describeExpression(*action.rhs)
               << " (target_width=" << targetWidth << ")";
        failureReason = reason.str();
        return false;
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
        std::ostringstream reason;
        reason << "unsupported always_comb element-select assignment base without fixed range: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
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
        std::ostringstream reason;
        reason << "width mismatch while lowering always_comb element-select assignment for "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
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
        std::ostringstream reason;
        reason << "unable to resolve dynamic index width in always_comb assignment LHS: "
               << describeExpression(assignedLHS);
        failureReason = reason.str();
        return false;
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
            failureReason =
              "failed to build selector decode while lowering always_comb element-select assignment";
            return false;
          }
          updateSlice(static_cast<size_t>(translated) * static_cast<size_t>(*elementWidth),
                      equalsIndexBit);
        }
        index += step;
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
      std::string& failureReason) {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return true;
      }
      if (isIgnorableSequentialTimingStatement(*current)) {
        return true;
      }
      if (current->kind == slang::ast::StatementKind::Empty) {
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Block) {
        return applyCombinationalStatementForLhs(
          design,
          current->as<slang::ast::BlockStatement>().body,
          lhsExpr,
          lhsBits,
          dataBits,
          tempIndex,
          failureReason);
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
            return applyCombinationalStatementForLhs(
              design,
              forStmt.body,
              lhsExpr,
              lhsBits,
              dataBits,
              tempIndex,
              failureReason);
          },
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& condStmt = current->as<slang::ast::ConditionalStatement>();
        if (condStmt.conditions.size() != 1) {
          std::ostringstream reason;
          reason << "unsupported always_comb conditional with condition count "
                 << condStmt.conditions.size();
          failureReason = reason.str();
          return false;
        }

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
                failureReason)) {
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
              failureReason)) {
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
                failureReason)) {
            return false;
          }
        }
        const bool falseBreak = hasLoopContext ? isCurrentForLoopBreakRequested() : false;

        if (trueBits.size() != lhsBits.size() || falseBits.size() != lhsBits.size()) {
          failureReason = "width mismatch while lowering always_comb conditional";
          return false;
        }

        std::vector<SNLBitNet*> mergedBits;
        mergedBits.reserve(lhsBits.size());
        auto condSourceRange = getSourceRange(condStmt);
        for (size_t i = 0; i < lhsBits.size(); ++i) {
          if (trueBits[i] == falseBits[i]) {
            mergedBits.push_back(trueBits[i]);
            continue;
          }
          auto* outBit = SNLScalarNet::create(design);
          annotateSourceInfo(outBit, condSourceRange);
          createMux2Instance(
            design,
            conditionBit,
            falseBits[i],
            trueBits[i],
            outBit,
            condSourceRange);
          mergedBits.push_back(outBit);
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
                failureReason)) {
            return false;
          }
          if (defaultBits.size() != lhsBits.size()) {
            failureReason = "width mismatch while lowering always_comb default case";
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
                failureReason)) {
            return false;
          }
          if (itemBits.size() != lhsBits.size() || mergedBits.size() != lhsBits.size()) {
            failureReason = "width mismatch while lowering always_comb case item";
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

          if (itemMatchBit == const0) {
            continue;
          }
          if (itemMatchBit == const1) {
            mergedBits = std::move(itemBits);
            continue;
          }

          std::vector<SNLBitNet*> selectedBits;
          selectedBits.reserve(lhsBits.size());
          auto itemSourceRange = getSourceRange(*itemIt->stmt);
          for (size_t i = 0; i < lhsBits.size(); ++i) {
            if (itemBits[i] == mergedBits[i]) {
              selectedBits.push_back(itemBits[i]);
              continue;
            }
            auto* outBit = SNLScalarNet::create(design);
            annotateSourceInfo(outBit, itemSourceRange);
            createMux2Instance(
              design,
              itemMatchBit,
              mergedBits[i],
              itemBits[i],
              outBit,
              itemSourceRange);
            selectedBits.push_back(outBit);
          }
          mergedBits = std::move(selectedBits);
          ++tempIndex;
        }

        dataBits = std::move(mergedBits);
        return true;
      }

      if (current->kind == slang::ast::StatementKind::VariableDeclaration) {
        // Local variable declarations in always_comb are bookkeeping only from the
        // point of view of tracked LHS rewriting; they are ignored here.
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Break) {
        return requestCurrentForLoopBreak(&failureReason);
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
      reason << "unsupported statement kind while lowering always_comb"
             << " (kind=" << current->kind << ")";
      failureReason = reason.str();
      return false;
    }

    bool lowerCombinationalProceduralBlock(
      SNLDesign* design,
      const Statement& stmt,
      const std::optional<slang::SourceRange>& sourceRange,
      std::string& failureReason) {
      std::vector<const Expression*> lhsExpressions;
      if (!collectAssignedLHSExpressions(stmt, lhsExpressions, &failureReason, true)) {
        return false;
      }

      for (const auto* lhsExpr : lhsExpressions) {
        if (!lhsExpr) {
          continue; // LCOV_EXCL_LINE
        }
        std::vector<SNLBitNet*> lhsBits;
        if (!resolveAssignmentLHSBits(design, *lhsExpr, lhsBits, &failureReason)) {
          return false;
        }
        if (lhsBits.empty()) {
          continue; // LCOV_EXCL_LINE
        }

        std::vector<SNLBitNet*> dataBits = lhsBits;
        size_t tempIndex = 0;
        if (!applyCombinationalStatementForLhs(
              design,
              stmt,
              *lhsExpr,
              lhsBits,
              dataBits,
              tempIndex,
              failureReason)) {
          return false;
        }
        if (dataBits.size() != lhsBits.size()) {
          std::ostringstream reason;
          reason << "width mismatch while lowering always_comb for LHS "
                 << describeExpression(*lhsExpr);
          failureReason = reason.str();
          return false;
        }
        for (size_t i = 0; i < lhsBits.size(); ++i) {
          if (dataBits[i] == lhsBits[i]) {
            continue;
          }
          createAssignInstance(design, dataBits[i], lhsBits[i], sourceRange);
        }
      }
      return true;
    }

    bool lowerSequentialMultiAssignmentConditional(
      SNLDesign* design,
      const Statement& stmt,
      SNLBitNet* clkNet,
      const Expression* asyncResetEventExpr,
      const std::optional<slang::SourceRange>& blockSourceRange,
      std::string& failureReason) {
      const Statement* current = unwrapStatement(stmt);
      if (!current || current->kind != slang::ast::StatementKind::Conditional) {
        failureReason = "top-level statement is not a conditional";
        return false;
      }

      const auto& topCond = current->as<slang::ast::ConditionalStatement>();
      if (topCond.conditions.size() != 1) {
        std::ostringstream reason;
        reason << "top-level conditional has unsupported condition count: "
               << topCond.conditions.size();
        failureReason = reason.str();
        return false;
      }

      std::vector<const Expression*> resetLHSExpressions;
      if (!collectAssignedLHSExpressions(topCond.ifTrue, resetLHSExpressions, &failureReason)) {
        return false;
      }
      if (resetLHSExpressions.empty()) {
        failureReason = "reset branch does not contain assignments";
        return false;
      }

      if (resetLHSExpressions.size() < 2) {
        failureReason = "fallback currently supports only multi-LHS reset branches";
        return false;
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
        if (lhsBits.empty()) {
          std::ostringstream reason;
          reason << "unable to collect LHS bits for '" << getExpressionBaseName(*lhsExpr) << "'";
          failureReason = reason.str();
          return false;
        }

        auto baseName = getExpressionBaseName(*lhsExpr);
        if (baseName.empty() && !lhsNet->isUnnamed()) { // LCOV_EXCL_LINE
          baseName = lhsNet->getName().getString(); // LCOV_EXCL_LINE
        } // LCOV_EXCL_LINE

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
        if (resetBits.size() != lhsBits.size()) {
          std::ostringstream reason;
          reason << "reset branch width mismatch for '" << getExpressionBaseName(*lhsExpr) << "'";
          failureReason = reason.str();
          return false;
        }

        bool useAsyncResetDFFRN = false;
        SNLBitNet* asyncResetNNet = nullptr;
        if (asyncResetEventExpr && !resetBits.empty()) {
          auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));
          bool resetToZero = true;
          for (auto* bit : resetBits) {
            if (bit != constZero) {
              resetToZero = false;
              break;
            }
          }
          if (resetToZero &&
              NLDB0::getDFFRN() &&
              isActiveLowResetConditionForSignal(
                *topCond.conditions[0].expr,
                *asyncResetEventExpr)) {
            asyncResetNNet =
              getSingleBitNet(resolveExpressionNet(design, *asyncResetEventExpr));
            useAsyncResetDFFRN = asyncResetNNet != nullptr;
          }
        }

        if (!useAsyncResetDFFRN) {
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
            std::ostringstream reason;
            reason << "reset mux width mismatch for '" << getExpressionBaseName(*lhsExpr) << "'";
            failureReason = reason.str();
            return false; // LCOV_EXCL_LINE
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

        for (size_t i = 0; i < lhsBits.size(); ++i) {
          if (useAsyncResetDFFRN) {
            createDFFRNInstance(
              design,
              clkNet,
              dataBits[i],
              asyncResetNNet,
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
      if (stripped->kind != slang::ast::ExpressionKind::IntegerLiteral) {
        return false;
      }
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
      if (!constant || !constant->isInteger()) {
        return false;
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
          if (bit.isUnknown()) { // LCOV_EXCL_LINE
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
      std::vector<SNLBitNet*> constantBits;
      if (rhsExpr &&
          resolveConstantExpressionBits(design, *rhsExpr, lhsBits.size(), constantBits)) {
        return constantBits;
      }
      if (rhsExpr && rhsExpr->kind == slang::ast::ExpressionKind::BinaryOp) {
        const auto& bin = rhsExpr->as<slang::ast::BinaryExpression>();
        if (bin.op != slang::ast::BinaryOperator::Add) {
          std::ostringstream reason;
          reason << "Unsupported binary operator in sequential assignment: "
                 << slang::ast::OpInfo::getText(bin.op);
          reportUnsupportedElement(reason.str(), sourceRange);
          return {};
        }
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
        std::ostringstream reason;
        reason << "Unsupported binary expression in sequential assignment: "
               << slang::ast::OpInfo::getText(bin.op);
        reportUnsupportedElement(reason.str(), sourceRange);
        return {};
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

      if (!rhsNet) {
        reportUnsupportedElement("Unsupported RHS in sequential assignment", sourceRange);
        return {};
      }
      std::ostringstream reason;
      reason << "Unsupported width mismatch in sequential assignment: lhs=" << lhsBits.size()
             << " rhs=" << rhsBits.size();
      reportUnsupportedElement(reason.str(), sourceRange);
      return {};
    }

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
            if (clockExpr) {
              reportUnsupportedElement(
                "Unsupported sequential event list; only one posedge clock event is supported",
                getSourceRange(timing));
              return nullptr;
            }
            clockExpr = &event.expr;
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

    const Expression* getSingleNegedgeEventExpression(const TimingControl& timing) const {
      if (timing.kind != slang::ast::TimingControlKind::EventList) {
        return nullptr;
      }
      const auto& eventList = timing.as<slang::ast::EventListControl>();
      const Expression* negedgeExpr = nullptr;
      for (const auto* eventCtrl : eventList.events) {
        if (!eventCtrl ||
            eventCtrl->kind != slang::ast::TimingControlKind::SignalEvent) {
          continue;
        }
        const auto& event = eventCtrl->as<slang::ast::SignalEventControl>();
        if (event.edge == slang::ast::EdgeKind::NegEdge) {
          if (negedgeExpr) {
            return nullptr;
          }
          negedgeExpr = &event.expr;
        }
      }
      return negedgeExpr;
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

    void createSequentialLogic(SNLDesign* design, const InstanceBodySymbol& body) {
      const auto moduleName = design->getName().getString();
      for (const auto& sym : body.members()) {
        if (sym.kind != SymbolKind::ProceduralBlock) {
          continue;
        }
        const auto& block = sym.as<slang::ast::ProceduralBlockSymbol>();
        auto blockSourceRange = getSourceRange(block);
        if (block.procedureKind == slang::ast::ProceduralBlockKind::AlwaysComb) {
          std::string combFailureReason;
          if (!lowerCombinationalProceduralBlock(
                design,
                block.getBody(),
                blockSourceRange,
                combFailureReason)) {
            std::ostringstream reason;
            reason << "Unsupported combinational block in module '" << moduleName << "'";
            if (!combFailureReason.empty()) {
              reason << ": " << combFailureReason;
            }
            reportUnsupportedElement(reason.str(), blockSourceRange);
          }
          continue;
        }

        if (block.procedureKind != slang::ast::ProceduralBlockKind::AlwaysFF &&
            block.procedureKind != slang::ast::ProceduralBlockKind::Always) {
          std::ostringstream reason;
          reason << "Unsupported procedural block in module '" << moduleName
                 << "': unsupported procedure kind " << block.procedureKind
                 << " (only always/always_ff/always_comb are currently lowered)";
          reportUnsupportedElement(reason.str(), blockSourceRange);
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
        auto statementSourceRange = stmt ? getSourceRange(*stmt) : blockSourceRange;

        SNLBitNet* clkNet = nullptr;
        const Expression* asyncResetEventExpr = nullptr;
        if (timing) {
          const auto* clockExpr = getClockExpression(*timing);
          if (clockExpr) {
            clkNet = getSingleBitNet(resolveExpressionNet(design, *clockExpr));
            asyncResetEventExpr = getSingleNegedgeEventExpression(*timing);
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
        if (!stmt || !extractAlwaysFFChain(*stmt, chain)) {
          std::string multiAssignFailureReason;
          if (stmt &&
              lowerSequentialMultiAssignmentConditional(
                design,
                *stmt,
                clkNet,
                asyncResetEventExpr,
                statementSourceRange,
                multiAssignFailureReason)) {
            continue;
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
        auto lhsNet = resolveExpressionNet(design, *chain.lhs);
        if (!lhsNet) {
          std::ostringstream reason;
          reason << "Unsupported sequential block in module '" << moduleName
                 << "': unable to resolve assignment LHS net";
          reportUnsupportedElement(reason.str(), statementSourceRange);
          continue;
        }
        auto lhsBits = collectBits(lhsNet);
        if (lhsBits.empty()) {
          std::ostringstream reason;
          reason << "Unsupported sequential block in module '" << moduleName
                 << "': unable to collect LHS bits";
          reportUnsupportedElement(reason.str(), statementSourceRange);
          continue; // LCOV_EXCL_LINE
        }

        auto baseName = getExpressionBaseName(*chain.lhs);
        if (baseName.empty() && !lhsNet->isUnnamed()) { // LCOV_EXCL_LINE
          baseName = lhsNet->getName().getString(); // LCOV_EXCL_LINE
        } // LCOV_EXCL_LINE

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

        std::vector<SNLBitNet*> defaultBits;
        if (chain.hasDefault) {
          defaultBits = buildAssignBits(
            design,
            chain.defaultAction,
            lhsNet,
            lhsBits,
            &incrementerBits,
            defaultSourceRange);
          if (defaultBits.empty()) {
            continue;
          }
        } else {
          defaultBits = lhsBits;
        }

        std::vector<SNLBitNet*> enableBits;
        if (chain.enableCond) {
          enableBits = buildAssignBits(
            design,
            chain.enableAction,
            lhsNet,
            lhsBits,
            &incrementerBits,
            getActionSourceRange(chain.enableAction));
          if (enableBits.empty()) {
            continue;
          }
        }

        std::vector<SNLBitNet*> resetBits;
        if (chain.resetCond) {
          resetBits = buildAssignBits(
            design,
            chain.resetAction,
            lhsNet,
            lhsBits,
            &incrementerBits,
            getActionSourceRange(chain.resetAction));
          if (resetBits.empty()) {
            continue;
          }
        }

        std::vector<SNLBitNet*> dataBits = defaultBits;
        if (chain.enableCond) {
          auto enableNet = resolveConditionNet(
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
          auto enNet = getOrCreateNamedNet(
            design,
            joinName("en", baseName),
            lhsNet,
            enableSourceRange);
          auto enBits = collectBits(enNet);
          if (enBits.size() != dataBits.size()) {
            std::ostringstream reason;
            reason << "Unsupported sequential block in module '" << moduleName
                   << "': enable mux width mismatch";
            reportUnsupportedElement(reason.str(), enableSourceRange);
            continue; // LCOV_EXCL_LINE
          }
          for (size_t i = 0; i < dataBits.size(); ++i) {
            createMux2Instance(
              design,
              enableNet,
              dataBits[i],
              enableBits[i],
              enBits[i],
              enableSourceRange);
          }
          dataBits = std::move(enBits);
        }

        bool useAsyncResetDFFRN = false;
        SNLBitNet* asyncResetNNet = nullptr;
        if (chain.resetCond && asyncResetEventExpr && !resetBits.empty()) {
          auto* constZero = static_cast<SNLBitNet*>(getConstNet(design, false));
          bool resetToZero = true;
          for (auto* bit : resetBits) {
            if (bit != constZero) {
              resetToZero = false;
              break;
            }
          }
          if (resetToZero &&
              NLDB0::getDFFRN() &&
              isActiveLowResetConditionForSignal(*chain.resetCond, *asyncResetEventExpr)) {
            asyncResetNNet = getSingleBitNet(resolveExpressionNet(design, *asyncResetEventExpr));
            useAsyncResetDFFRN = asyncResetNNet != nullptr;
          }
        }

        if (chain.resetCond && !useAsyncResetDFFRN) {
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
            std::ostringstream reason;
            reason << "Unsupported sequential block in module '" << moduleName
                   << "': reset mux width mismatch";
            reportUnsupportedElement(reason.str(), resetSourceRange);
            continue; // LCOV_EXCL_LINE
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

        for (size_t i = 0; i < lhsBits.size(); ++i) {
          if (useAsyncResetDFFRN) {
            createDFFRNInstance(
              design,
              clkNet,
              dataBits[i],
              asyncResetNNet,
              lhsBits[i],
              statementSourceRange);
          } else {
            createDFFInstance(design, clkNet, dataBits[i], lhsBits[i], statementSourceRange);
          }
        }
      }
    }

    void createContinuousAssigns(SNLDesign* design, const InstanceBodySymbol& body) {
      const auto moduleName = design->getName().getString();
      for (const auto& sym : body.members()) {
        if (sym.kind != SymbolKind::ContinuousAssign) {
          continue;
        }
        const auto& continuousAssign = sym.as<slang::ast::ContinuousAssignSymbol>();
        const auto& assignment = continuousAssign.getAssignment();
        if (assignment.kind != slang::ast::ExpressionKind::Assignment) {
          reportUnsupportedElement(
            "Unsupported continuous assignment form (expected assignment expression)",
            getSourceRange(continuousAssign));
          continue;
        }
        const auto& assignExpr = assignment.as<slang::ast::AssignmentExpression>();
        auto assignSourceRange = getSourceRange(assignExpr);
        auto lhsNet = resolveExpressionNet(design, assignExpr.left());
        std::vector<SNLBitNet*> lhsBits;
        bool lhsResolvedAsBitSlice = false;
        if (!lhsNet) {
          const auto* lhsExpr = stripConversions(assignExpr.left());
          const bool lhsIsSupportedSlice =
            lhsExpr &&
            (lhsExpr->kind == slang::ast::ExpressionKind::ElementSelect ||
             lhsExpr->kind == slang::ast::ExpressionKind::RangeSelect ||
             lhsExpr->kind == slang::ast::ExpressionKind::MemberAccess);
          if (lhsIsSupportedSlice) {
            auto lhsWidth = getIntegralExpressionBitWidth(assignExpr.left());
            if (lhsWidth && *lhsWidth) {
              const auto lhsWidthBits = static_cast<size_t>(*lhsWidth);
              if (resolveExpressionBits(design, assignExpr.left(), lhsWidthBits, lhsBits) &&
                  lhsBits.size() == lhsWidthBits &&
                  !lhsBits.empty()) {
                lhsResolvedAsBitSlice = true;
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
        if (!rhs) {
          std::ostringstream reason;
          reason << "Unsupported RHS in continuous assign in module '" << moduleName << "'";
          reportUnsupportedElement(reason.str(), assignSourceRange);
          continue;
        }

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
              break;
            }
            current = stripConversions(*args[0]);
          }
          return current;
        };

        const auto* rhsCastUnwrapped = unwrapSignedUnsignedCastCall(rhs);
        if (rhsCastUnwrapped && rhsCastUnwrapped->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = rhsCastUnwrapped->as<slang::ast::BinaryExpression>();
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight) {
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
          if (binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftRight) {
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
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
              binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft) {
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
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftRight) {
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
              continue;
            }
            continue;
          }
          if (binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftRight) {
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
              continue;
            }
            continue;
          }
          if (binaryExpr.op == slang::ast::BinaryOperator::LogicalShiftLeft ||
              binaryExpr.op == slang::ast::BinaryOperator::ArithmeticShiftLeft) {
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

          if (!dynamic_cast<SNLScalarNet*>(lhsNet)) {
            reportUnsupportedElement(
              "Unsupported LHS in continuous gate assign: expected scalar net",
              assignSourceRange);
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
              if (operandFailureReason.empty()) {
                failedOperandReason = describeExpression(*operand);
              } else {
                failedOperandReason = operandFailureReason;
              }
              ok = false;
              break;
            }
            if (operandBits.size() != 1) {
              std::ostringstream widthReason;
              widthReason << "width mismatch: expected 1, got " << operandBits.size()
                          << " (" << describeExpression(*operand) << ")";
              failedOperandIndex = operandIndex;
              failedOperandReason = widthReason.str();
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
            std::ostringstream reason;
            reason << "Unsupported gate construction in continuous assign"
                   << " (gate=" << gateType->getString()
                   << ", input_count=" << inputNets.size()
                   << ", output_name=" << gateOutName
                   << ", reason=createGateInstance failed)";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue; // LCOV_EXCL_LINE
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
          if (lhsAssignBits.empty()) {
            std::ostringstream reason;
            reason << "Unsupported LHS in continuous assign in module '" << moduleName << "'";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue;
          }

          std::vector<SNLBitNet*> rhsBits;
          if (!resolveExpressionBits(design, *rhs, lhsAssignBits.size(), rhsBits) ||
              rhsBits.size() != lhsAssignBits.size()) {
            std::ostringstream reason;
            reason << "Unsupported RHS in continuous assign in module '" << moduleName << "'";
            reportUnsupportedElement(reason.str(), assignSourceRange);
            continue;
          }
          for (size_t i = 0; i < lhsAssignBits.size(); ++i) {
            createAssignInstance(design, rhsBits[i], lhsAssignBits[i], assignSourceRange);
          }
          continue;
        }

        if (lhsResolvedAsBitSlice) {
          auto rhsWidth = getIntegralExpressionBitWidth(*rhs);
          if (!rhsWidth || !*rhsWidth ||
              static_cast<size_t>(*rhsWidth) != lhsBits.size()) {
            reportUnsupportedElement(
              "Unsupported net compatibility in continuous assign",
              assignSourceRange);
            continue;
          }
          const auto rhsWidthBits = static_cast<size_t>(*rhsWidth);
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
        if (!rhsNet) {
          auto lhsAssignBits = collectBits(lhsNet);
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
          reportUnsupportedElement(
            "Unsupported net compatibility in continuous assign",
            assignSourceRange);
        }
      }
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
        }
        term->setNet(net);
      }
    }

    void createInstances(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.members()) {
        if (sym.kind != SymbolKind::Instance) {
          continue;
        }
        const auto& instance = sym.as<InstanceSymbol>();
        auto modelDesign = buildDesign(instance.body);
        auto inst = SNLInstance::create(
          design,
          modelDesign,
          NLName(std::string(instance.name)));
        annotateSourceInfo(inst, getSourceRange(instance));
        connectInstance(inst, instance);
      }
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
        const Expression* connectionExpr = stripConnectionLValueArgConversions(*expr);
        if (!connectionExpr) {
          std::ostringstream reason;
          reason << "Unsupported instance connection expression for port '" << portName
                 << "' on instance '" << inst->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), getSourceRange(*expr));
          continue;
        }
        auto net = resolveExpressionNet(inst->getDesign(), *connectionExpr);

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
          [&](SNLBusTerm* busTerm, const std::vector<SNLBitNet*>& bits) {
          auto termMSB = busTerm->getMSB();
          auto termLSB = busTerm->getLSB();
          auto termStep = termLSB <= termMSB ? 1 : -1;
          size_t bitIndex = 0;
          for (auto termBit = termLSB; termBit != termMSB + termStep; termBit += termStep) {
            auto* bitTerm = busTerm->getBit(termBit);
            auto* instTerm = inst->getInstTerm(bitTerm);
            if (instTerm) {
              instTerm->setNet(bits[bitIndex]);
            }
            ++bitIndex;
          }
        };

        if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
          if (net) {
            auto instTerm = inst->getInstTerm(scalarTerm);
            if (instTerm) {
              instTerm->setNet(net);
            }
            continue;
          }
          std::vector<SNLBitNet*> connectionBits;
          if (resolveSelectableConnectionBits(1, connectionBits) ||
              resolveExactWidthConnectionBits(1, connectionBits)) {
            auto instTerm = inst->getInstTerm(scalarTerm);
            if (instTerm) {
              instTerm->setNet(connectionBits.front());
            }
            continue;
          }
          std::ostringstream reason;
          reason << "Unsupported instance connection expression for port '" << portName
                 << "' on instance '" << inst->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), getSourceRange(*expr));
          continue;
        }
        auto busTerm = dynamic_cast<SNLBusTerm*>(term);
        auto busNet = dynamic_cast<SNLBusNet*>(net);
        if (busTerm && !busNet) {
          std::vector<SNLBitNet*> connectionBits;
          if (resolveSelectableConnectionBits(busTerm->getWidth(), connectionBits) ||
              resolveExactWidthConnectionBits(busTerm->getWidth(), connectionBits)) {
            connectBusBits(busTerm, connectionBits);
            continue;
          }
        }
        if (!busTerm || !busNet) {
          std::ostringstream reason;
          reason << "Unsupported instance connection net/term compatibility for port '"
                 << portName << "' on instance '" << inst->getName().getString() << "'";
          reportUnsupportedElement(reason.str(), getSourceRange(*expr));
          continue;
        }
        connectBusBits(busTerm, collectBits(busNet));
      }
    }

  private:
    NLLibrary* library_ {nullptr};
    SNLSVConstructor::ConstructOptions options_ {};
    std::unordered_map<SNLDesign*, SNLScalarNet*> const0Nets_ {};
    std::unordered_map<SNLDesign*, SNLScalarNet*> const1Nets_ {};
    mutable std::vector<std::pair<const Symbol*, int64_t>> activeForLoopConstants_ {};
    mutable std::vector<bool> activeForLoopBreaks_ {};
    mutable std::vector<std::unordered_map<const Symbol*, SNLNet*>> activeFunctionArgumentNets_ {};
    mutable std::vector<const slang::ast::SubroutineSymbol*> activeInlinedCallSubroutines_ {};
    std::unique_ptr<slang::driver::Driver> driver_;
    std::unique_ptr<slang::ast::Compilation> compilation_;
    std::vector<std::shared_ptr<slang::syntax::SyntaxTree>> syntaxTrees_;
    std::unordered_map<std::string, SNLDesign*> nameToDesign_;
    std::unordered_map<const InstanceBodySymbol*, SNLDesign*> bodyToDesign_;
    std::vector<std::string> warnings_;
    std::unordered_set<std::string> emittedWarnings_;
    std::vector<std::string> unsupportedElements_;
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
  SNLSVConstructorImpl impl(library_, options);
  impl.construct(paths);
}

void SNLSVConstructor::construct(const std::filesystem::path& path, const ConstructOptions& options) {
  construct(Paths{path}, options);
}

}  // namespace naja::NL
