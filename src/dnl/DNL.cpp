// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stack>
#include <vector>
#include <iterator>

#include <tbb/task_arena.h>
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLUniverse.h"
#include "tbb/parallel_for.h"

using namespace naja::SNL;
using namespace naja::DNL;

// #define DEBUG_PRINTS

namespace naja::DNL {

DNL<DNLInstanceFull, DNLTerminalFull>* dnlFull_ = nullptr;
bool isCreated() {
  return dnlFull_ != nullptr;
}
DNL<DNLInstanceFull, DNLTerminalFull>* create() {
  assert(SNLUniverse::get());
  dnlFull_ = new DNL<DNLInstanceFull, DNLTerminalFull>(
      SNLUniverse::get()->getTopDesign());
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

DNLInstanceFull::DNLInstanceFull(SNLInstance* instance,
                                 DNLID id,
                                 DNLID parent)
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
};
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
    return SNLUniverse::get()->getTopDesign();
  }
}

SNLInstance* DNLInstanceFull::getSNLInstance() const {
  return instance_;
}
void DNLInstanceFull::setTermsIndexes(
    const std::pair<DNLID, DNLID>& termsIndexes) {
      //assert(termsIndexes.first < termsIndexes.second || 
      //(termsIndexes.first == DNLID_MAX && termsIndexes.second == DNLID_MAX));
  termsIndexes_ = termsIndexes;
}
void DNLInstanceFull::setChildrenIndexes(
    const std::pair<DNLID, DNLID>& childrenIndexes) {
      //assert(childrenIndexes.first < childrenIndexes.second || 
      //(childrenIndexes.first == DNLID_MAX && childrenIndexes.second == DNLID_MAX));
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
  // Find the child instance with the same SNLInstance by levrage the fact that
  // the children are sorted by SNLInstance id(getID()) using the binary search
  // with costum operator
  auto first = (*get()).getDNLInstances().begin();
  std::advance(first, childrenIndexes_.first);
  auto last = (*get()).getDNLInstances().begin();
  std::advance(last, childrenIndexes_.second + 1/*exclusive of this element*/);
  naja::SNL::SNLID::DesignObjectID id = snlInst->getID();
  auto result = std::lower_bound(
          first,
          last, id,
          [](const DNLInstanceFull& inst, naja::SNL::SNLID::DesignObjectID id) {
            return inst.getSNLInstance()->getID() < id;
          });
  if (result != last && (*result).getSNLInstance() == snlInst) {
    const DNLInstanceFull& childInstance = *result;
    return childInstance;
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstanceFull::getChildInstance - Return null instance\n");
  // LCOV_EXCL_STOP
#endif
  return (*get()).getDNLNullInstance();
}

const DNLTerminalFull& DNLInstanceFull::getTerminal(
    const SNLInstTerm* snlTerm) const {
  auto first = (*get()).getDNLTerms().begin();
  std::advance(first, termsIndexes_.first);
  auto last = (*get()).getDNLTerms().begin();
  std::advance(last, termsIndexes_.second + 1/*exclusive of this element*/);
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
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstanceFull::getTerminal - Return null terminal\n");
  // LCOV_EXCL_STOP
#endif
  return (*get()).getDNLNullTerminal();
}

const DNLTerminalFull& DNLInstanceFull::getTerminalFromBitTerm(
    const SNLBitTerm* snlTerm) const {
  /*for (DNLID term = termsIndexes_.first; term <= termsIndexes_.second; term++) {
    if ((*get()).getDNLTerminalFromID(term).getSnlBitTerm() == snlTerm) {
      return (*get()).getDNLTerminalFromID(term);
    }
  }*/
  auto first = (*get()).getDNLTerms().begin();
  std::advance(first, termsIndexes_.first);
  auto last = (*get()).getDNLTerms().begin();
  std::advance(last, termsIndexes_.second + 1/*exclusive of this element*/);
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
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstanceFull::getTerminal - Return null terminal\n");
  // LCOV_EXCL_STOP
#endif
  return (*get()).getDNLNullTerminal();
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
  };

DNLTerminalFull::DNLTerminalFull(DNLID DNLInstID,
                                 SNLInstTerm* terminal,
                                 DNLID id)
    : DNLInstID_(DNLInstID),
      terminal_(terminal),
      bitTerminal_(terminal->getBitTerm()),
      id_(id){};

DNLTerminalFull::DNLTerminalFull(DNLID DNLInstID,
                                 SNLBitTerm* terminal,
                                 DNLID id)
    : DNLInstID_(DNLInstID), bitTerminal_(terminal), id_(id){};
DNLID DNLTerminalFull::getID() const {
  return id_;
}

SNLBitTerm* DNLTerminalFull::getSnlBitTerm() const {
  return bitTerminal_ ? bitTerminal_ : terminal_->getBitTerm();
};
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

DNLIso::DNLIso(DNLID id) : id_(id){};

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
                    .getString() << std::endl;
      continue;
    }
    stream << "driver instance "
           << (*get())
                  .getDNLTerminalFromID(driver)
                  .getSnlTerm()
                  ->getInstance()
                  ->getName()
                  .getString()
           << std::endl
           << (*get())
                  .getDNLTerminalFromID(driver)
                  .getDNLInstance().getFullPath()
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
                    .getString() << std::endl;
      continue;
    }
    stream << "reader instance"
           << (*get())
                  .getDNLTerminalFromID(reader)
                  .getDNLInstance().getFullPath()
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