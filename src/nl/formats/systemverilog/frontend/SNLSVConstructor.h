// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <filesystem>
#include <vector>

namespace naja::NL {

class NLLibrary;

class SNLSVConstructor {
  public:
    using Paths = std::vector<std::filesystem::path>;

    SNLSVConstructor() = delete;
    SNLSVConstructor(const SNLSVConstructor&) = delete;
    SNLSVConstructor(NLLibrary* library);

    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);

  private:
    NLLibrary* library_ {nullptr};
};

}  // namespace naja::NL
