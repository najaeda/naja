// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_FIF_LOGICAL_NETLIST_H_
#define __SNL_FIF_LOGICAL_NETLIST_H_

#include <filesystem>

namespace naja { namespace SNL {

class SNLDB;

class SNLFIFLogicalNetlist {
  public:
    static SNLDB* load(const std::filesystem::path& path);
};

}} // namespace SNL // namespace naja

#endif // __SNL_FIF_LOGICAL_NETLIST_H_