// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#include "SNLXLSConstructor.h"

namespace naja::NL {

/** Read the versioned payload emitted by the separately built XLS bridge. */
class SNLXLSIRReader {
  public:
    static constexpr uint32_t SchemaVersion = 1;
    static constexpr std::string_view XLSRevision =
      "0a7c502ccaaa650cb2da1929c536c4ab100eafda";

    SNLXLSIRReader() = delete;
    static SNLXLSIRFunction load(const std::filesystem::path& path);
};

}  // namespace naja::NL
