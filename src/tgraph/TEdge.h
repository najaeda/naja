// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <memory>

#include <boost/dynamic_bitset.hpp>

namespace naja::TG {

class TEdgeProxy;
class TGraph;

class TEdge {
  public:
    using NodeId = std::size_t;
    using EdgeId = std::size_t;
    /// \brief bit i set means clock i (TClock::getIndex()) reaches this edge.
    /// TGraph keeps every edge's masks sized to its current clock count, so
    /// all edges always carry same-size FromClocks/ToClocks masks.
    using ClockMask = boost::dynamic_bitset<>;

    ~TEdge();

    EdgeId getIndex() const { return index_; }
    NodeId getSource() const { return source_; }
    NodeId getTarget() const { return target_; }
    bool isAlive() const { return alive_; }

    TEdgeProxy* getProxy() { return proxy_.get(); }
    const TEdgeProxy* getProxy() const { return proxy_.get(); }

    /// \return the mask of clocks reaching this edge from its source side.
    const ClockMask& getFromClocks() const { return fromClocks_; }
    /// \return the mask of clocks reaching this edge from its target side.
    const ClockMask& getToClocks() const { return toClocks_; }
    void setFromClocks(const ClockMask& mask) { fromClocks_ = mask; }
    void setToClocks(const ClockMask& mask) { toClocks_ = mask; }
    bool hasFromClock(std::size_t clock) const { return clock < fromClocks_.size() && fromClocks_.test(clock); }
    bool hasToClock(std::size_t clock) const { return clock < toClocks_.size() && toClocks_.test(clock); }
    /// \pre clock < getFromClocks().size(), i.e. clock is a TClock::ClockId
    /// already registered on the owning TGraph.
    void addFromClock(std::size_t clock) { fromClocks_.set(clock); }
    /// \pre clock < getToClocks().size(), i.e. clock is a TClock::ClockId
    /// already registered on the owning TGraph.
    void addToClock(std::size_t clock) { toClocks_.set(clock); }

    /// \return true if the set of clocks reaching this edge's source differs
    /// from the set reaching its target - a potential clock domain crossing.
    bool hasClockCrossing() const { return fromClocks_ != toClocks_; }

    /// \return true if this edge was cut by a loop-breaking pass (e.g.
    /// TLoopBreaker) and should be skipped by graph traversals such as
    /// clock domain propagation.
    bool isDisabled() const { return disabled_; }
    void setDisabled(bool disabled) { disabled_ = disabled; }

    TEdge(TEdge&&);
    TEdge& operator=(TEdge&&);

    static TEdge create(EdgeId index, NodeId source, NodeId target, TEdgeProxy* proxy);

  private:
    friend class TGraph;

    TEdge(EdgeId index, NodeId source, NodeId target, TEdgeProxy* proxy);
    TEdge(const TEdge&) = delete;
    TEdge& operator=(const TEdge&) = delete;

    void reset(EdgeId index, NodeId source, NodeId target, TEdgeProxy* proxy);
    void deactivate();

    /// \brief grow (or shrink, though TGraph never shrinks) both masks to
    /// exactly clockCount bits, zero-filling any new bits. TGraph calls this
    /// on every edge whenever a clock is registered, and on each edge it
    /// creates, so masks always stay sized to TGraph::getClocksCount().
    void resizeClockMasks(std::size_t clockCount) {
      fromClocks_.resize(clockCount);
      toClocks_.resize(clockCount);
    }

    EdgeId index_ {0};
    NodeId source_ {0};
    NodeId target_ {0};
    bool alive_ {false};
    std::unique_ptr<TEdgeProxy> proxy_;
    ClockMask fromClocks_;
    ClockMask toClocks_;
    bool disabled_ {false};
};

}  // namespace naja::TG
