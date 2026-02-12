// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSVConstructor.h"

#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>

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
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/Statement.h"
#include "slang/ast/TimingControl.h"
#include "slang/ast/expressions/AssignmentExpressions.h"
#include "slang/ast/expressions/ConversionExpression.h"
#include "slang/ast/expressions/LiteralExpressions.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/expressions/OperatorExpressions.h"
#include "slang/ast/statements/ConditionalStatements.h"
#include "slang/ast/statements/MiscStatements.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/MemberSymbols.h"
#include "slang/ast/symbols/PortSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/Type.h"
#include "slang/diagnostics/Diagnostics.h"
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

const Expression* stripConversions(const Expression& expr) {
  const Expression* current = &expr;
  while (current && current->kind == slang::ast::ExpressionKind::Conversion) {
    current = &current->as<slang::ast::ConversionExpression>().operand();
  }
  return current;
}

bool collectBinaryOperands(const Expression& expr, slang::ast::BinaryOperator op,
                           std::vector<const Expression*>& operands) {
  const Expression* current = stripConversions(expr);
  if (!current) {
    return false;
  }
  if (current->kind == slang::ast::ExpressionKind::BinaryOp) {
    const auto& binaryExpr = current->as<slang::ast::BinaryExpression>();
    if (binaryExpr.op == op) {
      if (!collectBinaryOperands(binaryExpr.left(), op, operands)) {
        return false;
      }
      if (!collectBinaryOperands(binaryExpr.right(), op, operands)) {
        return false;
      }
      return true;
    }
  }
  operands.push_back(current);
  return true;
}

std::optional<NLDB0::GateType> gateTypeFromBinary(slang::ast::BinaryOperator op) {
  switch (op) {
    case slang::ast::BinaryOperator::BinaryAnd:
      return NLDB0::GateType(NLDB0::GateType::And);
    case slang::ast::BinaryOperator::BinaryOr:
      return NLDB0::GateType(NLDB0::GateType::Or);
    case slang::ast::BinaryOperator::BinaryXor:
      return NLDB0::GateType(NLDB0::GateType::Xor);
    case slang::ast::BinaryOperator::BinaryXnor:
      return NLDB0::GateType(NLDB0::GateType::Xnor);
    default:
      return std::nullopt;
  }
}

SNLTerm::Direction toSNLDirection(ArgumentDirection direction) {
  switch (direction) {
    case ArgumentDirection::In:
      return SNLTerm::Direction::Input;
    case ArgumentDirection::Out:
      return SNLTerm::Direction::Output;
    case ArgumentDirection::InOut:
      return SNLTerm::Direction::InOut;
    default:
      return SNLTerm::Direction::Undefined;
  }
}

std::optional<slang::ConstantRange> getRangeFromType(const Type& type) {
  if (type.hasFixedRange()) {
    return type.getFixedRange();
  }
  auto width = type.getBitWidth();
  if (width > 1) {
    return slang::ConstantRange(int32_t(width - 1), 0);
  }
  return std::nullopt;
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
    }
    break;
  }
  current = current ? stripConversions(*current) : nullptr;
  if (current && slang::ast::ValueExpressionBase::isKind(current->kind)) {
    const auto& valueExpr = current->as<slang::ast::ValueExpressionBase>();
    const auto& symbol = valueExpr.symbol;
    return getOrCreateNet(design, std::string(symbol.name), symbol.getType());
  }
  return nullptr;
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
      syntaxTrees_.clear();
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

      auto& diags = compilation_->getAllDiagnostics();
      for (const auto& diag : diags) {
        if (diag.isError()) {
          throw SNLSVConstructorException("SystemVerilog compilation failed");
        }
      }

      const auto& root = compilation_->getRoot();
      for (const auto* top : root.topInstances) {
        buildDesign(top->body);
      }

      dumpElaboratedASTJson(root);
    }

  private:
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
        if (ec) {
          std::ostringstream reason;
          reason << "Failed to create elaborated AST JSON directory: " << parent.string();
          throw SNLSVConstructorException(reason.str());
        }
      }

      slang::JsonWriter writer;
      writer.setPrettyPrint(options_.prettyPrintElaboratedASTJson);
      auto& compilation = *compilation_;
      slang::ast::ASTSerializer serializer(compilation, writer);
      serializer.setIncludeSourceInfo(options_.includeSourceInfoInElaboratedASTJson);
      serializer.serialize(root);

      std::ofstream output(jsonPath, std::ios::out | std::ios::trunc);
      if (!output) {
        std::ostringstream reason;
        reason << "Failed to create elaborated AST JSON file: " << jsonPath.string();
        throw SNLSVConstructorException(reason.str());
      }
      output << writer.view();
      if (!output.good()) {
        std::ostringstream reason;
        reason << "Failed to write elaborated AST JSON file: " << jsonPath.string();
        throw SNLSVConstructorException(reason.str());
      }
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
      if (auto syntax = symbol.getSyntax()) {
        auto range = syntax->sourceRange();
        if (range.start().valid()) {
          return range;
        }
      }
      if (symbol.location.valid()) {
        return slang::SourceRange(symbol.location, symbol.location);
      }
      return std::nullopt;
    }

    std::optional<slang::SourceRange> getSourceRange(const Expression& expression) const {
      if (expression.sourceRange.start().valid()) {
        return expression.sourceRange;
      }
      return std::nullopt;
    }

    std::optional<slang::SourceRange> getSourceRange(const Statement& statement) const {
      if (statement.sourceRange.start().valid()) {
        return statement.sourceRange;
      }
      return std::nullopt;
    }

    std::optional<slang::SourceRange> getSourceRange(const TimingControl& timing) const {
      if (timing.sourceRange.start().valid()) {
        return timing.sourceRange;
      }
      return std::nullopt;
    }

    std::optional<SourceInfo> getSourceInfo(
      const std::optional<slang::SourceRange>& maybeRange) const {
      if (!maybeRange || !compilation_) {
        return std::nullopt;
      }
      auto sourceManager = compilation_->getSourceManager();
      if (!sourceManager) {
        return std::nullopt;
      }

      auto sourceRange = sourceManager->getFullyOriginalRange(*maybeRange);
      auto start = sourceRange.start();
      auto end = sourceRange.end();
      if (!start.valid()) {
        return std::nullopt;
      }

      start = sourceManager->getFullyOriginalLoc(start);
      if (!start.valid() || !sourceManager->isFileLoc(start)) {
        return std::nullopt;
      }

      if (end.valid()) {
        end = sourceManager->getFullyOriginalLoc(end);
      }
      if (!end.valid() || !sourceManager->isFileLoc(end) || end < start) {
        end = start;
      } else if (end > start) {
        // sourceRange end is exclusive: map to the previous character.
        end -= 1;
      }

      SourceInfo sourceInfo;
      sourceInfo.file = sourceManager->getFileName(start);
      if (sourceInfo.file.empty()) {
        sourceInfo.file = sourceManager->getRawFileName(start.buffer());
      }
      sourceInfo.line = sourceManager->getLineNumber(start);
      sourceInfo.column = sourceManager->getColumnNumber(start);
      sourceInfo.endLine = sourceManager->getLineNumber(end);
      sourceInfo.endColumn = sourceManager->getColumnNumber(end);
      return sourceInfo;
    }

    void addAttribute(
      NLObject* object,
      const char* name,
      SNLAttributeValue::Type type,
      const std::string& value) const {
      if (!object) {
        return;
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
        return;
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

    void createTerms(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.getPortList()) {
        if (sym->kind != SymbolKind::Port) {
          continue;
        }
        const auto& port = sym->as<PortSymbol>();
        std::string portName(port.name);
        auto direction = toSNLDirection(port.direction);
        auto range = getRangeFromType(port.getType());
        if (range && range->width() > 1) {
          auto term = SNLBusTerm::create(
            design,
            direction,
            static_cast<NLID::Bit>(range->left),
            static_cast<NLID::Bit>(range->right),
            NLName(portName));
          annotateSourceInfo(term, getSourceRange(port));
        } else {
          auto term = SNLScalarTerm::create(design, direction, NLName(portName));
          annotateSourceInfo(term, getSourceRange(port));
        }
      }
    }

    void createNets(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.members()) {
        if (sym.kind == SymbolKind::Net || sym.kind == SymbolKind::Variable) {
          const auto& valueSym = sym.as<ValueSymbol>();
          std::string name(valueSym.name);
          if (design->getNet(NLName(name))) {
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
        return nullptr;
      }
      if (type.isNInput()) {
        auto gate = NLDB0::getOrCreateNInputGate(type, inputNets.size());
        auto inst = SNLInstance::create(design, gate);
        annotateSourceInfo(inst, sourceRange);
        auto inputs = NLDB0::getGateNTerms(gate);
        auto output = NLDB0::getGateSingleTerm(gate);
        if (!inputs || !output) {
          return nullptr;
        }
        for (size_t i = 0; i < inputNets.size(); ++i) {
          auto bit = inputs->getBitAtPosition(i);
          if (!bit) {
            continue;
          }
          auto instTerm = inst->getInstTerm(bit);
          if (instTerm) {
            instTerm->setNet(inputNets[i]);
          }
        }
        if (!outNet) {
          outNet = SNLScalarNet::create(design);
          annotateSourceInfo(outNet, sourceRange);
        }
        if (auto outTerm = inst->getInstTerm(output)) {
          outTerm->setNet(outNet);
        }
        return inst;
      }
      if (type.isNOutput()) {
        if (inputNets.size() != 1) {
          return nullptr;
        }
        auto gate = NLDB0::getOrCreateNOutputGate(type, 1);
        auto inst = SNLInstance::create(design, gate);
        annotateSourceInfo(inst, sourceRange);
        auto input = NLDB0::getGateSingleTerm(gate);
        auto outputs = NLDB0::getGateNTerms(gate);
        if (!input || !outputs) {
          return nullptr;
        }
        if (auto instTerm = inst->getInstTerm(input)) {
          instTerm->setNet(inputNets[0]);
        }
        if (!outNet) {
          outNet = SNLScalarNet::create(design);
          annotateSourceInfo(outNet, sourceRange);
        }
        if (auto outBit = outputs->getBitAtPosition(0)) {
          if (auto outTerm = inst->getInstTerm(outBit)) {
            outTerm->setNet(outNet);
          }
        }
        return inst;
      }
      return nullptr;
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
        return false;
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
        return bits;
      }
      if (auto scalar = dynamic_cast<SNLScalarNet*>(net)) {
        bits.push_back(scalar);
        return bits;
      }
      auto bus = dynamic_cast<SNLBusNet*>(net);
      if (!bus) {
        return bits;
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
      return bus->getBit(bus->getMSB());
    }

    std::string getExpressionBaseName(const Expression& expr) const {
      const auto* stripped = stripConversions(expr);
      if (stripped && slang::ast::ValueExpressionBase::isKind(stripped->kind)) {
        const auto& valueExpr = stripped->as<slang::ast::ValueExpressionBase>();
        return std::string(valueExpr.symbol.name);
      }
      return {};
    }

    std::string joinName(const std::string& prefix, const std::string& base) const {
      if (prefix.empty()) {
        return base;
      }
      if (base.empty()) {
        return prefix;
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

    bool createMux2Instance(
      SNLDesign* design,
      SNLBitNet* select,
      SNLBitNet* inA,
      SNLBitNet* inB,
      SNLBitNet* outNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (!design || !select || !inA || !inB || !outNet) {
        return false;
      }
      auto mux2 = NLDB0::getMux2();
      if (!mux2) {
        return false;
      }
      auto inst = SNLInstance::create(design, mux2);
      annotateSourceInfo(inst, sourceRange);
      auto aTerm = NLDB0::getMux2InputA();
      auto bTerm = NLDB0::getMux2InputB();
      auto sTerm = NLDB0::getMux2Select();
      auto yTerm = NLDB0::getMux2Output();
      if (!aTerm || !bTerm || !sTerm || !yTerm) {
        return false;
      }
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
      return true;
    }

    SNLNet* createMux(
      SNLDesign* design,
      SNLNet* sel,
      SNLNet* a,
      SNLNet* b,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto one = getConstNet(design, true);
      auto selNot = createBinaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::Xor),
        sel,
        one,
        nullptr,
        sourceRange);
      auto t0 = createBinaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::And),
        selNot,
        a,
        nullptr,
        sourceRange);
      auto t1 = createBinaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::And),
        sel,
        b,
        nullptr,
        sourceRange);
      return createBinaryGate(
        design,
        NLDB0::GateType(NLDB0::GateType::Or),
        t0,
        t1,
        nullptr,
        sourceRange);
    }

    std::vector<SNLBitNet*> buildIncrementer(
      SNLDesign* design,
      const std::vector<SNLBitNet*>& inBits,
      const std::vector<SNLBitNet*>* sumOutBits = nullptr,
      const std::vector<SNLBitNet*>* carryOutBits = nullptr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      std::vector<SNLBitNet*> sumBits;
      sumBits.reserve(inBits.size());
      const bool useSumOut = sumOutBits && sumOutBits->size() == inBits.size();
      const bool useCarryOut = carryOutBits && carryOutBits->size() == inBits.size();
      SNLNet* carry = getConstNet(design, true);
      for (size_t i = 0; i < inBits.size(); ++i) {
        auto bit = inBits[i];
        auto sumOut = useSumOut ? (*sumOutBits)[i] : nullptr;
        auto carryOut = useCarryOut ? (*carryOutBits)[i] : nullptr;
        auto sum = createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::Xor),
          bit,
          carry,
          sumOut,
          sourceRange);
        auto nextCarry = createBinaryGate(
          design,
          NLDB0::GateType(NLDB0::GateType::And),
          bit,
          carry,
          carryOut,
          sourceRange);
        sumBits.push_back(static_cast<SNLBitNet*>(sum));
        carry = nextCarry;
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
          if (list.size() == 1) {
            current = list[0];
            continue;
          }
        }
        break;
      }
      return current;
    }

    const slang::ast::TimedStatement* findTimedStatement(const Statement& stmt) const {
      const Statement* current = unwrapStatement(stmt);
      if (!current) {
        return nullptr;
      }
      if (current->kind == slang::ast::StatementKind::Timed) {
        return &current->as<slang::ast::TimedStatement>();
      }
      if (current->kind == slang::ast::StatementKind::Block) {
        return findTimedStatement(current->as<slang::ast::BlockStatement>().body);
      }
      if (current->kind == slang::ast::StatementKind::List) {
        const auto& list = current->as<slang::ast::StatementList>().list;
        for (auto child : list) {
          if (!child) {
            continue;
          }
          if (auto timed = findTimedStatement(*child)) {
            return timed;
          }
        }
      }
      return nullptr;
    }

    bool extractAssignment(const Statement& stmt, const Expression*& lhs, AssignAction& action) const {
      auto current = unwrapStatement(stmt);
      if (!current) {
        return false;
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
        return false;
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
        return false;
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
          return false;
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

    bool getConstantBit(const Expression& expr, bool& value) const {
      if (expr.kind != slang::ast::ExpressionKind::IntegerLiteral) {
        return false;
      }
      const auto& literal = expr.as<slang::ast::IntegerLiteral>();
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

    std::vector<SNLBitNet*> buildAssignBits(
      SNLDesign* design,
      const AssignAction& action,
      SNLNet* lhsNet,
      const std::vector<SNLBitNet*>& lhsBits,
      const std::vector<SNLBitNet*>* incrementerBits = nullptr,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      if (action.increment) {
        if (incrementerBits && !incrementerBits->empty()) {
          return *incrementerBits;
        }
        return buildIncrementer(design, lhsBits, nullptr, nullptr, sourceRange);
      }
      if (action.rhs) {
        const auto* rhsExpr = stripConversions(*action.rhs);
        bool constValue = false;
        if (rhsExpr && getConstantBit(*rhsExpr, constValue)) {
          auto constantNet = getConstNet(design, constValue);
          return std::vector<SNLBitNet*>(lhsBits.size(), constantNet);
        }
        if (rhsExpr && rhsExpr->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& bin = rhsExpr->as<slang::ast::BinaryExpression>();
          if (bin.op == slang::ast::BinaryOperator::Add) {
            auto leftNet = resolveExpressionNet(design, bin.left());
            auto rightNet = resolveExpressionNet(design, bin.right());
            bool constOne = false;
            if (leftNet == lhsNet && rightNet && getConstantBit(bin.right(), constOne) && constOne) {
              if (incrementerBits && !incrementerBits->empty()) {
                return *incrementerBits;
              }
              return buildIncrementer(design, lhsBits, nullptr, nullptr, sourceRange);
            }
            if (rightNet == lhsNet && leftNet && getConstantBit(bin.left(), constOne) && constOne) {
              if (incrementerBits && !incrementerBits->empty()) {
                return *incrementerBits;
              }
              return buildIncrementer(design, lhsBits, nullptr, nullptr, sourceRange);
            }
          }
        }
        if (!rhsExpr) {
          return {};
        }
        auto rhsNet = resolveExpressionNet(design, *rhsExpr);
        if (rhsNet) {
          auto rhsBits = collectBits(rhsNet);
          if (rhsBits.size() == lhsBits.size()) {
            return rhsBits;
          }
        }
      }
      return {};
    }

    const Expression* getClockExpression(const TimingControl& timing) const {
      if (timing.kind == slang::ast::TimingControlKind::SignalEvent) {
        const auto& event = timing.as<slang::ast::SignalEventControl>();
        if (event.edge == slang::ast::EdgeKind::PosEdge) {
          return &event.expr;
        }
      } else if (timing.kind == slang::ast::TimingControlKind::EventList) {
        const auto& eventList = timing.as<slang::ast::EventListControl>();
        if (eventList.events.size() == 1) {
          return getClockExpression(*eventList.events[0]);
        }
      }
      return nullptr;
    }

    void createDFFInstance(
      SNLDesign* design,
      SNLNet* clkNet,
      SNLNet* dNet,
      SNLNet* qNet,
      const std::optional<slang::SourceRange>& sourceRange = std::nullopt) {
      auto dff = NLDB0::getDFF();
      if (!dff) {
        return;
      }
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

    void createSequentialLogic(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.members()) {
        if (sym.kind != SymbolKind::ProceduralBlock) {
          continue;
        }
        const auto& block = sym.as<slang::ast::ProceduralBlockSymbol>();
        auto blockSourceRange = getSourceRange(block);
        if (block.procedureKind != slang::ast::ProceduralBlockKind::AlwaysFF &&
            block.procedureKind != slang::ast::ProceduralBlockKind::Always) {
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
        if (timing) {
          const auto* clockExpr = getClockExpression(*timing);
          if (clockExpr) {
            clkNet = getSingleBitNet(resolveExpressionNet(design, *clockExpr));
          }
        }
        if (!clkNet) {
          clkNet = getSingleBitNet(design->getNet(NLName("clk")));
        }
        if (!clkNet) {
          continue;
        }

        AlwaysFFChain chain;
        if (!stmt || !extractAlwaysFFChain(*stmt, chain)) {
          continue;
        }
        auto lhsNet = resolveExpressionNet(design, *chain.lhs);
        if (!lhsNet) {
          continue;
        }
        auto lhsBits = collectBits(lhsNet);
        if (lhsBits.empty()) {
          continue;
        }

        auto baseName = getExpressionBaseName(*chain.lhs);
        if (baseName.empty() && !lhsNet->isUnnamed()) {
          baseName = lhsNet->getName().getString();
        }

        auto getActionSourceRange = [&](const AssignAction& action) {
          if (action.rhs) {
            return getSourceRange(*action.rhs);
          }
          return statementSourceRange;
        };
        auto defaultSourceRange = chain.hasDefault
          ? getActionSourceRange(chain.defaultAction)
          : statementSourceRange;
        auto enableSourceRange = chain.enableCond
          ? getSourceRange(*chain.enableCond)
          : statementSourceRange;
        auto resetSourceRange = chain.resetCond
          ? getSourceRange(*chain.resetCond)
          : statementSourceRange;

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
          bool constOne = false;
          if (leftNet == lhsNet && rightNet && getConstantBit(bin.right(), constOne) && constOne) {
            return true;
          }
          if (rightNet == lhsNet && leftNet && getConstantBit(bin.left(), constOne) && constOne) {
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
          if (incBits.size() == lhsBits.size() && carryBits.size() == lhsBits.size()) {
            incrementerBits = buildIncrementer(
              design,
              lhsBits,
              &incBits,
              &carryBits,
              statementSourceRange);
          } else {
            incrementerBits = buildIncrementer(
              design,
              lhsBits,
              nullptr,
              nullptr,
              statementSourceRange);
          }
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
          auto enableNet = getSingleBitNet(resolveExpressionNet(design, *chain.enableCond));
          if (!enableNet) {
            continue;
          }
          auto enNet = getOrCreateNamedNet(
            design,
            joinName("en", baseName),
            lhsNet,
            enableSourceRange);
          auto enBits = collectBits(enNet);
          if (enBits.size() != dataBits.size()) {
            continue;
          }
          for (size_t i = 0; i < dataBits.size(); ++i) {
            if (!createMux2Instance(
                  design,
                  enableNet,
                  dataBits[i],
                  enableBits[i],
                  enBits[i],
                  enableSourceRange)) {
              auto muxed = createMux(
                design,
                enableNet,
                dataBits[i],
                enableBits[i],
                enableSourceRange);
              if (muxed && enBits[i]) {
                createAssignInstance(design, muxed, enBits[i], enableSourceRange);
              }
            }
          }
          dataBits = std::move(enBits);
        }

        if (chain.resetCond) {
          auto resetNet = getSingleBitNet(resolveExpressionNet(design, *chain.resetCond));
          if (!resetNet) {
            continue;
          }
          auto rstNet = getOrCreateNamedNet(
            design,
            joinName("rst", baseName),
            lhsNet,
            resetSourceRange);
          auto rstBits = collectBits(rstNet);
          if (rstBits.size() != dataBits.size()) {
            continue;
          }
          for (size_t i = 0; i < dataBits.size(); ++i) {
            if (!createMux2Instance(
                  design,
                  resetNet,
                  dataBits[i],
                  resetBits[i],
                  rstBits[i],
                  resetSourceRange)) {
              auto muxed = createMux(
                design,
                resetNet,
                dataBits[i],
                resetBits[i],
                resetSourceRange);
              if (muxed && rstBits[i]) {
                createAssignInstance(design, muxed, rstBits[i], resetSourceRange);
              }
            }
          }
          dataBits = std::move(rstBits);
        }

        for (size_t i = 0; i < lhsBits.size(); ++i) {
          createDFFInstance(design, clkNet, dataBits[i], lhsBits[i], statementSourceRange);
        }
      }
    }

    void createContinuousAssigns(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.members()) {
        if (sym.kind != SymbolKind::ContinuousAssign) {
          continue;
        }
        const auto& continuousAssign = sym.as<slang::ast::ContinuousAssignSymbol>();
        const auto& assignment = continuousAssign.getAssignment();
        if (assignment.kind != slang::ast::ExpressionKind::Assignment) {
          continue;
        }
        const auto& assignExpr = assignment.as<slang::ast::AssignmentExpression>();
        auto assignSourceRange = getSourceRange(assignExpr);
        auto lhsNet = resolveExpressionNet(design, assignExpr.left());
        if (!lhsNet) {
          continue;
        }

        const auto* rhs = stripConversions(assignExpr.right());
        if (!rhs) {
          continue;
        }

        std::optional<NLDB0::GateType> gateType;
        std::vector<const Expression*> operands;
        if (rhs->kind == slang::ast::ExpressionKind::BinaryOp) {
          const auto& binaryExpr = rhs->as<slang::ast::BinaryExpression>();
          gateType = gateTypeFromBinary(binaryExpr.op);
          if (gateType && !collectBinaryOperands(*rhs, binaryExpr.op, operands)) {
            continue;
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
                break;
            }
            if (gateType && !collectBinaryOperands(*operandExpr, binaryExpr.op, operands)) {
              continue;
            }
          } else if (unaryExpr.op == slang::ast::UnaryOperator::BitwiseNot) {
            gateType = NLDB0::GateType(NLDB0::GateType::Not);
            if (operandExpr) {
              operands.push_back(operandExpr);
            }
          }
        }

        if (gateType && !operands.empty()) {
          if (!dynamic_cast<SNLScalarNet*>(lhsNet)) {
            continue;
          }
          std::vector<SNLNet*> inputNets;
          inputNets.reserve(operands.size());
          bool ok = true;
          for (const auto* operand : operands) {
            if (!operand) {
              ok = false;
              break;
            }
            auto net = resolveExpressionNet(design, *operand);
            if (!net) {
              ok = false;
              break;
            }
            inputNets.push_back(net);
          }
          if (!ok) {
            continue;
          }

          std::string baseName = getExpressionBaseName(assignExpr.left());
          if (baseName.empty() && !lhsNet->isUnnamed()) {
            baseName = lhsNet->getName().getString();
          }
          std::string gateOutName = joinName(gateType->getString(), baseName);
          if (gateOutName.empty()) {
            gateOutName = gateType->getString();
          }
          SNLNet* gateOutNet = getOrCreateNamedNet(
            design,
            gateOutName,
            lhsNet,
            assignSourceRange);
          if (!createGateInstance(
                design,
                *gateType,
                inputNets,
                gateOutNet,
                assignSourceRange) ||
              !gateOutNet) {
            continue;
          }
          createAssignInstance(design, gateOutNet, lhsNet, assignSourceRange);
          continue;
        }

        auto rhsNet = resolveExpressionNet(design, *rhs);
        if (!rhsNet) {
          continue;
        }
        createDirectAssign(design, rhsNet, lhsNet, assignSourceRange);
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
          continue;
        }
        std::string portName(conn->port.name);
        if (portName.empty()) {
          continue;
        }
        auto term = model->getTerm(NLName(portName));
        if (!term) {
          continue;
        }
        const Expression* expr = conn->getExpression();
        if (!expr) {
          continue;
        }
        auto net = resolveExpressionNet(inst->getDesign(), *expr);
        if (!net) {
          continue;
        }
        if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
          auto instTerm = inst->getInstTerm(scalarTerm);
          if (instTerm) {
            instTerm->setNet(net);
          }
          continue;
        }
        auto busTerm = dynamic_cast<SNLBusTerm*>(term);
        auto busNet = dynamic_cast<SNLBusNet*>(net);
        if (!busTerm || !busNet) {
          continue;
        }
        auto termBit = busTerm->getMSB();
        auto netBit = busNet->getMSB();
        auto termStop = busTerm->getMSB() > busTerm->getLSB() ? -1 : 1;
        auto netStop = busNet->getMSB() > busNet->getLSB() ? -1 : 1;
        while (termBit != busTerm->getLSB() + termStop &&
               netBit != busNet->getLSB() + netStop) {
          auto bitTerm = busTerm->getBit(termBit);
          auto instTerm = inst->getInstTerm(bitTerm);
          if (instTerm) {
            instTerm->setNet(busNet->getBit(netBit));
          }
          busTerm->getMSB() > busTerm->getLSB() ? --termBit : ++termBit;
          busNet->getMSB() > busNet->getLSB() ? --netBit : ++netBit;
        }
      }
    }

  private:
    NLLibrary* library_ {nullptr};
    SNLSVConstructor::ConstructOptions options_ {};
    std::unordered_map<SNLDesign*, SNLScalarNet*> const0Nets_ {};
    std::unordered_map<SNLDesign*, SNLScalarNet*> const1Nets_ {};
    std::unique_ptr<slang::ast::Compilation> compilation_;
    std::vector<std::shared_ptr<slang::syntax::SyntaxTree>> syntaxTrees_;
    std::unordered_map<std::string, SNLDesign*> nameToDesign_;
    std::unordered_map<const InstanceBodySymbol*, SNLDesign*> bodyToDesign_;
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
  SNLSVConstructorImpl impl(library_, options);
  impl.construct(paths);
}

void SNLSVConstructor::construct(const std::filesystem::path& path, const ConstructOptions& options) {
  construct(Paths{path}, options);
}

}  // namespace naja::NL
