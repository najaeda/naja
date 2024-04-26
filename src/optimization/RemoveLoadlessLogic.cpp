// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <spdlog/spdlog.h>
#include <vector>

#include "SNLBusNetBit.h"
#include "SNLDB0.h"
#include "SNLUniverse.h"

#include "RemoveLoadlessLogic.h"
#include "Utils.h"

using namespace naja::DNL;
using namespace naja::SNL;

// #define DEBUG_PRINTS

namespace naja::NAJA_OPT {

// Constructor
LoadlessLogicRemover::LoadlessLogicRemover() {}

// Given a DNL, getting all isos connected to top output.
std::vector<DNLID> LoadlessLogicRemover::getTopOutputIsos(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl) {
  std::vector<DNLID> topOutputIsos;
  for (DNLID term = dnl.getTop().getTermIndexes().first;
       term < dnl.getTop().getTermIndexes().second; term++) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("Checking %s\n",
           dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getString().c_str());
    printf("Direction %s\n", dnl.getDNLTerminalFromID(term)
                                 .getSnlBitTerm()
                                 ->getDirection()
                                 .getString()
                                 .c_str());
    // LCOV_EXCL_STOP
#endif
    if (dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
        SNLTerm::Direction::DirectionEnum::Input) {
      if (dnl.getIsoIdfromTermId(term) != DNLID_MAX) {
#ifdef DEBUG_PRINTS
        // LCOV_EXCL_START
        printf("Tracing %s\n", dnl.getDNLTerminalFromID(term)
                                   .getSnlBitTerm()
                                   ->getString()
                                   .c_str());

        // LCOV_EXCL_STOP
#endif
        topOutputIsos.push_back(dnl.getIsoIdfromTermId(term));
      } else {
        printf("No iso %s\n", dnl.getDNLTerminalFromID(term)
                                  .getSnlBitTerm()
                                  ->getString()
                                  .c_str());
      }
    }
  }
  return topOutputIsos;
}

// Giving a DNL and iso, get all isos traced back from this iso to top inputs
std::set<DNLID> LoadlessLogicRemover::getIsoTrace(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
    DNLID iso) {
  std::set<DNLID> isoTrace;
  std::vector<DNLID> isoQueue;
  isoQueue.push_back(iso);
  while (!isoQueue.empty()) {
    DNLID currentIsoID = isoQueue.back();
    isoQueue.pop_back();
    size_t check = isoTrace.size();
    isoTrace.insert(currentIsoID);
    if (isoTrace.size() == check) {
      continue;
    }
    const auto& currentIso =
        dnl.getDNLIsoDB().getIsoFromIsoIDconst(currentIsoID);
    for (const auto& driverID : currentIso.getDrivers()) {
      const auto& termDriver = dnl.getDNLTerminalFromID(driverID);
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("getIsoTrace Driver %s\n",
             termDriver.getSnlBitTerm()->getString().c_str());

      // LCOV_EXCL_STOP
#endif
      const DNLInstanceFull& inst = termDriver.getDNLInstance();
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("getIsoTrace Instance %s\n", inst.getFullPath().c_str());
      // LCOV_EXCL_STOP
#endif
      if (inst.isTop())
        continue;
      assert(inst.isLeaf());
      for (DNLID termId = inst.getTermIndexes().first;
           termId < inst.getTermIndexes().second; termId++) {
        const DNLTerminalFull& term = dnl.getDNLTerminalFromID(termId);
        assert(term.getSnlTerm() != nullptr);
        {
          if (term.getSnlTerm()->getDirection() !=
              SNLTerm::Direction::DirectionEnum::Output) {
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf(" - getIsoTrace Reader %s\n",
                   term.getSnlBitTerm()->getString().c_str());
            // LCOV_EXCL_STOP
#endif
            if (term.getSnlTerm()->getNet() != nullptr) {
#ifdef DEBUG_PRINTS
              // LCOV_EXCL_START
              printf(" - getIsoTrace Net %s\n",
                     term.getSnlTerm()->getNet()->getString().c_str());
              // LCOV_EXCL_STOP
#endif
            }
// const DNLInstanceFull &inst = term.getDNLInstance();
#ifdef DEBUG_PRINTS
            // LCOV_EXCL_START
            printf(" - getIsoTrace Instance %s\n", inst.getFullPath().c_str());
            // LCOV_EXCL_STOP
#endif
            isoQueue.push_back(term.getIsoID());
          }
        }
      }
    }
  }
  return isoTrace;
}

// Given a DNL, use getTopOutputIsos and getIsoTrace to
// trace back all isos from top ouput to top inputs
std::set<DNLID> LoadlessLogicRemover::getTracedIsos(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl) {
  std::vector<DNLID> topOutputIsos = getTopOutputIsos(dnl);
  std::set<DNLID> tracedIsos;
  for (DNLID iso : topOutputIsos) {
    std::set<DNLID> isoTrace = getIsoTrace(dnl, iso);
    tracedIsos.insert(isoTrace.begin(), isoTrace.end());
  }
  return tracedIsos;
}

// Get all isos that are not traced given a DNL and set of traced isos
std::vector<DNLID> LoadlessLogicRemover::getUntracedIsos(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
    const std::set<DNLID>& tracedIsos) {
  std::vector<DNLID> untracedIsos;
  for (DNLID iso = 0; iso < dnl.getDNLIsoDB().getNumIsos(); iso++) {
    if (tracedIsos.find(iso) == tracedIsos.end()) {
      untracedIsos.push_back(iso);
    }
  }
  return untracedIsos;
}

// Given a DNL and set of traced isos, collect all SNL instaces pointers that
// not drive any of them
std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>
LoadlessLogicRemover::getLoadlessInstances(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
    const std::set<DNLID>& tracedIsos) {
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> loadlessInstances;
  for (const auto& leaf : dnl.getLeaves()) {
    const auto& instance = dnl.getDNLInstanceFromID(leaf);
    if (instance.isTop())
      continue;
    if (instance.isNull())
      continue;
    bool isLoadless = true;
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    // printf("Instance %s (model %s)\n",
    // instance.getSNLInstance()->getString().c_str(),
    //   instance.getSNLInstance()->getModel()->getString().c_str());
    // LCOV_EXCL_STOP
#endif
    /*for (auto instTerm : instance.getSNLInstance()->getInstTerms())
    {
      #ifdef DEBUG_PRINTS
// LCOV_EXCL_START
printf("SNL Port %s direction %d\n", instTerm->getString().c_str()
    //  , (int)instTerm->getBitTerm()->getDirection());
    // LCOV_EXCL_STOP
#endif
    }*/
    for (DNLID term = instance.getTermIndexes().first;
         term < instance.getTermIndexes().second; term++) {
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "Port %s direction %d\n",
          dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getString().c_str(),
          (int)dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection());
      // LCOV_EXCL_STOP
#endif
      if (dnl.getIsoIdfromTermId(term) != DNLID_MAX &&
          tracedIsos.find(dnl.getIsoIdfromTermId(term)) != tracedIsos.end() &&
          dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
              SNLTerm::Direction::Input) {
        isLoadless = false;
        break;
      }
    }
    if (isLoadless) {
      std::vector<SNLInstance*> path;
      DNLInstanceFull currentInstance = instance;
      while (currentInstance.isTop() == false) {
        path.push_back(currentInstance.getSNLInstance());
        currentInstance = currentInstance.getParentInstance();
      }
      std::reverse(path.begin(), path.end());
      loadlessInstances.push_back(
          std::pair<std::vector<SNLInstance*>, DNLID>(path, instance.getID()));
    }
  }
  std::sort(loadlessInstances.begin(), loadlessInstances.end(),
            [](const std::pair<std::vector<SNLInstance*>, DNLID>& a,
               const std::pair<std::vector<SNLInstance*>, DNLID>& b) {
              return a.second < b.second;
            });
  return loadlessInstances;
}

std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>
LoadlessLogicRemover::normalizeLoadlessInstancesList(
    const std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>&
        loadlessInstances) {
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> normalizedList;

  for (const auto& path : loadlessInstances) {
    bool isUnder = false;
    for (const auto& pathToCheck : normalizedList) {
      assert(pathToCheck.second <= path.second);
      isUnder = dnl_->isInstanceChild(pathToCheck.second, path.second) ||
                pathToCheck.second == path.second;

      if (isUnder) {
        break;
      }
    }
    if (!isUnder) {
      normalizedList.push_back(path);
    }
  }
  return normalizedList;
}

// Given a list of loadless SNL instacnes, disconnect them and delete them from
// SNL For each instance:
// 1. Iterate over it's terminal and disconnect each one
// 2. Delete the instance
void LoadlessLogicRemover::removeLoadlessInstances(
    SNLDesign* top,
    std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>&
        loadlessInstances) {
  for (auto& path : loadlessInstances) {
    Uniquifier uniquifier(path.first, path.second);
    uniquifier.process();
    for (SNLInstTerm* term : uniquifier.getPathUniq().back()->getInstTerms()) {
      term->setNet(nullptr);
    }
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("deleting path %s\n", uniquifier.getFullPath().c_str());
    // LCOV_EXCL_STOP
#endif
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("deleting model %s\n",
           SNLDB0::isAssign(uniquifier.getPathUniq().back()->getModel())
               ? "true"
               : "false");

    // LCOV_EXCL_STOP
#endif
    uniquifier.getPathUniq().back()->destroy();
  }
  // #ifdef DEBUG_PRINTS
  //  LCOV_EXCL_START
  spdlog::info("Deleted {} leaf instances out of {}", loadlessInstances.size(),
               dnl_->getLeaves().size());
  // LCOV_EXCL_STOP
  /// #endif
}

// Given a DNL, remove all loadless logic
void LoadlessLogicRemover::removeLoadlessLogic() {
  assert(!isCreated());
  dnl_ = DNL::get();
  std::set<DNLID> tracedIsos = getTracedIsos(*dnl_);
  std::vector<DNLID> untracedIsos = getUntracedIsos(*dnl_, tracedIsos);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> loadlessInstances =
      getLoadlessInstances(*dnl_, tracedIsos);
  removeLoadlessInstances(SNLUniverse::get()->getTopDesign(),
                          loadlessInstances);
  DNL::destroy();
}

}  // namespace naja::NAJA_OPT