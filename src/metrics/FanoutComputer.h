// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"

namespace naja::NAJA_METRICS {

class FanoutComputer {
 public:
  FanoutComputer() {
    naja::DNL::destroy();
    dnl_ = naja::DNL::get();
  }
  void process();
  size_t getMaxFanout() const { return maxFanout_; }
  const std::vector<std::pair<naja::DNL::DNLID, std::vector<naja::DNL::DNLID>>>& getMaxFanoutTerms() const { return fanouts_; }

 private:
  std::vector<std::pair<naja::DNL::DNLID, std::vector<naja::DNL::DNLID>>> fanouts_; 
  size_t maxFanout_ = 0;
  naja::DNL::DNLFull* dnl_ = nullptr;
};

}  // namespace naja::NAJA_METRICS
