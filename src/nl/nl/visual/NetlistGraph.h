// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <fstream>
#include <set>
#include <sstream>
#include <chrono>

#include "NLUniverse.h"
#include "NLID.h"

#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLEquipotential.h"
#include "SNLPath.h"

using namespace naja::NL;

namespace naja {

typedef size_t InstNodeID;
typedef size_t WireEdgeID;
typedef size_t PortNodeID;
typedef size_t BusNodeID;

template <class InstData>
class InstNode {
 public:
  InstNode(const InstData& data, InstNodeID parentID, InstNodeID id)
      : _parentID(parentID), _id(id), _data(data) {}
  size_t getChildrenCount() const { return _children.size(); }
  const std::vector<InstNodeID>& getChildren() const { return _children; }
  // size_t getPortsCount() const { return _ports.size(); }
  const std::vector<PortNodeID>& getInPorts() const { return _inPorts; }
  const std::vector<BusNodeID>& getInBuses() const { return _inBuses; }
  const std::vector<PortNodeID>& getOutPorts() const { return _outPorts; }
  const std::vector<BusNodeID>& getOutBuses() const { return _outBuses; }
  void addInPort(PortNodeID id) { _inPorts.push_back(id); }
  void addOutPort(PortNodeID id) { _outPorts.push_back(id); }
  const auto& getData() const { return _data; }
  InstNodeID getParentId() const { return _parentID; }
  InstNodeID getId() const { return _id; }
  void addInBus(BusNodeID bus) { _inBuses.push_back(bus); }
  void addOutBus(BusNodeID bus) { _outBuses.push_back(bus); }
  void addChild(InstNodeID inst) { _children.push_back(inst); }
  void setInPortsLeaf(size_t portsLeafId) { _inPortLeafID = portsLeafId; }
  void setOutPortsLeaf(size_t portsLeafId) { _outPortLeafID = portsLeafId; }
  void addLeafId(size_t id) { _leavesIds.push_back(id); }
  const auto& getLeaves() const { return _leavesIds; }
  size_t getInPortLeafId() const { return _inPortLeafID; }
  size_t getOutPortLeafId() const { return _outPortLeafID; }
  auto& getPortName2PortId() { return _portName2PortId; }

 private:
  InstNodeID _parentID = 0;
  InstNodeID _id = 0;
  std::vector<InstNodeID> _children;
  std::vector<PortNodeID> _inPorts;
  std::vector<BusNodeID> _inBuses;
  std::vector<PortNodeID> _outPorts;
  std::vector<BusNodeID> _outBuses;
  InstData _data;
  std::vector<size_t> _leavesIds;
  size_t _inPortLeafID = (size_t) -1;
  size_t _outPortLeafID = (size_t) -1;
  std::map<std::string, size_t> _portName2PortId;
};

template <class PortData>
class PortNode {
 public:
  PortNode(const PortData& data, PortNodeID id) : _id(id), _data(data) {}
  auto& getData() const { return _data; }
  PortNodeID getId() const { return _id; }
  WireEdgeID getWireId() const { return _wireId; }
  void setWireId(auto id) { _wireId = id; }
  void setPortDotName(const std::string& portDotName) {
    _portDotName = portDotName;
  }
  const std::string& getPortDotName() const { return _portDotName; }

 private:
  WireEdgeID _wireId = (WireEdgeID) -1;
  PortNodeID _id = 0;
  PortData _data;
  std::string _portDotName;
};

template <class BusData, class PortData>
class BusNode {
 public:
  BusNode(const BusData& data, BusNodeID id) : _id(id), _data(data) {}
  size_t getPortsCount() const { return _ports.size(); }
  const std::vector<PortNodeID>& getPorts() const { return _ports; }
  const auto& getData() const { return _data; }
  BusNodeID getId() const { return _id; }
  void addPort(PortNodeID port) { _ports.push_back(port); }

 private:
  std::vector<PortNodeID> _ports;
  BusNodeID _id = 0;
  BusData _data;
};

template <class WireData>
class WireEdge {
 public:
  WireEdge(const WireData& data, WireEdgeID id) : _id(id), _data(data) {}
  size_t getPortsCount() const { return _ports.size(); }
  const auto& getData() const { return _data; }
  WireEdgeID getId() const { return _id; }
  void addPort(PortNodeID id) { _ports.push_back(id); }
  void addDriver(PortNodeID id) { _drivers.push_back(id); }
  const std::vector<PortNodeID>& getPorts() const { return _ports; }
  const std::vector<PortNodeID>& getDrivers() const { return _drivers; }

 private:
  WireEdgeID _id = 0;
  std::vector<PortNodeID> _ports;
  std::vector<PortNodeID> _drivers;
  WireData _data;
};

class InstData {
 public:
  InstData(){};
  virtual std::string getName() const = 0;
  virtual std::string getModelName() const = 0;
  virtual std::string getInstName() const = 0;
};

class PortData {
 public:
  PortData(){};
  virtual std::string getName() const = 0;
};

class WireData {
 public:
  WireData(){};
  virtual std::string getName() const = 0;
};

class BusData {
 public:
  BusData(){};
  virtual std::string getName() const = 0;
};

template <class InstData, class PortData, class WireData, class BusData>
class NetlistGraph {
 public:
  NetlistGraph() = default;
  using Nets = std::map<size_t, unsigned>;

  std::string SNLDirectionToJsonDirection(
      naja::NL::SNLTerm::Direction direction) {
    switch (direction) {
      case naja::NL::SNLTerm::Direction::Input:
        return "input";
      case naja::NL::SNLTerm::Direction::Output:
        return "output";
      case naja::NL::SNLTerm::Direction::InOut:
        return "inout";
      case naja::NL::SNLTerm::Direction::Undefined:
        return "undefined";
    }
    return "ERROR";
  }

  void dumpNetID(const size_t bitNet,
                 const Nets& nets,
                 std::ostream& stream,
                 size_t indent) {
    if (bitNet == (size_t)-1) {
      return;
    }
    size_t net = (size_t) -1;
    { net = bitNet; }
    auto it = nets.find(net);
    if (it == nets.end()) {
      // FIXME: ERROR
    } else {
      auto netID = it->second;
      auto bitNetID = netID;
      stream << std::string(indent + 2, ' ') << bitNetID;
    }
  }

  void dumpTerm(const size_t term,
                const Nets& nets,
                std::ostream& stream,
                size_t indent,
                const std::string& direction) {
    stream << std::string(indent, ' ') << "\""
           << getPort(term).getData().getFullName() << "\": {" << std::endl;
    stream << std::string(indent + 2, ' ') << "\"direction\":"
           << "\"" << direction << "\"," << std::endl;
    stream << std::string(indent + 2, ' ') << "\"bits\": [" << std::endl;
    if (getPort(term).getWireId() != (size_t)-1) {
      dumpNetID(getPort(term).getWireId(), nets, stream, indent + 2);
    }
    stream << std::endl;
    stream << std::string(indent + 2, ' ') << "]" << std::endl;
    stream << std::string(indent, ' ') << "}";
  }

  void dumpInstanceTerm(const size_t term,
                        std::ostream& stream,
                        size_t indent,
                        const std::string& direction) {
    stream << std::string(indent, ' ') << "\""
           << getPort(term).getData().getFullName() << "\": \"" << direction
           << "\"";
  }

  void dumpInstanceTermConnection(const InstNode<InstData>* instance,
                                  const size_t term,
                                  const Nets& nets,
                                  std::ostream& stream,
                                  size_t indent) {
    stream << std::string(indent, ' ') << "\""
           << getPort(term).getData().getFullName() << "\": [" << std::endl;
    {
      auto& instTerm = getPort(term);
      if (true) {
        auto net = instTerm.getWireId();
        dumpNetID(net, nets, stream, indent);
      } else {
        // FIXME !! ERROR
      }
    }
    stream << std::endl;
    stream << std::string(indent, ' ') << "]";
  }

  void dumpInterface(const InstNode<InstData>* design,
                     std::ostream& stream,
                     const Nets& nets,
                     size_t indent) {
    stream << std::string(indent, ' ') << "\"ports\": {" << std::endl;
    bool first = true;
    for (auto term : design->getInPorts()) {
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpTerm(term, nets, stream, indent + 2, "input");
    }
    for (auto term : design->getOutPorts()) {
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpTerm(term, nets, stream, indent + 2, "output");
    }
    for (auto bus : design->getInBuses()) {
      for (auto term : getBus(bus).getPorts()) {
        if (not first) {
          stream << "," << std::endl;
        }
        first = false;
        dumpTerm(term, nets, stream, indent + 2, "input");
      }
    }
    for (auto bus : design->getOutBuses()) {
      for (auto term : getBus(bus).getPorts()) {
        if (not first) {
          stream << "," << std::endl;
        }
        first = false;
        dumpTerm(term, nets, stream, indent + 2, "output");
      }
    }
    stream << std::endl;
    stream << std::string(indent, ' ') << "}," << std::endl;
  }

  void dumpInterface(const InstNode<InstData>* instance,
                     std::ostream& stream,
                     size_t indent) {
    stream << std::string(indent, ' ') << "\"port_directions\": {" << std::endl;
    // auto model = instance->getModel();
    bool first = true;
    for (auto term : instance->getInPorts()) {
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpInstanceTerm(term, stream, indent + 2, "input");
    }
    for (auto term : instance->getOutPorts()) {
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpInstanceTerm(term, stream, indent + 2, "output");
    }
    for (auto bus : instance->getInBuses()) {
      for (auto term : getBus(bus).getPorts()) {
        if (not first) {
          stream << "," << std::endl;
        }
        first = false;
        dumpInstanceTerm(term, stream, indent + 2, "input");
      }
    }
    for (auto bus : instance->getOutBuses()) {
      for (auto term : getBus(bus).getPorts()) {
        if (not first) {
          stream << "," << std::endl;
        }
        first = false;
        dumpInstanceTerm(term, stream, indent + 2, "output");
      }
    }
    stream << std::endl;
    stream << std::string(indent, ' ') << "}," << std::endl;
  }

  void dumpConnections(const InstNode<InstData>* instance,
                       std::ostream& stream,
                       const Nets& nets,
                       size_t indent) {
    stream << std::string(indent, ' ') << "\"connections\": {" << std::endl;
    // auto model = instance->getModel();
    bool first = true;
    for (auto term : instance->getInPorts()) {
      if (getPort(term).getWireId() == (size_t)-1) {
        continue;
      }
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpInstanceTermConnection(instance, term, nets, stream, indent + 2);
    }
    for (auto term : instance->getOutPorts()) {
      if (getPort(term).getWireId() == (size_t)-1) {
        continue;
      }
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpInstanceTermConnection(instance, term, nets, stream, indent + 2);
    }
    for (auto bus : instance->getInBuses()) {
      for (auto term : getBus(bus).getPorts()) {
        if (getPort(term).getWireId() == (size_t)-1) {
          continue;
        }
        if (not first) {
          stream << "," << std::endl;
        }
        first = false;
        dumpInstanceTermConnection(instance, term, nets, stream, indent + 2);
      }
    }
    for (auto bus : instance->getOutBuses()) {
      for (auto term : getBus(bus).getPorts()) {
        if (getPort(term).getWireId() == (size_t)-1) {
          continue;
        }
        if (not first) {
          stream << "," << std::endl;
        }
        first = false;
        dumpInstanceTermConnection(instance, term, nets, stream, indent + 2);
      }
    }
    stream << std::endl;
    stream << std::string(indent, ' ') << "}" << std::endl;
  }

  void dumpInstance(const InstNode<InstData>* instance,
                    std::ostream& stream,
                    const Nets& nets,
                    size_t indent) {
    stream << std::string(indent, ' ') << "\""
           << instance->getData().getInstName() << "\": {" << std::endl;
    // stream << std::string(indent + 2, ' ') << "\"type\": \"generic\"," <<
    // std::endl;
    stream << std::string(indent + 2, ' ') << "\"type\":"
           << "\"" << instance->getData().getInstName() << "\"," << std::endl;
    dumpInterface(instance, stream, indent + 2);
    dumpConnections(instance, stream, nets, indent + 2);
    stream << std::string(indent, ' ') << "}";
  }

  void dumpInstances(const InstNode<InstData>* design,
                     std::ostream& stream,
                     const Nets& nets,
                     size_t indent) {
    stream << std::string(indent, ' ') << "\"cells\": {" << std::endl;
    bool first = true;
    for (auto instance : design->getChildren()) {
      if (not first) {
        stream << "," << std::endl;
      }
      first = false;
      dumpInstance(&getInst(instance), stream, nets, indent + 2);
    }
    stream << std::endl;
    stream << std::string(indent, ' ') << "}" << std::endl;
  }

  void dumpTop(const InstNode<InstData>* design,
               std::ostream& stream,
               size_t indent) {
    stream << std::string(indent, ' ') << "\""
           << "top" << /*design->getData().getInstName() <<*/ "\": {"
           << std::endl;
    Nets nets;
    // create Nets map
    unsigned id = 2;  // 0 and 1 are constants ??
    for (auto& net : _wires) {
      nets[net.getId()] = id++;
    }
    dumpInterface(design, stream, nets, indent + 2);
    dumpInstances(design, stream, nets, indent + 2);
    stream << std::string(indent, ' ') << "}" << std::endl;
  }

  void dumpDesign(InstNode<InstData>* design) {
    std::filesystem::path filePath("./design.json");
    std::ofstream outFile;
    outFile.open(filePath);
    outFile << "{" << std::endl;
    outFile << std::string(2, ' ') << "\"modules\": {" << std::endl;
    dumpTop(design, outFile, 4);
    outFile << std::string(2, ' ') << "}" << std::endl;
    outFile << "}" << std::endl;
  }
  void dumpDotFile(std::string fileName);
  void dumpDotFileRec(InstNode<InstData>* node,
                      std::fstream& myfile,
                      size_t& i);
  void alignRec(InstNode<InstData>* node, std::fstream& myfile) {
    if (node->getInPortLeafId() != -1) {
      if (node != &getTop()) {
        myfile << "->";
      }
      myfile << "leaf" << node->getInPortLeafId();
    }
    for (size_t child : node->getChildren()) {
      //printf("alignRec child\n");
      alignRec(&getInst(child), myfile);
    }
    size_t count = 0;
    for (auto id : node->getLeaves()) {
      if (count) {
        myfile << ",";
      } else {
        myfile << "->";
      }
      myfile << "leaf" << id;
      count++;
    }
    if (node->getOutPortLeafId() != -1) {
      myfile << "->";
      myfile << "leaf" << node->getOutPortLeafId();
    }
  }
  void addConnectivity(std::fstream& myfile);
  InstNode<InstData>& getInst(InstNodeID id) { return _insts[id]; }
  PortNode<PortData>& getPort(PortNodeID id) { return _ports[id]; }
  BusNode<BusData, PortData>& getBus(PortNodeID id) { return _buses[id]; }
  WireEdge<WireData>& getWire(PortNodeID id) { return _wires[id]; }

  void addInst(const InstNode<InstData>& inst) { _insts.push_back(inst); }
  void addPort(const PortNode<PortData>& port) { _ports.push_back(port); }
  void addWire(const WireEdge<WireData>& wire) { _wires.push_back(wire); }
  void addBus(const BusNode<BusData, PortData>& bus) { _buses.push_back(bus); }

  const auto& getInsts() const { return _insts; }
  const auto& getPorts() const { return _ports; }
  const auto& getBuses() const { return _buses; }
  auto& getWires() { return _wires; }
  auto getWiresCount() const { return _wires.size(); }

  void setTop(InstNodeID id) { _top = id; }
  auto& getTop() { return getInst(_top); }

 private:
  std::vector<InstNode<InstData>> _insts;
  std::vector<PortNode<PortData>> _ports;
  std::vector<BusNode<BusData, PortData>> _buses;
  std::vector<WireEdge<WireData>> _wires;
  InstNodeID _top = (InstNodeID) -1;
};

class InstDataSnl : InstData {
 public:
  InstDataSnl() : InstData() {}
  InstDataSnl(SNLInstance* snlInst)
      : InstData(), _snlInst(snlInst), _snlModel(snlInst->getModel()){};
  InstDataSnl(SNLDesign* snlDesign) : InstData(), _snlModel(snlDesign){};
  std::string getName() const {
    // LCOV_EXCL_START
    std::string name("");
    if (_snlInst != nullptr)
      name = _snlInst->getName().getString();
    else
      name = _snlModel->getName().getString();
    if (name == std::string("")) {
      name = _snlInst->getNLID().getString();
    }
    return name;
    // LCOV_EXCL_STOP
  }
  auto getSnlInst() const { return _snlInst; }
  auto getSnlModel() const { return _snlModel; }
  std::string getModelName() const {
    std::string name = _snlModel->getName().getString();
    //LCOV_EXCL_START
    if (name == std::string("")) {
      if (_snlModel->isAssign()) {
        return std::string(std::string("assign_") +
                       std::to_string(_snlModel->getID()));
      }
      std::string nameAnon(std::string("anonymous_") +
                       std::to_string(_snlModel->getID()));
      return nameAnon;
    }
    //LCOV_EXCL_STOP
    return name;
  }

  std::string getInstName() const {
    std::string name = _snlInst->getName().getString();
    //LCOV_EXCL_START
    if (name == std::string("")) {
      if (_snlModel->isAssign()) {
        return std::string(std::string("assign_") +
                       std::to_string(_snlInst->getID()));
      }
      std::string nameAnon(std::string("anonymous_") +
                       std::to_string(_snlInst->getID()));
      return nameAnon;
    }
    //LCOV_EXCL_STOP
    return name;
  }

 private:
  SNLInstance* _snlInst = nullptr;
  SNLDesign* _snlModel = nullptr;
};

class PortDataSnl : PortData {
 public:
  PortDataSnl(SNLTerm* snlTerm) : PortData(), _snlTerm(snlTerm) {}
  std::string getName() const {
    std::string name("");
    if (dynamic_cast<SNLBusTermBit*>(_snlTerm)) {
      name = std::to_string((dynamic_cast<SNLBusTermBit*>(_snlTerm))->getBit());
    } else {
      name = _snlTerm->getName().getString();
      if (name == std::string("")) {
        return std::to_string(_snlTerm->getFlatID());
      }
    }
    return name;
  }
  auto getSnlTerm() const { return _snlTerm; }

 private:
  SNLTerm* _snlTerm;
};

class BusDataSnl : BusData {
 public:
  BusDataSnl(SNLBusTerm* snlBusTerm) : BusData(), _snlBusTerm(snlBusTerm) {}
  std::string getName() const { return _snlBusTerm->getName().getString(); }

  auto getSnlBus() const { return _snlBusTerm; }

 private:
  SNLBusTerm* _snlBusTerm;
};

class WireDataSnl : WireData {
 public:
  WireDataSnl(SNLBitNet* snlBitNet) : WireData(), _snlBitNet(snlBitNet) {}
  std::string getName() const {
    if (_snlBitNet->getName().getString() == std::string("")) {
      return std::string("No Name");
    }
    return _snlBitNet->getName().getString();
  }
  auto getSnlBitNet() const { return _snlBitNet; }

 private:
  SNLBitNet* _snlBitNet;
};

class SnlVisualiser {
 public:
  SnlVisualiser(SNLDesign* top, bool recursive = true, SNLEquipotential* equi = nullptr) : 
    _topSnl(top), _recursive(recursive) { if (equi != nullptr) { _equis.push_back(*equi); } }
  SnlVisualiser(SNLDesign* top, std::vector<SNLEquipotential> equis, bool recursive = true) :
    _topSnl(top), _recursive(recursive), _equis(equis) {
  }
  void process();
  void processRec(InstNodeID instId, const SNLPath& path);
  auto& getNetlistGraph() { return _snlNetlistGraph; }

 private:
  NetlistGraph<InstDataSnl, PortDataSnl, WireDataSnl, BusDataSnl>
      _snlNetlistGraph;
  SNLDesign* _topSnl;
  bool _recursive = true;
  //SNLEquipotential* _equi = nullptr;
  std::vector<SNLEquipotential> _equis;
  std::set<SNLPath> _equiPaths;
  std::set<SNLBitNet*> _equiNets;
};
#include "NetlistGraph_impl.h"
}  // namespace naja
