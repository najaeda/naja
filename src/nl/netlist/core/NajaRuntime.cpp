// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaRuntime.h"

#include <string>

#include "NajaVersion.h"

namespace {

const unsigned char kNajaRuntimeIdentity = 0;

const std::string& getBuildID() {
  static const std::string buildID =
    "naja-native-v1:" + naja::NAJA_VERSION + ":" + naja::NAJA_GIT_HASH;
  return buildID;
}

}  // namespace

extern "C" const void* Naja_GetRuntimeIdentity(void) {
  return &kNajaRuntimeIdentity;
}

extern "C" const char* Naja_GetNativeBuildID(void) {
  return getBuildID().c_str();
}

