// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <filesystem>

namespace naja::NL {

class NLLibrary;

class SNLLibertyConstructor {
  public:
    SNLLibertyConstructor() = delete;
    SNLLibertyConstructor(const SNLLibertyConstructor&) = delete;
    SNLLibertyConstructor(NLLibrary* library);

    void construct(const std::filesystem::path& path);
  private:
    NLLibrary*  library_;
};

} // namespace naja::NL
