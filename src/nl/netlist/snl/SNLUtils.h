// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>

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
    static void prepareForConcurrentAccess(const SNLDesign* design);
    static std::string getUnnamedIDString(const char* tag, std::uint64_t id);
    static std::string getNamedString(const NLName& name, const char* tag, std::uint64_t id);
    static std::string getBusNamedString(
        const NLName& name,
        const char* tag,
        std::uint64_t id,
        NLID::Bit msb,
        NLID::Bit lsb);
    static std::string getBitNamedString(
        const NLName& name,
        const char* tag,
        std::uint64_t id,
        NLID::Bit bit);
};

} // namespace naja::NL
