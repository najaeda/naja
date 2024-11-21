// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "Utils.h"

#include <vector>
#include <sstream>

#include "SNLUniverse.h"
#include "SNLBusNetBit.h"
#include "SNLDB0.h"

using namespace naja::SNL;
using namespace naja::DNL;

// #define DEBUG_PRINTS

namespace naja::BNE {

  void NetlistStatistics::process()
  {
    std::map<std::string, size_t> pritmitveCount;
    std::stringstream ss;
    for (auto leaf : dnl_.getLeaves())
    {
      pritmitveCount[dnl_.getDNLInstanceFromID(leaf).getSNLModel()->getName().getString()]++;
    }
    for (const auto &entry : pritmitveCount)
    {
      ss << entry.first << " : " << entry.second << std::endl;
    }
    report_ = ss.str();
  }

  SNLInstance *getInstanceForPath(const std::vector<SNLID::DesignObjectID> &pathToModel)
  {
    std::vector<SNLID::DesignObjectID> path = pathToModel;
    SNLDesign *top = SNLUniverse::get()->getTopDesign();
    SNLDesign *designToSet = top;
    SNLInstance *inst = nullptr;
#ifdef DEBUG_PRINTS
    printf("path ");
#endif
    while (!path.empty())
    {
      SNLID::DesignObjectID name = path.front();
      path.erase(path.begin());
      inst = designToSet->getInstance(name);
#ifdef DEBUG_PRINTS
      printf("%s(%s) ", inst->getName().getString().c_str(), inst->getModel()->getName().getString().c_str());
#endif
      assert(inst);
      designToSet = inst->getModel();
    }
#ifdef DEBUG_PRINTS
    printf("\n");
#endif
    return inst;
  }
} // namespace naja::BNE
