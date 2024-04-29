// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DNL.h"
#include "tbb/concurrent_unordered_set.h"

using namespace naja::DNL;
using namespace naja::SNL;

namespace naja::NAJA_OPT {

class LoadlessLogicRemover {
 public:
  LoadlessLogicRemover();
  void process() { removeLoadlessLogic(); }
  std::vector<DNLID> getTopOutputIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  static void getIsoTrace(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      DNLID iso, tbb::concurrent_unordered_set<DNLID>& isoTrace);
  tbb::concurrent_unordered_set<DNLID> getTracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  std::vector<DNLID> getUntracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const tbb::concurrent_unordered_set<DNLID>& tracedIsos);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>> getLoadlessInstances(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const tbb::concurrent_unordered_set<DNLID>& tracedIsos);
  std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>
  normalizeLoadlessInstancesList(
      const std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>&
          loadlessInstances);
  void removeLoadlessInstances(
      SNLDesign* top,
      std::vector<std::pair<std::vector<SNLInstance*>, DNLID>>&
          loadlessInstances);
  void removeLoadlessLogic();

 private:
  naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>* dnl_;
};

}  // namespace naja::NAJA_OPT