// SPDX-FileCopyrightText: 2024 The Naja liberty authors <https://github.com/najaeda/naja-liberty/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "LibertyConstructor.h"

#include <fstream>
#include "LibertyException.h"
#include "LibertyParser.h"

namespace naja { namespace liberty {

//LCOV_EXCL_START
LibertyConstructor::Location LibertyConstructor::getCurrentLocation() const {
  return Location(getCurrentPath(), line_, column_);
}
//LCOV_EXCL_STOP

LibertyConstructor::~LibertyConstructor() {
}

Yosys::LibertyAst* LibertyConstructor::parse(const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::string reason(path.string() + " does not exist");
    throw LibertyException(reason);
  }
  currentPath_ = path;
  std::ifstream inFile(path);
  //LCOV_EXCL_START
  if (not inFile.good()) {
    std::string reason(path.string() + " is not a readable file");
    throw LibertyException(reason);
  }
  //LCOV_EXCL_STOP
  auto ast = internalParse(inFile);
  return ast;
}

void LibertyConstructor::parse(const LibertyConstructor::Paths& paths) {
  for (auto path: paths) {
    parse(path);
  }
}

Yosys::LibertyAst* LibertyConstructor::internalParse(std::istream &stream) {
  auto parser = new Yosys::LibertyParser(stream);
  return parser->ast;
}

}} // namespace liberty // namespace naja
