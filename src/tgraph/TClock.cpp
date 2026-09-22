// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TClock.h"

namespace naja::TG {

TClock::TClock(ClockId index, const std::string& name, naja::NL::SNLNetComponent* target, NodeId node)
  : index_(index),
    name_(name),
    target_(target),
    node_(node) {
}

}  // namespace naja::TG
