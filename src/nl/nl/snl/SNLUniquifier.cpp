// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLUniquifier.h"

#include "NajaLog.h"

#include "NLUniverse.h"

#include "SNLInstance.h"
#include "SNLPath.h"

namespace naja { namespace NL {

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
  NAJA_LOG_TRACE("SNLUniquifier::process() - dnlid {}", id_);
  NAJA_LOG_TRACE("SNLUniquifier::process() - final inst {}",
    path_.back()->getName().getString().c_str());
  SNLDesign *currentDesign = NLUniverse::get()->getTopDesign();
  for (size_t i = 0; i < path_.size(); i++) {
    NAJA_LOG_TRACE("SNLUniquifier::process() - - looking now at inst {}",
        path_[i]->getName().getString().c_str());
    NAJA_LOG_TRACE("SNLUniquifier::process() - - looking now at design {}",
        path_[i]->getDesign()->getName().getString().c_str());
    for (auto inst: currentDesign->getInstances()) {
      NAJA_LOG_TRACE(" - child inst {}", inst->getName().getString().c_str());
    }
    SNLInstance *inst = currentDesign->getInstance(path_[i]);
    inst->getModel()->recursiveRevisionIncrement();
    if ((i == path_.size() - 1) && !uniquifyTail_) {
      // If we are at the last instance, we can keep it
      pathUniq_.push_back(inst);
      break;
    }
    //assert(!inst->getModel()->getInstances().empty());
    NAJA_LOG_TRACE("{}({}) ", path_[i], inst->getModel()->getSlaveInstances().size());
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
}

SNLInstance* SNLUniquifier::replaceWithClone(SNLInstance* inst) {
  SNLDesign *clone = inst->getModel()->clone(
  NLName(std::string(inst->getModel()->getName().getString()) +
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

}} // namespace NL // namespace naja
