// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "DNL.h"

namespace naja::NAJA_METRICS {

class LogicLevelComputer {
 public:
  LogicLevelComputer() {
    naja::DNL::destroy();
    dnl_ = naja::DNL::get();
  }
  void process();
  size_t getMaxLogicLevel() const { return maxLogicLevel_; }
  const std::vector<std::vector<std::pair<naja::DNL::DNLID, naja::DNL::DNLID>>>& getMaxLogicLevelPaths() const { return logicLevels_; }

 private:
  size_t maxLogicLevel_ = 0;
  naja::DNL::DNLFull* dnl_ = nullptr;
  std::vector<std::vector<std::pair<naja::DNL::DNLID, naja::DNL::DNLID>>> logicLevels_;
};

}  // namespace naja::NAJA_METRICS
