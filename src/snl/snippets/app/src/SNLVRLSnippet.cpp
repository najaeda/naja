#include <filesystem>
#include <fstream>

#include "SNLVRLConstructor.h"
#include "SNLVRLDumper.h"

#include "SNLUniverse.h"

using namespace naja::SNL;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    
    
  }
  std::string fileName = argv[1];
  std::filesystem::path filePath(fileName);

  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLName("DESIGNS"));

  SNLVRLConstructor constructor(library);
  constructor.construct(filePath);


  if (db->getTopDesign()) {
    std::ofstream output("design.v");
    SNLVRLDumper dumper;
    dumper.setSingleFile(true);
    dumper.dumpDesign(db->getTopDesign(), output);
  }
  return 0;
}