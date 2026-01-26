// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibertyConstructor.h"

#include <fstream>
#include <sstream>

#include "YosysLibertyParser.h"

#include "NLLibrary.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBooleanTree.h"
#include "SNLDesignModeling.h"
#include "SNLLibertyConstructorException.h"

#include <boost/iostreams/detail/config/auto_link.hpp>
#include <boost/iostreams/detail/config/disable_warnings.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <stdexcept>

namespace {

using namespace naja::NL;

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

enum class FunctionParsingType {
  Ignore,
  Sequential,
  Combinational
};

void parseTerms(
  SNLDesign* primitive,
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* cell,
  FunctionParsingType functionParsingType = FunctionParsingType::Ignore) {
  using TermFunctions = std::map<SNLScalarTerm*, std::string, SNLScalarTerm::PointerLess>;
  TermFunctions termFunctions;
  TermFunctions seqTermClocks; //for sequential terms, key is clock term name
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
      SNLScalarTerm* constructedScalarTerm = nullptr;
      SNLBusTerm* constructedBusTerm = nullptr;
      auto directionNode = child->find("direction");
      if (directionNode) {
        auto direction = directionNode->value;
        if (direction == "internal") {
          continue; //do not create
        } else {
          auto snlDirection = getSNLDirection(direction);
          if (busType.name.empty()) {
            constructedScalarTerm = SNLScalarTerm::create(primitive, snlDirection, NLName(pinName));
          } else {
            constructedBusTerm = SNLBusTerm::create(primitive, snlDirection, busType.msb, busType.lsb, NLName(pinName));
          }
        }
      } else {
        std::ostringstream reason;
        reason << "Direction not found for " << child->id << " " << pinName;
        throw SNLLibertyConstructorException(reason.str());
      }
      if (functionParsingType == FunctionParsingType::Sequential
        and constructedScalarTerm) {
        //look for timing
        auto timingNode = child->find("timing");
        if (timingNode) {
          //look for timing_type
          auto timingTypeNode = timingNode->find("timing_type");
          if (timingTypeNode) {
            // Parse timing information
            auto timingType = timingTypeNode->value;
            if (timingType == "hold_rising" or timingType == "setup_rising"
              or timingType == "rising_edge") {
              // Handle sequential logic
              auto ckPin = timingNode->find("related_pin");
              if (ckPin) {
                auto ckPinName = ckPin->value;
                seqTermClocks[constructedScalarTerm] = ckPinName;
              }
            }
          }
        }
      }
      if (functionParsingType == FunctionParsingType::Combinational
        and constructedScalarTerm
        and constructedScalarTerm->getDirection() == SNLTerm::Direction::Output) {
        termFunctions[constructedScalarTerm] = pinName;
        //parse function
        auto functionNode = child->find("function");
        if (functionNode) {
          auto function = functionNode->value;
          termFunctions[constructedScalarTerm] = function;
        }
      } else if (not (functionParsingType == FunctionParsingType::Ignore)
        and constructedBusTerm
        and constructedBusTerm->getDirection() == SNLTerm::Direction::Output
        && (child->find("function") != nullptr)) {
        // Throw an exception if there are bus outputs
        std::ostringstream reason;
        reason << "No support for function for bus term while processing " << child->id << " " << pinName;
        throw SNLLibertyConstructorException(reason.str());
      }
    }
  }
  if (seqTermClocks.size() > 0) {
    for (const auto& pair: seqTermClocks) {
      auto term = pair.first;
      const auto& clockName = pair.second;
      auto clock = dynamic_cast<SNLScalarTerm*>(primitive->getTerm(NLName(clockName)));
      if (clock) {
        if (term->getDirection() == SNLTerm::Direction::Input) {
          SNLDesignModeling::addInputsToClockArcs({term}, clock);
        } else if (term->getDirection() == SNLTerm::Direction::Output) {
          SNLDesignModeling::addClockToOutputsArcs(clock, {term});
        }
      }
    }
  } else if (termFunctions.size() == 1) {
    auto function = termFunctions.begin()->second;
    auto tree = std::make_unique<naja::NL::SNLBooleanTree>();
    //std::cerr << "Parsing function: " << function << std::endl;
    tree->parse(primitive, function);
    naja::NL::SNLBooleanTree::Terms terms;
    for (auto term: primitive->getBitTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Input) {
        terms.push_back(term);
      }
    }
    std::reverse(terms.begin(), terms.end());
    auto truthTable = tree->getTruthTable(terms);
    naja::NL::SNLDesignModeling::setTruthTable(primitive, truthTable);
  } else if (termFunctions.size() > 1) {  
    std::vector<SNLTruthTable> truthTables;
    // Assuming termFunctions is ordered based on termIDs!
    for (auto& [term, function]: termFunctions) {
      auto tree = std::make_unique<naja::NL::SNLBooleanTree>();
      //std::cerr << "Parsing function: " << function << std::endl;
      tree->parse(primitive, function);
      naja::NL::SNLBooleanTree::Terms terms;
      for (auto term: primitive->getBitTerms()) {
        if (term->getDirection() == SNLTerm::Direction::Input) {
          terms.push_back(term);
        }
      }
      std::reverse(terms.begin(), terms.end());
      auto truthTable = tree->getTruthTable(terms);
      truthTables.push_back(truthTable);
    }
    naja::NL::SNLDesignModeling::setTruthTables(primitive, truthTables);
  }
}

void parseCell(NLLibrary* library, const Yosys::LibertyAst* top, Yosys::LibertyAst* cell) {
  auto cellName = cell->args[0];
  auto primitive = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName(cellName));
  //std::cerr << "Parse cell: " << cellName << std::endl;
  FunctionParsingType type = FunctionParsingType::Combinational;
  if (cell->find("ff")) {
    type = FunctionParsingType::Sequential;
  } else if (cell->find("latch")) {
    type = FunctionParsingType::Ignore; //LCOV_EXCL_LINE
  }
  parseTerms(primitive, top, cell, type);
}

void parseCells(NLLibrary* library, const Yosys::LibertyAst* ast) {
  for (auto child: ast->children) {
    if (child->id == "cell") {
      parseCell(library, ast, child);
    }
  }
}

}

namespace naja::NL {

SNLLibertyConstructor::SNLLibertyConstructor(NLLibrary* library):
  library_(library)
{}

void SNLLibertyConstructor::construct(const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    throw SNLLibertyConstructorException("Liberty parser: " + path.string() + " does not exist");
  }

  // Open underlying file in binary mode; must outlive filtering_istream
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    // LCOV_EXCL_START
    throw SNLLibertyConstructorException("Liberty parser: failed to open file: " + path.string());
    // LCOV_EXCL_STOP
  }

  // Build filtering stream: gzip_decompressor only for .gz files
  boost::iostreams::filtering_istream in;
  try {
    if (path.extension() == ".gz") {
      in.push(boost::iostreams::gzip_decompressor());
    }
    in.push(file);
  } catch (const std::exception &e) {
    // LCOV_EXCL_START
    throw SNLLibertyConstructorException(std::string("Liberty parser: gzip error: ") + e.what());
    // LCOV_EXCL_STOP
  }

  // Use the same parser API that accepts an istream
  auto parser = std::make_unique<Yosys::LibertyParser>(in);
  auto ast = parser->ast;
  if (ast == nullptr) {
    // LCOV_EXCL_START
    throw SNLLibertyConstructorException("Liberty parser: failed to parse the file: " + path.string());
    // LCOV_EXCL_STOP
  }

  auto libraryName = ast->args[0];
  library_->setName(NLName(libraryName));
  parseCells(library_, ast);
}

}  // namespace naja::NL