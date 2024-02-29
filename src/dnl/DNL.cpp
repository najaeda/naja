// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stack>
#include <vector>

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
DNL<DNLInstanceFull, DNLTerminalFull>* create() {
  assert(SNLUniverse::get());
  dnlFull_ = new DNL<DNLInstanceFull, DNLTerminalFull>(SNLUniverse::get()->getTopDesign());
  dnlFull_->process();
  return dnlFull_;
}

 DNL<DNLInstanceFull, DNLTerminalFull>* get() {
  if (dnlFull_ == nullptr) {
    dnlFull_ =  create();
  }
  return dnlFull_;
}

void destroy() {
  delete dnlFull_;
  dnlFull_ = nullptr;
}
}

DNLInstanceFull::DNLInstanceFull(const SNLInstance* instance, DNLID id, DNLID parent)
    : instance_(instance), id_(id), parent_(parent) {}

void DNLInstanceFull::display() const {
  if (isTop()) {
    printf("DNLInstance ID %zu %s\n", getID(), "Is Top.");
    return;
  }
  printf("DNLInstance ID %zu %s\n", getID(),
         getSNLInstance()->getString().c_str());
  for (DNLID term = getTermIndexes().first; term < getTermIndexes().second;
       term++) {
    printf("- DNLTerm %zu %d %s\n", term,
           (int)(*get())
               .getDNLTerminalFromID(term)
               .getSnlTerm()
               ->getDirection(),
           (*get())
               .getDNLTerminalFromID(term)
               .getSnlTerm()
               ->getString()
               .c_str());
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

const SNLInstance* DNLInstanceFull::getSNLInstance() const {
  return instance_;
}
void DNLInstanceFull::setTermsIndexes(const std::pair<DNLID, DNLID>& termsIndexes) {
  termsIndexes_ = termsIndexes;
}
void DNLInstanceFull::setChildrenIndexes(
    const std::pair<DNLID, DNLID>& childrenIndexes) {
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
  for (DNLID child = childrenIndexes_.first + 1;
       child <= childrenIndexes_.second; child++) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "DNLInstanceFull::getChildInstance - Searched child SNL Instance: %p Found "
        "child SNL instance %p\n",
        snlInst, (*get()).getDNLInstanceFromID(child).getSNLInstance());
    // LCOV_EXCL_STOP
#endif
    if ((*get()).getDNLInstanceFromID(child).getSNLInstance() == snlInst) {
      return (*get()).getDNLInstanceFromID(child);
    }
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstanceFull::getChildInstance - Return null instance\n");
  // LCOV_EXCL_STOP
#endif
  return (*get()).getDNLNullInstance();
}
const DNLTerminalFull& DNLInstanceFull::getTerminal(const SNLInstTerm* snlTerm) const {
  for (DNLID term = termsIndexes_.first; term < termsIndexes_.second; term++) {
    if ((*get()).getDNLTerminalFromID(term).getSnlTerm() == snlTerm) {
      return (*get()).getDNLTerminalFromID(term);
    }
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
  for (DNLID term = termsIndexes_.first; term < termsIndexes_.second; term++) {
    if ((*get()).getDNLTerminalFromID(term).getSnlBitTerm() == snlTerm) {
      return (*get()).getDNLTerminalFromID(term);
    }
  }
  assert(false);
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstanceFull::getTerminalFromBitTerm - Return null terminal\n");
  // LCOV_EXCL_STOP
#endif
  return (*get()).getDNLNullTerminal();
}

DNLTerminalFull::DNLTerminalFull(DNLID id) : id_(id){};
DNLTerminalFull::DNLTerminalFull(DNLID DNLInstID, SNLInstTerm* terminal, DNLID id)
    : _DNLInstID(DNLInstID), _terminal(terminal), id_(id){};

DNLTerminalFull::DNLTerminalFull(DNLID DNLInstID, SNLBitTerm* terminal, DNLID id)
    : _DNLInstID(DNLInstID), _bitTerminal(terminal), id_(id){};
DNLID DNLTerminalFull::getID() const {
  return id_;
}

SNLBitTerm* DNLTerminalFull::getSnlBitTerm() const {
  return _bitTerminal ? _bitTerminal : _terminal->getTerm();
};
SNLInstTerm* DNLTerminalFull::getSnlTerm() const {
  return _terminal;
}
const DNLInstanceFull& DNLTerminalFull::getDNLInstance() const {
  return (*get()).getDNLInstanceFromID(_DNLInstID);
}

void DNLTerminalFull::setIsoID(DNLID isoID) {
  (*get()).setIsoIdforTermId(isoID, id_);
}
DNLID DNLTerminalFull::getIsoID() const {
  return (*get()).getIsoIdfromTermId(id_);
}