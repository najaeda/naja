// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "SNLDesign.h"

namespace naja::NL {

class NLLibrary;
class SNLDesign;

class SNLUtils {
  public:
    using DesignsLevel = std::map<const SNLDesign*, unsigned, SNLDesign::PointerLess>;
    static unsigned levelize(const SNLDesign* design, DesignsLevel& designsLevel);
    using DesignLevel = std::pair<const SNLDesign*, unsigned>;
    using SortedDesigns = std::vector<DesignLevel>;
    static void getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns);
    static NLID::Bit getWidth(int msb, int lsb);
    static SNLDesign* findTop(const NLLibrary* library);
};

} // namespace naja::NL