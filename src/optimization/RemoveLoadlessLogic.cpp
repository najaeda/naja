// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "RemoveLoadlessLogic.h"

#include <vector>
#include <sstream>

#include "tbb/enumerable_thread_specific.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusNetBit.h"
#include "SNLUniquifier.h"

#include "BNE.h"

#include "debug/getTopOutputIsos.hpp"

using namespace naja::DNL;
using namespace naja::NL;
using namespace naja::BNE;

//#define DEBUG_PRINTS

using namespace naja::NAJA_OPT;

// Constructor
LoadlessLogicRemover::LoadlessLogicRemover() {}

// Given a DNL, getting all isos connected to top output.
std::vector<DNLID> LoadlessLogicRemover::getTopOutputIsos(const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl)
{
  std::vector<DNLID> topOutputIsos;
  for (DNLID term = DNL_GET_FIRST_TERM;
       term <= DNL_GET_SECOND_TERM
	 && term != DNLID_MAX;
       term++)
    {
      assert(DNLID_MAX != term);
      LCOV_EXCL_DIRECTION("Checking %s\n", term, dnl);
      if (DNL_BIT_IS_INPUT)
	{
	  if (!DNL_IS_AT_MAX)
	    {
	      LCOV_EXCL("Tracing %s\n", term, dnl);
	      topOutputIsos.push_back(dnl.getIsoIdfromTermId(term));
	    }
	  else
	    LCOV_EXCL("No iso %s\n", term, dnl);
	}
    }
  return topOutputIsos;
}

// Giving a DNL and iso, get all isos traced back from this iso to top inputs
void LoadlessLogicRemover::getIsoTrace(const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
				       DNLID iso,
				       tbb::concurrent_unordered_set<DNLID,
				       std::hash<DNLID>,
				       std::equal_to<DNLID>,
				       tbb::scalable_allocator<DNLID>>& isoTrace)
{
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
           termId <= inst.getTermIndexes().second; termId++) {
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
            if (term.getSnlTerm()->getNet() != nullptr) {
              // LCOV_EXCL_START
              printf(" - getIsoTrace Net %s\n",
                     term.getSnlTerm()->getNet()->getString().c_str());
              // LCOV_EXCL_STOP
            }
#endif
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
}

// Given a DNL, use getTopOutputIsos and getIsoTrace to
// trace back all isos from top ouput to top inputs
tbb::concurrent_unordered_set<DNLID> LoadlessLogicRemover::getTracedIsos(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl) {
  std::vector<DNLID> topOutputIsos = getTopOutputIsos(dnl);
  for (DNLID leaf : dnl.getLeaves()) {
    size_t num_outputs = 0;
    const auto& instance = dnl.getDNLInstanceFromID(leaf);
    for (DNLID term = instance.getTermIndexes().first;
        term != DNLID_MAX and term <= instance.getTermIndexes().second;
        term++) {
      if (dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
          SNLTerm::Direction::Input) {
        num_outputs++;
      }
    }
    if (num_outputs == 0) {
      for (DNLID term = instance.getTermIndexes().first;
          term != DNLID_MAX and term <= instance.getTermIndexes().second;
          term++) {
        if (dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
            SNLTerm::Direction::Output && dnl.getIsoIdfromTermId(term) != DNLID_MAX) {
          topOutputIsos.push_back(dnl.getIsoIdfromTermId(term));
        }
      }
    }
  }
  tbb::concurrent_unordered_set<DNLID> result;
  tbb::enumerable_thread_specific<tbb::concurrent_unordered_set<
      DNLID, std::hash<DNLID>, std::equal_to<DNLID>,
      tbb::scalable_allocator<DNLID>>>
      tracedIsos;
  if (!getenv("NON_MT")) {
    tbb::task_arena arena(tbb::task_arena::automatic);
    tbb::parallel_for(tbb::blocked_range<DNLID>(0, topOutputIsos.size()),
                      [&](const tbb::blocked_range<DNLID>& r) {
                        for (DNLID i = r.begin(); i < r.end(); ++i) {
                          getIsoTrace(dnl, topOutputIsos[i],
                                      tracedIsos.local());
                        }
                      });
  } else {
    for (DNLID iso : topOutputIsos) {
      getIsoTrace(dnl, iso, tracedIsos.local());
    }
  }
  for (auto tracedIsosPartial : tracedIsos) {
    result.insert(tracedIsosPartial.begin(), tracedIsosPartial.end());
  }
  return result;
}

// Get all isos that are not traced given a DNL and set of traced isos
std::vector<DNLID> LoadlessLogicRemover::getUntracedIsos(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
    const tbb::concurrent_unordered_set<DNLID>& tracedIsos) {
  std::vector<DNLID> untracedIsos;
  // print traced isos
  for (DNLID iso = 0; iso < dnl.getDNLIsoDB().getNumIsos(); iso++) {
    if (tracedIsos.find(iso) == tracedIsos.end()) {
      untracedIsos.push_back(iso);
    }
  }
  return untracedIsos;
}

// Given a DNL and set of traced isos, collect all SNL instaces pointers that
// not drive any of them
std::vector<std::pair<std::vector<NLID::DesignObjectID>, DNLID>>
LoadlessLogicRemover::getLoadlessInstances(
    const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
    const tbb::concurrent_unordered_set<DNLID>& tracedIsos) {
  std::vector<std::pair<std::vector<NLID::DesignObjectID>, DNLID>>
      loadlessInstances;
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
    size_t output = 0;
    for (DNLID term = instance.getTermIndexes().first;
         term != DNLID_MAX and term <= instance.getTermIndexes().second; term++) {
      assert(DNLID_MAX != term);
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf(
          "Port %s direction %d\n",
          dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getString().c_str(),
          (int)dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection());
      // LCOV_EXCL_STOP
#endif
      if (dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
          SNLTerm::Direction::Input) {
        output++;
      }
      if (dnl.getIsoIdfromTermId(term) != DNLID_MAX &&
          tracedIsos.find(dnl.getIsoIdfromTermId(term)) != tracedIsos.end() &&
          dnl.getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
              SNLTerm::Direction::Input) {
        isLoadless = false;
        break;
      }
    }
    if (output == 0) {
      isLoadless = false;
    }
    if (isLoadless) {
      std::vector<NLID::DesignObjectID> path;
      DNLInstanceFull currentInstance = instance;
      while (currentInstance.isTop() == false) {
        path.push_back(currentInstance.getSNLInstance()->getID());
        currentInstance = currentInstance.getParentInstance();
      }
      std::reverse(path.begin(), path.end());
      loadlessInstances.push_back(
          std::pair<std::vector<NLID::DesignObjectID>, DNLID>(
              path, instance.getID()));
    }
  }
  std::sort(loadlessInstances.begin(), loadlessInstances.end(),
            [](const std::pair<std::vector<NLID::DesignObjectID>, DNLID>& a,
               const std::pair<std::vector<NLID::DesignObjectID>, DNLID>& b) {
              return a.second < b.second;
            });
  return loadlessInstances;
}

// Given a list of loadless SNL instacnes, disconnect them and delete them from
// SNL For each instance:
// 1. Iterate over it's terminal and disconnect each one
// 2. Delete the instance
void LoadlessLogicRemover::removeLoadlessInstances(
    SNLDesign* top,
    std::vector<std::pair<std::vector<NLID::DesignObjectID>, DNLID>>&
        loadlessInstances) {
  BNE::BNE bne;
  for (auto& path : loadlessInstances) {
    if (!normalizedUniquification_) {
      SNLUniquifier uniquifier(path.first, path.second);
      uniquifier.process();
      for (SNLInstTerm* term : uniquifier.getPathUniq().back()->getInstTerms()) {
        auto net = term->getNet();
        term->setNet(nullptr);
        if (net != nullptr) {
          if (net->getInstTerms().size() + net->getBitTerms().size() == 0) {
            net->destroy();
          }
        }
      }
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("deleting path %s\n", uniquifier.getFullPath().c_str());
      // LCOV_EXCL_STOP
#endif
#ifdef DEBUG_PRINTS
      // LCOV_EXCL_START
      printf("deleting model %s\n",
             NLDB0::isAssign(uniquifier.getPathUniq().back()->getModel())
                 ? "true"
                 : "false");

    // LCOV_EXCL_STOP
#endif
      uniquifier.getPathUniq().back()->destroy();
    } else {
      bne.addDeleteAction(path.first);
    }
  }
  if (normalizedUniquification_) {
    bne.process();
  }
  // #ifdef DEBUG_PRINTS
  //  LCOV_EXCL_START
  std::cout << "Deleted " << loadlessInstances.size() << " leaf instances out of " << dnl_->getLeaves().size() << std::endl;
  // LCOV_EXCL_STOP
  /// #endif
}

// Given a DNL, remove all loadless logic
void LoadlessLogicRemover::removeLoadlessLogic() {
  dnl_ = DNL::get();
  tbb::concurrent_unordered_set<DNLID> tracedIsos = getTracedIsos(*dnl_);
  std::vector<DNLID> untracedIsos = getUntracedIsos(*dnl_, tracedIsos);
  loadlessInstances_ = getLoadlessInstances(*dnl_, tracedIsos);
  report_ = collectStatistics();
  removeLoadlessInstances(NLUniverse::get()->getTopDesign(),
                          loadlessInstances_);
  //spdlog::info(report_);
  DNL::destroy();
}

std::string LoadlessLogicRemover::collectStatistics() const {
  /*std::stringstream ss;
  std::map<std::string, size_t> deletedInstances;
  for (const auto& entry : loadlessInstances_) {
    deletedInstances[entry.first.back()->getModel()->getName().getString()]++;
  }
  ss << "DLE report:" << std::endl;
  for (const auto& entry : deletedInstances) {
    ss << entry.first << " : " << entry.second << std::endl;
  }
  return ss.str();*/
  return std::string();
}
