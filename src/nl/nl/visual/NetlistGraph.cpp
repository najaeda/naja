// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NetlistGraph.h"
#include "SNLPath.h"

using namespace naja::NL;
using namespace naja;

void SnlVisualiser::process() {
  if (!_equis.empty()) {
    for (const auto& equi : _equis) {
      for (auto oc : equi.getInstTermOccurrences()) {
        auto path = oc.getPath();
        while (path.size() > 0) {
          _equiPaths.insert(path);
          path = path.getHeadPath();
        }
        _equiNets.insert(oc.getInstTerm()->getNet());
        _equiNets.insert(oc.getInstTerm()->getBitTerm()->getNet());
        auto termPath = SNLPath(oc.getPath(), oc.getInstTerm()->getInstance());
        while (termPath.size() > 0) {
          _equiPaths.insert(termPath);
          termPath = termPath.getHeadPath();
        }
      }
      for (auto term : equi.getTerms()) {
        _equiNets.insert(term->getNet());
        _equiPaths.insert(SNLPath());
      }
    }
  }
  SNLPath path;
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
    if (!_equis.empty()) {
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
        if (!_equis.empty()) {
          if (_equiNets.find(term->getNet()) == _equiNets.end()) {
            continue;
          }
        }
        if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Output) {
          // guard: ensure net2wireId contains the net before using it
          auto it = net2wireId.find(term->getNet());
          if (it == net2wireId.end()) {
            // LCOV_EXCL_START
            // net not created in this scope; skip wiring to avoid UB
            //continue;
            throw std::runtime_error(
                "SnlVisualiser::processRec: output term net not found in net2wireId map");
            // LCOV_EXCL_STOP
          }
          _snlNetlistGraph.getWire(it->second)
              .addPort(_snlNetlistGraph.getInst(instId)
                           .getPortName2PortId()[name + std::to_string(
                                                            term->getBit())]);
        } else {
          auto it = net2wireId.find(term->getNet());
          if (it == net2wireId.end()) {
            //continue;
            // LCOV_EXCL_START
            throw std::runtime_error(
                "SnlVisualiser::processRec: input term net not found in net2wireId map");
            // LCOV_EXCL_STOP
          }
          _snlNetlistGraph.getWire(it->second)
              .addDriver(_snlNetlistGraph.getInst(instId)
                             .getPortName2PortId()[name + std::to_string(
                                                              term->getBit())]);
        }
      }
    }
    _snlNetlistGraph.addBus(bus);
    if (busterm->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
      _snlNetlistGraph.getInst(instId).addInBus(bus.getId());
    } else {
      _snlNetlistGraph.getInst(instId).addOutBus(bus.getId());
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
      if (!_equis.empty()) {
        if (_equiNets.find(term->getNet()) == _equiNets.end()) {
          continue;
        }
      }
      auto it = net2wireId.find(term->getNet());
      if (it == net2wireId.end()) {
        // net not created in this scope; skip wiring
        //continue;
        // LCOV_EXCL_START
        throw std::runtime_error(
            "SnlVisualiser::processRec: scalar term net not found in net2wireId map");
        // LCOV_EXCL_STOP
      }
      if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Output) {
        _snlNetlistGraph.getWire(it->second)
            .addPort(
                _snlNetlistGraph.getInst(instId).getPortName2PortId()[name]);
      } else {
        _snlNetlistGraph.getWire(it->second)
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
    if (!_equis.empty()) {
      SNLPath childPath = SNLPath(localPath, instChild);
      bool foundPartial = false;
      // SNLPath instPath = SNLPath(localPath, instChild);
      for (const auto& path : _equiPaths) {
        auto pathCopy = path;
        if (childPath.size() == 0) {
          foundPartial = true;
          break;
        }
        while (pathCopy.size() > 0) {
          if (pathCopy == childPath) {
            foundPartial = true;
            break;
          }
          pathCopy = pathCopy.getHeadPath();
        }
      }
      bool foundExact = false;
      for (const auto& equi : _equis) {
        for (auto oc : equi.getInstTermOccurrences()) {
          SNLPath termPath = SNLPath(oc.getPath(), oc.getInstTerm()->getInstance());
          if (termPath == childPath) {
            foundExact = true;
            break;
          }
        }
        if (foundExact) {
          break;
        }
      }
      if (!foundPartial && !foundExact) {
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
        bool isInEquis = false;
        SNLPath termPath = SNLPath(localPath);
        for (const auto& equi : _equis) {
          for (auto oc : equi.getInstTermOccurrences()) {
            if (oc.getPath() == termPath &&
                oc.getInstTerm()->getBitTerm() == term) {
              isInEquis = true;
              break;
            }
          }
          if (isInEquis) {
            break;
          }
        }
        if (!_equis.empty() && !isInEquis) {
          continue;
        }
        PortDataSnl portDataSnl((SNLTerm*)term);
        PortNode<PortDataSnl> port(portDataSnl,
                                   _snlNetlistGraph.getPorts().size());
        _snlNetlistGraph.addPort(port);
        if (child.getData().getSnlInst()->getInstTerm(term)->getNet()) {
          SNLInstTerm* netTerm =
              child.getData().getSnlInst()->getInstTerm(term);
          if (!_equis.empty()) {
            // LCOV_EXCL_START
            if (_equiNets.find(netTerm->getNet()) == _equiNets.end()) {
              // do not register port wiring for nets outside equiNets
              // still keep the port created so port IDs remain consistent
              std::string name = term->getName().getString();
              if (name == "") {
                name = std::to_string(term->getFlatID());
              }
              child
                  .getPortName2PortId()[name + std::to_string(term->getBit())] =
                  port.getId();
              bus.addPort(port.getId());
              continue;
            }
            // LCOV_EXCL_STOP
          }
          // net is allowed — ensure the corresponding wire exists in this scope
          auto it = net2wireId.find(netTerm->getNet());
          if (it == net2wireId.end()) {
            // // wire not found at this scope: skip wiring to avoid UB
            // std::string name = term->getName().getString();
            // if (name == "") {
            //   name = std::to_string(term->getFlatID());
            // }
            // child.getPortName2PortId()[name + std::to_string(term->getBit())] =
            //     port.getId();
            // bus.addPort(port.getId());
            // continue;
            // LCOV_EXCL_START
            throw std::runtime_error(
                "SnlVisualiser internal error: wire for net not found");
            // LCOV_EXCL_STOP
          }
          if (term->getDirection() ==
              SNLTerm::Direction::DirectionEnum::Input) {
            _snlNetlistGraph.getWire(it->second).addPort(port.getId());
          } else {
            _snlNetlistGraph.getWire(it->second).addDriver(port.getId());
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
      bool isInEquis = false;
      SNLPath termPath = SNLPath(localPath);
      for (const auto& equi : _equis) {
        for (auto oc : equi.getInstTermOccurrences()) {
          if (oc.getPath() == termPath &&
              oc.getInstTerm()->getBitTerm() == term) {
            isInEquis = true;
            break;
          }
        }
        if (isInEquis) {
          break;
        }
      }
      if (!_equis.empty() && !isInEquis) {
        continue;
      }
      PortDataSnl portDataSnl(term);
      PortNode<PortDataSnl> port(portDataSnl,
                                 _snlNetlistGraph.getPorts().size());
      _snlNetlistGraph.addPort(port);
      if (child.getData().getSnlInst()->getInstTerm(term)->getNet()) {
        SNLInstTerm* netTerm = child.getData().getSnlInst()->getInstTerm(term);
        if (!_equis.empty()) {
          if (_equiNets.find(netTerm->getNet()) == _equiNets.end()) {
            // keep port created for id consistency but skip wiring
            std::string name = term->getName().getString();
            if (name == "") {
              name = std::to_string(term->getFlatID());
            }
            child.getPortName2PortId()[name] = port.getId();
            if (term->getDirection() ==
                SNLTerm::Direction::DirectionEnum::Input) {
              child.addInPort(port.getId());
            } else {
              child.addOutPort(port.getId());
            }
            continue;
          }
        }
        auto it = net2wireId.find(netTerm->getNet());
        if (it == net2wireId.end()) {
          // LCOV_EXCL_START
          // skip wiring if wire not found locally
          std::string name = term->getName().getString();
          if (name == "") {
            name = std::to_string(term->getFlatID());
          }
          child.getPortName2PortId()[name] = port.getId();
          if (term->getDirection() ==
              SNLTerm::Direction::DirectionEnum::Input) {
            child.addInPort(port.getId());
          } else {
            child.addOutPort(port.getId());
          }
          continue;
          // LCOV_EXCL_STOP
        }
        if (term->getDirection() == SNLTerm::Direction::DirectionEnum::Input) {
          _snlNetlistGraph.getWire(it->second).addPort(port.getId());
        } else {
          _snlNetlistGraph.getWire(it->second).addDriver(port.getId());
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
    if (!_equis.empty()) {
      bool foundPartial = false;
      SNLPath instPath = SNLPath(localPath, instChild);
      for (const auto& path : _equiPaths) {
        auto pathCopy = path;
        if (instPath.size() == 0) {
          foundPartial = true;
          break;
        }
        while (pathCopy.size() > 0) {
          if (pathCopy == instPath) {
            foundPartial = true;
            break;
          }
          pathCopy = pathCopy.getHeadPath();
        }
      }
      if (!foundPartial) {
        continue;
      }
    }
    if (_recursive) {
      processRec(child.getId(), localPath);
    }
  }
}
