// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <string>

namespace naja::NL {

class SNLNetComponent;

}  // namespace naja::NL

namespace naja::TG {

class TClock {
  public:
    using ClockId = std::size_t;
    /// \brief same underlying type as TGraph::NodeId (TClock can't include
    /// TGraph.h - TGraph.h includes TClock.h).
    using NodeId = std::size_t;

    TClock(ClockId index, const std::string& name, naja::NL::SNLNetComponent* target, NodeId node);

    ClockId getIndex() const { return index_; }
    const std::string& getName() const { return name_; }

    /// \return this clock's target: either a top terminal (SNLBitTerm) or
    /// an instance terminal (SNLInstTerm) - both are SNLNetComponent.
    naja::NL::SNLNetComponent* getTarget() const { return target_; }

    /// \return the TGraph node this clock is seeded at - TClockPropagator's
    /// forward pass starts from here.
    NodeId getNode() const { return node_; }

  private:
    ClockId index_;
    std::string name_;
    naja::NL::SNLNetComponent* target_ {nullptr};
    NodeId node_;
};

}  // namespace naja::TG
