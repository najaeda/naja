// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <DNL.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/scalable_allocator.h>

using namespace naja::DNL;

namespace naja::NAJA_OPT {

class IsoTracer {
 public:
  static std::vector<DNLID> getTopOutputIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  
  static void getIsoTrace(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      DNLID iso, tbb::concurrent_unordered_set<DNLID, std::hash<DNLID>, std::equal_to<DNLID>, tbb::scalable_allocator<DNLID>>& isoTrace);
  
  static tbb::concurrent_unordered_set<DNLID> getTracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl);
  
  static std::vector<DNLID> getUntracedIsos(
      const naja::DNL::DNL<DNLInstanceFull, DNLTerminalFull>& dnl,
      const tbb::concurrent_unordered_set<DNLID>& tracedIsos);
};

}  // namespace naja::NAJA_OPT