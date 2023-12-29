// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PYLOADER_H_
#define __SNL_PYLOADER_H_

#include <filesystem>

namespace naja { namespace SNL {

class SNLDB;
class SNLLibrary;

class SNLPyLoader {
  public:
    static void loadDB(
      SNLDB* db,
      const std::filesystem::path& scriptPath);
    static void loadLibrary(
      SNLLibrary* library,
      const std::filesystem::path& scriptPath,
      bool loadPrimitives=false);
    static void loadPrimitives(
      SNLLibrary* library,
      const std::filesystem::path& scriptPath);
};

}} // namespace SNL // namespace naja

#endif // __SNL_PYLOADER_H_