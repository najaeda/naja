// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <iostream>
#include <fstream>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h> // support for basic file logging
#include <spdlog/sinks/stdout_color_sinks.h>

#include "SNLException.h"
#include "SNLPyLoader.h"
#include "SNLPyEdit.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"
#include "SNLUtils.h"

#include "SNLUniverse.h"
#include "SNLCapnP.h"

using namespace naja::SNL;

namespace {

enum class FormatType { UNKNOWN, VERILOG, SNL };

FormatType argToFormatType(const std::string& inputFormat) {
  if (inputFormat == "verilog") {
    return FormatType::VERILOG;
  } else if (inputFormat == "snl") {
    return FormatType::SNL;
  } else {
    return FormatType::UNKNOWN;
  }
}

}

int main(int argc, char* argv[]) {
  argparse::ArgumentParser program("naja_edit");
  program.add_argument("-f", "--from_format")
    .required()
    .help("from/input format");
  program.add_argument("-t", "--to_format")
    .help("to/output format");
  program.add_argument("-i", "--input")
    .required()
    .help("input netlist");
  program.add_argument("-o", "--output")
    .help("output netlist");
  program.add_argument("-p", "--primitives")
    .help("input primitives");
  program.add_argument("-d", "--dump_primitives")
    .help("dump primitives library in verilog");
  program.add_argument("-e", "--edit")
    .help("edit netlist using python script");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }


  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::warn);
  console_sink->set_pattern("[naja_edit] [%^%l%$] %v");
  sinks.push_back(console_sink);

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("naja_edit.log", true);
  file_sink->set_level(spdlog::level::trace);
  sinks.push_back(file_sink);

  auto edit_logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
  edit_logger->set_level(spdlog::level::trace);

  spdlog::set_default_logger(edit_logger);
  spdlog::flush_every(std::chrono::seconds(3));
  spdlog::info("########################################################");
  spdlog::info("naja_edit");
  spdlog::info("########################################################");


  bool argError = false;
  auto inputFormatArg = program.present("-f");
  std::string inputFormat = *inputFormatArg;
  std::string outputFormat;
  if (auto outputFormatArg = program.present("-t")) {
    outputFormat = *outputFormatArg;
  } else {
    outputFormat = inputFormat;
  }
  FormatType inputFormatType = argToFormatType(inputFormat);
  FormatType outputFormatType = argToFormatType(outputFormat);
  if (inputFormatType == FormatType::UNKNOWN) {
    spdlog::critical("Unrecognized input format type: {}", inputFormat);
    argError = true;
  }
  if (outputFormatType == FormatType::UNKNOWN) {
    spdlog::critical("Unrecognized output format type: {}", outputFormat);
    argError = true;
  }

  std::filesystem::path primitivesPath;
  if (auto primitives = program.present("-p")) {
    if (inputFormatType == FormatType::SNL) {
      spdlog::critical("primitives option (-p) is incompatible with input format 'SNL'");
      argError = true;
    }
    primitivesPath = std::filesystem::path(*primitives);
  } else {
    if (inputFormatType != FormatType::SNL) {
      spdlog::critical("primitives option (-p) is mandatory when the input format is not 'SNL'");
      argError = true;
    }
  }

  if (argError) {
    std::exit(-1);
  }
  
  std::filesystem::path inputPath;
  std::filesystem::path outputPath;
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

    SNLDB* db = nullptr;
    SNLLibrary* primitivesLibrary = nullptr;
    if (inputFormatType == FormatType::SNL) {
      db = SNLCapnP::load(inputPath);
      SNLUniverse::get()->setTopDesign(db->getTopDesign());
    } else if (inputFormatType == FormatType::VERILOG) {
      db = SNLDB::create(SNLUniverse::get());
      primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
      SNLPyLoader::loadPrimitives(primitivesLibrary, primitivesPath);

      auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
      SNLVRLConstructor constructor(designLibrary);
      constructor.construct(inputPath);

      auto top = SNLUtils::findTop(designLibrary);
      if (top) {
        SNLUniverse::get()->setTopDesign(top);
        spdlog::info("Found top design: " + top->getString());
      } else {
        spdlog::error("No top design was found after parsing verilog");
      }
    } else {
      spdlog::critical("Unrecognized input format type: {}", inputFormat);
      std::exit(EXIT_FAILURE);
    }

    if (program.is_used("-e")) {
      auto editPath = std::filesystem::path(program.get<std::string>("-e"));
      SNLPyEdit::edit(editPath);
    }

    if (outputFormatType == FormatType::SNL) {
      SNLCapnP::dump(db, outputPath);
    } else if (outputFormatType == FormatType::VERILOG) {
      if (db->getTopDesign()) {
        std::ofstream output(outputPath);
        SNLVRLDumper dumper;
        dumper.setSingleFile(true);
        dumper.dumpDesign(db->getTopDesign(), output);
      } else {
        db->debugDump(0);
      }
    }
    
    if (program.is_used("-d") and primitivesLibrary) {
      auto outputPrimitivesPath = std::filesystem::path(program.get<std::string>("-d"));
      std::ofstream output(outputPrimitivesPath);
      SNLVRLDumper dumper;
      dumper.setSingleFile(true);
      dumper.dumpLibrary(primitivesLibrary, output);
    }
  } catch (const SNLException& e) {
    spdlog::critical("Caught SNL error: {}", e.getReason());
    std::exit(EXIT_FAILURE);
  }
  std::exit(EXIT_SUCCESS);
}
