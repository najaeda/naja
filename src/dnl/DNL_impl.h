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
    visited& visitedDB,
    bool updateIsoID) {
  std::vector<bool>& visited = visitedDB.visited;
  visited.resize(dnl_.getDNLTerms().size(), false);
  visited.assign(visited.size(), false);
  std::vector<bool>& toVisitAsInstTerm = visitedDB.toVisitAsInstTerm;
  toVisitAsInstTerm.resize(dnl_.getDNLTerms().size(), false);
  toVisitAsInstTerm.assign(toVisitAsInstTerm.size(), false);
  std::vector<bool>& toVisitAsBitTerm = visitedDB.toVisitAsBitTerm;
  toVisitAsBitTerm.resize(dnl_.getDNLTerms().size(), false);
  toVisitAsBitTerm.assign(toVisitAsBitTerm.size(), false);
  std::stack<DNLID> stack;
  stack.push(term.getID());
  // Collect all terms on this iso
  while (!stack.empty()) {
    DNLID fid = stack.top();
    stack.pop();
    if (visited[fid]) {
      continue;
    }
    const DNLTerminal& fterm = dnl_.getDNLTerminalFromID(fid);
    if (fterm.getDNLInstance().isTop()) {
      if (fterm.getSnlBitTerm()->getDirection() ==
          SNLTerm::Direction::DirectionEnum::Output) {
        DNLIso.addReader(fid);
        if (updateIsoID) {
          dnl_.getDNLTerminalFromID(fid).setIsoID(DNLIso.getIsoID());
        }
      } else {
        DNLIso.addDriver(fid);
        if (updateIsoID) {
          dnl_.getDNLTerminalFromID(fid).setIsoID(DNLIso.getIsoID());
        }
      }
    } else if (fterm.getDNLInstance()
                   .getSNLInstance()
                   ->getModel()
                   ->getInstances()
                   .empty()) {
      if (fterm.getSnlTerm()->getDirection() ==
          SNLTerm::Direction::DirectionEnum::Input) {
        DNLIso.addReader(fid);
        if (updateIsoID) {
          dnl_.getDNLTerminalFromID(fid).setIsoID(DNLIso.getIsoID());
        }
      } else {
        DNLIso.addDriver(fid);
        if (updateIsoID) {
          dnl_.getDNLTerminalFromID(fid).setIsoID(DNLIso.getIsoID());
        }
      }
    } else {
      DNLIso.addHierTerm(fid);
      if (updateIsoID) {
        dnl_.getDNLTerminalFromID(fid).setIsoID(DNLIso.getIsoID());
      }
    }
    visited[fid] = true;
    if (!toVisitAsBitTerm[fterm.getID()] && fterm.getSnlBitTerm()->getNet()) {
      DNLInstance finstance = fterm.getDNLInstance();
      for (SNLInstTerm* instTerm :
           fterm.getSnlBitTerm()->getNet()->getInstTerms()) {
        DNLID finstTermId = finstance.getChildInstance(instTerm->getInstance())
                                .getTerminal(instTerm)
                                .getID();
        if (toVisitAsInstTerm[finstTermId]) {
          continue;
        }
        /*if (finstTermId == fid) {
          continue;
        }*/
        if (visited[finstTermId]) {
          continue;
        }
        stack.push(finstTermId);
        toVisitAsInstTerm[finstTermId] = true;
      }
      for (SNLBitTerm* bitTerm :
           fterm.getSnlBitTerm()->getNet()->getBitTerms()) {
        DNLID fbitTermId = finstance.getTerminalFromBitTerm(bitTerm).getID();
        if (toVisitAsBitTerm[fbitTermId]) {
          continue;
        }
        if (fbitTermId == fid) {
          continue;
        }
        if (visited[fbitTermId]) {
          continue;
        }
        stack.push(fbitTermId);
        toVisitAsBitTerm[fbitTermId] = true;
      }
    }
    if (!toVisitAsInstTerm[fterm.getID()] && !fterm.getDNLInstance().isTop() &&
        fterm.getSnlTerm()->getNet()) {
      DNLInstance fparent = fterm.getDNLInstance().getParentInstance();
      for (SNLInstTerm* instTerm :
           fterm.getSnlTerm()->getNet()->getInstTerms()) {
        DNLID finstTermId = fparent.getChildInstance(instTerm->getInstance())
                                .getTerminal(instTerm)
                                .getID();
        if (toVisitAsInstTerm[finstTermId]) {
          continue;
        }
        if (finstTermId == fid) {
          continue;
        }
        if (visited[finstTermId]) {
          continue;
        }
        stack.push(finstTermId);
        toVisitAsInstTerm[finstTermId] = true;
      }
      for (SNLBitTerm* bitTerm : fterm.getSnlTerm()->getNet()->getBitTerms()) {
        DNLID fbitTermId = fparent.getTerminalFromBitTerm(bitTerm).getID();
        if (toVisitAsBitTerm[fbitTermId]) {
          continue;
        }
        /*if (fbitTermId == fid) {
          continue;
        }*/
        if (visited[fbitTermId]) {
          continue;
        }
        stack.push(fbitTermId);
        toVisitAsBitTerm[fbitTermId] = true;
      }
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
    if (/*inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive()
           ||*/
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
      if (/*inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive()
             ||*/
          inst->getModel()->getInstances().empty()) {
        leaves_.push_back(DNLInstances_.back().getID());
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf(
            "DNL::process - Pushing to leaf instances with DNLID: %zu "
            "Instance "
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
            "DNL::process - DNLTerm with DNLID: %zu for SNLInstanceTerm: "
            "%s\n",
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
      tasks.push_back(term);
      dnl_.getDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
    }
  }
  std::vector<DNLID> multiDriverIsos;
  tbb::enumerable_thread_specific<visited> visit;
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
                        visit.local(), false);
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("treatDriver %lu %lu\n",
                   (size_t)tbb::task_arena::current_thread_index(), tasks[i]);
        // LCOV_EXCL_STOP
#endif
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
          visit.local(), false);
    }
  }
  std::vector<DNLID> uniqueDriverIsos;
  for (DNLID iso = 0; iso < db_.getNumIsos() + 1; iso++) {
    if (db_.getIsoFromIsoID(iso).getDrivers().size() > 1) {
      multiDriverIsos.push_back(iso);
    } else {
      uniqueDriverIsos.push_back(iso);
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
    }
  }
  tasks.clear();  // reuse tasks for new round
  std::vector<DNLID>
      multiDriverIsosRound2;  // reuse multiDriverIsos for new round
  for (DNLID driver : driversToTreat) {
    tasks.push_back(driver);
  }
  for (DNLID iso : uniqueDriverIsos) {
    tasks.push_back(db_.getIsoFromIsoIDconst(iso).getDrivers()[0]);
  }
  db_.emptyIsos();
  for (DNLID driver : tasks) {
    DNLIso& DNLIso = addIsoToDB();
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
                        visit.local(), true);
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
          visit.local(), true);
    }
  }
  for (DNLID iso = 0; iso < db_.getNumIsos() + 1; iso++) {
    /*for (DNLID driver : db_.getIsoFromIsoID(iso).getDrivers()) {
      assert(dnl_.getDNLTerminalFromID(driver).getIsoID() == iso);
    }
    for (DNLID reader : db_.getIsoFromIsoID(iso).getReaders()) {
      assert(dnl_.getDNLTerminalFromID(reader).getIsoID() == iso);
    }*/
    if (db_.getIsoFromIsoID(iso).getDrivers().size() > 1) {
      multiDriverIsosRound2.push_back(iso);
    }
  }
  // assert(driversToTreat.size() == multiDriverIsosRound2.size());
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
  DNLIsoDBBuilder<DNLInstance, DNLTerminal> fidbb(fidb, *this);
  visited visitedDB;
  fidbb.treatDriver(getDNLTerminalFromID(
                        fidb_.getIsoFromIsoIDconst(dnlIsoId).getDrivers()[0]),
                    DNLIso, visitedDB);
}

#endif  // DNL_IMPL_H
