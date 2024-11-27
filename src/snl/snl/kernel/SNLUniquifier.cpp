// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLUniquifier.h"

#include "SNLUniverse.h"
#include "SNLInstance.h"
#include "SNLPath.h"

namespace naja { namespace SNL {

SNLUniquifier::SNLUniquifier(const SNLPath& path, bool uniquifyTail):
  uniquifyTail_(uniquifyTail) {
  //Create the path_ by retriving the SNLID::DesignObjectID from the SNLPath
  SNLPath snlpath = path;
  while (!snlpath.empty()) {
    path_.push_back(snlpath.getHeadInstance()->getID());
    id_ += std::string("_") + std::to_string(snlpath.getHeadInstance()->getID());
    snlpath = snlpath.getTailPath();
  }
}

void SNLUniquifier::process() {
  std::vector<SNLInstance *> instancesToDelete;
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("SNLUniquifier::process() - dnlid %s\n", id_);
  printf("SNLUniquifier::process() - final inst %s\n",
    path_.back()->getName().getString().c_str());
  // LCOV_EXCL_STOP
#endif
  SNLDesign *currentDesign = SNLUniverse::get()->getTopDesign();
  for (size_t i = 0; i < path_.size(); i++) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("SNLUniquifier::process() - - looking now at inst %s\n",
      path_[i]->getName().getString().c_str());
    printf("SNLUniquifier::process() - - looking now at design %s\n",
      path_[i]->getDesign()->getName().getString().c_str());
    for (auto inst: currentDesign->getInstances()) {
      printf(" - child inst %s\n", inst->getName().getString().c_str());
    }
    // LCOV_EXCL_STOP
#endif
    SNLInstance *inst = currentDesign->getInstance(path_[i]);

    if ((i == path_.size() - 1) && !uniquifyTail_) {
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
    if (inst->getModel()->getSlaveInstances().size() == 1 || inst->getModel()->isPrimitive()) {
      // If the instance has only one slave, we can keep it
      // This only works when you work top down
      pathUniq_.push_back(inst);
    } else {
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

SNLInstance* SNLUniquifier::replaceWithClone(SNLInstance* inst) {
  SNLDesign *clone = inst->getModel()->clone(
  SNLName(std::string(inst->getModel()->getName().getString()) +
          std::string("_clone_") + id_));
  assert(clone->getSlaveInstances().size() == 0);
  inst->setModel(clone);
  assert(clone->getSlaveInstances().size() == 1);
  return inst;
}

// LCOV_EXCL_START
std::string SNLUniquifier::getFullPath() const {
  std::string fullPath;
  for (auto id: path_) {
    fullPath += std::to_string(id) + "/";
  }
  return fullPath;
}
// LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
