// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <filesystem>
#include "SNLDump.h"

namespace naja::NL {

class SNLDumpManifest {
  public:
    static constexpr std::string_view ManifestFileName = "snl.mf";
    SNLDump::Version getVersion() const { return version_; }
    SNLDump::Version getSchemaVersion() const { return version_; }
    std::string getProducerVersion() const { return producerVersion_; }
    std::string getProducerGitHash() const { return producerGitHash_; }
    static SNLDumpManifest load(const std::filesystem::path& snlDir);
    static void dump(const std::filesystem::path& snlDir);
  private:
    SNLDumpManifest(): version_(0, 0, 0) {}
    SNLDump::Version  version_;
    std::string       producerVersion_;
    std::string       producerGitHash_;
};

}  // namespace naja::NL
