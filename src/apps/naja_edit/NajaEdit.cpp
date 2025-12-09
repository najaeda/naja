// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <fstream>
#include <iostream>

#include <argparse/argparse.hpp>

// All DEBUG/TRACE statements will be removed by the pre-processor
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h> // support for basic file logging

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "NajaVersion.h"
#include "NajaPerf.h"
#include "NajaUtils.h"

#include "NLUniverse.h"
#include "NLException.h"
#include "SNLPyEdit.h"
#include "SNLPyLoader.h"
#include "SNLUtils.h"
#include "SNLLibertyConstructor.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include "ConstantPropagation.h"
#include "RemoveLoadlessLogic.h"
#include "SNLCapnP.h"
#include "Reduction.h"
#include "Utils.h"
#include "NetlistGraph.h"

using namespace naja::NL;
using namespace naja::NAJA_OPT;

namespace {

enum class FormatType { NOT_PROVIDED, UNKNOWN, VERILOG, SNL, DOT, SVG};
FormatType argToFormatType(const std::string& inputFormat) {
  if (inputFormat.empty()) {
    return FormatType::NOT_PROVIDED;
  } else if (inputFormat == "verilog") {
    return FormatType::VERILOG;
  } else if (inputFormat == "snl") {
    return FormatType::SNL;
  } else if (inputFormat == "dot") {
    return FormatType::DOT;
  /*} else if (inputFormat == "svg") {
    return FormatType::SVG;*/
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

using Paths = std::vector<std::filesystem::path>;

const std::string NAJA_EDIT_MAJOR("0");
const std::string NAJA_EDIT_MINOR("1");
const std::string NAJA_EDIT_REVISION("0");
const std::string NAJA_EDIT_VERSION(
  NAJA_EDIT_MAJOR + "." +
  NAJA_EDIT_MINOR + "." +
  NAJA_EDIT_REVISION);
}  // namespace

int main(int argc, char* argv[]) {
  const auto najaEditStart{std::chrono::steady_clock::now()};
  argparse::ArgumentParser program("naja_edit", NAJA_EDIT_VERSION);
  program.add_description(
      "Edit gate level netlists using python script and apply optimizations");
  program.add_argument("-f", "--from_format").help("from/input format");
  program.add_argument("-t", "--to_format").help("to/output format");
  program.add_argument("-i", "--input").append().help("input netlist paths");
  program.add_argument("-o", "--output").help("output netlist");
  program.add_argument("-p", "--primitives")
    .nargs(argparse::nargs_pattern::at_least_one)
    .help("input primitives: list of path to primitives files (liberty format or Naja python format)");
  program.add_argument("-d", "--dump_primitives")
    .help("dump primitives library in verilog");
  program.add_argument("-e", "--pre_edit")
    .help(
      "edit netlist using python script after loading netlist and before "
      "applying optimizations");
  program.add_argument("-z", "--post_edit")
    .help(
      "edit netlist using python script after optimizations and before "
      "dumping netlist");
  program.add_argument("-a", "--apply")
    .help(
      "apply optimization: dle (remove loadless logic), all (all "
      "optimizations)");
  program.add_argument("-l", "--log")
    .default_value(std::string("naja_edit.log"))
    .help("Dump log file (default name: naja_edit.log)");
  program.add_argument("-s", "--stats")
    .default_value(std::string("naja_stats.log"))
    .help("Dump stats log file named: naja_stats.log");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  std::filesystem::path statsPath("naja_stats.log");
  if (program.is_used("--stats")) {
    statsPath = program.get<std::string>("--stats");
  }
  naja::NajaPerf::create(statsPath, "naja_edit");

  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  console_sink->set_pattern("[naja_edit] [%^%l%$] %v");
  sinks.push_back(console_sink);

  if (program.is_used("--log")) {
    auto logName = program.get<std::string>("--log");
    {
      std::ofstream logFile(logName, std::ios::out);
      if (logFile.is_open()) {
        std::string bannerTitle = "naja_edit " + NAJA_EDIT_VERSION;
        std::ostringstream bannerStream;
        naja::NajaUtils::createBanner(logFile, bannerTitle, "#");
        logFile << std::endl;
        logFile.close();
      }
    }
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logName);
    file_sink->set_level(spdlog::level::trace);
    sinks.push_back(file_sink);
  }

  auto edit_logger =
      std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
  edit_logger->set_level(spdlog::level::trace);

  spdlog::set_default_logger(edit_logger);
  spdlog::flush_every(std::chrono::seconds(3));

  bool argError = false;

  std::string inputFormat;
  if (auto inputFormatArg = program.present("-f")) {
    inputFormat = *inputFormatArg;
  }
  FormatType inputFormatType = argToFormatType(inputFormat);
  if (inputFormatType == FormatType::UNKNOWN) {
    SPDLOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
    argError = true;
  }

  if (program.present("-i")) {
    if (inputFormatType == FormatType::NOT_PROVIDED) {
      SPDLOG_CRITICAL("output option (-f) is mandatory when the input is provided");
      std::exit(EXIT_FAILURE);
    }
  }

  std::string outputFormat;
  if (auto outputFormatArg = program.present("-t")) {
    outputFormat = *outputFormatArg;
  } else {
    if (auto output = program.is_used("-o")) {
      // in case output format is not provided and output path is provided
      // output format is same as input format
      if (inputFormatType == FormatType::NOT_PROVIDED) {
        SPDLOG_CRITICAL("output format option (-t) is mandatory");
        argError = true;
      } else {
        outputFormat = inputFormat;
      }
    }
  }
  FormatType outputFormatType = argToFormatType(outputFormat);
  
  if (outputFormatType == FormatType::UNKNOWN) {
    SPDLOG_CRITICAL("Unrecognized output format type: {}", outputFormat);
    argError = true;
  }

  Paths primitivesPaths;
  if (auto primitives = program.present("-p")) {
    if (inputFormatType == FormatType::SNL) {
      SPDLOG_CRITICAL("primitives option (-p) is incompatible with input format 'SNL'");
      argError = true;
    }
    auto primitivesPathsString = program.get<std::vector<std::string>>("-p");
    primitivesPaths.resize(primitivesPathsString.size());
    std::transform(primitivesPathsString.begin(),
      primitivesPathsString.end(),
      primitivesPaths.begin(),
      [](const std::string& str) {
        return std::filesystem::path(str);
      }
    );
  } else {
    if (inputFormatType != FormatType::SNL and inputFormatType != FormatType::NOT_PROVIDED) {
      SPDLOG_CRITICAL("primitives option (-p) is mandatory when the input format is not 'SNL'");
      argError = true;
    }
  }

  OptimizationType optimizationType = OptimizationType::NOT_PROVIDED;
  if (auto optimizationArg = program.present("-a")) {
    std::string optimization = *optimizationArg;
    optimizationType = argToOptimizationType(optimization);
    if (optimizationType == OptimizationType::UNKNOWN) {
      SPDLOG_CRITICAL("Unrecognized optimization type: {}", optimization);
      argError = true;
    }
  }

  if (argError) {
    std::exit(-1);
  }

  using StringPaths = std::vector<std::string>;
  StringPaths inputStringPaths = program.get<StringPaths>("-i");

  using Paths = std::vector<std::filesystem::path>;
  Paths inputPaths;
  std::transform(inputStringPaths.begin(), inputStringPaths.end(),
                 std::back_inserter(inputPaths),
                 [](const std::string& sp) -> std::filesystem::path {
                   return std::filesystem::path(sp);
                 });

  std::filesystem::path outputPath;
  if (auto output = program.present("-o")) {
    outputPath = std::filesystem::path(*output);
  } else {
    if (outputFormatType != FormatType::NOT_PROVIDED) {
      SPDLOG_CRITICAL("output option (-o) is mandatory when the output format provided");
      std::exit(EXIT_FAILURE);
    }
  }

  try {
    NLDB* db = nullptr;
    NLLibrary* primitivesLibrary = nullptr;
    {
      naja::NajaPerf::Scope scope("SNL Creation");
      NLUniverse::create();

      if (inputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Parsing SNL format");
        if (inputPaths.size() > 1) {
          SPDLOG_CRITICAL("Multiple input paths are not supported for SNL format");
          std::exit(EXIT_FAILURE);
        }
        const auto start{std::chrono::steady_clock::now()};
        auto inputPath = inputPaths[0];
        db = SNLCapnP::load(inputPath);
        NLUniverse::get()->setTopDesign(db->getTopDesign());
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          SPDLOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Parsing verilog");
        db = NLDB::create(NLUniverse::get());
        primitivesLibrary = NLLibrary::create(db, NLLibrary::Type::Primitives,
                                               NLName("PRIMS"));
        for (const auto& path : primitivesPaths) {
          SPDLOG_INFO("Parsing primitives file: {}", path.string());
          auto extension = path.extension();
          if (extension.empty()) {
            SPDLOG_CRITICAL("Primitives path should end with an extension");
            std::exit(EXIT_FAILURE);
          } else if (extension == ".py") {
            SNLPyLoader::loadPrimitives(primitivesLibrary, path);
          } else if (extension == ".lib") {
            SNLLibertyConstructor constructor(primitivesLibrary);
            constructor.construct(path);
          } else {
            SPDLOG_CRITICAL("Unknow extension in Primitives path");
            std::exit(EXIT_FAILURE);
          }
        }

        auto designLibrary = NLLibrary::create(db, NLName("DESIGN"));
        SNLVRLConstructor constructor(designLibrary);
        const auto start{std::chrono::steady_clock::now()};
        {
          std::ostringstream oss;
          oss << "Parsing verilog file(s): ";
          size_t i = 0;
          for (auto path : inputPaths) {
            if (i++ >= 4) {
              oss << std::endl;
              i = 0;
            }
            oss << path << " ";
          }
          SPDLOG_INFO(oss.str());
        }
        constructor.construct(inputPaths);
        auto top = SNLUtils::findTop(designLibrary);
        if (top) {
          NLUniverse::get()->setTopDesign(top);
          SPDLOG_INFO("Found top design: " + top->getString());
        } else {
          SPDLOG_ERROR("No top design was found after parsing verilog");
        }
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          SPDLOG_INFO(oss.str());
        }
    } else if (inputFormatType == FormatType::NOT_PROVIDED) {
      db = NLDB::create(NLUniverse::get());
      NLUniverse::get()->setTopDB(db);
      } else {
        SPDLOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
        std::exit(EXIT_FAILURE);
      }
    }

    if (program.is_used("-e")) {
      naja::NajaPerf::Scope scope("Python Pre Editing");
      const auto start{std::chrono::steady_clock::now()};
      auto editPath = std::filesystem::path(program.get<std::string>("-e"));
      SPDLOG_INFO("Editing netlist using python script (post netlist loading): {}", editPath.string());
      SNLPyEdit::edit(editPath);
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Editing done in: " << elapsed_seconds.count() << "s";
        SPDLOG_INFO(oss.str());
      }
    }
    bool useBNE = true;
    if (std::getenv("NAJA_DISABLE_BNE")) {
      useBNE = false;
    }
    if (optimizationType == OptimizationType::DLE) {
      naja::NajaPerf::Scope scope("Optimization_DLE");
      const auto start{std::chrono::steady_clock::now()};
      SPDLOG_INFO("Starting removal of loadless logic");
      LoadlessLogicRemover remover;
      remover.setNormalizedUniquification(useBNE);
      remover.process();
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Removal of loadless logic done in: " << elapsed_seconds.count() << "s";
        SPDLOG_INFO(oss.str());
      } 
      //NetlistStatistics stats(*get());
      //stats.process();
      //spdlog::info(stats.getReport());
    } else if (optimizationType == OptimizationType::ALL) {
      naja::NajaPerf::Scope scope("Optimization_ALL");
      const auto start{std::chrono::steady_clock::now()};
      spdlog::info("Starting full optimization (constant propagation and removal of loadless logic)");
      ConstantPropagation cp;
      cp.setTruthTableEngine(true);
      cp.setNormalizedUniquification(useBNE);
      cp.run();
      ReductionOptimization reductionOptimization(cp.getPartialConstantReaders());
      reductionOptimization.setNormalizedUniquification(useBNE);
      reductionOptimization.run();
      LoadlessLogicRemover remover;
      remover.setNormalizedUniquification(useBNE);
      remover.process();
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Removal of loadless logic done in: " << elapsed_seconds.count()
            << "s";
        spdlog::info(oss.str());
      }
      //NetlistStatistics stats(*get());
      //stats.process();
      //spdlog::info(stats.getReport());
    }

    if (program.is_used("-z")) {
      naja::NajaPerf::Scope scope("Python Post Editing");
      const auto start{std::chrono::steady_clock::now()};
      auto editPath = std::filesystem::path(program.get<std::string>("-z"));
      SPDLOG_INFO("Post editing netlist using python script: {}", editPath.string());
      SNLPyEdit::edit(editPath);
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Post editing done in: " << elapsed_seconds.count() << "s";
        SPDLOG_INFO(oss.str());
      }
    }

    {
      naja::NajaPerf::Scope scope("Dumping Netlist");
      if (outputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Dumping SNL format");
        SPDLOG_INFO("Dumping netlist in SNL format to {}", outputPath.string());
        SNLCapnP::dump(db, outputPath);
      } else if (outputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Dumping verilog");
        if (db->getTopDesign()) {
          std::ofstream output(outputPath);
          SNLVRLDumper dumper;
          dumper.setSingleFile(true);
          SPDLOG_INFO("Dumping netlist in verilog format to {}", outputPath.string());
          dumper.dumpDesign(db->getTopDesign(), output);
        } else {
          db->debugDump(0);
        }
      } else if (outputFormatType == FormatType::DOT) {
        naja::NajaPerf::Scope scope("Dumping DOT format");
        std::string dotFileName(outputPath.string());
        naja::SnlVisualiser snl(db->getTopDesign());
        snl.process();
        snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
      } /*else if (outputFormatType == FormatType::SVG) {
          std::string dotFileName(outputPath.string());
          std::string svgFileName(
              outputPath.string() + std::string(".svg"));
          naja::SnlVisualiser snl(db->getTopDesign());
          snl.process();
          snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
          system(std::string(std::string("dot -Tsvg ") + dotFileName +
                            std::string(" -o ") + svgFileName)
                    .c_str());
      }*/
    }

    if (program.is_used("-d")) {
      if (not primitivesLibrary and inputFormatType==FormatType::SNL) {
        auto primitiveLibraries = db->getPrimitiveLibraries();
        if (not primitiveLibraries.empty()) {}
          primitivesLibrary = *(db->getPrimitiveLibraries().begin());
      }
      if (primitivesLibrary) {
        auto outputPrimitivesPath =
            std::filesystem::path(program.get<std::string>("-d"));
        std::ofstream output(outputPrimitivesPath);
        SNLVRLDumper dumper;
        dumper.setSingleFile(true);
        dumper.dumpLibrary(primitivesLibrary, output);
      }
    }
  } catch (const NLException& e) {
    SPDLOG_CRITICAL("Caught Naja error: {}\n{}",
      e.what(), e.trace().to_string()); 
    //SPDLOG_CRITICAL("Caught SNL error: {}\n", e.what());
    std::exit(EXIT_FAILURE);
  }
  const auto najaEditEnd{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> najaElapsedSeconds{najaEditEnd - najaEditStart};
  auto memInfo = naja::NajaPerf::getMemoryUsage();
  auto vmRSS = memInfo.first;
  auto vmPeak = memInfo.second;
  SPDLOG_INFO("########################################################");
  {
    std::ostringstream oss;
    oss << "naja_edit done in: " << najaElapsedSeconds.count() << "s";
    if (vmRSS != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(RSS): " << vmRSS / 1024.0 << "Mb";
    }
    if (vmPeak != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(Peak): " << vmPeak / 1024.0 << "Mb";
    }
    SPDLOG_INFO(oss.str());
  }
  SPDLOG_INFO("########################################################");
  std::exit(EXIT_SUCCESS);
}
