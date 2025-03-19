// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PYLOADER_H_
#define __SNL_PYLOADER_H_

#include <filesystem>

namespace naja { namespace NL {

class NLDB;
class NLLibrary;
class SNLDesign;

class SNLPyLoader {
  public:
    static void loadDB(
      NLDB* db,
      const std::filesystem::path& scriptPath);
    static void loadLibrary(
      NLLibrary* library,
      const std::filesystem::path& scriptPath,
      bool loadPrimitives=false);
    static void loadDesign(
      SNLDesign* design,
      const std::filesystem::path& scriptPath);
    static void loadPrimitives(
      NLLibrary* library,
      const std::filesystem::path& scriptPath);
};

}} // namespace NL // namespace naja

#endif // __SNL_PYLOADER_H_
