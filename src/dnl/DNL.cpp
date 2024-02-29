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
DNL* dnl_ = nullptr;
}

DNLInstance::DNLInstance(const SNLInstance* instance, DNLID id, DNLID parent)
    : instance_(instance), id_(id), parent_(parent) {}

void DNLInstance::display() const {
  if (isTop()) {
    printf("DNLInstance ID %zu %s\n", getID(), "Is Top.");
    return;
  }
  printf("DNLInstance ID %zu %s\n", getID(),
         getSNLInstance()->getString().c_str());
  for (DNLID term = getTermIndexes().first; term < getTermIndexes().second;
       term++) {
    printf("- DNLTerm %zu %d %s\n", term,
           (int)(*DNL::get())
               .getDNLTerminalFromID(term)
               .getSnlTerm()
               ->getDirection(),
           (*DNL::get())
               .getDNLTerminalFromID(term)
               .getSnlTerm()
               ->getString()
               .c_str());
  }
}

const DNLInstance& DNLInstance::getParentInstance() const {
  return (*DNL::get()).getDNLInstanceFromID(parent_);
};
DNLID DNLInstance::getID() const {
  return id_;
}
DNLID DNLInstance::getParentID() const {
  return parent_;
}

const SNLDesign* DNLInstance::getSNLModel() const {
  if (instance_) {
    return instance_->getModel();
  } else {
    return SNLUniverse::get()->getTopDesign();
  }
}

const SNLInstance* DNLInstance::getSNLInstance() const {
  return instance_;
}
void DNLInstance::setTermsIndexes(const std::pair<DNLID, DNLID>& termsIndexes) {
  termsIndexes_ = termsIndexes;
}
void DNLInstance::setChildrenIndexes(
    const std::pair<DNLID, DNLID>& childrenIndexes) {
  childrenIndexes_ = childrenIndexes;
}
const DNLInstance& DNLInstance::getChildInstance(
    const SNLInstance* snlInst) const {
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf(
      "DNLInstance::getChildInstance - DNLID of Instance %zu First child "
      "index: %zu Last child index: %zu Is Top: %d\n",
      id_, childrenIndexes_.first, childrenIndexes_.second, (int)isTop());
  // LCOV_EXCL_STOP
#endif
  for (DNLID child = childrenIndexes_.first + 1;
       child <= childrenIndexes_.second; child++) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "DNLInstance::getChildInstance - Searched child SNL Instance: %p Found "
        "child SNL instance %p\n",
        snlInst, (*DNL::get()).getDNLInstanceFromID(child).getSNLInstance());
    // LCOV_EXCL_STOP
#endif
    if ((*DNL::get()).getDNLInstanceFromID(child).getSNLInstance() == snlInst) {
      return (*DNL::get()).getDNLInstanceFromID(child);
    }
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstance::getChildInstance - Return null instance\n");
  // LCOV_EXCL_STOP
#endif
  return (*DNL::get()).getDNLNullInstance();
}
const DNLTerminal& DNLInstance::getTerminal(const SNLInstTerm* snlTerm) const {
  for (DNLID term = termsIndexes_.first; term < termsIndexes_.second; term++) {
    if ((*DNL::get()).getDNLTerminalFromID(term).getSnlTerm() == snlTerm) {
      return (*DNL::get()).getDNLTerminalFromID(term);
    }
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstance::getTerminal - Return null terminal\n");
  // LCOV_EXCL_STOP
#endif
  return (*DNL::get()).getDNLNullTerminal();
}
const DNLTerminal& DNLInstance::getTerminalFromBitTerm(
    const SNLBitTerm* snlTerm) const {
  for (DNLID term = termsIndexes_.first; term < termsIndexes_.second; term++) {
    if ((*DNL::get()).getDNLTerminalFromID(term).getSnlBitTerm() == snlTerm) {
      return (*DNL::get()).getDNLTerminalFromID(term);
    }
  }
  assert(false);
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNLInstance::getTerminalFromBitTerm - Return null terminal\n");
  // LCOV_EXCL_STOP
#endif
  return (*DNL::get()).getDNLNullTerminal();
}

DNLTerminal::DNLTerminal(DNLID id) : id_(id){};
DNLTerminal::DNLTerminal(DNLID DNLInstID, SNLInstTerm* terminal, DNLID id)
    : _DNLInstID(DNLInstID), _terminal(terminal), id_(id){};

DNLTerminal::DNLTerminal(DNLID DNLInstID, SNLBitTerm* terminal, DNLID id)
    : _DNLInstID(DNLInstID), _bitTerminal(terminal), id_(id){};
DNLID DNLTerminal::getID() const {
  return id_;
}

SNLBitTerm* DNLTerminal::getSnlBitTerm() const {
  return _bitTerminal ? _bitTerminal : _terminal->getTerm();
};
SNLInstTerm* DNLTerminal::getSnlTerm() const {
  return _terminal;
}
const DNLInstance& DNLTerminal::getDNLInstance() const {
  return (*DNL::get()).getDNLInstanceFromID(_DNLInstID);
}

void DNLTerminal::setIsoID(DNLID isoID) {
  (*DNL::get()).setIsoIdforTermId(isoID, id_);
}
DNLID DNLTerminal::getIsoID() const {
  return (*DNL::get()).getIsoIdfromTermId(id_);
}

DNLIso::DNLIso(DNLID id) : id_(id){};

void DNLIso::addDriver(DNLID driver) {
  if ((*DNL::get()).getDNLTerminalFromID(driver).isTopPort() ||
      (*DNL::get())
          .getDNLTerminalFromID(driver)
          .getDNLInstance()
          .getSNLInstance()
          ->getModel()
          ->getInstances()
          .empty()) {
    drivers_.push_back(driver);
  } else {
    addHierTerm(driver);
  }
}

void DNLIso::addReader(DNLID reader) {
  if ((*DNL::get()).getDNLTerminalFromID(reader).isTopPort() ||
      (*DNL::get())
          .getDNLTerminalFromID(reader)
          .getDNLInstance()
          .getSNLInstance()
          ->getModel()
          ->getInstances()
          .empty()) {
    readers_.push_back(reader);
  } else {
    addHierTerm(reader);
  }
}
void DNLIso::display(std::ostream& stream) const {
  for (auto& driver : drivers_) {
    stream << "driver instance"
           << (*DNL::get())
                  .getDNLTerminalFromID(driver)
                  .getSnlTerm()
                  ->getInstance()
                  ->getName()
                  .getString()
           << std::endl
           << (*DNL::get())
                  .getDNLTerminalFromID(driver)
                  .getSnlTerm()
                  ->getInstance()
                  ->getDescription()
           << std::endl;
    ;
    stream
        << "driver "
        << (*DNL::get()).getDNLTerminalFromID(driver).getSnlTerm()->getString()
        << std::endl;
    stream << "driver "
           << (*DNL::get())
                  .getDNLTerminalFromID(driver)
                  .getSnlTerm()
                  ->getDescription()
           << std::endl;
  }
  for (auto& reader : readers_) {
    stream << "reader instance"
           << (*DNL::get())
                  .getDNLTerminalFromID(reader)
                  .getSnlTerm()
                  ->getInstance()
                  ->getName()
                  .getString()
           << std::endl;
    ;
    stream
        << "reader"
        << (*DNL::get()).getDNLTerminalFromID(reader).getSnlTerm()->getString()
        << std::endl;
    ;
  }
}

DNLIsoDB::DNLIsoDB() {}

DNLIso& DNLIsoDB::addIso() {
  isos_.push_back(DNLIso(isos_.size()));
  return isos_.back();
}

DNLIsoDBBuilder::DNLIsoDBBuilder(DNLIsoDB& db) : db_(db) {
  visited_.resize((*DNL::get()).getDNLTerms().size(), false);
}

void DNLIsoDBBuilder::process() {
  // iterate on all leaf drivers
  std::vector<DNLID> tasks;
  for (DNLID leaf : (*DNL::get()).getLeaves()) {
    for (DNLID term =
             (*DNL::get()).getDNLInstanceFromID(leaf).getTermIndexes().first;
         term <
         (*DNL::get()).getDNLInstanceFromID(leaf).getTermIndexes().second;
         term++) {
      if ((*DNL::get())
                  .getDNLTerminalFromID(term)
                  .getSnlTerm()
                  ->getDirection() ==
              SNLTerm::Direction::DirectionEnum::Output &&
          (*DNL::get()).getDNLTerminalFromID(term).getSnlTerm()->getNet()) {
        DNLIso& DNLIso = addIsoToDB();

        DNLIso.addDriver(term);
        tasks.push_back(term);
        (*DNL::get()).getDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
      }
    }
  }
  for (DNLID term = (*DNL::get()).getTop().getTermIndexes().first;
       term < (*DNL::get()).getTop().getTermIndexes().second; term++) {
    if ((*DNL::get())
                .getDNLTerminalFromID(term)
                .getSnlBitTerm()
                ->getDirection() == SNLTerm::Direction::DirectionEnum::Input &&
        (*DNL::get()).getDNLTerminalFromID(term).getSnlBitTerm()->getNet()) {
      DNLIso& DNLIso = addIsoToDB();
      visited_[term] = true;
      DNLIso.addDriver(term);
      tasks.push_back(term);
      (*DNL::get()).getDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
    }
  }
  if (!getenv("NON_MT")) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("MT\n");
    // LCOV_EXCL_STOP
#endif
    tbb::task_arena arena(tbb::task_arena::automatic);
    tbb::parallel_for(
        tbb::blocked_range<DNLID>(0, tasks.size()),
        [&](const tbb::blocked_range<DNLID>& r) {
          for (DNLID i = r.begin(); i < r.end(); ++i) {
            treatDriver(
                (*DNL::get()).getDNLTerminalFromID(tasks[i]),
                db_.getIsoFromIsoID(
                    (*DNL::get()).getDNLTerminalFromID(tasks[i]).getIsoID()));
          }
        });
  } else {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Non MT\n");
    // LCOV_EXCL_STOP
#endif
    for (auto task : tasks) {
      treatDriver((*DNL::get()).getDNLTerminalFromID(task),
                  db_.getIsoFromIsoID(
                      (*DNL::get()).getDNLTerminalFromID(task).getIsoID()));
    }
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("num fi %zu\n", (*DNL::get()).getDNLInstances().size());
  printf("num ft %zu\n", (*DNL::get()).getDNLTerms().size());
  printf("num leaves %zu\n", (*DNL::get()).getLeaves().size());
  printf("num isos %zu\n", db_.getNumIsos());
  // LCOV_EXCL_STOP
#endif
}

void DNLIsoDB::display() const {
  printf("----------ISODB - BEGIN----------\n");
  for (const DNLIso& iso : isos_) {
    printf("----------new iso----------\n");
    iso.display();
  }
  printf("----------ISODB - END----------\n");
}

void DNLIsoDBBuilder::treatDriver(const DNLTerminal& term, DNLIso& DNLIso) {
  std::stack<DNLID> stack;
  if (term.getDNLInstance().isTop()) {
    assert(term.getSnlBitTerm()->getDirection() ==
           SNLTerm::Direction::DirectionEnum::Input);
    assert(term.getSnlBitTerm()->getNet());
    for (SNLInstTerm* reader : term.getSnlBitTerm()->getNet()->getInstTerms()) {
      DNLID freader = term.getDNLInstance()
                          .getChildInstance(reader->getInstance())
                          .getTerminal(reader)
                          .getID();

      if (freader == term.getID())
        continue;
      (*DNL::get()).getDNLTerminalFromID(freader).setIsoID(DNLIso.getIsoID());
      if ((*DNL::get())
              .getDNLTerminalFromID(freader)
              .getDNLInstance()
              .getSNLInstance()
              ->getModel()
              ->getInstances()
              .empty()) {
        DNLIso.addReader(freader);
        continue;
      }
      stack.push(freader);
    }
  } else {
    // do DFS on the driver
    stack.push(term.getID());
  }

  // Start traversing from driver
  while (!stack.empty()) {
    DNLID id = stack.top();
    stack.pop();
    const DNLTerminal& fterm = (*DNL::get()).getDNLTerminalFromID(id);
    assert(fterm.getID() == id);
    if (visited_[id]) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Visited continue\n");
      // LCOV_EXCL_STOP
#endif
      continue;
    }
    if (fterm.getDNLInstance().isTop() &&
        fterm.getSnlBitTerm()->getDirection() ==
            SNLTerm::Direction::DirectionEnum::Output) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Is top output so adding as reader\n");
      // LCOV_EXCL_STOP
#endif
      DNLIso.addReader(fterm.getID());
      (*DNL::get())
          .getDNLTerminalFromID(fterm.getID())
          .setIsoID(DNLIso.getIsoID());
      continue;
    }
    assert(fterm.getDNLInstance().isTop() == false);
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("---------------------------------\n");
    printf("DNLIsoDBBuilder::treatDriver - Visiting '%zu %p %s %s\n", id, fterm.getSnlTerm()->getInstance(),
           fterm.getSnlTerm()->getString().c_str(),
           fterm.getSnlTerm()->getDirection().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    visited_[id] = true;
    SNLBitNet* snlNet = fterm.getSnlTerm()->getNet();
    DNLIso.addNet(snlNet);
    const DNLInstance* DNLParent = &fterm.getDNLInstance().getParentInstance();
    // Get snl bit net connected to the snl term object of the DNL terminal
    bool goDown = false;
    if (!fterm.getSnlTerm()
             ->getInstance()
             ->getModel()
             ->getInstances()
             .empty()) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Hier inst\n");
      // LCOV_EXCL_STOP
#endif
    }
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("DNLIsoDBBuilder::treatDriver - term direction %d\n", fterm.getSnlTerm()->getDirection());
    printf("DNLIsoDBBuilder::treatDriver - term direction %s\n",
           fterm.getSnlTerm()->getDirection().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    if (!fterm.getSnlTerm()
             ->getInstance()
             ->getModel()
             ->getInstances()
             .empty() &&
        fterm.getSnlTerm()->getDirection() ==
            SNLTerm::Direction::DirectionEnum::Input) {
      // The current explored terminal is hierarchical input, need to go into
      // the instance(get bit term -> get net)
      snlNet = fterm.getSnlTerm()->getTerm()->getNet();
      DNLParent = &fterm.getDNLInstance();
      goDown = true;
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Going down\n");
      // LCOV_EXCL_STOP
#endif
    } else {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Same level\n");
      // LCOV_EXCL_STOP
#endif
    }
    if (snlNet == nullptr)
      continue;
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("DNLIsoDBBuilder::treatDriver - Visiting net '%p %s\n", snlNet, snlNet->getString().c_str());
    // LCOV_EXCL_STOP
#endif
    for (SNLInstTerm* instTerm : snlNet->getInstTerms()) {
      SNLInstance* termSnlInst = instTerm->getInstance();
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Inst %p Term %p\n", termSnlInst, instTerm);
      // LCOV_EXCL_STOP
#endif
      const DNLTerminal& ftermNew = goDown ? fterm.getDNLInstance()
                                                 .getChildInstance(termSnlInst)
                                                 .getTerminal(instTerm)
                                           : fterm.getDNLInstance()
                                                 .getParentInstance()
                                                 .getChildInstance(termSnlInst)
                                                 .getTerminal(instTerm);
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Visiting snl it %zu %p %s %s\n", ftermNew.getID(), termSnlInst,
             instTerm->getString().c_str(),
             instTerm->getDirection().getString().c_str());
      printf("DNLIsoDBBuilder::treatDriver - Visiting DNL it %zu %p %s %s\n", ftermNew.getID(),
             ftermNew.getSnlTerm(), ftermNew.getSnlTerm()->getString().c_str(),
             ftermNew.getSnlTerm()->getDirection().getString().c_str());
      // LCOV_EXCL_STOP
#endif
      if (termSnlInst->getModel()->getInstances().empty()) {
        if (ftermNew.getSnlTerm()->getDirection() ==
            SNLTerm::Direction::DirectionEnum::Output) {
          // if (ftermNew.getID() == term.getID()) continue;
          if (term.getID() != ftermNew.getID()) {
            assert(false);
            DNLIso.addDriver(ftermNew.getID());
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("DNLIsoDBBuilder::treatDriver -- Add driver\n\n");
            // LCOV_EXCL_STOP
#endif
          } else {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("DNLIsoDBBuilder::treatDriver --- Original driver -> skipping\n\n");
            // LCOV_EXCL_STOP
#endif
          }
          // assert(0);
        } else {
          DNLIso.addReader(ftermNew.getID());
          (*DNL::get())
              .getDNLTerminalFromID(ftermNew.getID())
              .setIsoID(DNLIso.getIsoID());
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf("DNLIsoDBBuilder::treatDriver -- Add reader\n\n");
          // LCOV_EXCL_STOP
#endif
        }
      } else {
        stack.push(ftermNew.getID());
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("DNLIsoDBBuilder::treatDriver --- Pushing to stuck %s %s\n",
               ftermNew.getSnlTerm()->getString().c_str(),
               ftermNew.getSnlTerm()->getDirection().getString().c_str());
        // LCOV_EXCL_STOP
#endif
      }
    }
    for (SNLBitTerm* bitTerm : snlNet->getBitTerms()) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Visiting bt %p %s %s\n", bitTerm, bitTerm->getString().c_str(),
             bitTerm->getDirection().getString().c_str());
      // LCOV_EXCL_STOP
#endif
    }
    for (SNLBitTerm* bitTerm : snlNet->getBitTerms()) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNLIsoDBBuilder::treatDriver - Visiting bt %p %s %s\n", bitTerm, bitTerm->getString().c_str(),
             bitTerm->getDirection().getString().c_str());
      DNLParent->display();
      if (!DNLParent->isTop()) {
        DNLParent->getParentInstance().display();
      }

      printf("DNLIsoDBBuilder::treatDriver - Visiting bt %zu %p %s %s\n",
             DNLParent->getTerminalFromBitTerm(bitTerm).getID(), bitTerm,
             bitTerm->getString().c_str(),
             bitTerm->getDirection().getString().c_str());
      // LCOV_EXCL_STOP
#endif
      const DNLTerminal& ftermNew = DNLParent->getTerminalFromBitTerm(bitTerm);
      if (visited_[ftermNew.getID()]) {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("DNLIsoDBBuilder::treatDriver - Visited continue\n");
        // LCOV_EXCL_STOP
#endif
        continue;
      }
      stack.push(ftermNew.getID());
    }
  }
}

DNL* DNL::create() {
  assert(SNLUniverse::get());
  dnl_ = new DNL(SNLUniverse::get()->getTopDesign());
  dnl_->process();
  return dnl_;
}

DNL* DNL::get() {
  if (dnl_ == nullptr) {
    create();
  }
  return dnl_;
}

void DNL::destroy() {
  delete dnl_;
  dnl_ = nullptr;
}

DNL::DNL(const SNLDesign* top) : top_(top) {}

void DNL::display() const {
  printf("---------FV--------\n");
  for (const DNLInstance& inst : DNLInstances_) {
    if (inst.getSNLInstance() == nullptr)
      continue;  // top
    printf("fi %zu %s\n", inst.getID(),
           inst.getSNLInstance()->getString().c_str());
    for (DNLID term = inst.getTermIndexes().first;
         term < inst.getTermIndexes().second; term++) {
      printf("- ft %zu %d %s\n", term,
             (int)getDNLTerminalFromID(term).getSnlTerm()->getDirection(),
             getDNLTerminalFromID(term).getSnlTerm()->getString().c_str());
    }
  }
  fidb_.display();
}

void DNL::process() {
  std::vector<DNLID> stack;
  DNLInstances_.push_back(
      DNLInstance(nullptr, DNLInstances_.size(), DNLID_MAX));
  assert(DNLInstances_.back().getID() == DNLInstances_.size() - 1);
  DNLID parentId = DNLInstances_.back().getID();
  std::pair<DNLID, DNLID> childrenIndexes;
  childrenIndexes.first = DNLInstances_.back().getID();
  std::pair<DNLID, DNLID> termIndexes;
  termIndexes.first = DNLTerms_.size();
  for (SNLBitTerm* bitterm : top_->getBitTerms()) {
    DNLTerms_.push_back(DNLTerminal(parentId, bitterm, DNLTerms_.size()));
  }
  termIndexes.second = DNLTerms_.size();
  DNLInstances_.back().setTermsIndexes(termIndexes);
  for (auto inst : top_->getInstances()) {
    DNLInstances_.push_back(DNLInstance(inst, DNLInstances_.size(), parentId));
    assert(DNLInstances_.back().getID() > 0);
    stack.push_back(DNLInstances_.back().getID());
    std::pair<DNLID, DNLID> termIndexes;
    termIndexes.first = DNLTerms_.size();
    if (inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive() ||
        inst->getModel()->getInstances().empty()) {
      leaves_.push_back(DNLInstances_.back().getID());
    }
    for (auto term : inst->getInstTerms()) {
      DNLTerms_.push_back(
          DNLTerminal(DNLInstances_.back().getID(), term, DNLTerms_.size()));
    }
    termIndexes.second = DNLTerms_.size();
    DNLInstances_.back().setTermsIndexes(termIndexes);
  }
  childrenIndexes.second = DNLInstances_.back().getID();
  getNonConstDNLInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("DNL::process - Verify Top: %p\n", DNLInstances_[0].getSNLInstance());
  // LCOV_EXCL_STOP
#endif
  while (!stack.empty()) {
    assert(stack.back() > 0);
    const SNLInstance* parent =
        getNonConstDNLInstanceFromID((stack.back())).getSNLInstance();
    DNLID parentId = getNonConstDNLInstanceFromID((stack.back())).getID();
    stack.pop_back();
    std::pair<DNLID, DNLID> childrenIndexes;
    childrenIndexes.first = DNLInstances_.back().getID();
    for (auto inst : parent->getModel()->getInstances()) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("DNL::process- Push New DNL Instance for SNL instance(p):%p\n",
             inst);
      printf("DNL::process - Push New DNL Instance for SNL instance(name):%s\n",
             inst->getName().getString().c_str());
      // LCOV_EXCL_STOP
#endif
      DNLInstances_.push_back(
          DNLInstance(inst, DNLInstances_.size(), parentId));
      stack.push_back(DNLInstances_.back().getID());
      if (inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive() ||
          inst->getModel()->getInstances().empty()) {
        leaves_.push_back(DNLInstances_.back().getID());
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("DNL::process - Pushing to leaf instances with DNLID: %zu Instance name: -%s\n", DNLInstances_.back().getID(),
               inst->getName().getString().c_str());
        // LCOV_EXCL_STOP
#endif
      }
      std::pair<DNLID, DNLID> termIndexes;
      termIndexes.first = DNLTerms_.size();
      for (auto term : inst->getInstTerms()) {
        DNLTerms_.push_back(
            DNLTerminal(DNLInstances_.back().getID(), term, DNLTerms_.size()));
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("DNL::process - DNLTerm with DNLID: %zu for SNLInstanceTerm: %s\n", DNLTerms_.back().getID(),
               term->getString().c_str());
        // LCOV_EXCL_STOP
#endif
      }
      termIndexes.second = DNLTerms_.size();
      DNLInstances_.back().setTermsIndexes(termIndexes);
    }
    childrenIndexes.second = DNLInstances_.back().getID();
    getNonConstDNLInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
  }
  initTermId2isoId();
  DNLIsoDBBuilder fidbb(fidb_);
  fidbb.process();
}

bool DNL::isInstanceChild(DNLID parent, DNLID child) const {
  DNLID inst = child;
  if (parent == 0) {
    return true;
  }
  while (getDNLInstanceFromID(inst).getParentID() != 0) {
    if (parent == inst) {
      return true;
    }
    inst = getDNLInstanceFromID(inst).getParentID();
  }
  return false;
}