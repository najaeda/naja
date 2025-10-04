// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <stack>
#include <vector>

#include <tbb/task_arena.h>
#include "NLUniverse.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLPath.h"
#include "tbb/parallel_for.h"

#include "SNLBitNetOccurrence.h"
#include "SNLDesignModeling.h"
#include "SNLEquipotential.h"
#include "SNLNetComponentOccurrence.h"

using namespace naja::NL;
using namespace naja::DNL;

// #define DEBUG_PRINTS

namespace naja {

void OrderIDInitializer::process() {
  std::stack<SNLDesign*> designs;
  designs.push(NLUniverse::get()->getTopDesign());
  naja::NL::NLID::DesignObjectID bitTermId = 0;
  for (auto term : NLUniverse::get()->getTopDesign()->getBitTerms()) {
    term->setOrderID(bitTermId);
    bitTermId++;
  }
  while (!designs.empty()) {
    SNLDesign* design = designs.top();
    designs.pop();
    naja::NL::NLID::DesignObjectID id = 0;
    for (auto inst : design->getInstances()) {
      inst->setOrderID(id);
      id++;
      designs.push(inst->getModel());
      naja::NL::NLID::DesignObjectID TermId = 0;
      for (auto term : inst->getInstTerms()) {
        term->getBitTerm()->setOrderID(TermId);
        TermId++;
      }
    }
  }
}

}  // namespace naja

namespace naja::DNL {

DNL<DNLInstanceFull, DNLTerminalFull>* dnlFull_ = nullptr;
bool isCreated() {
  return dnlFull_ != nullptr;
}
DNL<DNLInstanceFull, DNLTerminalFull>* create() {
  assert(NLUniverse::get());
  dnlFull_ = new DNL<DNLInstanceFull, DNLTerminalFull>(
      NLUniverse::get()->getTopDesign());
  dnlFull_->process();
  return dnlFull_;
}

DNL<DNLInstanceFull, DNLTerminalFull>* get() {
  if (dnlFull_ == nullptr) {
    dnlFull_ = create();
  }
  return dnlFull_;
}

void destroy() {
  delete dnlFull_;
  dnlFull_ = nullptr;
}
}  // namespace naja::DNL

DNLInstanceFull::DNLInstanceFull(SNLInstance* instance, DNLID id, DNLID parent)
    : instance_(instance), id_(id), parent_(parent) {}

void DNLInstanceFull::display() const {
  if (isTop()) {
    printf("DNLInstance ID %zu %s\n", getID(), "Is Top.");
    return;
  }
  printf("DNLInstance ID %zu %s\n", getID(),
         getSNLInstance()->getString().c_str());
  for (DNLID term = getTermIndexes().first; term <= getTermIndexes().second;
       term++) {
    printf(
        "- DNLTerm %zu %d %s\n", term,
        (int)(*get()).getDNLTerminalFromID(term).getSnlTerm()->getDirection(),
        (*get()).getDNLTerminalFromID(term).getSnlTerm()->getString().c_str());
  }
}

const DNLInstanceFull& DNLInstanceFull::getParentInstance() const {
  return (*get()).getDNLInstanceFromID(parent_);
}

DNLID DNLInstanceFull::getID() const {
  return id_;
}

DNLID DNLInstanceFull::getParentID() const {
  return parent_;
}

const SNLDesign* DNLInstanceFull::getSNLModel() const {
  if (instance_) {
    return instance_->getModel();
  } else {
    return NLUniverse::get()->getTopDesign();
  }
}

SNLInstance* DNLInstanceFull::getSNLInstance() const {
  return instance_;
}
void DNLInstanceFull::setTermsIndexes(
    const std::pair<DNLID, DNLID>& termsIndexes) {
  // assert(termsIndexes.first < termsIndexes.second ||
  //(termsIndexes.first == DNLID_MAX && termsIndexes.second == DNLID_MAX));
  termsIndexes_ = termsIndexes;
}
void DNLInstanceFull::setChildrenIndexes(
    const std::pair<DNLID, DNLID>& childrenIndexes) {
  // assert(childrenIndexes.first < childrenIndexes.second ||
  //(childrenIndexes.first == DNLID_MAX && childrenIndexes.second ==
  // DNLID_MAX));
  childrenIndexes_ = childrenIndexes;
}
const DNLInstanceFull& DNLInstanceFull::getChildInstance(
    const SNLInstance* snlInst) const {
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf(
      "DNLInstanceFull::getChildInstance - DNLID of Instance %zu First child "
      "index: %zu Last child index: %zu Is Top: %d\n",
      id_, childrenIndexes_.first, childrenIndexes_.second, (int)isTop());
  // LCOV_EXCL_STOP
#endif
  if (getSNLModel() != snlInst->getDesign()) {
    return (*get()).getDNLNullInstance();
  }
  return (*get())
      .getDNLInstances()[childrenIndexes_.first + snlInst->getOrderID()];
  /*auto first = (*get()).getDNLInstances().begin();
  std::advance(first, childrenIndexes_.first);
  auto last = (*get()).getDNLInstances().begin();
  std::advance(last, childrenIndexes_.second + 1);// "+ 1" <- exclusive of this
  element naja::NL::NLID::DesignObjectID id = snlInst->getID(); auto result =
  std::lower_bound( first, last, id,
          [](const DNLInstanceFull& inst, naja::NL::NLID::DesignObjectID id) {
            return inst.getSNLInstance()->getID() < id;
          });
  if (result != last && (*result).getSNLInstance() == snlInst) {
    const DNLInstanceFull& childInstance = *result;
    return childInstance;
  }*/
}

const DNLTerminalFull& DNLInstanceFull::getTerminal(
    const SNLInstTerm* snlTerm) const {
  if (snlTerm == nullptr || snlTerm->getInstance() != instance_) {
    return (*get()).getDNLNullTerminal();
  }
  return (*get())
      .getDNLTerms()[termsIndexes_.first + snlTerm->getBitTerm()->getOrderID()];
  /*auto first = (*get()).getDNLTerms().begin();
  std::advance(first, termsIndexes_.first);
  auto last = (*get()).getDNLTerms().begin();
  std::advance(last, termsIndexes_.second + 1);
  auto bitTerm = snlTerm->getBitTerm();
  auto result = std::lower_bound(
          first,
          last, bitTerm,
          [](const DNLTerminalFull& term, const SNLBitTerm* bitTerm) {
            return SNLBitTermCompare()(
                term.getSnlBitTerm(), bitTerm);
          });
  if (result != last && (*result).getSnlTerm() == snlTerm) {
    const DNLTerminalFull& term = *result;
    return term;
  }*/
}

const DNLTerminalFull& DNLInstanceFull::getTerminalFromBitTerm(
    const SNLBitTerm* snlTerm) const {
  if (snlTerm == nullptr || snlTerm->getDesign() != getSNLModel()) {
    return (*get()).getDNLNullTerminal();
  }
  return (*get()).getDNLTerms()[termsIndexes_.first + snlTerm->getOrderID()];
  /*auto first = (*get()).getDNLTerms().begin();
  std::advance(first, termsIndexes_.first);
  auto last = (*get()).getDNLTerms().begin();
  std::advance(last, termsIndexes_.second + 1);
  auto result = std::lower_bound(
          first,
          last, snlTerm,
          [](const DNLTerminalFull& term, const SNLBitTerm* bitTerm) {
            return SNLBitTermCompare()(
                term.getSnlBitTerm(), bitTerm);
          });
  if (result != last && (*result).getSnlBitTerm() == snlTerm) {
    const DNLTerminalFull& term = *result;
    return term;
  }*/
}

std::string DNLInstanceFull::getFullPath() const {
  std::vector<SNLInstance*> path;
  DNLID instID = getID();
  DNLInstanceFull inst = *this;
  SNLInstance* snlInst = inst.getSNLInstance();
  if (snlInst != nullptr) {
    path.push_back(snlInst);
  }
  instID = inst.getParentID();
  while (instID != DNLID_MAX) {
    inst = inst.getParentInstance();
    snlInst = inst.getSNLInstance();
    if (snlInst != nullptr) {
      path.push_back(snlInst);
    }
    instID = inst.getParentID();
  }
  std::string fullPath;
  for (auto it = path.rbegin(); it != path.rend(); ++it) {
    fullPath += (*it)->getName().getString() + "/";
  }
  return fullPath;
}

SNLPath DNLInstanceFull::getPath() const {
  std::vector<std::string> path;
  naja::DNL::DNLInstanceFull currentInstance = *this;
  while (currentInstance.isTop() == false) {
    path.push_back(currentInstance.getSNLInstance()->getName().getString());
    currentInstance = currentInstance.getParentInstance();
  }
  std::reverse(path.begin(), path.end());
  SNLPath snlPath(NLUniverse::get()->getTopDesign(), path);
  return snlPath;
}

DNLTerminalFull::DNLTerminalFull(DNLID DNLInstID,
                                 SNLInstTerm* terminal,
                                 DNLID id)
    : DNLInstID_(DNLInstID),
      terminal_(terminal),
      bitTerminal_(terminal->getBitTerm()),
      id_(id) {}

DNLTerminalFull::DNLTerminalFull(DNLID DNLInstID,
                                 SNLBitTerm* terminal,
                                 DNLID id)
    : DNLInstID_(DNLInstID), bitTerminal_(terminal), id_(id) {}

DNLID DNLTerminalFull::getID() const {
  return id_;
}

bool DNLTerminalFull::isSequential() const {
  std::set<SNLBitTerm*> seq;
  DNLInstanceFull inst = (*get()).getDNLInstanceFromID(DNLInstID_);
  for (DNLID term = inst.getTermIndexes().first;
       term <= inst.getTermIndexes().second; term++) {
    const DNLTerminalFull& dnlTerm = (*get()).getDNLTerminalFromID(term);
    auto clockRelatedOutputs =
        SNLDesignModeling::getClockRelatedOutputs(dnlTerm.getSnlBitTerm());
    for (auto snlBitTerm : clockRelatedOutputs) {
      if (this->getSnlBitTerm() == snlBitTerm) {
        return true;
      }
    }
    auto clockRelatedInputs =
        SNLDesignModeling::getClockRelatedInputs(dnlTerm.getSnlBitTerm());
    for (auto snlBitTerm : clockRelatedInputs) {
      if (this->getSnlBitTerm() == snlBitTerm) {
        return true;
      }
    }
    if ((!clockRelatedOutputs.empty() ||
        !clockRelatedInputs.empty()) && term == this->getID()) {
      return true;
    }
  }
  return false;
}

std::vector<naja::NL::NLID::DesignObjectID> DNLTerminalFull::getFullPathIDs() const {
  auto path = this->getDNLInstance().getPath();
  std::vector<naja::NL::NLID::DesignObjectID> fullPathIDs = path.getPathIDs();
  fullPathIDs.push_back(this->getDNLInstance().getSNLInstance()->getID());
  fullPathIDs.push_back(this->getSnlBitTerm()->getID());
  fullPathIDs.push_back(this->getSnlBitTerm()->getBit());
  return fullPathIDs;
}

bool DNLTerminalFull::isCombinatorial() const {
  std::set<SNLBitTerm*> comb;
  DNLInstanceFull inst = (*get()).getDNLInstanceFromID(DNLInstID_);
  for (DNLID term = inst.getTermIndexes().first;
       term <= inst.getTermIndexes().second; term++) {
    const DNLTerminalFull& dnlTerm = (*get()).getDNLTerminalFromID(term);
    for (auto snlBitTerm :
         SNLDesignModeling::getCombinatorialOutputs(dnlTerm.getSnlBitTerm())) {
      if (this->getSnlBitTerm() == snlBitTerm) {
        return true;
      }
    }
    for (auto snlBitTerm :
         SNLDesignModeling::getCombinatorialInputs(dnlTerm.getSnlBitTerm())) {
      if (this->getSnlBitTerm() == snlBitTerm) {
        return true;
      }
    }
  }
  return false;
}

SNLBitTerm* DNLTerminalFull::getSnlBitTerm() const {
  return bitTerminal_ ? bitTerminal_ : terminal_->getBitTerm();
}

SNLInstTerm* DNLTerminalFull::getSnlTerm() const {
  return terminal_;
}

const DNLInstanceFull& DNLTerminalFull::getDNLInstance() const {
  return (*get()).getDNLInstanceFromID(DNLInstID_);
}

void DNLTerminalFull::setIsoID(DNLID isoID) {
  (*get()).setIsoIdforTermId(isoID, id_);
}

DNLID DNLTerminalFull::getIsoID() const {
  return (*get()).getIsoIdfromTermId(id_);
}

SNLNetComponentOccurrence DNLTerminalFull::getOccurrence() const {
  if (this->getDNLInstance().isTop()) {
    naja::NL::SNLNetComponentOccurrence occurrence(
        this->getDNLInstance().getPath(), this->getSnlBitTerm());
    return occurrence;
  }
  naja::NL::SNLInstTermOccurrence occurrence(
      this->getDNLInstance().getPath().getHeadPath(), this->getSnlTerm());
  return occurrence;
}

SNLEquipotential DNLTerminalFull::getEquipotential() const {
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("path: %s\n", this->getDNLInstance().getPath().getString().c_str());
// LCOV_EXCL_STOP
#endif
  if (this->getDNLInstance().isTop()) {
    naja::NL::SNLNetComponentOccurrence occurrence(
        this->getDNLInstance().getPath(), this->getSnlBitTerm());
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("occurrence: %s\n", occurrence.getString().c_str());
    SNLEquipotential equipotential(occurrence);
    printf("equipotential: %s\n", equipotential.getString().c_str());
// LCOV_EXCL_STOP
#endif
    return SNLEquipotential(occurrence);
  }
  naja::NL::SNLInstTermOccurrence occurrence(
      this->getDNLInstance().getPath().getHeadPath(), this->getSnlTerm());
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("occurrence: %s\n", occurrence.getString().c_str());
  SNLEquipotential equipotential(occurrence);
  printf("equipotential: %s\n", equipotential.getString().c_str());
// LCOV_EXCL_STOP
#endif
  return SNLEquipotential(occurrence);
}

DNLIso::DNLIso(DNLID id) : id_(id) {}

void DNLIso::addDriver(DNLID driver) {
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf(" - DNLIso::addDriver(DNLID driver) %zu %s %s %s\n", driver,
         (*get())
             .getDNLTerminalFromID(driver)
             .getSnlBitTerm()
             ->getString()
             .c_str(),
         (*get())
             .getDNLTerminalFromID(driver)
             .getSnlBitTerm()
             ->getDirection()
             .getString()
             .c_str(),
         (*get())
             .getDNLTerminalFromID(driver)
             .getSnlBitTerm()
             ->getDesign()
             ->getName()
             .getString()
             .c_str());
  // LCOV_EXCL_STOP
#endif
  drivers_.push_back(driver);
}

void DNLIso::addReader(DNLID reader) {
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf(" - DNLIso::addReader(DNLID driver) %zu %s %s %s\n", reader,
         (*get())
             .getDNLTerminalFromID(reader)
             .getSnlBitTerm()
             ->getString()
             .c_str(),
         (*get())
             .getDNLTerminalFromID(reader)
             .getSnlBitTerm()
             ->getDirection()
             .getString()
             .c_str(),
         (*get())
             .getDNLTerminalFromID(reader)
             .getSnlBitTerm()
             ->getDesign()
             ->getName()
             .getString()
             .c_str());
  // LCOV_EXCL_STOP
#endif
  readers_.push_back(reader);
}
void DNLIso::display(std::ostream& stream) const {
  for (auto& driver : drivers_) {
    if ((*get()).getDNLTerminalFromID(driver).isTopPort()) {
      stream << "driver top port "
             << (*get())
                    .getDNLTerminalFromID(driver)
                    .getSnlBitTerm()
                    ->getName()
                    .getString()
             << std::endl;
      continue;
    }
    stream
        << "driver instance "
        << (*get())
               .getDNLTerminalFromID(driver)
               .getSnlTerm()
               ->getInstance()
               ->getName()
               .getString()
        << std::endl
        << (*get()).getDNLTerminalFromID(driver).getDNLInstance().getFullPath()
        << std::endl;
    ;
    stream << "driver "
           << (*get()).getDNLTerminalFromID(driver).getSnlTerm()->getString()
           << std::endl;
    stream
        << "driver "
        << (*get()).getDNLTerminalFromID(driver).getSnlTerm()->getDescription()
        << std::endl;
  }
  for (auto& reader : readers_) {
    if ((*get()).getDNLTerminalFromID(reader).isTopPort()) {
      stream << "reader top port "
             << (*get())
                    .getDNLTerminalFromID(reader)
                    .getSnlBitTerm()
                    ->getName()
                    .getString()
             << std::endl;
      continue;
    }
    stream
        << "reader instance"
        << (*get()).getDNLTerminalFromID(reader).getDNLInstance().getFullPath()
        << std::endl;
    ;
    stream << "reader"
           << (*get()).getDNLTerminalFromID(reader).getSnlTerm()->getString()
           << std::endl;
    ;
  }
}

DNLIsoDB::DNLIsoDB() {}

DNLIso& DNLIsoDB::addIso() {
  isos_.push_back(DNLIso(isos_.size()));
  return isos_.back();
}

void DNLIsoDB::display() const {
  printf("----------ISODB - BEGIN----------\n");
  for (const DNLIso& iso : isos_) {
    printf("----------new iso----------\n");
    iso.display();
  }
  printf("----------ISODB - END----------\n");
}
