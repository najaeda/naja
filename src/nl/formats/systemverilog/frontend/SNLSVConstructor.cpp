// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSVConstructor.h"

#include <algorithm>
#include <cstddef>
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
#include "slang/ast/statements/MiscStatements.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/MemberSymbols.h"
#include "slang/ast/symbols/PortSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/Type.h"
#include "slang/diagnostics/DiagnosticEngine.h"
#include "slang/diagnostics/Diagnostics.h"
#include "slang/driver/Driver.h"
#include "slang/syntax/SyntaxTree.h"
#include "slang/text/Json.h"
#include "slang/text/SourceManager.h"

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
      if (containsCommandFileOption(paths)) {
        constructWithSlangDriver(paths);
      } else {
        compilation_ = std::make_unique<slang::ast::Compilation>();

        for (const auto& path : paths) {
          auto treeOrError = slang::syntax::SyntaxTree::fromFile(path.string());
          if (!treeOrError) {
            std::ostringstream reason;
            reason << "Failed to load SystemVerilog file: " << path.string();
            throw SNLSVConstructorException(reason.str());
          }
          syntaxTrees_.push_back(*treeOrError);
          compilation_->addSyntaxTree(*treeOrError);
        }
      }

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
    static bool containsCommandFileOption(const SNLSVConstructor::Paths& paths) {
      for (const auto& path : paths) {
        const auto option = path.string();
        if (option == "-f" || option == "-F" || option == "-C") {
          return true;
        }
      }
      return false;
    }

    void constructWithSlangDriver(const SNLSVConstructor::Paths& paths) {
      driver_ = std::make_unique<slang::driver::Driver>();
      auto& driver = *driver_;
      driver.addStandardArgs();

      std::vector<std::string> args;
      args.reserve(paths.size() + 1);
      args.emplace_back("snl_sv_constructor");
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

    SNLNet* resolveExpressionNet(SNLDesign* design, const Expression& expr) {
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
      current = current ? stripConversions(*current) : nullptr;
      if (current && slang::ast::ValueExpressionBase::isKind(current->kind)) {
        const auto& valueExpr = current->as<slang::ast::ValueExpressionBase>();
        const auto& symbol = valueExpr.symbol;
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

    bool getConstantUnsigned(const Expression& expr, uint64_t& value) const {
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
          if (!operandWidth || !*operandWidth) {
            return false;
          }
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

        auto* selectBit = resolveConditionNet(
          design,
          *conditionalExpr.conditions.front().expr,
          getExpressionBaseName(conditionalExpr.left()),
          getSourceRange(*stripped));
        if (!selectBit) {
          return false;
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

      if (stripped->kind == slang::ast::ExpressionKind::ElementSelect) {
        const auto& elementExpr = stripped->as<slang::ast::ElementSelectExpression>();
        const auto* valueExpr = stripConversions(elementExpr.value());
        if (valueExpr) {
          const auto& valueType = valueExpr->type->getCanonicalType();
          if (valueType.hasFixedRange()) {
            auto valueNet = resolveExpressionNet(design, *valueExpr);
            auto valueBits = collectBits(valueNet);
            if (!valueBits.empty()) {
              const auto* elementType = valueType.getArrayElementType();
              if (elementType) {
                auto elementRange = getRangeFromType(*elementType);
                const auto elementWidth = elementRange ? static_cast<size_t>(elementRange->width()) : 1u;
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

      auto resolveSingleBitExpression = [&](const Expression& expr) -> SNLBitNet* {
        const auto* strippedExpr = stripConversions(expr);
        if (!strippedExpr) {
          return nullptr; // LCOV_EXCL_LINE
        }
        // Keep sequential condition support conservative: do not lower binary
        // or nested conditional trees here.
        if (strippedExpr->kind == slang::ast::ExpressionKind::BinaryOp ||
            strippedExpr->kind == slang::ast::ExpressionKind::ConditionalOp) {
          return nullptr;
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
      if (kind == slang::ast::ExpressionKind::ConditionalOp) {
        return "ConditionalOp";
      }
      if (kind == slang::ast::ExpressionKind::IntegerLiteral) {
        return "IntegerLiteral";
      }
      if (kind == slang::ast::ExpressionKind::UnbasedUnsizedIntegerLiteral) {
        return "UnbasedUnsizedIntegerLiteral";
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
      const Expression* rhs {nullptr};
      bool increment {false};
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
        action.rhs = &assign.right();
        action.increment = false;
        return true;
      }
      if (expr.kind == slang::ast::ExpressionKind::UnaryOp) {
        const auto& unary = expr.as<slang::ast::UnaryExpression>();
        if (unary.op == slang::ast::UnaryOperator::Postincrement ||
            unary.op == slang::ast::UnaryOperator::Preincrement) {
          lhs = &unary.operand();
          action.rhs = &unary.operand();
          action.increment = true;
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
      if (leftExpr && rightExpr &&
          slang::ast::ValueExpressionBase::isKind(leftExpr->kind) &&
          slang::ast::ValueExpressionBase::isKind(rightExpr->kind)) {
        const auto& leftSym = leftExpr->as<slang::ast::ValueExpressionBase>().symbol;
        const auto& rightSym = rightExpr->as<slang::ast::ValueExpressionBase>().symbol;
        return &leftSym == &rightSym || leftSym.name == rightSym.name;
      }
      return left == right;
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
      if (action.increment) {
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

      if (current->kind == slang::ast::StatementKind::Block) {
        return collectAssignedLHSExpressions(
          current->as<slang::ast::BlockStatement>().body,
          lhsExpressions,
          failureReason);
      }

      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (const auto* item : list) {
          if (!item) {
            continue; // LCOV_EXCL_LINE
          }
          if (!collectAssignedLHSExpressions(*item, lhsExpressions, failureReason)) {
            return false;
          }
        }
        return true;
      }

      if (current->kind == slang::ast::StatementKind::Conditional) {
        const auto& conditional = current->as<slang::ast::ConditionalStatement>();
        if (!collectAssignedLHSExpressions(conditional.ifTrue, lhsExpressions, failureReason)) {
          return false;
        }
        if (conditional.ifFalse &&
            !collectAssignedLHSExpressions(*conditional.ifFalse, lhsExpressions, failureReason)) {
          return false;
        }
        return true;
      }

      const Expression* lhsExpr = nullptr;
      AssignAction action;
      if (extractAssignment(*current, lhsExpr, action)) {
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
      reason << "unsupported statement kind while collecting always_comb assignments"
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

    bool buildCombinationalAssignBits(
      SNLDesign* design,
      const AssignAction& action,
      const std::vector<SNLBitNet*>& lhsBits,
      std::vector<SNLBitNet*>& assignedBits,
      std::string& failureReason) {
      if (action.increment) {
        failureReason = "unsupported increment/decrement assignment in always_comb";
        return false;
      }
      if (!action.rhs) {
        failureReason = "missing RHS expression in always_comb assignment";
        return false;
      }
      if (!resolveExpressionBits(design, *action.rhs, lhsBits.size(), assignedBits) ||
          assignedBits.size() != lhsBits.size()) {
        std::ostringstream reason;
        reason << "unable to resolve always_comb RHS bits for "
               << describeExpression(*action.rhs)
               << " (target_width=" << lhsBits.size() << ")";
        failureReason = reason.str();
        return false;
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
        }
        return true;
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

        if (trueBits.size() != lhsBits.size() || falseBits.size() != lhsBits.size()) {
          failureReason = "width mismatch while lowering always_comb conditional";
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
        if (conditionBit == const1) {
          dataBits = std::move(trueBits);
          return true;
        }
        if (conditionBit == const0) {
          dataBits = std::move(falseBits);
          return true;
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
        return true;
      }

      const Expression* assignedLHS = nullptr;
      AssignAction action;
      if (extractAssignment(*current, assignedLHS, action)) {
        if (!sameLhs(assignedLHS, &lhsExpr)) {
          return true;
        }
        std::vector<SNLBitNet*> assignedBits;
        if (!buildCombinationalAssignBits(
              design,
              action,
              lhsBits,
              assignedBits,
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
      if (!collectAssignedLHSExpressions(stmt, lhsExpressions, &failureReason)) {
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

      std::vector<std::pair<const Expression*, AssignAction>> resetAssignments;
      if (!collectDirectAssignments(topCond.ifTrue, resetAssignments, &failureReason)) {
        return false;
      }
      if (resetAssignments.empty()) {
        failureReason = "reset branch does not contain direct assignments";
        return false;
      }

      std::vector<std::pair<const Expression*, AssignAction>> plans;
      for (const auto& [lhs, action] : resetAssignments) {
        bool found = false;
        for (auto& [existingLHS, existingAction] : plans) {
          if (sameLhs(existingLHS, lhs)) {
            existingAction = action;
            found = true;
            break;
          }
        }
        if (!found) {
          plans.emplace_back(lhs, action);
        }
      }
      if (plans.size() < 2) {
        failureReason = "fallback currently supports only multi-LHS reset branches";
        return false;
      }

      for (const auto& [lhsExpr, resetAction] : plans) {
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
        if (needsIncrementerForAction(design, lhsNet, resetAction) &&
            incrementerBits.empty()) {
          auto incNet = getOrCreateNamedNet(
            design,
            joinName("inc", baseName),
            lhsNet,
            resetSourceRange);
          auto incBits = collectBits(incNet);
          auto incCarryNet = getOrCreateNamedNet(
            design,
            joinName("inc_carry", baseName),
            lhsNet,
            resetSourceRange);
          auto carryBits = collectBits(incCarryNet);
          incrementerBits = buildIncrementer(
            design,
            lhsBits,
            incBits,
            carryBits,
            resetSourceRange);
        }

        auto actionSourceRange = resetAction.rhs
          ? getSourceRange(*resetAction.rhs)
          : resetSourceRange;
        auto resetBits = buildAssignBits(
          design,
          resetAction,
          lhsNet,
          lhsBits,
          incrementerBits.empty() ? nullptr : &incrementerBits,
          actionSourceRange);
        if (resetBits.empty()) {
          std::ostringstream reason;
          reason << "failed to lower reset assignment for '"
                 << getExpressionBaseName(*lhsExpr) << "'";
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
      if (action.increment) {
        return getIncrementerBits();
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
      if (!rhsNet) {
        reportUnsupportedElement("Unsupported RHS in sequential assignment", sourceRange);
        return {};
      }
      auto rhsBits = collectBits(rhsNet);
      if (rhsBits.size() != lhsBits.size()) {
        std::ostringstream reason;
        reason << "Unsupported width mismatch in sequential assignment: lhs=" << lhsBits.size()
               << " rhs=" << rhsBits.size();
        reportUnsupportedElement(reason.str(), sourceRange);
        return {};
      }
      return rhsBits;
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
          if (action.increment) {
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
        auto net = resolveExpressionNet(inst->getDesign(), *expr);

        auto resolveSelectableConnectionBits =
          [&](size_t targetWidth, std::vector<SNLBitNet*>& bits) -> bool {
          const auto* strippedExpr = stripConversions(*expr);
          if (!strippedExpr) {
            return false; // LCOV_EXCL_LINE
          }
          const bool isSelectable =
            strippedExpr->kind == slang::ast::ExpressionKind::ElementSelect ||
            strippedExpr->kind == slang::ast::ExpressionKind::RangeSelect ||
            strippedExpr->kind == slang::ast::ExpressionKind::MemberAccess;
          if (!isSelectable) {
            return false;
          }
          if (!resolveExpressionBits(inst->getDesign(), *strippedExpr, targetWidth, bits)) {
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
          if (resolveSelectableConnectionBits(1, connectionBits)) {
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
          if (resolveSelectableConnectionBits(busTerm->getWidth(), connectionBits)) {
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
