// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include <vector>

#include "BNE.h"

#include "SNLTruthTable.h"
#include "SNLLibraryTruthTables.h"

#include "DNL.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::BNE;

namespace naja::NAJA_OPT {

class ReductionOptimization {
 public:
  ReductionOptimization(const std::vector<std::tuple<std::vector<SNLID::DesignObjectID>,
                         std::vector<std::pair<SNLID::DesignObjectID, int>>,
                         DNLID>>& partialConstantReaders);
  static SNLTruthTable reduceTruthTable(SNLInstance* uniquifiedCandidate,
    const SNLTruthTable& truthTable,
    const std::vector<std::pair<SNLID::DesignObjectID, int>>& constTerms);
  void run();
  std::string collectStatistics() const;
  void setNormalizedUniquification(bool normalizedUniquification) {
    normalizedUniquification_ = normalizedUniquification;
  }
 private:
  void replaceInstance(SNLInstance* instance, const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result);
  void reducPartialConstantInstanceWithNormalizedUniquification(std::tuple<std::vector<SNLID::DesignObjectID>,
                     std::vector<std::pair<SNLID::DesignObjectID, int>>,
                     DNLID>& candidate);
  void reducPartialConstantInstance(std::tuple<std::vector<SNLID::DesignObjectID>,
                     std::vector<std::pair<SNLID::DesignObjectID, int>>,
                     DNLID>& candidate);
  std::vector<std::tuple<std::vector<SNLID::DesignObjectID>,
                         std::vector<std::pair<SNLID::DesignObjectID, int>>,
                         DNLID>>
      partialConstantReaders_;
  std::map<std::pair<std::string, std::string>, size_t> reductionStatistics_;
  std::string report_;
  BNE::BNE bne_;
  bool normalizedUniquification_ = true;
};

}  // namespace naja::NAJA_OPT
