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

#include "SNLDumper.h"

#include <map>
#include <fstream>

#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLDumpManifest.h" 

namespace {
using namespace naja::SNL;
using DesignsLevel = std::map<const SNLDesign*, unsigned>;

unsigned levelize(const SNLDesign* design, DesignsLevel& designsLevel) {
  unsigned maxLevel = 0;
  for (auto instance: design->getInstances()) {
    unsigned level = 0;
    auto model = instance->getModel();
    auto it = designsLevel.find(model);
    if (it == designsLevel.end()) {
      level = levelize(model, designsLevel);
      designsLevel[model] = level;
    } else {
      level = it->second;
    }
    if (level > maxLevel) {
      maxLevel = level;
    }
  }
  return maxLevel;
}

void dumpParameter(const SNLParameter* parameter, std::ostream& stream) {
}

void dumpTerm(const SNLTerm* term, std::ostream& stream) {
}

void dumpNet(const SNLNet* net, std::ostream& stream) {
}

void dumpInstance(const SNLInstance* instance, std::ostream& stream) {
}

void dumpDesign(const SNLDesign* design, std::ostream& stream) {
  stream << "D"
    << " " << design->getLibrary()->getID()
    << " " << design->getID()
    << " " << design->getName().getString() << std::endl;
  for (auto parameter: design->getParameters()) {
    dumpParameter(parameter, stream);
  }
  for (auto net: design->getNets()) {
    dumpNet(net, stream);
  }
  for (auto term: design->getTerms()) {
    dumpTerm(term, stream);
  }
  for (auto instance: design->getInstances()) {
    dumpInstance(instance, stream);
  }
}

}

namespace naja { namespace SNL {

void SNLDumper::dump(const SNLDesign* top, const std::filesystem::path& path) {
  //create directory .snl with top name
  std::string dirName;
  if (not top->isAnonymous()) {
    dirName = top->getName().getString();
  } else {
    dirName = "anon";
  }
  dirName += ".snl";
  std::filesystem::path dir(dirName);
  if (std::filesystem::exists(dir)) {
    std::filesystem::remove_all(dir);
  }
  std::filesystem::create_directory(dirName);
  //publish manifest
  SNLDumpManifest::create(top, dir);

  DesignsLevel designsLevel;
  levelize(top, designsLevel);

  using DesignLevel = std::pair<const SNLDesign*, unsigned>;
  using SortedDesigns = std::vector<DesignLevel>;
  SortedDesigns designs(designsLevel.begin(), designsLevel.end());
  std::sort(designs.begin(), designs.end(),
    [](const DesignLevel& ldl, const DesignLevel& rdl) { 
      return ldl.second < rdl.second;
    }
  );
  std::filesystem::path dumpPath(dir/"design.db");
  std::ofstream dumpStream(dumpPath);
  for (const DesignLevel& dl: designs) {
    dumpDesign(dl.first, dumpStream);
  }
}

}} // namespace SNL // namespace naja