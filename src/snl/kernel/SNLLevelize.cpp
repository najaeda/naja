/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
