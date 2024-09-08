// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <set>
#include <vector>
#include "DNL.h"
#include "SNLTruthTable.h"
#include "SNLLibraryTruthTables.h"
#include "bne.h"
using namespace naja::DNL;
using namespace naja::SNL;

class ReductionOptimization {
 public:
  ReductionOptimization(const std::vector<std::tuple<std::vector<SNLID::DesignObjectID>,
                         std::vector<std::pair<SNLID::DesignObjectID, int>>,
                         DNLID>>& partialConstantReaders);
  static SNLTruthTable reduceTruthTable(SNLInstance* uniquifiedCandidate,
    const SNLTruthTable& truthTable,
    const std::vector<std::pair<SNLID::DesignObjectID, int>>& constTerms);
  void run();

 private:
  void reducPartialConstantInstance(std::tuple<std::vector<SNLID::DesignObjectID>,
                     std::vector<std::pair<SNLID::DesignObjectID, int>>,
                     DNLID>& candidate);
  void replaceInstance(SNLInstance* instance, const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result);
  std::string collectStatistics() const;
  std::vector<std::tuple<std::vector<SNLID::DesignObjectID>,
                         std::vector<std::pair<SNLID::DesignObjectID, int>>,
                         DNLID>>
      partialConstantReaders_;
  std::map<std::pair<std::string, std::string>, size_t> reductionStatistics_;
  std::string report_;
  BNE bne_;
};