// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_TRUTH_TABLE_H_
#define __SNL_DESIGN_TRUTH_TABLE_H_

#include "SNLTruthTable.h"

namespace naja { namespace SNL {

class SNLDesign;

class SNLDesignTruthTable {
  public:
    static void setTruthTable(SNLDesign* design, const SNLTruthTable& truthTable);
    static SNLTruthTable getTruthTable(const SNLDesign* design);
    static bool isConst0(const SNLDesign* design);
    static bool isConst1(const SNLDesign* design);
    static bool isConst(const SNLDesign* design);
    static bool isInv(const SNLDesign* design);
    static bool isBuf(const SNLDesign* design);
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_H_