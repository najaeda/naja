// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <SNLTruthTable.h>
#include <SNLInstance.h>
#include <NLID.h>

using namespace naja::NL;

namespace naja::NAJA_OPT {

class TruthTableReducer {
 public:
  static SNLTruthTable reduceTruthTable(SNLInstance* uniquifiedCandidate,
    const SNLTruthTable& truthTable,
    const std::vector<std::pair<NLID::DesignObjectID, int>>& constTerms);
};

}  // namespace naja::NAJA_OPT