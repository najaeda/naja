// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace naja::NL {

class NLLibrary;

class SNLSVConstructor {
  public:
    using Paths = std::vector<std::filesystem::path>;
    struct ConstructOptions {
      std::optional<std::filesystem::path> elaboratedASTJsonPath {};
      bool prettyPrintElaboratedASTJson {true};
      bool includeSourceInfoInElaboratedASTJson {true};
    };

    SNLSVConstructor() = delete;
    SNLSVConstructor(const SNLSVConstructor&) = delete;
    SNLSVConstructor(NLLibrary* library);

    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);
    void construct(const Paths& paths, const ConstructOptions& options);
    void construct(const std::filesystem::path& path, const ConstructOptions& options);

  private:
    NLLibrary* library_ {nullptr};
};

}  // namespace naja::NL
