// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include "DNL.h"
#include "tbb/concurrent_unordered_set.h"
#include "tbb/scalable_allocator.h"

using namespace naja::DNL;
using namespace naja::NL;

namespace naja::NAJA_OPT {

class LoadlessLogicRemover {
 public:
  LoadlessLogicRemover();
  void process() { removeLoadlessLogic(); }
  std::vector<DNLID> getTopOutputIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  static void getIsoTrace(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      DNLID iso, tbb::concurrent_unordered_set<DNLID, std::hash<DNLID>, std::equal_to<DNLID>, tbb::scalable_allocator<DNLID>>& isoTrace);
  tbb::concurrent_unordered_set<DNLID> getTracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  std::vector<DNLID> getUntracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const tbb::concurrent_unordered_set<DNLID>& tracedIsos);
  std::vector<std::pair<std::vector<NLID::DesignObjectID>, DNLID>> getLoadlessInstances(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const tbb::concurrent_unordered_set<DNLID>& tracedIsos);
  void removeLoadlessInstances(
      SNLDesign* top,
      std::vector<std::pair<std::vector<NLID::DesignObjectID>, DNLID>>&
          loadlessInstances);
  void removeLoadlessLogic();
  std::string collectStatistics() const;
  std::string getReport() const { return report_; }
  void setNormalizedUniquification(bool normalizedUniquification) {
    normalizedUniquification_ = normalizedUniquification;
  }
 private:
  naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>* dnl_;
  std::vector<std::pair<std::vector<NLID::DesignObjectID>, DNLID>> loadlessInstances_;
  std::string report_;
  bool normalizedUniquification_ = true;
};

}  // namespace naja::NAJA_OPT
