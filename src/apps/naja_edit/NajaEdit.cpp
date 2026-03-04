// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <fstream>
#include <iostream>

#include <argparse/argparse.hpp>

#include "NajaVersion.h"
#include "NajaPerf.h"
#include "NajaUtils.h"
#include "NajaLog.h"

#include "NLUniverse.h"
#include "NLException.h"
#include "SNLPyEdit.h"
#include "SNLPyLoader.h"
#include "SNLUtils.h"
#include "SNLLibertyConstructor.h"
#include "SNLSVConstructor.h"
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

enum class FormatType { NOT_PROVIDED, UNKNOWN, VERILOG, SYSTEMVERILOG, SNL, DOT, SVG};
FormatType argToFormatType(const std::string& inputFormat) {
  if (inputFormat.empty()) {
    return FormatType::NOT_PROVIDED;
  } else if (inputFormat == "verilog") {
    return FormatType::VERILOG;
  } else if (inputFormat == "systemverilog" || inputFormat == "sv") {
    return FormatType::SYSTEMVERILOG;
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

}

int main(int argc, char* argv[]) {
  const auto najaEditStart{std::chrono::steady_clock::now()};
  argparse::ArgumentParser program("naja_edit", naja::NAJA_VERSION);
  program.add_description(
      "Edit gate level netlists using python script and apply optimizations");
  program.add_argument("-f", "--from_format")
    .help("from/input format (verilog|systemverilog|sv|snl)");
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
  program.add_argument("--sv_flist")
    .help("SystemVerilog slang command file path (passed as -f <file>)");
  program.add_argument("--sv_top")
    .help("SystemVerilog top module name (injected as --top)");
  program.add_argument("--sv_elaborated_ast_json_path")
    .help("Dump Slang elaborated AST JSON for SystemVerilog parsing");
  program.add_argument("--sv_diagnostics_report_path")
    .help("Dump SystemVerilog diagnostics report path");
  program.add_argument("--sv_no_pretty_print_elaborated_ast_json")
    .default_value(false)
    .implicit_value(true)
    .help("Disable pretty-print formatting for SystemVerilog elaborated AST JSON dump");
  program.add_argument("--sv_no_source_info_in_elaborated_ast_json")
    .default_value(false)
    .implicit_value(true)
    .help("Disable source info in SystemVerilog elaborated AST JSON dump");

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

  naja::log::init();
  naja::log::setPattern("[naja_edit] [%^%l%$] %v");
  naja::log::setLevel(spdlog::level::trace);

  if (program.is_used("--log")) {
    auto logName = program.get<std::string>("--log");
    {
      std::ofstream logFile(logName, std::ios::out);
      if (logFile.is_open()) {
        std::string bannerTitle = "naja_edit";
        std::ostringstream bannerStream;
        naja::NajaUtils::createBanner(logFile, bannerTitle, "#");
        logFile << std::endl;
        logFile.close();
      }
    }
    naja::log::addFileSink(logName, spdlog::level::trace);
  }

  naja::log::flushEvery(std::chrono::seconds(3));

  bool argError = false;

  std::string inputFormat;
  if (auto inputFormatArg = program.present("-f")) {
    inputFormat = *inputFormatArg;
  }
  FormatType inputFormatType = argToFormatType(inputFormat);
  if (inputFormatType == FormatType::UNKNOWN) {
    NAJA_LOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
    argError = true;
  }

  if (program.present("-i")) {
    if (inputFormatType == FormatType::NOT_PROVIDED) {
      NAJA_LOG_CRITICAL("output option (-f) is mandatory when the input is provided");
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
        NAJA_LOG_CRITICAL("output format option (-t) is mandatory");
        argError = true;
      } else {
        outputFormat = inputFormat;
      }
    }
  }
  FormatType outputFormatType = argToFormatType(outputFormat);
  
  if (outputFormatType == FormatType::UNKNOWN) {
    NAJA_LOG_CRITICAL("Unrecognized output format type: {}", outputFormat);
    argError = true;
  }

  Paths primitivesPaths;
  if (auto primitives = program.present("-p")) {
    if (inputFormatType == FormatType::SNL) {
      NAJA_LOG_CRITICAL("primitives option (-p) is incompatible with input format 'SNL'");
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
  }

  bool hasSvFlistPath = false;
  std::filesystem::path svFlistPath;
  if (auto svFlist = program.present("--sv_flist")) {
    hasSvFlistPath = true;
    svFlistPath = std::filesystem::path(*svFlist);
  }

  bool hasSvTop = false;
  std::string svTop;
  if (auto svTopArg = program.present("--sv_top")) {
    hasSvTop = true;
    svTop = *svTopArg;
    if (svTop.empty()) {
      NAJA_LOG_CRITICAL("SystemVerilog top name (--sv_top) cannot be empty");
      argError = true;
    }
  }

  bool hasSvElaboratedASTJsonPath = false;
  std::filesystem::path svElaboratedASTJsonPath;
  if (auto svAstPath = program.present("--sv_elaborated_ast_json_path")) {
    hasSvElaboratedASTJsonPath = true;
    svElaboratedASTJsonPath = std::filesystem::path(*svAstPath);
  }

  bool hasSvDiagnosticsReportPath = false;
  std::filesystem::path svDiagnosticsReportPath;
  if (auto svDiagPath = program.present("--sv_diagnostics_report_path")) {
    hasSvDiagnosticsReportPath = true;
    svDiagnosticsReportPath = std::filesystem::path(*svDiagPath);
  }

  const bool svPrettyPrintElaboratedASTJson =
    !program.get<bool>("--sv_no_pretty_print_elaborated_ast_json");
  const bool svIncludeSourceInfoInElaboratedASTJson =
    !program.get<bool>("--sv_no_source_info_in_elaborated_ast_json");

  const bool hasSystemVerilogSpecificOption =
    hasSvFlistPath || hasSvTop || hasSvElaboratedASTJsonPath || hasSvDiagnosticsReportPath ||
    !svPrettyPrintElaboratedASTJson || !svIncludeSourceInfoInElaboratedASTJson;
  if (hasSystemVerilogSpecificOption && inputFormatType != FormatType::SYSTEMVERILOG) {
    NAJA_LOG_CRITICAL(
      "SystemVerilog parsing options (--sv_*) are only valid with input format "
      "'systemverilog' (or 'sv')");
    argError = true;
  }
  if (inputFormatType == FormatType::SYSTEMVERILOG &&
      !program.present("-i") && !hasSvFlistPath) {
    NAJA_LOG_CRITICAL(
      "For SystemVerilog input, provide at least one --input (-i) file or --sv_flist");
    argError = true;
  }

  OptimizationType optimizationType = OptimizationType::NOT_PROVIDED;
  if (auto optimizationArg = program.present("-a")) {
    std::string optimization = *optimizationArg;
    optimizationType = argToOptimizationType(optimization);
    if (optimizationType == OptimizationType::UNKNOWN) {
      NAJA_LOG_CRITICAL("Unrecognized optimization type: {}", optimization);
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
      NAJA_LOG_CRITICAL("output option (-o) is mandatory when the output format provided");
      std::exit(EXIT_FAILURE);
    }
  }

  try {
    NLDB* db = nullptr;
    NLLibrary* primitivesLibrary = nullptr;
    auto loadPrimitivesLibrary = [&](NLDB* currentDB) -> NLLibrary* {
      if (primitivesPaths.empty()) {
        return nullptr;
      }
      auto* currentPrimitivesLibrary =
        NLLibrary::create(currentDB, NLLibrary::Type::Primitives, NLName("PRIMS"));
      for (const auto& path : primitivesPaths) {
        NAJA_LOG_INFO("Parsing primitives file: {}", path.string());
        auto extension = path.extension();
        if (extension.empty()) {
          NAJA_LOG_CRITICAL("Primitives path should end with an extension");
          std::exit(EXIT_FAILURE);
        } else if (extension == ".py") {
          SNLPyLoader::loadPrimitives(currentPrimitivesLibrary, path);
        } else if (extension == ".lib") {
          SNLLibertyConstructor constructor(currentPrimitivesLibrary);
          constructor.construct(path);
        } else {
          NAJA_LOG_CRITICAL("Unknow extension in Primitives path");
          std::exit(EXIT_FAILURE);
        }
      }
      return currentPrimitivesLibrary;
    };
    {
      naja::NajaPerf::Scope scope("SNL Creation");
      NLUniverse::create();

      if (inputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Parsing SNL format");
        if (inputPaths.size() > 1) {
          NAJA_LOG_CRITICAL("Multiple input paths are not supported for SNL format");
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
          NAJA_LOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Parsing verilog");
        db = NLDB::create(NLUniverse::get());
        primitivesLibrary = loadPrimitivesLibrary(db);

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
          NAJA_LOG_INFO(oss.str());
        }
        constructor.construct(inputPaths);
        auto top = SNLUtils::findTop(designLibrary);
        if (top) {
          NLUniverse::get()->setTopDesign(top);
          NAJA_LOG_INFO("Found top design: " + top->getString());
        } else {
          NAJA_LOG_ERROR("No top design was found after parsing verilog");
        }
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          NAJA_LOG_INFO(oss.str());
        }
      } else if (inputFormatType == FormatType::SYSTEMVERILOG) {
        naja::NajaPerf::Scope scope("Parsing systemverilog");
        db = NLDB::create(NLUniverse::get());
        primitivesLibrary = loadPrimitivesLibrary(db);

        auto designLibrary = NLLibrary::create(db, NLName("DESIGN"));
        SNLSVConstructor constructor(designLibrary);
        SNLSVConstructor::ConstructOptions options;
        options.prettyPrintElaboratedASTJson = svPrettyPrintElaboratedASTJson;
        options.includeSourceInfoInElaboratedASTJson =
          svIncludeSourceInfoInElaboratedASTJson;
        if (hasSvElaboratedASTJsonPath) {
          options.elaboratedASTJsonPath = svElaboratedASTJsonPath;
        }
        if (hasSvDiagnosticsReportPath) {
          options.diagnosticsReportPath = svDiagnosticsReportPath;
        }

        Paths svInputPaths = inputPaths;
        bool removeTemporaryTopFlist = false;
        std::filesystem::path temporaryTopFlistPath;
        auto removeTemporaryTopFlistIfNeeded = [&]() {
          if (!removeTemporaryTopFlist) {
            return;
          }
          std::error_code ec;
          std::filesystem::remove(temporaryTopFlistPath, ec);
        };

        const auto quotePathForSlangCommandFile = [](const std::filesystem::path& path) {
          std::string quoted;
          quoted.reserve(path.string().size() + 2);
          quoted.push_back('"');
          for (const auto c : path.string()) {
            if (c == '\\' || c == '"') {
              quoted.push_back('\\');
            }
            quoted.push_back(c);
          }
          quoted.push_back('"');
          return quoted;
        };

        if (hasSvTop) {
          temporaryTopFlistPath =
            std::filesystem::temp_directory_path() /
            std::filesystem::path(
              "naja_edit_sv_top_" +
              std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
              ".f");
          std::ofstream topFlist(temporaryTopFlistPath, std::ios::out | std::ios::trunc);
          if (!topFlist) {
            NAJA_LOG_CRITICAL(
              "Failed to create temporary SystemVerilog command file: {}",
              temporaryTopFlistPath.string());
            std::exit(EXIT_FAILURE);
          }
          topFlist << "--top " << svTop << "\n";
          if (hasSvFlistPath) {
            topFlist << "-f " << quotePathForSlangCommandFile(svFlistPath) << "\n";
          }
          for (const auto& svInputPath : inputPaths) {
            topFlist << quotePathForSlangCommandFile(svInputPath) << "\n";
          }
          topFlist.close();
          svInputPaths.clear();
          svInputPaths.emplace_back(std::filesystem::path("-f"));
          svInputPaths.emplace_back(temporaryTopFlistPath);
          removeTemporaryTopFlist = true;
        } else if (hasSvFlistPath) {
          svInputPaths.insert(svInputPaths.begin(), svFlistPath);
          svInputPaths.insert(svInputPaths.begin(), std::filesystem::path("-f"));
        }

        const auto start{std::chrono::steady_clock::now()};
        {
          std::ostringstream oss;
          oss << "Parsing SystemVerilog input(s): ";
          size_t i = 0;
          for (const auto& path : svInputPaths) {
            if (i++ >= 4) {
              oss << std::endl;
              i = 0;
            }
            oss << path << " ";
          }
          NAJA_LOG_INFO(oss.str());
        }
        try {
          constructor.construct(svInputPaths, options);
        } catch (...) {
          removeTemporaryTopFlistIfNeeded();
          throw;
        }
        removeTemporaryTopFlistIfNeeded();

        auto top = SNLUtils::findTop(designLibrary);
        if (top) {
          NLUniverse::get()->setTopDesign(top);
          NAJA_LOG_INFO("Found top design: " + top->getString());
        } else {
          NAJA_LOG_ERROR("No top design was found after parsing systemverilog");
        }
        const auto end{std::chrono::steady_clock::now()};
        const std::chrono::duration<double> elapsed_seconds{end - start};
        {
          std::ostringstream oss;
          oss << "Parsing done in: " << elapsed_seconds.count() << "s";
          NAJA_LOG_INFO(oss.str());
        }
    } else if (inputFormatType == FormatType::NOT_PROVIDED) {
      db = NLDB::create(NLUniverse::get());
      NLUniverse::get()->setTopDB(db);
      } else {
        NAJA_LOG_CRITICAL("Unrecognized input format type: {}", inputFormat);
        std::exit(EXIT_FAILURE);
      }
    }

    if (program.is_used("-e")) {
      naja::NajaPerf::Scope scope("Python Pre Editing");
      const auto start{std::chrono::steady_clock::now()};
      auto editPath = std::filesystem::path(program.get<std::string>("-e"));
      NAJA_LOG_INFO("Editing netlist using python script (post netlist loading): {}", editPath.string());
      SNLPyEdit::edit(editPath);
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Editing done in: " << elapsed_seconds.count() << "s";
        NAJA_LOG_INFO(oss.str());
      }
    }
    bool useBNE = true;
    if (std::getenv("NAJA_DISABLE_BNE")) {
      useBNE = false;
    }
    if (optimizationType == OptimizationType::DLE) {
      naja::NajaPerf::Scope scope("Optimization_DLE");
      const auto start{std::chrono::steady_clock::now()};
      NAJA_LOG_INFO("Starting removal of loadless logic");
      LoadlessLogicRemover remover;
      remover.setNormalizedUniquification(useBNE);
      remover.process();
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Removal of loadless logic done in: " << elapsed_seconds.count() << "s";
        NAJA_LOG_INFO(oss.str());
      } 
      //NetlistStatistics stats(*get());
      //stats.process();
      //spdlog::info(stats.getReport());
    } else if (optimizationType == OptimizationType::ALL) {
      naja::NajaPerf::Scope scope("Optimization_ALL");
      const auto start{std::chrono::steady_clock::now()};
      NAJA_LOG_INFO("Starting full optimization (constant propagation and removal of loadless logic)");
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
        NAJA_LOG_INFO(oss.str());
      }
      //NetlistStatistics stats(*get());
      //stats.process();
      //spdlog::info(stats.getReport());
    }

    if (program.is_used("-z")) {
      naja::NajaPerf::Scope scope("Python Post Editing");
      const auto start{std::chrono::steady_clock::now()};
      auto editPath = std::filesystem::path(program.get<std::string>("-z"));
      NAJA_LOG_INFO("Post editing netlist using python script: {}", editPath.string());
      SNLPyEdit::edit(editPath);
      const auto end{std::chrono::steady_clock::now()};
      const std::chrono::duration<double> elapsed_seconds{end - start};
      {
        std::ostringstream oss;
        oss << "Post editing done in: " << elapsed_seconds.count() << "s";
        NAJA_LOG_INFO(oss.str());
      }
    }

    {
      naja::NajaPerf::Scope scope("Dumping Netlist");
      if (outputFormatType == FormatType::SNL) {
        naja::NajaPerf::Scope scope("Dumping SNL format");
        NAJA_LOG_INFO("Dumping netlist in SNL format to {}", outputPath.string());
        SNLCapnP::dump(db, outputPath);
      } else if (outputFormatType == FormatType::VERILOG) {
        naja::NajaPerf::Scope scope("Dumping verilog");
        if (db->getTopDesign()) {
          std::ofstream output(outputPath);
          SNLVRLDumper dumper;
          dumper.setSingleFile(true);
          NAJA_LOG_INFO("Dumping netlist in verilog format to {}", outputPath.string());
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
    //SPDLOG_CRITICAL("Caught Naja error: {}\n{}",
    //  e.what(), e.trace().to_string()); 
    NAJA_LOG_CRITICAL("Caught SNL error: {}\n", e.what());
    std::exit(EXIT_FAILURE);
  }
  const auto najaEditEnd{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> najaElapsedSeconds{najaEditEnd - najaEditStart};
  auto memInfo = naja::NajaPerf::getMemoryUsage();
  auto vmRSS = memInfo.first;
  auto vmPeak = memInfo.second;
  NAJA_LOG_INFO("########################################################");
  {
    std::ostringstream oss;
    oss << "naja_edit done in: " << najaElapsedSeconds.count() << "s";
    if (vmRSS != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(RSS): " << vmRSS / 1024.0 << "Mb";
    }
    if (vmPeak != naja::NajaPerf::UnknownMemoryUsage) {
      oss << " VM(Peak): " << vmPeak / 1024.0 << "Mb";
    }
    NAJA_LOG_INFO(oss.str());
  }
  NAJA_LOG_INFO("########################################################");
  std::exit(EXIT_SUCCESS);
}
