// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDump.h"

namespace naja { namespace NL {

const SNLDump::Version SNLDump::version_ = SNLDump::Version(0, 1, 0);

//LCOV_EXCL_START
std::string SNLDump::Version::getString() {
  return std::to_string(getMajor())
    + "." + std::to_string(getMinor())
    + "." + std::to_string(getRevision());
}
//LCOV_EXCL_STOP

}} // namespace NL // namespace naja
