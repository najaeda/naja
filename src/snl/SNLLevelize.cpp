#include "SNLLevelize.h"

#include <map>

#include "SNLDesign.h"

namespace {

using DesignLevels = std::map<SNL::SNLDesign*, unsigned, SNL::SNLDesign::Less>;

void explore(const SNL::SNLDesign* design, DesignLevels& levels) {
  auto designIt = levels.find(design);
  if (designIt == levels.end()) {
    //internal error
  }
  unsigned designLevel = designIt.second;
  SNL::SNLCollection<SNL::SNLInstance> instances = design->getInstances();
  for (SNL::SNLIterator<SNL::SNLInstance> iit = instances.getIterator();
      iit.isValid();
      ++iit) {
    SNL::SNLInstance* instance = iit.getElement();
    SNL::SNLDesign* model = instance->getModel();
    if (model->isPrimitive()) {
      continue;
    }
    auto it = levels.find(design);
    if (it != levels.end()) {
      unsigned level = it.second;
      if (level > designLevel) {
        continue;
      }
    }
    levels[model] = designLevel+1;
  }
}

}

namespace SNL {

SNLLevelize::SNLLevelize(const SNLDesign* top) {
  DesignLevels levels;
  levels[top] = 0;
  explore(top, levels);
}

}
