// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace naja::TG {

class TNodeProxy;
class TGraph;

class TNode {
  public:
    using EdgeId = std::size_t;
    using CaptureEdgeId = std::size_t;

    enum class CellKind { Combinational, FlipFlop, Latch, Unknown };
    enum class PinRole {
      Unknown, DataInput, DataOutput, Clock, Gate, Enable,
      SyncReset, SyncSet, AsyncReset, AsyncSet
    };

    ~TNode();

    std::size_t getIndex() const { return index_; }
    bool isAlive() const { return alive_; }

    /// Classification supplied by the netlist builder; unknown until assigned.
    /// Polarity and active clock edge are independent of these roles.
    CellKind getCellKind() const { return cellKind_; }
    void setCellKind(CellKind kind) { cellKind_ = kind; }
    PinRole getPinRole() const { return pinRole_; }
    void setPinRole(PinRole role) { pinRole_ = role; }

    bool isCombinational() const { return cellKind_ == CellKind::Combinational; }
    bool isFlipFlop() const { return cellKind_ == CellKind::FlipFlop; }
    bool isLatch() const { return cellKind_ == CellKind::Latch; }
    bool isFlopDataInput() const { return isFlipFlop() && pinRole_ == PinRole::DataInput; }
    bool isFlopDataOutput() const { return isFlipFlop() && pinRole_ == PinRole::DataOutput; }
    bool isFlopClock() const { return isFlipFlop() && pinRole_ == PinRole::Clock; }
    bool isLatchDataInput() const { return isLatch() && pinRole_ == PinRole::DataInput; }
    bool isLatchDataOutput() const { return isLatch() && pinRole_ == PinRole::DataOutput; }
    bool isLatchGate() const { return isLatch() && pinRole_ == PinRole::Gate; }
    bool isCombinationalReceiver() const { return isCombinational() && pinRole_ == PinRole::DataInput; }
    bool isCombinationalDriver() const { return isCombinational() && pinRole_ == PinRole::DataOutput; }

    TNodeProxy* getProxy() { return proxy_.get(); }
    const TNodeProxy* getProxy() const { return proxy_.get(); }

    const std::vector<EdgeId>& getInEdges() const { return inEdges_; }
    const std::vector<EdgeId>& getOutEdges() const { return outEdges_; }
    std::size_t getInDegree() const { return inEdges_.size(); }
    std::size_t getOutDegree() const { return outEdges_.size(); }

    const std::vector<CaptureEdgeId>& getCaptureInEdges() const { return captureInEdges_; }
    const std::vector<CaptureEdgeId>& getCaptureOutEdges() const { return captureOutEdges_; }

    TNode(TNode&&);
    TNode& operator=(TNode&&);

    static TNode create(std::size_t index, TNodeProxy* proxy);

  private:
    friend class TGraph;

    TNode(std::size_t index, TNodeProxy* proxy);
    TNode(const TNode&) = delete;
    TNode& operator=(const TNode&) = delete;

    void reset(std::size_t index, TNodeProxy* proxy);
    void deactivate();
    void addInEdge(EdgeId edge);
    void addOutEdge(EdgeId edge);
    void removeInEdge(EdgeId edge);
    void removeOutEdge(EdgeId edge);
    void addCaptureInEdge(CaptureEdgeId edge);
    void addCaptureOutEdge(CaptureEdgeId edge);
    void removeCaptureInEdge(CaptureEdgeId edge);
    void removeCaptureOutEdge(CaptureEdgeId edge);

    std::size_t index_ {0};
    bool alive_ {false};
    CellKind cellKind_ {CellKind::Unknown};
    PinRole pinRole_ {PinRole::Unknown};
    std::unique_ptr<TNodeProxy> proxy_;
    std::vector<EdgeId> inEdges_;
    std::vector<EdgeId> outEdges_;
    std::vector<CaptureEdgeId> captureInEdges_;
    std::vector<CaptureEdgeId> captureOutEdges_;
};

}  // namespace naja::TG
