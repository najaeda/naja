// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __FIF_PHYSICAL_NETLIST_H_
#define __FIF_PHYSICAL_NETLIST_H_

#include <filesystem>

namespace naja {

class FIFPhysicalNetlist {
  public:
    static void load(const std::filesystem::path& path);
};

} // namespace naja

#endif // __FIF_PHYSICAL_NETLIST_H_