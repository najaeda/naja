// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NetlistGraph.h"
#include "SNLPath.h"

using namespace naja::SNL;
using namespace naja;

void SnlVisualiser::process() {
  SNLPath path;
  if (_equi != nullptr) {
    for (auto oc : _equi->getInstTermOccurrences()) {
      auto path = oc.getPath();
      while (path.size() > 0) {
        _equiPaths.insert(path);
        path = path.getHeadPath();
      }
      _equiNets.insert(oc.getNet());
      auto termPath = SNLPath(oc.getPath(), oc.getInstTerm()->getInstance());
      _equiPaths.insert(termPath);
    }
    for (auto term : _equi->getTerms()) {
      _equiNets.insert(term->getNet());
    }
  }
  InstDataSnl instChildData(_topSnl);
  InstNode<InstDataSnl> child(instChildData, 0,
                              _snlNetlistGraph.getInsts().size());
  std::map<SNLBitNet*, size_t> net2wireId;
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
      std::string name = term->getName().getString();
      if (name == "") {
        name = std::to_string(term->getFlatID());
      }
      child.getPortName2PortId()[name + std::to_string(term->getBit())] =
          port.getId();
    }
    _snlNetlistGraph.addBus(bus);
    if (busterm->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
      child.addInBus(bus.getId());
    } else {
      child.addOutBus(bus.getId());
    }
  }
  for (SNLTerm* term : child.getData().getSnlModel()->getScalarTerms()) {
    PortDataSnl portDataSnl(term);
    PortNode<PortDataSnl> port(portDataSnl, _snlNetlistGraph.getPorts().size());
    _snlNetlistGraph.addPort(port);
    if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
      child.addInPort(port.getId());
    } else {
      child.addOutPort(port.getId());
    }
    std::string name = term->getName().getString();
    if (name == "") {
      name = std::to_string(term->getFlatID());
    }
    child.getPortName2PortId()[name] = port.getId();
  }
  _snlNetlistGraph.addInst(child);
  _snlNetlistGraph.setTop(child.getId());
  processRec(child.getId(), path);
}

void SnlVisualiser::processRec(InstNodeID instId, const SNLPath& path) {
  std::map<SNLBitNet*, size_t> net2wireId;
  SNLInstance* inst = _snlNetlistGraph.getInst(instId).getData().getSnlInst();
  SNLPath localPath;
  if (inst != nullptr) {
    localPath =
        SNLPath(path, _snlNetlistGraph.getInst(instId).getData().getSnlInst());
  } else {
    localPath = path;
  }
  // Nets are not conditoned by the equi
  for (SNLBitNet* net :
       _snlNetlistGraph.getInst(instId).getData().getSnlModel()->getBitNets()) {
    if (_equi != nullptr) {
      if (_equiNets.find(net) == _equiNets.end()) {
        continue;
      }
    }
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
      std::string name = term->getName().getString();
      if (name == "") {
        name = std::to_string(term->getFlatID());
      }
      if (term->getNet()) {
        if (_equi != nullptr) {
          if (_equiNets.find(term->getNet()) == _equiNets.end()) {
            continue;
          }
        }
        if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Output) {
          _snlNetlistGraph.getWire(net2wireId[term->getNet()])
              .addPort(_snlNetlistGraph.getInst(instId)
                           .getPortName2PortId()[name + std::to_string(
                                                            term->getBit())]);
        } else {
          _snlNetlistGraph.getWire(net2wireId[term->getNet()])
              .addDriver(_snlNetlistGraph.getInst(instId)
                             .getPortName2PortId()[name + std::to_string(
                                                              term->getBit())]);
        }
      }
    }
  }
  for (auto* term : _snlNetlistGraph.getInst(instId)
                        .getData()
                        .getSnlModel()
                        ->getScalarTerms()) {
    std::string name = term->getName().getString();
    if (name == "") {
      name = std::to_string(term->getFlatID());
    }
    if (term->getNet()) {
      if (_equi != nullptr) {
        if (_equiNets.find(term->getNet()) == _equiNets.end()) {
          continue;
        }
      }
      if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Output) {
        _snlNetlistGraph.getWire(net2wireId[term->getNet()])
            .addPort(
                _snlNetlistGraph.getInst(instId).getPortName2PortId()[name]);
      } else {
        _snlNetlistGraph.getWire(net2wireId[term->getNet()])
            .addDriver(
                _snlNetlistGraph.getInst(instId).getPortName2PortId()[name]);
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
    if (_equi != nullptr) {
      SNLPath newPath(localPath, instChild);
      if (_equiPaths.find(newPath) == _equiPaths.end()) {
        continue;
      }
    }
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
          SNLInstTerm* netTerm =
              child.getData().getSnlInst()->getInstTerm(term);
          if (_equi != nullptr) {
            if (_equiNets.find(netTerm->getNet()) == _equiNets.end()) {
              continue;
            }
          }
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
        std::string name = term->getName().getString();
        if (name == "") {
          name = std::to_string(term->getFlatID());
        }
        child.getPortName2PortId()[name + std::to_string(term->getBit())] =
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
        SNLInstTerm* netTerm = child.getData().getSnlInst()->getInstTerm(term);
        if (_equi != nullptr) {
          if (_equiNets.find(netTerm->getNet()) == _equiNets.end()) {
            continue;
          }
        }
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
      std::string name = term->getName().getString();
      if (name == "") {
        name = std::to_string(term->getFlatID());
      }
      child.getPortName2PortId()[name] = port.getId();
      if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
        child.addInPort(port.getId());
      } else {
        child.addOutPort(port.getId());
      }
    }
    _snlNetlistGraph.addInst(child);
    _snlNetlistGraph.getInst(instId).addChild(child.getId());
    if (_recursive) {
      processRec(child.getId(), localPath);
    }
  }
}
