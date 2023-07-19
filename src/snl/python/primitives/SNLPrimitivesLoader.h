// Copyright 2023 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PRIMITIVES_LOADER_H_
#define __SNL_PRIMITIVES_LOADER_H_

#include <filesystem>

namespace naja { namespace SNL {

class SNLLibrary;

class SNLPrimitivesLoader {
  public:
    static void load(
      SNLLibrary* library,
      const std::filesystem::path& primitivesPath);
};

}} // namespace SNL // namespace naja

#endif // __SNL_PRIMITIVES_LOADER_H_