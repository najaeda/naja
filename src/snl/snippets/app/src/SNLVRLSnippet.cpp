#include <filesystem>
#include <iostream>
#include <fstream>

#include "SNLException.h"
#include "SNLPrimitivesLoader.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"
#include "SNLUtils.h"

#include "SNLUniverse.h"

using namespace naja::SNL;

namespace {

void usage() {
  std::cerr << "snl_vrl_app path_to_primitives.py path_to_design.v" << std::endl;
  exit(-1);
}

}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    usage();
  }

  std::string primitives = argv[1];
  std::string design = argv[2];

  std::filesystem::path primitivesPath(primitives);
  std::filesystem::path designPath(design);

  try {
    SNLUniverse::create();
    auto db = SNLDB::create(SNLUniverse::get());
    auto primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
    SNLPrimitivesLoader::load(primitivesLibrary, primitivesPath);

    auto designLibrary = SNLLibrary::create(db, SNLName("DESIGN"));
    SNLVRLConstructor constructor(designLibrary);
    constructor.construct(designPath);

    auto top = SNLUtils::findTop(designLibrary);
    if (top) {
      db->setTopDesign(top);
    }

    if (db->getTopDesign()) {
      std::ofstream output("design.v");
      SNLVRLDumper dumper;
      dumper.setSingleFile(true);
      dumper.dumpDesign(db->getTopDesign(), output);
    }
  } catch (const SNLException& e) {
    std::cerr << "Caught SNL error: " << e.getReason() << std::endl;
  }
  return 0;
}