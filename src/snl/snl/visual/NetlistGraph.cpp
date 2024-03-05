// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NetlistGraph.h"

using namespace naja::SNL;
using namespace naja;

void SnlVisualiser::process() {
  InstDataSnl instChildData(_topSnl);
  InstNode<InstDataSnl> child(instChildData, 0,
                              _snlNetlistGraph.getInsts().size());

  for (SNLBusTerm* busterm : child.getData().getSnlModel()->getBusTerms()) {
    BusDataSnl busDataSnl(busterm);
    BusNode<BusDataSnl, PortDataSnl> bus(busDataSnl,
                                         _snlNetlistGraph.getBuses().size());
    for (SNLBusTermBit* term : busterm->getBusBits()) {
      PortDataSnl portDataSnl((SNLTerm*)term);
      PortNode<PortDataSnl> port(portDataSnl,
                                 _snlNetlistGraph.getPorts().size());
      _snlNetlistGraph.addPort(port);
      bus.addPort(port.getId());
      child.getPortName2PortId()[term->getName().getString() +
                                 std::to_string(term->getBit())] = port.getId();
    }
    _snlNetlistGraph.addBus(bus);
    if (busterm->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
      child.addInBus(bus.getId());
    } else {
      child.addOutBus(bus.getId());
    }
  }
  for (SNLTerm* term : child.getData().getSnlModel()->getTerms()) {
    PortDataSnl portDataSnl(term);
    PortNode<PortDataSnl> port(portDataSnl, _snlNetlistGraph.getPorts().size());
    _snlNetlistGraph.addPort(port);
    if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
      child.addInPort(port.getId());
    } else {
      child.addOutPort(port.getId());
    }
    child.getPortName2PortId()[term->getName().getString()] = port.getId();
  }
  _snlNetlistGraph.addInst(child);
  _snlNetlistGraph.setTop(child.getId());
  processRec(child.getId());
}

void SnlVisualiser::processRec(InstNodeID instId) {
  std::map<SNLBitNet*, size_t> net2wireId;
  for (SNLBitNet* net :
       _snlNetlistGraph.getInst(instId).getData().getSnlModel()->getBitNets()) {
    WireDataSnl edgeData(net);
    WireEdge<WireDataSnl> wire(edgeData, _snlNetlistGraph.getWires().size());
    net2wireId[net] = _snlNetlistGraph.getWires().size();
    _snlNetlistGraph.addWire(wire);
  }
  for (SNLBusTerm* busterm : _snlNetlistGraph.getInst(instId)
                                 .getData()
                                 .getSnlModel()
                                 ->getBusTerms()) {
    BusDataSnl busDataSnl(busterm);
    BusNode<BusDataSnl, PortDataSnl> bus(busDataSnl,
                                         _snlNetlistGraph.getBuses().size());
    for (SNLBusTermBit* term : busterm->getBusBits()) {
      if (term->getNet()) {
        if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Output) {
          _snlNetlistGraph.getWire(net2wireId[term->getNet()])
              .addPort(
                  _snlNetlistGraph.getInst(instId)
                      .getPortName2PortId()[term->getName().getString() +
                                            std::to_string(term->getBit())]);
        } else {
          _snlNetlistGraph.getWire(net2wireId[term->getNet()])
              .addDriver(
                  _snlNetlistGraph.getInst(instId)
                      .getPortName2PortId()[term->getName().getString() +
                                            std::to_string(term->getBit())]);
        }
      }
    }
  }
  for (auto* term : _snlNetlistGraph.getInst(instId)
                        .getData()
                        .getSnlModel()
                        ->getScalarTerms()) {
    if (term->getNet()) {
      if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Output) {
        _snlNetlistGraph.getWire(net2wireId[term->getNet()])
            .addPort(_snlNetlistGraph.getInst(instId)
                         .getPortName2PortId()[term->getName().getString()]);
      } else {
        _snlNetlistGraph.getWire(net2wireId[term->getNet()])
            .addDriver(_snlNetlistGraph.getInst(instId)
                           .getPortName2PortId()[term->getName().getString()]);
      }
    }
  }
  if (_snlNetlistGraph.getInst(instId)
          .getData()
          .getSnlModel()
          ->getInstances()
          .size() == 0) {
    return;
  }

  for (SNLInstance* instChild : _snlNetlistGraph.getInst(instId)
                                    .getData()
                                    .getSnlModel()
                                    ->getInstances()) {
    InstDataSnl instChildData(instChild);
    InstNode<InstDataSnl> child(instChildData,
                                _snlNetlistGraph.getInst(instId).getId(),
                                _snlNetlistGraph.getInsts().size());

    for (SNLBusTerm* busterm : child.getData().getSnlModel()->getBusTerms()) {
      BusDataSnl busDataSnl(busterm);
      BusNode<BusDataSnl, PortDataSnl> bus(busDataSnl,
                                           _snlNetlistGraph.getBuses().size());
      for (SNLBusTermBit* term : busterm->getBusBits()) {
        PortDataSnl portDataSnl((SNLTerm*)term);
        PortNode<PortDataSnl> port(portDataSnl,
                                   _snlNetlistGraph.getPorts().size());
        _snlNetlistGraph.addPort(port);
        if (child.getData().getSnlInst()->getInstTerm(term)->getNet()) {
          if (term->getDirection() ==
              SNLTerm::Direction::DirectionEnum::Input) {
            _snlNetlistGraph
                .getWire(net2wireId[child.getData()
                                        .getSnlInst()
                                        ->getInstTerm(term)
                                        ->getNet()])
                .addPort(port.getId());
          } else {
            _snlNetlistGraph
                .getWire(net2wireId[child.getData()
                                        .getSnlInst()
                                        ->getInstTerm(term)
                                        ->getNet()])
                .addDriver(port.getId());
          }
        }
        child.getPortName2PortId()[term->getName().getString() +
                                   std::to_string(term->getBit())] =
            port.getId();
        bus.addPort(port.getId());
      }
      _snlNetlistGraph.addBus(bus);
      if (busterm->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
        child.addInBus(bus.getId());
      } else {
        child.addOutBus(bus.getId());
      }
    }
    for (auto* term : child.getData().getSnlModel()->getScalarTerms()) {
      PortDataSnl portDataSnl(term);
      PortNode<PortDataSnl> port(portDataSnl,
                                 _snlNetlistGraph.getPorts().size());
      _snlNetlistGraph.addPort(port);
      if (child.getData().getSnlInst()->getInstTerm(term)->getNet()) {
        if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
          _snlNetlistGraph
              .getWire(net2wireId[child.getData()
                                      .getSnlInst()
                                      ->getInstTerm(term)
                                      ->getNet()])
              .addPort(port.getId());
        } else {
          _snlNetlistGraph
              .getWire(net2wireId[child.getData()
                                      .getSnlInst()
                                      ->getInstTerm(term)
                                      ->getNet()])
              .addDriver(port.getId());
        }
      }
      child.getPortName2PortId()[term->getName().getString()] = port.getId();
      if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
        child.addInPort(port.getId());
      } else {
        child.addOutPort(port.getId());
      }
    }
    _snlNetlistGraph.addInst(child);
    _snlNetlistGraph.getInst(instId).addChild(child.getId());
    processRec(child.getId());
  }
}
