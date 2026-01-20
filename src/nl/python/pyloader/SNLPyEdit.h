// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <filesystem>

namespace naja::NL {

class SNLPyEdit {
  public:
    static void edit(const std::filesystem::path& scriptPath);
};

}  // namespace naja::NL