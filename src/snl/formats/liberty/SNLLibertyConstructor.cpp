// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibertyConstructor.h"

#include <fstream>
#include <sstream>

#include "YosysLibertyParser.h"

#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBooleanTree.h"
#include "SNLDesignTruthTable.h"
#include "SNLLibertyConstructorException.h"

namespace {

using namespace naja::SNL;

SNLTerm::Direction getSNLDirection(const std::string& direction) {
  if (direction == "input") {
    return SNLTerm::Direction::Input;
  } else if (direction == "output") {
    return SNLTerm::Direction::Output;
  } else if (direction == "inout") {
    return SNLTerm::Direction::InOut;
  } else {
    std::ostringstream reason;
    reason << "Unknown direction: " << direction;
    throw SNLLibertyConstructorException(reason.str());
  }
}

struct BusType {
  std::string name  {};
  int msb           {0};
  int lsb           {0};
};

BusType findBusType(const Yosys::LibertyAst* ast, const std::string& busType) {
  for (auto child: ast->children) {
    if (child->id == "type" and not child->args.empty() and child->args[0] == busType) {
      int from = 0;
      int to = 0;
      bool downto = true;
      for (auto subChild: child->children) {
        if (subChild->id == "bit_from") {
          from = std::stoi(subChild->value);
        } else if (subChild->id == "bit_to") {
          to = std::stoi(subChild->value);
        } else if (subChild->id == "downto") {
          downto = subChild->value == "true";
        }
      }
      return BusType{busType, downto ? from : to, downto ? to : from};
    }
  }
  return BusType();
}

void parseTerms(
  SNLDesign* primitive,
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* cell,
  bool ignoreFunction = false) {
  using TermFunctions = std::map<SNLScalarTerm*, std::string, SNLScalarTerm::PointerLess>;
  TermFunctions termFunctions;
  for (auto child: cell->children) {
    if (child->id == "pin" or child->id == "bus") {
      auto pinName = child->args[0];
      BusType busType;
      if (child->id == "bus") {
        //which bus type?
        for (auto busTypeChild: child->children) {
          if (busTypeChild->id == "bus_type") {
            auto busTypeName = busTypeChild->value;
            busType = findBusType(top, busTypeName);
            if (busType.name.empty()) {
              std::string message;
              message += "While constructing " + primitive->getName().getString() + " interface,";
              message += " cannot find bus type: " + busTypeName;
              throw SNLLibertyConstructorException(message);
            }
          }
        }
      }
      bool foundDirection = false;
      SNLScalarTerm* constructedScalarTerm = nullptr;
      auto directionNode = child->find("direction");
      if (directionNode) {
        auto direction = directionNode->value;
        if (direction == "internal") {
          continue; //do not create
        } else {
          auto snlDirection = getSNLDirection(direction);
          if (busType.name.empty()) {
            constructedScalarTerm = SNLScalarTerm::create(primitive, snlDirection, SNLName(pinName));
          } else {
            SNLBusTerm::create(primitive, snlDirection, busType.msb, busType.lsb, SNLName(pinName));
          }
        }
      } else {
        std::ostringstream reason;
        reason << "Direction not found for " << child->id << " " << pinName;
        throw SNLLibertyConstructorException(reason.str());
      }
      if (not ignoreFunction
        and constructedScalarTerm
        and constructedScalarTerm->getDirection() == SNLTerm::Direction::Output) {
        termFunctions[constructedScalarTerm] = pinName;
        //parse function
        auto functionNode = child->find("function");
        if (functionNode) {
          auto function = functionNode->value;
          termFunctions[constructedScalarTerm] = function;
        }
      }
    }
  }
  if (termFunctions.size() == 1) {
    auto function = termFunctions.begin()->second;
    auto tree = std::make_unique<naja::SNL::SNLBooleanTree>();
    //std::cerr << "Parsing function: " << function << std::endl;
    tree->parse(primitive, function);
    naja::SNL::SNLBooleanTree::Terms terms;
    for (auto term: primitive->getBitTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Input) {
        terms.push_back(term);
      }
    }
    std::reverse(terms.begin(), terms.end());
    auto truthTable = tree->getTruthTable(terms);
    naja::SNL::SNLDesignTruthTable::setTruthTable(primitive, truthTable);
  }
}

void parseCell(SNLLibrary* library, const Yosys::LibertyAst* top, Yosys::LibertyAst* cell) {
  auto cellName = cell->args[0];
  auto primitive = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName(cellName));
  //std::cerr << "Parse cell: " << cellName << std::endl;
  auto ff = cell->find("ff");
  auto latch = cell->find("latch");
  bool ignoreFunction = ff or latch;
  parseTerms(primitive, top, cell, ignoreFunction);
}

void parseCells(SNLLibrary* library, const Yosys::LibertyAst* ast) {
  for (auto child: ast->children) {
    if (child->id == "cell") {
      parseCell(library, ast, child);
    }
  }
}

}

namespace naja { namespace SNL {

SNLLibertyConstructor::SNLLibertyConstructor(SNLLibrary* library):
  library_(library)
{}

void SNLLibertyConstructor::construct(const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::string reason(path.string() + " does not exist");
    throw SNLLibertyConstructorException(reason);
  }
  std::ifstream inFile(path);
  if (not inFile.good()) {
    //LCOV_EXCL_START
    std::string reason(path.string() + " is not a readable file");
    throw SNLLibertyConstructorException(reason);
    //LCOV_EXCL_STOP
  }
  auto parser = std::make_unique<Yosys::LibertyParser>(inFile);
  auto ast = parser->ast;
  if (ast == nullptr) {
    //LCOV_EXCL_START
    std::string reason("Failed to parse the file");
    throw SNLLibertyConstructorException(reason);
    //LCOV_EXCL_STOP
  }
  auto libraryName = ast->args[0];
  //find a policy for multiple libs
  library_->setName(SNLName(libraryName));
  parseCells(library_, ast);
}

}} // namespace SNL // namespace naja