// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBRARY_TRUTH_TABLES_H_
#define __SNL_LIBRARY_TRUTH_TABLES_H_

#include <map>
#include "SNLTruthTable.h"

namespace naja { namespace SNL {

class SNLLibrary;
class SNLDesign;

class SNLLibraryTruthTables {
    public:
      using TruthTables = std::map<SNLTruthTable, SNLDesign*>;
      static TruthTables construct(SNLLibrary* library);
      static SNLDesign* getDesignForTruthTable(const TruthTables& truthTables, const SNLTruthTable& tt);
};

}} // namespace SNL // namespace naja

#endif // __SNL_LIBRARY_TRUTH_TABLES_H_