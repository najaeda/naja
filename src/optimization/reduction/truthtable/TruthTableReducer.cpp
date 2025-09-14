// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <TruthTableReducer.h>
#include <SNLInstTerm.h>
#include <SNLBitTerm.h>

using namespace naja::NL;
using namespace naja::NL;
using namespace naja::NAJA_OPT;

SNLTruthTable TruthTableReducer::reduceTruthTable(
    SNLInstance* uniquifiedCandidate,
    const SNLTruthTable& truthTable,
    const std::vector<std::pair<NLID::DesignObjectID, int>>& constTerms) {
  assert(constTerms.size() <= truthTable.size());
  std::map<size_t, size_t> termID2index;
  size_t index = 0;
  using ConstantInput = std::pair<uint32_t, bool>;
  using ConstantInputs = std::vector<ConstantInput>;
  ConstantInputs constInputs;
  for (auto term : uniquifiedCandidate->getInstTerms()) {
    if (term->getDirection() != SNLInstTerm::Direction::Input) {
      continue;
    }
    termID2index[term->getBitTerm()->getID()] = index;
    index++;
  }
  for (auto& constTerm : constTerms) {
    constInputs.push_back(
        ConstantInput(termID2index[constTerm.first], constTerm.second));
  }
  return truthTable.getReducedWithConstants(constInputs);
}