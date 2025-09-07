// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "LogicLevelComputer.h"
#include <queue>
#include <set>

#include <tbb/combinable.h>
#include <tbb/concurrent_map.h>
#include <tbb/parallel_for_each.h>
#include "SNLDesignModeling.h"

// Debug prints
// #define DEBUG_PRINTS

using namespace naja::NAJA_METRICS;
using namespace naja::DNL;

void LogicLevelComputer::process() {
  // 1) Build the set of starting terms exactly as before
  std::set<DNLID> termsToTrace;
  // add top output terms
  auto topDesign = dnl_->getTopDesign();
  for (const auto& term : topDesign->getBitTerms()) {
    if (term->getDirection() != SNLTerm::Direction::Input) {
      termsToTrace.insert(term->getID());
    }
  }
#ifdef DEBUG_PRINTS
  printf("Logic Level Computer: %zu top level terms to trace\n",
         termsToTrace.size());
  printf("number of leaves: %zu\n", dnl_->getLeaves().size());
#endif
  for (DNLID leaf : dnl_->getLeaves()) {
    const auto& instance = dnl_->getDNLInstanceFromID(leaf);
    for (DNLID term = instance.getTermIndexes().first;
         term != DNLID_MAX && term <= instance.getTermIndexes().second;
         ++term) {
      if (dnl_->getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
          SNLTerm::Direction::Output) {
        if (!dnl_->getDNLTerminalFromID(term).isSequential()) {
          continue;  // Skip non sequential terms
        }
        termsToTrace.insert(term);
      }
    }
  }
#ifdef DEBUG_PRINTS
  printf("Logic Level Computer: %zu terms to trace\n", termsToTrace.size());
#endif
  // 2) Prepare a combinable to hold each thread's local max
  tbb::enumerable_thread_specific<size_t> localMaxLogicLevel;

  tbb::concurrent_map<size_t, size_t> dnlID2ll;
  // 3) Parallel BFS starting from each term, updating only thread-local max
  tbb::parallel_for_each(
      termsToTrace.begin(), termsToTrace.end(), [&](DNLID term) {
        // for (DNLID term : termsToTrace) {
        //  Reference to this thread's local maximum
        size_t& localMax = localMaxLogicLevel.local();

        std::queue<std::pair<DNLID, std::vector<DNLID>>> queue;
        queue.push({term, std::vector<DNLID>()});
        while (!queue.empty()) {
          auto [currentTerm, path] = queue.front();
          queue.pop();
          // Update thread-local max only
          if (path.size() > localMax) {
#ifdef DEBUG_PRINTS
            printf("Term %s instance %s at level %zu\n",
                   dnl_->getDNLTerminalFromID(currentTerm)
                       .getSnlBitTerm()
                       ->getName()
                       .getString()
                       .c_str(),
                   dnl_->getDNLTerminalFromID(currentTerm)
                       .getDNLInstance()
                       .getSNLInstance()
                       ->getName()
                       .getString()
                       .c_str(),
                   path.size());
#endif
            localMax = path.size();
          }
          const auto& iso = dnl_->getDNLIsoDB().getIsoFromIsoIDconst(
              dnl_->getDNLTerminalFromID(currentTerm).getIsoID());

          for (const auto& driver : iso.getDrivers()) {
            const DNLInstanceFull& driverInstance =
                dnl_->getDNLTerminalFromID(driver).getDNLInstance();
            if (dnl_->getDNLTerminalFromID(driver).isSequential() ||
                dnl_->getDNLTerminalFromID(driver).isTopPort()) {
              continue;
            }
            if (dnlID2ll.find(driver) != dnlID2ll.end() &&
                dnlID2ll[driver] > path.size()) {
              // LCOV_EXCL_START
              continue;
              // LCOV_EXCL_STOP
            }
            dnlID2ll[driver] = path.size();
            auto CombinatorialInputsCollection =
                SNLDesignModeling::getCombinatorialInputs(
                    dnl_->getDNLTerminalFromID(driver).getSnlBitTerm());
            std::set<SNLBitTerm*> CombinatorialInputs(
                CombinatorialInputsCollection.begin(),
                CombinatorialInputsCollection.end());
            for (DNLID termId = driverInstance.getTermIndexes().first;
                 termId != DNLID_MAX &&
                 termId <= driverInstance.getTermIndexes().second;
                 ++termId) {
              const auto& driverInputTerm = dnl_->getDNLTerminalFromID(termId);
              // Only consider combinatorial outputs
              if (CombinatorialInputs.find(driverInputTerm.getSnlBitTerm()) ==
                  CombinatorialInputs.end()) {
                continue;
              }
              if (driverInputTerm.getSnlBitTerm()->getDirection() !=
                  SNLTerm::Direction::Output) {
                if (std::find(path.begin(), path.end(), termId) != path.end()) {
                  continue;  // Avoid cycles
                }
                auto newPath = path;
                newPath.push_back(termId);
                queue.push({termId, newPath});
#ifdef DEBUG_PRINTS
                printf(
                    "From driver %s of instance  %s  enqueuing term %s of "
                    "instance %s with logic level %zu\n",
                    dnl_->getDNLTerminalFromID(driver)
                        .getSnlBitTerm()
                        ->getName()
                        .getString()
                        .c_str(),
                    driverInstance.getSNLInstance()
                        ->getName()
                        .getString()
                        .c_str(),
                    driverInputTerm.getSnlBitTerm()
                        ->getName()
                        .getString()
                        .c_str(),
                    driverInstance.getSNLInstance()
                        ->getName()
                        .getString()
                        .c_str(),
                    path.size() + 1);
#endif
              }
            }
          }
        }
      });

  // 4) Combine all thread-local maxima once, store in member
  for (const auto& localMax : localMaxLogicLevel) {
    if (localMax > maxLogicLevel_) {
      maxLogicLevel_ = localMax;
    }
  }
}
