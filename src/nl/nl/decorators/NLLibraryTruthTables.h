// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <vector>
#include <map>
#include "SNLTruthTable.h"

namespace naja::NL {

class NLLibrary;
class SNLDesign;

class NLLibraryTruthTables {
    public:
      using Primitives = std::vector<SNLDesign*>;
      using LibraryTruthTables = std::map<SNLTruthTable, Primitives>;
      static LibraryTruthTables construct(NLLibrary* library);
      static LibraryTruthTables getTruthTables(const NLLibrary* library);
      using Indexes = std::vector<uint32_t>;
      static std::pair<SNLDesign*, Indexes> getDesignForTruthTable(const NLLibrary* library, const SNLTruthTable& tt);
};

}  // namespace naja::NL