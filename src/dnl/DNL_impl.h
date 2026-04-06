// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "DNL.h"

// #define DEBUG_PRINTS

template <class DNLInstance, class DNLTerminal>
void DNLIsoDBBuilder<DNLInstance, DNLTerminal>::treatDriver(
    const DNLTerminal& term,
    DNLIso& DNLIso,
    visited& visitedDB,
    bool updateReadersIsoID,
    bool updateDriverIsoID,
    std::function<void(DNLID)> updateConst) {
  // if isoid already define for term so return
  std::stack<DNLID> stack;
  auto& visited = visitedDB;
  visited.beginTraversal(dnl_.getDNLTerms().size());
  stack.push(term.getID());
  // Collect all terms on this iso
  while (!stack.empty()) {
    DNLID fid = stack.top();
    stack.pop();
    if (visited.test(fid)) {
      continue;
    }
    const DNLTerminal& fterm = dnl_.getDNLTerminalFromID(fid);
    if (fterm.getDNLInstance().isTop()) {
      if (fterm.getSnlBitTerm()->getDirection() ==
          SNLTerm::Direction::DirectionEnum::Output) {
        DNLIso.addReader(fid);
        if (updateReadersIsoID) {
          updateConst(fid);
        }
      } else {
        DNLIso.addDriver(fid);
        if (updateDriverIsoID) {
          updateConst(fid);
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
        if (updateReadersIsoID) {
          updateConst(fid);
        }
      } else {
        DNLIso.addDriver(fid);
        if (updateDriverIsoID) {
          updateConst(fid);
        }
      }
    } else {
      DNLIso.addHierTerm(fid);
      if (updateReadersIsoID) {
        updateConst(fid);
      }
    }
    if (fterm.getSnlBitTerm()->getNet() &&
        !fterm.getDNLInstance().getSNLModel()->isAssign()) {
      DNLIso.addNet(fterm.getSnlBitTerm()->getNet());
      if (updateConst && DNLIso.isConstant()) {
        // check if contradicting constant, if yes set to AMBIGUOUS and remove from const isos
        if (DNLIso.isConstant1() && fterm.getSnlBitTerm()->getNet()->isConstant0()) {
          db_.removeConstant1Iso(DNLIso.getIsoID());
          DNLIso.setIsoType(DNLIso::IsoType::AMBIGUOUS);
        } else if (DNLIso.isConstant0() && fterm.getSnlBitTerm()->getNet()->isConstant1()) {
          db_.removeConstant0Iso(DNLIso.getIsoID());
          DNLIso.setIsoType(DNLIso::IsoType::AMBIGUOUS);
        }
      } else if (updateConst && fterm.getSnlBitTerm()->getNet()->isConstant1()) {
        addConstantIso1(DNLIso.getIsoID());
      } else if (updateConst &&
                 fterm.getSnlBitTerm()->getNet()->isConstant0()) {
        addConstantIso0(DNLIso.getIsoID());
      }
      bool netAlreadyVisited = false;
      DNLInstance finstance = fterm.getDNLInstance();
      for (SNLInstTerm* instTerm :
           fterm.getSnlBitTerm()->getNet()->getInstTerms()) {
        DNLID finstTermId = finstance.getChildInstance(instTerm->getInstance())
                                .getTerminal(instTerm)
                                .getID();
        if (visited.test(finstTermId)) {
          netAlreadyVisited = true;
          break;  // If the term was visited, by definition the current net was handled.
        }
        assert(finstTermId != DNLID_MAX);
        stack.push(finstTermId);
      }
      if (!netAlreadyVisited) {
        for (SNLBitTerm* bitTerm :
             fterm.getSnlBitTerm()->getNet()->getBitTerms()) {
          DNLID fbitTermId = finstance.getTerminalFromBitTerm(bitTerm).getID();
          if (fbitTermId == fid) {
            continue;
          }
          if (visited.test(fbitTermId)) {
            break;  // If the term was visited, by definition the current net was handled.
          }
          assert(fbitTermId != DNLID_MAX);
          stack.push(fbitTermId);
        }
      }
    }
    // Going inside the module
    if (!fterm.getDNLInstance().isTop() &&
        fterm.getSnlTerm()->getNet() != nullptr) {
      DNLIso.addNet(fterm.getSnlTerm()->getNet());
      if (updateConst && DNLIso.isConstant()) {
        // check if contradicting constant, if yes set to AMBIGUOUS and remove from const isos
        if (DNLIso.isConstant1() && fterm.getSnlTerm()->getNet()->isConstant0()) {
          db_.removeConstant1Iso(DNLIso.getIsoID());
          DNLIso.setIsoType(DNLIso::IsoType::AMBIGUOUS);
        } else if (DNLIso.isConstant0() && fterm.getSnlTerm()->getNet()->isConstant1()) {
          db_.removeConstant0Iso(DNLIso.getIsoID());
          DNLIso.setIsoType(DNLIso::IsoType::AMBIGUOUS);
        }
      } else if (updateConst && fterm.getSnlTerm()->getNet()->isConstant1()) {
        addConstantIso1(DNLIso.getIsoID());
      } else if (updateConst && fterm.getSnlTerm()->getNet()->isConstant0()) {
        addConstantIso0(DNLIso.getIsoID());
      }
      bool netAlreadyVisited = false;
      DNLInstance fparent = fterm.getDNLInstance().getParentInstance();
      for (SNLInstTerm* instTerm :
           fterm.getSnlTerm()->getNet()->getInstTerms()) {
        DNLID finstTermId = fparent.getChildInstance(instTerm->getInstance())
                                .getTerminal(instTerm)
                                .getID();
        if (visited.test(finstTermId)) {
          netAlreadyVisited = true;
          break;  // If the term was visited, by definition the current net was handled.
        }
        if (finstTermId == fid) {
          continue;
        }
        assert(finstTermId != DNLID_MAX);
        stack.push(finstTermId);
      }
      if (!netAlreadyVisited) {
        for (SNLBitTerm* bitTerm :
             fterm.getSnlTerm()->getNet()->getBitTerms()) {
          DNLID fbitTermId = fparent.getTerminalFromBitTerm(bitTerm).getID();
          /*if (fbitTermId == fid) {
            continue;
          }*/
          if (visited.test(fbitTermId)) {
            break;  // If the term was visited, by definition the current net was handled.
          }
          assert(fbitTermId != DNLID_MAX);
          stack.push(fbitTermId);
        }
      }
    }
    visited.set(fid);
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
         term <= inst.getTermIndexes().second; term++) {
      assert(DNLID_MAX != term);
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
  // Creating the top
  OrderIDInitializer orderIDInitializer;
  orderIDInitializer.process();
  DNLInstances_.push_back(
      DNLInstance(nullptr, DNLInstances_.size(), DNLID_MAX));
  assert(DNLInstances_.back().getID() == DNLInstances_.size() - 1);
  DNLID parentId = DNLInstances_.back().getID();
  std::pair<DNLID, DNLID> childrenIndexes;
  std::pair<DNLID, DNLID> termIndexes;
  termIndexes.first = DNLTerms_.size();
  std::set<SNLBitTerm*, SNLBitTermCompare> sortedBitTerms;
  for (SNLBitTerm* bitterm : top_->getBitTerms()) {
    sortedBitTerms.insert(bitterm);
  }
  for (SNLBitTerm* bitterm : sortedBitTerms) {
    DNLTerms_.push_back(DNLTerminal(parentId, bitterm, DNLTerms_.size()));
  }
  if (termIndexes.first == DNLTerms_.size()) {
    // No terms are associated
    termIndexes.first = DNLID_MAX;
    termIndexes.second = DNLID_MAX;
  } else {
    termIndexes.second = DNLTerms_.back().getID();
  }
  DNLInstances_.back().setTermsIndexes(termIndexes);
  childrenIndexes.first = DNLInstances_.size();
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
    std::set<SNLInstTerm*, SNLInstTermCompare> sortedInstTerms;
    for (auto term : inst->getInstTerms()) {
      sortedInstTerms.insert(term);
    }
    for (auto term : sortedInstTerms) {
      DNLTerms_.push_back(
          DNLTerminal(DNLInstances_.back().getID(), term, DNLTerms_.size()));
    }
    if (termIndexes.first == DNLTerms_.size()) {
      // No terms are associated
      termIndexes.first = DNLID_MAX;
      termIndexes.second = DNLID_MAX;
    } else {
      termIndexes.second = DNLTerms_.back().getID();
    }
    DNLInstances_.back().setTermsIndexes(termIndexes);
  }
  if (childrenIndexes.first == DNLInstances_.size()) {
    // No terms are associated
    childrenIndexes.first = DNLID_MAX;
    childrenIndexes.second = DNLID_MAX;
  } else {
    childrenIndexes.second = DNLInstances_.back().getID();
  }
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
    childrenIndexes.first = DNLInstances_.size();
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
      std::set<SNLInstTerm*, SNLInstTermCompare> sortedInstTerms;
      for (auto term : inst->getInstTerms()) {
        sortedInstTerms.insert(term);
      }
      for (auto term : sortedInstTerms) {
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
      termIndexes.second = DNLTerms_.back().getID();
      if (termIndexes.first == DNLTerms_.size()) {
        // No terms are associated
        termIndexes.first = DNLID_MAX;
        termIndexes.second = DNLID_MAX;
      }
      DNLInstances_.back().setTermsIndexes(termIndexes);
    }
    if (childrenIndexes.first == DNLInstances_.size()) {
      // No terms are associated
      childrenIndexes.first = DNLID_MAX;
      childrenIndexes.second = DNLID_MAX;
    } else {
      childrenIndexes.second = DNLInstances_.back().getID();
    }
    getNonConstDNLInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
  }
  DNLInstances_.push_back(DNLInstance());
  DNLTerms_.push_back(DNLTerminal());
  initTermId2isoId();
  DNLIsoDBBuilder<DNLInstance, DNLTerminal> fidbb(fidb_, *this);
  fidbb.process();
  fidb_.addIso().setId(DNLID_MAX);  // addNullIso
#ifdef DEBUG_PRINTS
                                    // LCOV_EXCL_START
  printf("DNL creation done.\n");
  // LCOV_EXCL_STOP
#endif
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
  auto& constDNL = const_cast<DNL<DNLInstance, DNLTerminal>&>(dnl_);
  std::vector<DNLID> tasks;
  for (DNLID leaf : dnl_.getLeaves()) {
    if (dnl_.getDNLInstanceFromID(leaf).getTermIndexes().first != DNLID_MAX) {
      for (DNLID term = dnl_.getDNLInstanceFromID(leaf).getTermIndexes().first;
           term <= dnl_.getDNLInstanceFromID(leaf).getTermIndexes().second;
           term++) {
        assert(DNLID_MAX != term);
        const auto& termRef = dnl_.getDNLTerminalFromID(term);
        if (termRef.getSnlTerm()
                    ->getDirection() !=
                SNLTerm::Direction::DirectionEnum::Input &&
            termRef.getSnlTerm()->getNet()) {
          DNLIso& DNLIso = addIsoToDB();

          tasks.push_back(term);
          constDNL.getNonConstDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
        }
      }
    }
  }
  if (dnl_.getTop().getTermIndexes().first != DNLID_MAX) {
    for (DNLID term = dnl_.getTop().getTermIndexes().first;
         term <= dnl_.getTop().getTermIndexes().second; term++) {
      assert(DNLID_MAX != term);
      const auto& termRef = dnl_.getDNLTerminalFromID(term);
      if (termRef.getSnlBitTerm()
                  ->getDirection() !=
              SNLTerm::Direction::DirectionEnum::Output &&
          termRef.getSnlBitTerm()->getNet()) {
        DNLIso& DNLIso = addIsoToDB();
        tasks.push_back(term);
        constDNL.getNonConstDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
      }
    }
  }
  std::vector<DNLID> multiDriverIsos;
  tbb::enumerable_thread_specific<visited> visit;
  // auto visitL = visit.local( );
  if (!getenv("NON_MT")) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("MT\n");
    // LCOV_EXCL_STOP
#endif
    tbb::task_arena arena(tbb::task_arena::automatic);
    tbb::parallel_for(
        tbb::blocked_range<DNLID>(0, tasks.size(), 1000),
        [&](const tbb::blocked_range<DNLID>& r) {
          for (DNLID i = r.begin(); i < r.end(); ++i) {
            const auto& taskTerm = dnl_.getDNLTerminalFromID(tasks[i]);
            auto& iso =
                db_.getIsoFromIsoID(taskTerm.getIsoID());
            treatDriver(
                taskTerm,
                iso,
                visit.local(),
                true,
                false,
                [&constDNL, &iso](DNLID fid) {
                  constDNL.getNonConstDNLTerminalFromID(fid).setIsoID(iso.getIsoID());
                });
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf("treatDriver %lu %lu\n",
                   (size_t)tbb::task_arena::current_thread_index(), tasks[i]);
        // LCOV_EXCL_STOP
#endif
          }
        },
        tbb::simple_partitioner());
  } else {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Non MT\n");
    // LCOV_EXCL_STOP
#endif
    for (auto task : tasks) {
      const auto& taskTerm = dnl_.getDNLTerminalFromID(task);
      auto& iso = db_.getIsoFromIsoID(taskTerm.getIsoID());
      treatDriver(taskTerm,
                  iso,
                  visit.local(),
                  true,
                  false,
                  [&constDNL, &iso](DNLID fid) {
                    constDNL.getNonConstDNLTerminalFromID(fid).setIsoID(iso.getIsoID());
                  });
    }
  }
  for (DNLID iso = 0; iso < db_.getNumIsos() + 1; iso++) {
    if (db_.getIsoFromIsoID(iso).getDrivers().size() > 1) {
      multiDriverIsos.push_back(iso);
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
      db_.makeShadow(iso);
    }
  }
  tasks.clear();  // reuse tasks for new round
  std::vector<DNLID>
      multiDriverIsosRound2;  // reuse multiDriverIsos for new round
  for (DNLID driver : driversToTreat) {
    tasks.push_back(driver);
  }
  for (DNLID driver : tasks) {
    DNLIso& DNLIso = addIsoToDB();
    constDNL.getNonConstDNLTerminalFromID(driver).setIsoID(DNLIso.getIsoID());
  }
  if (!getenv("NON_MT")) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("MT\n");
    // LCOV_EXCL_STOP
#endif
    tbb::task_arena arena(tbb::task_arena::automatic);
    tbb::parallel_for(
        tbb::blocked_range<DNLID>(0, tasks.size(), 1000),
        [&](const tbb::blocked_range<DNLID>& r) {
          for (DNLID i = r.begin(); i < r.end(); ++i) {
            const auto& taskTerm = dnl_.getDNLTerminalFromID(tasks[i]);
            auto& iso =
                db_.getIsoFromIsoID(taskTerm.getIsoID());
            treatDriver(
                taskTerm,
                iso,
                visit.local(),
                true,
                true,
                [&constDNL, &iso](DNLID fid) {
                  constDNL.getNonConstDNLTerminalFromID(fid).setIsoID(iso.getIsoID());
                });
          }
        },
        tbb::simple_partitioner());
  } else {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Non MT\n");
    // LCOV_EXCL_STOP
#endif
    for (auto task : tasks) {
      const auto& taskTerm = dnl_.getDNLTerminalFromID(task);
      auto& iso = db_.getIsoFromIsoID(taskTerm.getIsoID());
      treatDriver(taskTerm,
                  iso,
                  visit.local(),
                  true,
                  true,
                  [&constDNL, &iso](DNLID fid) {
                    constDNL.getNonConstDNLTerminalFromID(fid).setIsoID(iso.getIsoID());
                  });
    }
  }
  for (DNLID iso = 0; iso < db_.getNumIsos() + 1; iso++) {
    if (db_.getIsoFromIsoID(iso).getDrivers().size() > 1) {
      multiDriverIsosRound2.push_back(iso);
    }
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("md check: %lu %lu\n", driversToTreat.size(),
         multiDriverIsosRound2.size());
  // LCOV_EXCL_STOP
#endif
  assert(driversToTreat.size() == multiDriverIsosRound2.size());
  // Handling dangling nets
  for (DNLID termid = 0; termid < dnl_.getDNLTerms().size() - 1; termid++) {
    auto term = dnl_.getDNLTerms()[termid];
    // Check if the term have net but not iso
    if (((term.getSnlBitTerm()->getNet() != nullptr && !term.getDNLInstance().isLeaf()) 
        || (!term.getDNLInstance().isTop() && term.getSnlTerm()->getNet() != nullptr)) &&
        term.getIsoID() == DNLID_MAX) {
      DNLIso& dnlIso = addIsoToDB();
      term.setIsoID(dnlIso.getIsoID());
      treatDriver(term,
                  dnlIso,
                  visit.local(),
                  true,
                  true,
                  [&constDNL, &dnlIso](DNLID fid) {
                    constDNL.getNonConstDNLTerminalFromID(fid).setIsoID(dnlIso.getIsoID());
                  });
      assert(dnlIso.getDrivers().size() == 0);
    }
  }
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
  const auto& iso = fidb_.getIsoFromIsoIDconst(dnlIsoId);
  DNLID seed = DNLID_MAX;
  if (!iso.getDrivers().empty()) {
    seed = iso.getDrivers()[0];
  } else if (!iso.getReaders().empty()) {
    seed = iso.getReaders()[0];
  } else {
    return;
  }
  fidbb.treatDriver(getDNLTerminalFromID(seed), DNLIso, visitedDB);
}
