// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <iostream>
#include <fstream>

#include <argparse/argparse.hpp>

#include "SNLException.h"
#include "SNLPrimitivesLoader.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"
#include "SNLUtils.h"

#include "SNLUniverse.h"

using namespace naja::SNL;

int main(int argc, char* argv[]) {
  argparse::ArgumentParser program("snl_vrl_app");

  program.add_argument("-p", "--primitives")
    .required()
    .help("input primitives");
  program.add_argument("-i", "--input")
    .required()
    .help("input netlist");
  program.add_argument("-o", "--output")
    .required()
    .help("output netlist");
  program.add_argument("-d", "--dump_primitives")
    .help("dump primitives library in verilog");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }
  
  std::filesystem::path primitivesPath;
  std::filesystem::path inputPath;
  std::filesystem::path outputPath;
  if (auto primitives = program.present("-p")) {
    primitivesPath = std::filesystem::path(*primitives);
  } else {
    //error
  }
  if (auto input = program.present("-i")) {
    inputPath = std::filesystem::path(*input);
  } else {
    //error
  }
  if (auto output = program.present("-o")) {
    outputPath = std::filesystem::path(*output);
  } else {
    //error
  }
  
  try {
    SNLUniverse::create();
    auto db = SNLDB::create(SNLUniverse::get());
    auto primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
    SNLPrimitivesLoader::load(primitivesLibrary, primitivesPath);

    auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
    SNLVRLConstructor constructor(designLibrary);
    constructor.construct(inputPath);

    auto top = SNLUtils::findTop(designLibrary);
    if (top) {
      db->setTopDesign(top);
    }

    if (db->getTopDesign()) {
      std::ofstream output(outputPath);
      SNLVRLDumper dumper;
      dumper.setSingleFile(true);
      dumper.dumpDesign(db->getTopDesign(), output);
    }
    
    if (program.is_used("-d")) {
      auto outputPrimitivesPath = std::filesystem::path(program.get<std::string>("-d"));
      std::ofstream output(outputPrimitivesPath);
      SNLVRLDumper dumper;
      dumper.setSingleFile(true);
      dumper.dumpLibrary(primitivesLibrary, output);
    }
  } catch (const SNLException& e) {
    std::cerr << "Caught SNL error: " << e.getReason() << std::endl;
    return 1;
  }
  return 0;
}
