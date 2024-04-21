// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef DNL_IMPL_H
#define DNL_IMPL_H

#include "DNL.h"
// #define DEBUG_PRINTS

template <class DNLInstance, class DNLTerminal>
void DNLIsoDBBuilder<DNLInstance, DNLTerminal>::treatDriver(
    const DNLTerminal& term,
    DNLIso& DNLIso,
    bool updateIsoID) {
  std::vector<bool> visited;
  visited.resize(dnl_.getDNLTerms().size(), false);
#ifdef DEBUG_PRINTS
  printf("------------------------treatDriver-----------------------\n");
#endif
  std::stack<DNLID> stack;
  if (term.getDNLInstance().isTop()) {
    assert(term.getSnlBitTerm()->getDirection() !=
           SNLTerm::Direction::DirectionEnum::Output);
    assert(term.getSnlBitTerm()->getNet());
    for (SNLInstTerm* reader : term.getSnlBitTerm()->getNet()->getInstTerms()) {
      DNLID freader = term.getDNLInstance()
                          .getChildInstance(reader->getInstance())
                          .getTerminal(reader)
                          .getID();

      if (freader == term.getID())
        continue;
      if (updateIsoID) {
        dnl_.getDNLTerminalFromID(freader).setIsoID(DNLIso.getIsoID());
      }
      if (dnl_.getDNLTerminalFromID(freader)
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
    for (SNLBitTerm* bitTerm : term.getSnlBitTerm()->getNet()->getBitTerms()) {
      DNLID fbitTerm = term.getDNLInstance().getTerminalFromBitTerm(bitTerm).getID();
      if (fbitTerm == term.getID())
        continue;
      stack.push(fbitTerm);
    }
  } else {
    // do DFS on the driver
    stack.push(term.getID());
  }

  // Start traversing from driver
  while (!stack.empty()) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("New terminal to treat\n");
    // LCOV_EXCL_STOP
#endif
    DNLID id = stack.top();
    stack.pop();
    const DNLTerminal& fterm = dnl_.getDNLTerminalFromID(id);
    assert(fterm.getID() == id);
    if (visited[id]) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Visited "
          "continue\n");
      // LCOV_EXCL_STOP
#endif
      continue;
    }

    //------------------------------ Top output -> add as reader and continue
    //------------------------------
    if (fterm.getDNLInstance().isTop() &&
        fterm.getSnlBitTerm()->getDirection() !=
            SNLTerm::Direction::DirectionEnum::Input) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Is top "
          "output so adding as reader\n");
      // LCOV_EXCL_STOP
#endif
      DNLIso.addReader(fterm.getID());
      if (updateIsoID) {
        dnl_.getDNLTerminalFromID(fterm.getID()).setIsoID(DNLIso.getIsoID());
      }
      continue;
    }

    if (fterm.getDNLInstance().isTop() &&
        fterm.getSnlBitTerm()->getDirection() !=
            SNLTerm::Direction::DirectionEnum::Output
        && fterm.getID() != term.getID()) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Is top "
          "input so adding as driver\n");
      // LCOV_EXCL_STOP
#endif
      DNLIso.addDriver(fterm.getID());
      if (updateIsoID) {
        dnl_.getDNLTerminalFromID(fterm.getID()).setIsoID(DNLIso.getIsoID());
      }
      continue;
    }

    //-------------------------------- Not top output -> treating
    //------------------------------
    assert(fterm.getDNLInstance().isTop() == false);
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "DNLIsoDBBuilder::treatDriver - Treating "
        "%zu %s %s\n",
        id, fterm.getSnlTerm()->getString().c_str(),
        fterm.getSnlTerm()->getDirection().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    visited[id] = true;
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
//-------------------------------- Terminal is hierarchical
//------------------------------
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Term is of hier "
          "inst\n");
      // LCOV_EXCL_STOP
#endif
      DNLIso.addHierTerm(fterm.getID());
      if (updateIsoID) {
        dnl_.getDNLTerminalFromID(fterm.getID()).setIsoID(DNLIso.getIsoID());
      }
    }

#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "DNLIsoDBBuilder::treatDriver - term "
        "direction %s\n",
        fterm.getSnlTerm()->getDirection().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    if (!fterm.getSnlTerm()
             ->getInstance()
             ->getModel()
             ->getInstances()
             .empty() &&
        fterm.getSnlTerm()->getDirection() !=
            SNLTerm::Direction::DirectionEnum::Output) {
      //-------------------------------- Input of hierarchical instance
      //------------------------------
      //-------------------------------- Go down into the instance model and
      // process the net ------------------------------
      snlNet = fterm.getSnlTerm()->getBitTerm()->getNet();
      DNLParent = &fterm.getDNLInstance();
      goDown = true;
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Input so going "
          "down\n");
      // LCOV_EXCL_STOP
#endif
    } else {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Output so analyzing Same "
          "level\n");
      // LCOV_EXCL_STOP
#endif
    }
    if (snlNet == nullptr)
      continue;
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "DNLIsoDBBuilder::treatDriver - Visiting net "
        "%s\n",
        snlNet->getName().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    //-------------------------------- Iterate ovet inst terms of the net
    //------------------------------
    for (SNLInstTerm* instTerm : snlNet->getInstTerms()) {
      SNLInstance* termSnlInst = instTerm->getInstance();
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Inst %s "
          "Term %s\n",
          termSnlInst->getString().c_str(), instTerm->getString().c_str());
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
      printf(
          "DNLIsoDBBuilder::treatDriver - Visiting "
          "snl DNLID %zu %s %s\n",
          ftermNew.getID(), instTerm->getString().c_str(),
          instTerm->getDirection().getString().c_str());
      printf(
          "DNLIsoDBBuilder::treatDriver - Visiting "
          "DNLID %zu %s %s\n",
          ftermNew.getID(), ftermNew.getSnlTerm()->getString().c_str(),
          ftermNew.getSnlTerm()->getDirection().getString().c_str());
      // LCOV_EXCL_STOP
#endif
      if (termSnlInst->getModel()->getInstances().empty()) {
        if (ftermNew.getSnlTerm()->getDirection() !=
            SNLTerm::Direction::DirectionEnum::Input) {
          //-------------------------------- Output of blackbox -> add as driver
          //------------------------------
          // if (ftermNew.getID() == term.getID()) continue;
          if (term.getID() != ftermNew.getID()) {
            // assert(false);
            DNLIso.addDriver(ftermNew.getID());
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf(
                "DNLIsoDBBuilder::treatDriver -- Add "
                "driver\n\n");
            // LCOV_EXCL_STOP
#endif
          } else {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf(
                "DNLIsoDBBuilder::treatDriver --- "
                "Original driver -> skipping\n");
            // LCOV_EXCL_STOP
#endif
          }
          // assert(0);
        } else {
          //-------------------------------- Input of blakbox -> add as reader
          //------------------------------
          DNLIso.addReader(ftermNew.getID());
          if (updateIsoID) {
            dnl_.getDNLTerminalFromID(ftermNew.getID())
                .setIsoID(DNLIso.getIsoID());
          }
#ifdef DEBUG_PRINTS
          // LCOV_EXCL_START
          printf(
              "DNLIsoDBBuilder::treatDriver -- Add "
              "reader\n\n");
          // LCOV_EXCL_STOP
#endif
        }
      } else {
        //-------------------------------- Hier instance -> add as reader
        //------------------------------
        stack.push(ftermNew.getID());
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf(
            "DNLIsoDBBuilder::treatDriver --- "
            "Pushing to stuck %s %s\n",
            ftermNew.getSnlTerm()->getString().c_str(),
            ftermNew.getSnlTerm()->getDirection().getString().c_str());
        // LCOV_EXCL_STOP
#endif
      }
    }
    //-------------------------------- Process the bit terms of the net
    //------------------------------
    for (SNLBitTerm* bitTerm : snlNet->getBitTerms()) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "DNLIsoDBBuilder::treatDriver - Visiting "
          "bt %s %s %s\n",
          bitTerm->getString().c_str(),
          bitTerm->getDirection().getString().c_str(),
          bitTerm->getDesign()->getString().c_str());
      DNLParent->display();
      if (!DNLParent->isTop()) {
        DNLParent->getParentInstance().display();
      }

      printf(
          "DNLIsoDBBuilder::treatDriver - Visiting "
          "bt %zu %s %s %s\n",
          DNLParent->getTerminalFromBitTerm(bitTerm).getID(),
          bitTerm->getString().c_str(),
          bitTerm->getDirection().getString().c_str(),
          bitTerm->getDesign()->getString().c_str());
      // LCOV_EXCL_STOP
#endif
      const DNLTerminal& ftermNew = DNLParent->getTerminalFromBitTerm(bitTerm);
      if (visited[ftermNew.getID()]) {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf(
            "DNLIsoDBBuilder::treatDriver - Visited "
            "continue\n");
        // LCOV_EXCL_STOP
#endif
        continue;
      }
      stack.push(ftermNew.getID());
    }
  }
}

template <class DNLInstance, class DNLTerminal>
DNL<DNLInstance, DNLTerminal>::DNL(const SNLDesign* top) : top_(top) {}

template <class DNLInstance, class DNLTerminal>
void DNL<DNLInstance, DNLTerminal>::display() const {
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

template <class DNLInstance, class DNLTerminal>
void DNL<DNLInstance, DNLTerminal>::process() {
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
  printf("DNL::process - Verify Top: %p\n",
         (void*)DNLInstances_[0].getSNLInstance());
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
             (void*)inst);
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
        printf(
            "DNL::process - Pushing to leaf instances with DNLID: %zu Instance "
            "name: -%s\n",
            DNLInstances_.back().getID(), inst->getName().getString().c_str());
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
        printf(
            "DNL::process - DNLTerm with DNLID: %zu for SNLInstanceTerm: %s\n",
            DNLTerms_.back().getID(), term->getString().c_str());
        // LCOV_EXCL_STOP
#endif
      }
      termIndexes.second = DNLTerms_.size();
      DNLInstances_.back().setTermsIndexes(termIndexes);
    }
    childrenIndexes.second = DNLInstances_.back().getID();
    getNonConstDNLInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
  }
  DNLInstances_.push_back(DNLInstance());
  DNLTerms_.push_back(DNLTerminal());
  initTermId2isoId();
  DNLIsoDBBuilder<DNLInstance, DNLTerminal> fidbb(fidb_, *this);
  fidbb.process();
  fidb_.addIso().setId(DNLID_MAX);  // addNullIso
}

template <class DNLInstance, class DNLTerminal>
bool DNL<DNLInstance, DNLTerminal>::isInstanceChild(DNLID parent,
                                                    DNLID child) const {
  DNLID inst = child;
  if (parent == 0) {
    return true;
  }
  printf("isInstanceChild\n");
  while (inst != 0) {
    if (parent == inst) {
      return true;
    }
    printf("isInstanceChild %lu\n", inst);
    inst = getDNLInstanceFromID(inst).getParentID();
  }
  return false;
}

template <class DNLInstance, class DNLTerminal>
DNLIsoDBBuilder<DNLInstance, DNLTerminal>::DNLIsoDBBuilder(
    DNLIsoDB& db,
    const DNL<DNLInstance, DNLTerminal>& dnl)
    : db_(db), dnl_(dnl) {}

template <class DNLInstance, class DNLTerminal>
void DNLIsoDBBuilder<DNLInstance, DNLTerminal>::process() {
  // iterate on all leaf drivers
  std::vector<DNLID> tasks;
  for (DNLID leaf : dnl_.getLeaves()) {
    for (DNLID term = dnl_.getDNLInstanceFromID(leaf).getTermIndexes().first;
         term < dnl_.getDNLInstanceFromID(leaf).getTermIndexes().second;
         term++) {
      if (dnl_.getDNLTerminalFromID(term).getSnlTerm()->getDirection() !=
              SNLTerm::Direction::DirectionEnum::Input &&
          dnl_.getDNLTerminalFromID(term).getSnlTerm()->getNet()) {
        DNLIso& DNLIso = addIsoToDB();

        DNLIso.addDriver(term);
        tasks.push_back(term);
        dnl_.getDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
      }
    }
  }
  for (DNLID term = dnl_.getTop().getTermIndexes().first;
       term < dnl_.getTop().getTermIndexes().second; term++) {
    if (dnl_.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
            SNLTerm::Direction::DirectionEnum::Output &&
        dnl_.getDNLTerminalFromID(term).getSnlBitTerm()->getNet()) {
      DNLIso& DNLIso = addIsoToDB();
      DNLIso.addDriver(term);
      tasks.push_back(term);
      dnl_.getDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
    }
  }
  tbb::concurrent_vector<DNLID> multiDriverIsos;
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
            treatDriver(dnl_.getDNLTerminalFromID(tasks[i]),
                        db_.getIsoFromIsoID(
                            dnl_.getDNLTerminalFromID(tasks[i]).getIsoID()),
                        true);
            if (db_.getIsoFromIsoID(
                       dnl_.getDNLTerminalFromID(tasks[i]).getIsoID())
                    .getDrivers()
                    .size() > 1) {
              multiDriverIsos.push_back(
                  dnl_.getDNLTerminalFromID(tasks[i]).getIsoID());
            }
          }
        });
  } else {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Non MT\n");
    // LCOV_EXCL_STOP
#endif
    for (auto task : tasks) {
      treatDriver(
          dnl_.getDNLTerminalFromID(task),
          db_.getIsoFromIsoID(dnl_.getDNLTerminalFromID(task).getIsoID()),
          true);
      if (db_.getIsoFromIsoID(dnl_.getDNLTerminalFromID(task).getIsoID())
              .getDrivers()
              .size() > 1) {
        multiDriverIsos.push_back(dnl_.getDNLTerminalFromID(task).getIsoID());
      }
    }
  }
  std::set<DNLID> driversToTreat;
  if (!multiDriverIsos.empty()) {
    for (DNLID iso : multiDriverIsos) {
      std::set<DNLID> drivers;
      for (DNLID driver : db_.getIsoFromIsoID(iso).getDrivers()) {
        drivers.insert(driver);
      }
      driversToTreat.insert(*drivers.begin());
      db_.getIsoFromIsoID(iso).makeShadow();
      db_.incrementShadow();
    }
  }
  tasks.clear();  // reuse tasks for new round
  multiDriverIsos.clear(); // reuse multiDriverIsos for new round
  for (DNLID driver : driversToTreat) {
    DNLIso& DNLIso = addIsoToDB();
    DNLIso.addDriver(driver);
    tasks.push_back(driver);
    dnl_.getDNLTerminalFromID(driver).setIsoID(DNLIso.getIsoID());
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
            treatDriver(dnl_.getDNLTerminalFromID(tasks[i]),
                        db_.getIsoFromIsoID(
                            dnl_.getDNLTerminalFromID(tasks[i]).getIsoID()),
                        true);
            if (db_.getIsoFromIsoID(
                       dnl_.getDNLTerminalFromID(tasks[i]).getIsoID())
                    .getDrivers()
                    .size() > 1) {
              multiDriverIsos.push_back(
                  dnl_.getDNLTerminalFromID(tasks[i]).getIsoID());
            }
          }
        });
  } else {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Non MT\n");
    // LCOV_EXCL_STOP
#endif
    for (auto task : tasks) {
      treatDriver(
          dnl_.getDNLTerminalFromID(task),
          db_.getIsoFromIsoID(dnl_.getDNLTerminalFromID(task).getIsoID()),
          true);
      if (db_.getIsoFromIsoID(dnl_.getDNLTerminalFromID(task).getIsoID())
              .getDrivers()
              .size() > 1) {
        multiDriverIsos.push_back(dnl_.getDNLTerminalFromID(task).getIsoID());
      }
    }
  }
  assert(multiDriverIsos.size() == tasks.size());
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("num fi %zu\n", dnl_.getDNLInstances().size());
  printf("num ft %zu\n", dnl_.getDNLTerms().size());
  printf("num leaves %zu\n", dnl_.getLeaves().size());
  printf("num isos %zu\n", db_.getNumIsos());
  // LCOV_EXCL_STOP
#endif
}

template <class DNLInstance, class DNLTerminal>
void DNL<DNLInstance, DNLTerminal>::getCustomIso(DNLID dnlIsoId,
                                                 DNLIso& DNLIso) const {
  DNLIso.setId(dnlIsoId);
  DNLIsoDB fidb;
  DNLIso.addDriver(fidb_.getIsoFromIsoIDconst(dnlIsoId).getDrivers()[0]);
  DNLIsoDBBuilder<DNLInstance, DNLTerminal> fidbb(fidb, *this);
  fidbb.treatDriver(getDNLTerminalFromID(
                        fidb_.getIsoFromIsoIDconst(dnlIsoId).getDrivers()[0]),
                    DNLIso);
}

#endif  // DNL_IMPL_H
