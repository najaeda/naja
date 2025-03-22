// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLDesign.h"

namespace naja { namespace NL {

NLDesign::CompareType::CompareType(const CompareTypeEnum& typeEnum):
  typeEnum_(typeEnum) 
{}

//LCOV_EXCL_START
std::string NLDesign::CompareType::getString() const {
  switch (typeEnum_) {
    case CompareType::Complete: return "Complete";
    case CompareType::IgnoreID: return "IgnoreID";
    case CompareType::IgnoreIDAndName: return "IgnoreIDAndName";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP
}} // namespace NL // namespace naja
