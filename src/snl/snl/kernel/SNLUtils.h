// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_UTILS_H_
#define __SNL_UTILS_H_

#include "SNLDesign.h"

namespace naja { namespace SNL {

class SNLLibrary;
class SNLDesign;

class SNLUtils {
  public:
    using DesignsLevel = std::map<const SNLDesign*, unsigned, SNLDesign::PointerLess>;
    static unsigned levelize(const SNLDesign* design, DesignsLevel& designsLevel);
    using DesignLevel = std::pair<const SNLDesign*, unsigned>;
    using SortedDesigns = std::vector<DesignLevel>;
    static void getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns);
    static SNLID::Bit getSize(int msb, int lsb);
    static SNLDesign* findTop(const SNLLibrary* library);
};

}} // namespace SNL // namespace naja

#endif // __SNL_UTILS_H_
