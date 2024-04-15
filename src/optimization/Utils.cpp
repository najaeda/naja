// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "Utils.h"
#include <vector>
#include "SNLBusNetBit.h"
#include "SNLDB0.h"
#include "SNLUniverse.h"

using namespace naja::DNL;
using namespace naja::SNL;

// #define DEBUG_PRINTS

namespace naja::NAJA_OPT {

void Uniquifier::process() {
  std::vector<SNLInstance*> instancesToDelete;
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
  printf("Uniquifier::process() - dnlid %lu\n", id_);
  printf("Uniquifier::process() - final inst %s\n", path_.back()->getName().getString().c_str());
  // LCOV_EXCL_STOP
#endif
  SNLDesign* currentDesign = path_[0]->getDesign();
  for (size_t i = 0; i < path_.size(); i++) {
  #ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
    printf("Uniquifier::process() - - looking now at inst %s\n", path_[i]->getName().getString().c_str());
    printf("Uniquifier::process() - - looking now at design %s\n",
    path_[i]->getDesign()->getName().getString().c_str()); for (auto inst :
    currentDesign->getInstances()) { printf(" - child inst %s\n",
    inst->getName().getString().c_str());
    }
    // LCOV_EXCL_STOP
#endif
    SNLInstance* inst = currentDesign->getInstance(path_[i]->getID());

    if (i == path_.size() - 1) {
      // If we are at the last instance, we can keep it
      pathUniq_.push_back(inst);
      break;
    }
    if (inst->getModel()->getSlaveInstances().size() == 1 ||
        inst->getModel()->getSlaveInstances().size() == 0) {
      // If the instance has only one slave, we can keep it
      // This only works when you work top down
      pathUniq_.push_back(inst);
    } else {
      // If the instance has more than one slave, we need to clone it
      pathUniq_.push_back(replaceWithClone(inst));
    }
    currentDesign = pathUniq_.back()->getModel();
  }
}

SNLInstance* Uniquifier::replaceWithClone(SNLInstance* inst) {
  SNLDesign* clone = inst->getModel()->clone(
      SNLName(std::string(inst->getModel()->getName().getString()) +
              std::string("_clone_") + std::to_string(id_)));
  inst->setModel(clone);
  return inst;
}

std::string Uniquifier::getFullPath() {
  std::string fullPath;
  for (auto inst : pathUniq_) {
    fullPath += inst->getName().getString() + "/";
  }
  return fullPath;
}

}  // namespace naja::NAJA_OPT