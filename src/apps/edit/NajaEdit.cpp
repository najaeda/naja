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
#include "DNL.h"
#include "RemoveLoadlessLogic.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::NAJA_OPT;
using namespace naja::SNL;

namespace {

enum class FormatType { NOT_PROVIDED, UNKNOWN, VERILOG, SNL };
FormatType argToFormatType(const std::string& inputFormat) {
  if (inputFormat.empty()) {
    return FormatType::NOT_PROVIDED;
  } else if (inputFormat == "verilog") {
    return FormatType::VERILOG;
  } else if (inputFormat == "snl") {
    return FormatType::SNL;
  } else {
    return FormatType::UNKNOWN;
  }
}

enum class OptimizationType { NOT_PROVIDED, UNKNOWN, DLE, ALL };
OptimizationType argToOptimizationType(const std::string& optimization) {
  if (optimization.empty()) {
    return OptimizationType::NOT_PROVIDED;
  } else if (optimization == "dle") {
    return OptimizationType::DLE;
  } else if (optimization == "all") {
    return OptimizationType::ALL;
  } else {
    return OptimizationType::UNKNOWN;
  }
}

}

int main(int argc, char* argv[]) {
  argparse::ArgumentParser program("naja_edit");
  program.add_description("Edit gate level netlists using python script and apply optimizations");
  program.add_argument("-f", "--from_format")
    .required()
    .help("from/input format");
  program.add_argument("-t", "--to_format")
    .help("to/output format");
  program.add_argument("-i", "--input")
    .required()
    .append()
    .help("input netlist paths");
  program.add_argument("-o", "--output")
    .help("output netlist");
  program.add_argument("-p", "--primitives")
    .help("input primitives");
  program.add_argument("-d", "--dump_primitives")
    .help("dump primitives library in verilog");
  program.add_argument("-e", "--pre_edit")
    .help("edit netlist using python script after loading netlist and before applying optimizations");
  program.add_argument("-z", "--post_edit")
    .help("edit netlist using python script after optimizations and before dumping netlist");
  program.add_argument("-a", "--apply")
    .help("apply optimization: dle (remove loadless logic), all (all optimizations)");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
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
    if (auto output = program.is_used("-o")) {
      //in case output format is not provided and output path is provided
      //output format is same as input format
      outputFormat = inputFormat;
    }
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

  auto optimizationArg = program.present("-a");
  std::string optimization = *optimizationArg;
  OptimizationType optimizationType = argToOptimizationType(optimization);
  if (optimizationType == OptimizationType::UNKNOWN) {
    spdlog::critical("Unrecognized optimization type: {}", optimization);
    argError = true;
  }

  if (argError) {
    std::exit(-1);
  }
  
  using StringPaths = std::vector<std::string>;
  StringPaths inputStringPaths = program.get<StringPaths>("-i");

  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  std::transform(
    inputStringPaths.begin(), inputStringPaths.end(),
    std::back_inserter(inputPaths),
    [](const std::string& sp) -> std::filesystem::path { return std::filesystem::path(sp); });

  std::filesystem::path outputPath;
  if (inputPaths.empty()) {
    spdlog::critical("No input path was provided");
    std::exit(EXIT_FAILURE);
  }
  if (auto output = program.present("-o")) {
    outputPath = std::filesystem::path(*output);
  } else {
    if (outputFormatType != FormatType::NOT_PROVIDED) {
      spdlog::critical("output option (-o) is mandatory when the output format provided");
      std::exit(EXIT_FAILURE);
    }
  }
  
  try {
    SNLUniverse::create();

    SNLDB* db = nullptr;
    SNLLibrary* primitivesLibrary = nullptr;
    if (inputFormatType == FormatType::SNL) {
      if (inputPaths.size() > 1) {
        spdlog::critical("Multiple input paths are not supported for SNL format");
        std::exit(EXIT_FAILURE);
      }
      const auto start{std::chrono::steady_clock::now()};
      auto inputPath = inputPaths[0];
      db = SNLCapnP::load(inputPath);
      SNLUniverse::get()->setTopDesign(db->getTopDesign());
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Parsing done in: " << elapsed_seconds.count() << "s";
        spdlog::info(oss.str());
      }
    } else if (inputFormatType == FormatType::VERILOG) {
      db = SNLDB::create(SNLUniverse::get());
      primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
      SNLPyLoader::loadPrimitives(primitivesLibrary, primitivesPath);

      auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
      SNLVRLConstructor constructor(designLibrary);
      const auto start{std::chrono::steady_clock::now()};
      {
        std::ostringstream oss;
        oss << "Parsing verilog files: ";
        size_t i = 0;
        for (auto path: inputPaths) {
          if (i++ >= 4) {
            oss << std::endl;
            i = 0;
          }
          oss << path << " ";
        }
        spdlog::info(oss.str());
      }
      constructor.construct(inputPaths);
      auto top = SNLUtils::findTop(designLibrary);
      if (top) {
        SNLUniverse::get()->setTopDesign(top);
        spdlog::info("Found top design: " + top->getString());
      } else {
        spdlog::error("No top design was found after parsing verilog");
      }
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Parsing done in: " << elapsed_seconds.count() << "s";
        spdlog::info(oss.str());
      }
    } else {
      spdlog::critical("Unrecognized input format type: {}", inputFormat);
      std::exit(EXIT_FAILURE);
    }

    if (program.is_used("-e")) {
      auto editPath = std::filesystem::path(program.get<std::string>("-e"));
      spdlog::info("Post editing netlist using python script: {}", editPath.string());
      SNLPyEdit::edit(editPath);
    }

    if (optimizationType != OptimizationType::DLE
        or optimizationType != OptimizationType::ALL) {
      const auto start{std::chrono::steady_clock::now()};
      spdlog::info("Starting removal of loadless logic");
      LoadlessLogicRemover remover;
      remover.process();
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Removal of loadless logic done in: " << elapsed_seconds.count() << "s";
        spdlog::info(oss.str());
      } 
    }

    if (program.is_used("-z")) {
      auto editPath = std::filesystem::path(program.get<std::string>("-z"));
      spdlog::info("Post editing netlist using python script: {}", editPath.string());
      SNLPyEdit::edit(editPath);
    }

    if (outputFormatType == FormatType::SNL) {
      spdlog::info("Dumping netlist in SNL format to {}", outputPath.string());
      SNLCapnP::dump(db, outputPath);
    } else if (outputFormatType == FormatType::VERILOG) {
      if (db->getTopDesign()) {
        std::ofstream output(outputPath);
        SNLVRLDumper dumper;
        dumper.setSingleFile(true);
        spdlog::info("Dumping netlist in verilog format to {}", outputPath.string());
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
