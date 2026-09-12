// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaSlangRuntime.h"

#include <string>

#include "slang/util/VersionInfo.h"

namespace {

const unsigned char kNajaSlangRuntimeIdentity = 0;

const std::string& getBuildID() {
  static const std::string buildID =
    "naja-slang-native-v1:" + slang::VersionInfo::getVersionString();
  return buildID;
}

}  // namespace

extern "C" const void* NajaSlang_GetRuntimeIdentity(void) {
  return &kNajaSlangRuntimeIdentity;
}

extern "C" const char* NajaSlang_GetNativeBuildID(void) {
  return getBuildID().c_str();
}
