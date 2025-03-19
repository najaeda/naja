// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DUMP_MANIFEST_H_
#define __SNL_DUMP_MANIFEST_H_

#include <filesystem>
#include "SNLDump.h"

namespace naja { namespace SNL {

class SNLDumpManifest {
  public:
    static constexpr std::string_view ManifestFileName = "snl.mf";
    SNLDump::Version getVersion() const { return version_; }
    static SNLDumpManifest load(const std::filesystem::path& snlDir);
    static void dump(const std::filesystem::path& snlDir);
  private:
    SNLDumpManifest(): version_(0, 0, 0) {}
    SNLDump::Version  version_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_DUMP_MANIFEST_H_