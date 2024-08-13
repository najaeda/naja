// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibertyConstructor.h"

#include <fstream>
#include "LibertyParser.h"
#include "SNLLibertyConstructorException.h"

namespace {

void parseCells(Yosys::LibertyAst* ast) {
  for (auto child: ast->children) {
    if (child->id == "cell") {
    }
  }
}

}

namespace naja { namespace SNL {

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
  auto parser = new Yosys::LibertyParser(inFile);
  auto ast = parser->parse();
  if (ast == nullptr) {
    std::string reason("Failed to parse the file");
    throw SNLLibertyConstructorException(reason);
  }
  parseCells(ast);
}

}} // namespace SNL // namespace naja