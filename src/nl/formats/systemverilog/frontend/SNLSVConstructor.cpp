// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSVConstructor.h"

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

#include "NLID.h"
#include "NLName.h"
#include "NLLibrary.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTermBit.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

#include "SNLSVConstructorException.h"

#include "slang/ast/Compilation.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/PortSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/Type.h"
#include "slang/diagnostics/Diagnostics.h"
#include "slang/syntax/SyntaxTree.h"

namespace naja::NL {

namespace {

using slang::ast::ArgumentDirection;
using slang::ast::Expression;
using slang::ast::InstanceBodySymbol;
using slang::ast::InstanceSymbol;
using slang::ast::PortSymbol;
using slang::ast::Symbol;
using slang::ast::SymbolKind;
using slang::ast::Type;
using slang::ast::ValueSymbol;

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
  if (slang::ast::ValueExpressionBase::isKind(expr.kind)) {
    const auto& valueExpr = expr.as<slang::ast::ValueExpressionBase>();
    const auto& symbol = valueExpr.symbol;
    return getOrCreateNet(design, std::string(symbol.name), symbol.getType());
  }
  return nullptr;
}

}  // namespace

class SNLSVConstructorImpl {
  public:
    explicit SNLSVConstructorImpl(NLLibrary* library): library_(library) {}

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
    }

  private:
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

      createTerms(design, body);
      createNets(design, body);
      connectTermsToNets(design);
      createInstances(design, body);

      return design;
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
          SNLBusTerm::create(
            design,
            direction,
            static_cast<NLID::Bit>(range->left),
            static_cast<NLID::Bit>(range->right),
            NLName(portName));
        } else {
          SNLScalarTerm::create(design, direction, NLName(portName));
        }
      }
    }

    void createNets(SNLDesign* design, const InstanceBodySymbol& body) {
      for (const auto& sym : body.members()) {
        if (sym.kind == SymbolKind::Net || sym.kind == SymbolKind::Variable) {
          const auto& valueSym = sym.as<ValueSymbol>();
          std::string name(valueSym.name);
          getOrCreateNet(design, name, valueSym.getType());
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
    std::unique_ptr<slang::ast::Compilation> compilation_;
    std::vector<std::shared_ptr<slang::syntax::SyntaxTree>> syntaxTrees_;
    std::unordered_map<std::string, SNLDesign*> nameToDesign_;
    std::unordered_map<const InstanceBodySymbol*, SNLDesign*> bodyToDesign_;
};

SNLSVConstructor::SNLSVConstructor(NLLibrary* library):
  library_(library)
{}

void SNLSVConstructor::construct(const Paths& paths) {
  SNLSVConstructorImpl impl(library_);
  impl.construct(paths);
}

void SNLSVConstructor::construct(const std::filesystem::path& path) {
  construct(Paths{path});
}

}  // namespace naja::NL
