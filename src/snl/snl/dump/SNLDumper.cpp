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

#include "SNLDump.h"

#include <map>
#include <fstream>

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLDump.h"
#include "SNLDumpManifest.h"

namespace {
using namespace naja::SNL;
using DesignsLevel = std::map<const SNLDesign*, unsigned>;

unsigned levelize(const SNLDesign* design, DesignsLevel& designsLevel) {
  unsigned maxLevel = 0;
  if (design->getInstances().empty()) {
    designsLevel[design] = 0;
  } else {
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
  }
  return maxLevel;
}

void dumpParameter(const SNLParameter* parameter, std::ostream& stream) {
  stream << SNLDump::Tag::Parameter
    << " " << parameter->getName().getString()
    << " " << parameter->getValue()
    << std::endl;
} 

void dumpTerm(const SNLTerm* term, std::ostream& stream) {
  stream << "T"
    << " " << term->getID()
    << " " << term->getDirection();
  if (not term->isAnonymous()) {
    stream << " " << term->getName().getString();
  }
  stream << std::endl;
}

void dumpNet(const SNLNet* net, std::ostream& stream) {
  stream << "N"
    << " " << net->getID();
  if (not net->isAnonymous()) {
    stream << " " << net->getName().getString();
  }
  stream << std::endl;
}

void dumpInstance(const SNLInstance* instance, std::ostream& stream) {
  stream << SNLDump::Tag::Instance
    << " " << instance->getModel()->getDB()->getID()
    << " " << instance->getModel()->getLibrary()->getID()
    << " " << instance->getModel()->getID()
    << " " << instance->getID()
    << std::endl;
}

void dumpDesign(const SNLDesign* design, std::ostream& stream) {
  stream << SNLDump::Tag::Design
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

void SNLDump::dump(const SNLDesign* top, const std::filesystem::path& path) {
  //create directory .snl with top name
  std::string dirName;
  if (not top->isAnonymous()) {
    dirName = top->getName().getString();
  } else {
    dirName = "anon";
  }
  dirName += ".snl";
  std::filesystem::path dir(path/dirName);
  if (std::filesystem::exists(dir)) {
    std::filesystem::remove_all(dir);
  }
  std::filesystem::create_directory(dir);
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