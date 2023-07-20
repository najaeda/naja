// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SNL_CAPNP_H_
#define __SNL_CAPNP_H_

#include <filesystem>

namespace naja { namespace SNL {

class SNLDB;

class SNLCapnP {
  public:
    static constexpr std::string_view InterfaceName = "db_interface.snl";
    static constexpr std::string_view ImplementationName = "db_implementation.snl";
    static void dump(const SNLDB* db, const std::filesystem::path& dumpPath);
    static SNLDB* load(const std::filesystem::path& dumpPath);
    static void dumpInterface(const SNLDB* snlDB, const std::filesystem::path& interfacePath);
    static SNLDB* loadInterface(const std::filesystem::path& interfacePath);
    static void dumpImplementation(const SNLDB* db, const std::filesystem::path& implementationPath);
    static SNLDB* loadImplementation(const std::filesystem::path& implementationPath);
};

}} // namespace SNL // namespace naja

#endif // __SNL_CAPNP_H_
