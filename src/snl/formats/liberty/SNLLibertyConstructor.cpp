// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibertyConstructor.h"

#include <fstream>
#include "YosysLibertyParser.h"

#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLLibertyConstructorException.h"

namespace {

using namespace naja::SNL;

SNLTerm::Direction findDirection(const Yosys::LibertyAst* cell) {
  for (auto child: cell->children) {
    if (child->id == "direction") {
      auto direction = child->value;
      if (direction == "input") {
        return SNLTerm::Direction::Input;
      } else if (direction == "output") {
        return SNLTerm::Direction::Output;
      } else if (direction == "inout") {
        return SNLTerm::Direction::InOut;
      } else {
        throw SNLLibertyConstructorException("Unknown direction");
      }
    }
  }
  throw SNLLibertyConstructorException("Direction not found");
}

void parseTerms(SNLDesign* primitive, const Yosys::LibertyAst* cell) {
  for (auto child: cell->children) {
    if (child->id == "pin") {
      auto pinName = child->args[0];
      auto direction = findDirection(child);
      auto term = SNLScalarTerm::create(primitive, direction, SNLName(pinName));
    }
  }
}

void parseCell(SNLLibrary* library, const Yosys::LibertyAst* cell) {
  auto cellName = cell->args[0];
  auto primitive = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName(cellName));
  parseTerms(primitive, cell);
}

void parseCells(SNLLibrary* library, const Yosys::LibertyAst* ast) {
  for (auto child: ast->children) {
    if (child->id == "cell") {
      parseCell(library, child);
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
  //LCOV_EXCL_START
  if (not inFile.good()) {
    std::string reason(path.string() + " is not a readable file");
    throw SNLLibertyConstructorException(reason);
  }
  //LCOV_EXCL_STOP
  auto parser = std::make_unique<Yosys::LibertyParser>(inFile);
  auto ast = parser->ast;
  if (ast == nullptr) {
    std::string reason("Failed to parse the file");
    throw SNLLibertyConstructorException(reason);
  }
  auto libraryName = ast->args[0];
  library_->setName(SNLName(libraryName));
  parseCells(library_, ast);
}

}} // namespace SNL // namespace naja