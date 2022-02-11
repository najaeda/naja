#include "SNLDump.h"

namespace {

using DesignsLevel = map<SNLDesign*, unsigned>;

unsigned levelize(top, DesignsLevel& designsLevel) {
  unsigned maxLevel = 0;
  for auto instance: top->getInstances() {
    unsigned level = 0;
    auto model = instance->getModel();
    auto it = designsLevel.find(model);
    if (it == designsLevel.end()) {
      level = levelize(model, designsLevel);
      designsLevel.insert(model, level);
    } else {
      level = it->second;
    }
    if (level > maxLevel) {
      maxLevel = level;
    }
  }
  return maxLevel;
}

void dumpDesign(SNLDesign* design, file) {
  file << "D " << name;
  for (auto parameter: design->getParameters()) {
    dumpParameter(parameter, file);
  }
  for (auto net: design->getNets()) {
    dumpNet(net, file);
  }
  for (auto term: design->getTerms()) {
    dumpTerm(term, file);
  }
  for (auto instance: design->getInstances()) {
    dumpInstance(instance, file);
  }
}

}

namespace SNL {

void SNLDump::dump(const SNLDesign* top, const std::filesystem::path& path) {
  //create directory .snl with top name
  string dirName;
  if (not top->isAnonymous()) {
    dirName = top->getName().getString();
  } else {
    dirName = "anon";
  }
  dirName += ".snl";
  if (filesystem::exists(dirName)) {
    filesystem::rmdir(dirName);
  }
  filesystem::create(dirName);
  //publish manifest
  SNLDumpManifest::create(top, snlDumpDir);

  //sort levels
  for auto design: levels {
    dumpDesign(design, dumpFile);
  }
}

}
