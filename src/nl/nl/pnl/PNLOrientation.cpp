// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLOrientation.h"

namespace naja { namespace NL {

PNLOrientation::Type::Type(const TypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

std::string PNLOrientation::Type::getString() const {
  switch (typeEnum_) {
    case R0: return "R0";
    case R90: return "R90";
    case R180: return "R180";
    case R270: return "R270";
    case MY: return "MY";
    case MYR90: return "MYR90";
    case MX: return "MX";
    case MXR90: return "MXR90";
  }
  // LCOV_EXCL_START
  return "Unknown";
  // LCOV_EXCL_STOP
}

}} // namespace PNL // namespace naja