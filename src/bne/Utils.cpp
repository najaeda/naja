// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "Utils.h"
#include <vector>
#include "SNLBusNetBit.h"
#include "SNLDB0.h"
#include "SNLUniverse.h"
#include <sstream>

using namespace naja::DNL;
using namespace naja::SNL;

// #define DEBUG_PRINTS

namespace naja::BNE
{

  void Uniquifier::process()
  {
    std::vector<SNLInstance *> instancesToDelete;
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Uniquifier::process() - dnlid %s\n", id_);
    printf("Uniquifier::process() - final inst %s\n", path_.back()->getName().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    SNLDesign *currentDesign = SNLUniverse::get()->getTopDesign();
    ;
    for (size_t i = 0; i < path_.size(); i++)
    {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("Uniquifier::process() - - looking now at inst %s\n", path_[i]->getName().getString().c_str());
      printf("Uniquifier::process() - - looking now at design %s\n",
             path_[i]->getDesign()->getName().getString().c_str());
      for (auto inst :
           currentDesign->getInstances())
      {
        printf(" - child inst %s\n",
               inst->getName().getString().c_str());
      }
      // LCOV_EXCL_STOP
#endif
      SNLInstance *inst = currentDesign->getInstance(path_[i]);

      if ((i == path_.size() - 1) && !uniquifyTail_)
      {
        // If we are at the last instance, we can keep it
        pathUniq_.push_back(inst);
        break;
      }
      //assert(!inst->getModel()->getInstances().empty());
#ifdef DEBUG_PRINTS
      printf("%u(%lu) ", path_[i], inst->getModel()->getSlaveInstances().size());
#endif
      //For primitives slave instances are not cached
      assert(inst->getModel()->getSlaveInstances().size() > 0 || inst->getModel()->isPrimitive());
      if (inst->getModel()->getSlaveInstances().size() == 1 || inst->getModel()->isPrimitive())
      {
        // If the instance has only one slave, we can keep it
        // This only works when you work top down
        pathUniq_.push_back(inst);
      }
      else
      {
        // If the instance has more than one slave, we need to clone it
        pathUniq_.push_back(replaceWithClone(inst));
      }
      assert(pathUniq_.back()->getModel()->getSlaveInstances().size() == 1 || inst->getModel()->isPrimitive());
      currentDesign = pathUniq_.back()->getModel();
    }
#ifdef DEBUG_PRINTS
    printf("\n");
#endif
  }

  SNLInstance *Uniquifier::replaceWithClone(SNLInstance *inst)
  {
    SNLDesign *clone = inst->getModel()->clone(
        SNLName(std::string(inst->getModel()->getName().getString()) +
                std::string("_clone_") + id_));
    assert(clone->getSlaveInstances().size() == 0);
    inst->setModel(clone);
    assert(clone->getSlaveInstances().size() == 1);
    return inst;
  }

  std::string Uniquifier::getFullPath()
  {
    std::string fullPath;
    for (auto inst : pathUniq_)
    {
      fullPath += inst->getName().getString() + "/";
    }
    return fullPath;
  }

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

} // namespace naja::NAJA_OPT