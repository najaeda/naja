// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <set>
#include <vector>
#include "DNL.h"
#include "SNLTruthTable.h"
#include "SNLLibraryTruthTables.h"

using namespace naja::DNL;
using namespace naja::SNL;

class ReductionOptimization {
 public:
  ReductionOptimization(const std::vector<std::tuple<std::vector<SNLInstance*>,
                         std::vector<std::pair<SNLInstTerm*, int>>,
                         DNLID>>& partialConstantReaders);
  void run();

 private:
  void reducPartialConstantInstance(std::tuple<std::vector<SNLInstance*>,
                     std::vector<std::pair<SNLInstTerm*, int>>,
                     DNLID>& candidate);
  SNLTruthTable reduceTruthTable(
    const SNLTruthTable& truthTable,
    const std::vector<std::pair<SNLInstTerm*, int>>& constTerms);
  void replaceInstance(SNLInstance* instance, const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result);
  std::vector<std::tuple<std::vector<SNLInstance*>,
                         std::vector<std::pair<SNLInstTerm*, int>>,
                         DNLID>>
      partialConstantReaders_;
  DNLFull* dnl_ = nullptr;
};