// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBERTY_CONSTRUCTOR_H_
#define __SNL_LIBERTY_CONSTRUCTOR_H_

#include <filesystem>

namespace naja { namespace SNL {

class SNLLibrary;

class SNLLibertyConstructor {
  public:
    SNLLibertyConstructor() = delete;
    SNLLibertyConstructor(const SNLLibertyConstructor&) = delete;
    SNLLibertyConstructor(SNLLibrary* library);

    void construct(const std::filesystem::path& path);
  private:
    SNLLibrary* library_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_LIBERTY_CONSTRUCTOR_H_
