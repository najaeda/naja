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
  argparse::ArgumentParser program("naja_x2y");
  program.add_argument("-a", "--input_format")
    .required()
    .help("input format");
  program.add_argument("-b", "--output_format")
    .required()
    .help("output format");
  program.add_argument("-i", "--input")
    .required()
    .help("input netlist");
  program.add_argument("-o", "--output")
    .required()
    .help("output netlist");
  program.add_argument("-p", "--primitives")
    .help("input primitives");
  program.add_argument("-d", "--dump_primitives")
    .help("dump primitives library in verilog");

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }
  auto inputFormatArg = program.present("-a");
  auto outputFormatArg = program.present("-b");
  std::string inputFormat = *inputFormatArg;
  std::string outputFormat = *outputFormatArg;
  FormatType inputFormatType = argToFormatType(inputFormat);
  FormatType outputFormatType = argToFormatType(outputFormat);
  if (inputFormatType == FormatType::UNKNOWN or outputFormatType == FormatType::UNKNOWN) {
    return 4;
  }

  std::filesystem::path primitivesPath;
  if (auto primitives = program.present("-p")) {
    if (inputFormatType == FormatType::SNL) {
      return 4;
    }
    primitivesPath = std::filesystem::path(*primitives);
  } else {
    if (inputFormatType != FormatType::SNL) {
      return 4;
    }
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
    } else if (inputFormatType == FormatType::VERILOG) {
      db = SNLDB::create(SNLUniverse::get());
      primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
      SNLPrimitivesLoader::load(primitivesLibrary, primitivesPath);

      auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
      SNLVRLConstructor constructor(designLibrary);
      constructor.construct(inputPath);

      auto top = SNLUtils::findTop(designLibrary);
      if (top) {
        db->setTopDesign(top);
      } else {
        
      }
    } else {
      return -1;
    }

    if (db->getTopDesign()) {
      std::ofstream output(outputPath);
      SNLVRLDumper dumper;
      dumper.setSingleFile(true);
      dumper.dumpDesign(db->getTopDesign(), output);
    } else {
      db->debugDump(0);
    }
    
    if (program.is_used("-d") and primitivesLibrary) {
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
