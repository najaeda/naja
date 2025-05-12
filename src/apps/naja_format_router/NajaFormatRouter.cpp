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
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>

#include "NajaPerf.h"
#include "NajaUtils.h"
#include "NajaVersion.h"

#include "NLException.h"
#include "SNLLibertyConstructor.h"
#include "SNLPyEdit.h"
#include "SNLPyLoader.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include "ConstantPropagation.h"
#include "DNL.h"
#include "NetlistGraph.h"
#include "Reduction.h"
#include "RemoveLoadlessLogic.h"
#include "SNLCapnP.h"
#include "NLUniverse.h"
#include "Utils.h"
#include "LEFConstructor.h"
#include "LEFDumper.h"
#include "DEFConstructor.h"
#include "DEFDumper.h"
#include "NLLibrary.h"
#include "PNLInstTerm.h"
#include "PNLDesign.h"

using namespace naja::NL;
using namespace naja::DNL;
using namespace naja::NAJA_OPT;

namespace {

enum class FormatType { NOT_PROVIDED, UNKNOWN, VERILOG, SNL, DOT, SVG };
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
const std::string NAJA_EDIT_VERSION(NAJA_EDIT_MAJOR + "." + NAJA_EDIT_MINOR +
                                    "." + NAJA_EDIT_REVISION);
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
      .help(
          "input primitives: list of path to primitives files (liberty format "
          "or Naja python format)");
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

  //naja::NajaPerf::Scope scope("Parsing Lef");
  NLUniverse::create();
  auto lib = LEFConstructor::construct("./file.lef"); 
  printf("Library loaded %s\n", lib->getName().getString().c_str());
  for (auto design : lib->getPNLDesigns()) {
    printf("Design: %s\n", design->getName().getString().c_str());
    for (auto pin : design->getTerms()) {
      printf("- Pin: %s\n", pin->getName().getString().c_str());
    }
    for (auto net : design->getNets()) {
      printf("- Net: %s\n", net->getName().getString().c_str());
    }
  }
  
  naja::NL::PNLDesign* design = DEFConstructor::construct("./file.def", DEFConstructor::FitAbOnDesigns, lib->getDB());
  for (auto instance : design->getInstances()) {
    printf("instance: %s\n", instance->getName().getString().c_str());
  }
  for (auto net : design->getNets()) {
    printf("net %s:\n", net->getName().getString().c_str());
    PNLBitNet* bitNet = dynamic_cast<PNLBitNet*>(net);
    for (auto term : bitNet->getInstTerms() ) {
      printf("--term(%s): %s\n", term->getInstance()->getName().getString().c_str(), term->getName().getString().c_str());
    }
  }
  naja::NL::LEFDumper::dump(lib);
  DEFDumper::drive(design, DEFDumper::WithLEF);
  printf("Done\n");
}
