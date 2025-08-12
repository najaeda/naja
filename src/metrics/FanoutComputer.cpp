// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "FanoutComputer.h"
#include <set>
#include <vector>

#include <tbb/combinable.h>
#include <tbb/parallel_for_each.h>

using namespace naja::NAJA_METRICS;
using namespace naja::DNL;

void FanoutComputer::process() {
  // 1) Collect all terms whose direction != Input
  std::vector<DNLID> termsToTrace;
  termsToTrace.reserve(dnl_->getLeaves().size());

  for (DNLID leaf : dnl_->getLeaves()) {
    const auto& instance = dnl_->getDNLInstanceFromID(leaf);
    for (DNLID term = instance.getTermIndexes().first;
         term != DNLID_MAX && term <= instance.getTermIndexes().second;
         ++term) {
      if (dnl_->getDNLTerminalFromID(term).getSnlBitTerm()->getDirection() !=
          SNLTerm::Direction::Input) {
        termsToTrace.push_back(term);
      }
    }
  }

  // 2) Per-thread holder for the maximum fanout seen locally
  tbb::enumerable_thread_specific<size_t> localMaxFanout;

  // 3) Parallel scan over each term, updating only thread-local max
  tbb::parallel_for_each(
      termsToTrace.begin(), termsToTrace.end(), [&](DNLID term) {
        // Compute this term's fanout
        size_t fanout = dnl_->getDNLIsoDB()
                            .getIsoFromIsoIDconst(
                                dnl_->getDNLTerminalFromID(term).getIsoID())
                            .getReaders()
                            .size();

        // Update local max if this fanout is larger
        if (fanout > localMaxFanout.local()) {
          localMaxFanout.local() = fanout;
        }
      });

  // 4) Single reduction of all local maxima into the class member
  for (const auto& localMax : localMaxFanout) {
    if (localMax > maxFanout_) {
      maxFanout_ = localMax;
    }
  }
}
