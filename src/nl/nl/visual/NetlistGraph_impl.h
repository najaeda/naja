
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NETLIST_GTAPH_IMPL_H
#define NETLIST_GTAPH_IMPL_H

#include "NetlistGraph.h"

template <class InstData, class PortData, class WireData, class BusData>
void NetlistGraph<InstData, PortData, WireData, BusData>::dumpDotFile(
    std::string fileName) {
  std::string filename(fileName);
  std::fstream myfile;

  myfile.open(filename, std::ios_base::out);
  // std::ofstream myfile;
  // std::ofstream myfile("./graph.dot");
  // myfile.open("./graph.dot");
  myfile << "digraph "
         << ""
         << " {\n rankdir=LR\n"
         << "style =bold\n";
  size_t i = 0;
  dumpDotFileRec(&getInst(_top), myfile, i);
  int count = 0;
  alignRec(&getInst(_top), myfile, count);
  myfile << "[ constraint=true  style=invis ];" << std::endl;
  addConnectivity(myfile);
  myfile << "}";
  myfile.close();
}

template <class InstData, class PortData, class WireData, class BusData>
void NetlistGraph<InstData, PortData, WireData, BusData>::dumpDotFileRec(
    InstNode<InstData>* node,
    std::fstream& myfile,
    size_t& i) {
  if (node != &getInst(_top) || true) {
    if (node == &getInst(_top)) {
      myfile << "subgraph cluster_" << i << " {" << std::endl;
      myfile << "label = <<B>"
             << "top" << /*node->getData().getModelName() <<*/ "</B>>;"
             << std::endl;
      ;
      myfile << "shape=Mrecord" << std::endl;
      myfile << "style=bold" << std::endl;
      i++;
    } else {
      myfile << "subgraph cluster_" << i << " {" << std::endl;
      myfile << "label = <<B>" << node->getData().getInstName() << "("
             << node->getData().getModelName() << ")"
             << "</B>>;" << std::endl;
      myfile << "shape=box3d" << std::endl;
      myfile << "style=bold" << std::endl;
      i++;
    }
  }

  if (node->getChildren().empty() && node->getInBuses().empty() &&
      node->getInPorts().empty() && node->getOutBuses().empty() &&
      node->getOutPorts().empty()) {
    myfile << "leaf" << i << " [style=invis]" << std::endl;
    node->addLeafId(i);
    i++;
  }

  /*for (size_t child : node->getChildren()) {
    //printf("----%s\n", getInst(child).getData().getName().c_str());
  }*/

  for (size_t child : node->getChildren()) {
    dumpDotFileRec(&getInst(child), myfile, i);
  }

  if (!node->getInBuses().empty() || !node->getInPorts().empty()) {
    size_t totalIn = 0;
    myfile << "leaf" << i
           << " [rank=min, shape=record, style=bold, label=\"{ { ";
    node->setInPortsLeaf(i);
    // if (dynamic_cast<const SNLTreeInstanceNode*>(node) != nullptr ||
    // dynamic_cast<const SNLTreeTopNode*>(node) != nullptr) {
    size_t count = 0;
    for (BusNodeID id : node->getInBuses()) {
      if (totalIn >= 100) {
        // LCOV_EXCL_START
        totalIn = 0;
        count = 0;
        i++;
        myfile << " } }\"];" << std::endl;
        myfile << "leaf" << i
           << " [rank=min, shape=record, style=bold, label=\"{ { ";
        // LCOV_EXCL_STOP
      }
      if (count != 0 ) {
        myfile << " | ";
      }
      count++;
      BusNode<BusData, PortData>& bus = getBus(id);

      // IO [shape=record,label="{  IO | { <IO0>0|<IO>1|<IO>2} }"];
      myfile << "{ " << bus.getData().getName() << " | {";
      // i++;
      size_t bit = 0;
      for (PortNodeID id : bus.getPorts()) {
        PortNode<PortData>& port = getPort(id);
        port.setPortDotName(std::string("leaf") + std::to_string(i) +
                            std::string(":") + bus.getData().getName() +
                            port.getData().getName());
        // //printf("----####%s\n", port->getData().getName().c_str());
        myfile << "<" << bus.getData().getName() << port.getData().getName()
               << ">" << port.getData().getName();
        if (bit != bus.getPortsCount() - 1)
          myfile << " | ";
        bit++;
        totalIn++;
      }
      myfile << " } }";
    }
    if (!node->getInBuses().empty() && !node->getInPorts().empty()) {
      myfile << " | ";
    }
    size_t k = 0;
    for (PortNodeID id : node->getInPorts()) {
      if (totalIn >= 100) {
        // LCOV_EXCL_START
        totalIn = 0;
        count = 0;
        i++;
        myfile << " } }\"];" << std::endl;
        myfile << "leaf" << i
           << " [rank=min, shape=record, style=bold, label=\"{ { ";
        // LCOV_EXCL_STOP
      }
      //printf("port %lu\n", id);
      PortNode<PortData>& port = getPort(id);
      port.setPortDotName(std::string("leaf") + std::to_string(i) +
                          std::string(":") + port.getData().getName());
      myfile << " "
             << "<" << port.getData().getName() << ">"
             << port.getData().getName();
      //printf("----$$$$%s\n", port.getData().getName().c_str());
      // i++;

      if (k < node->getInPorts().size() - 1 && totalIn < 100) {
        myfile << "|";
      }
      k++;
      totalIn++;
    }
    // if (node != _top) {
    myfile << " } }\"];" << std::endl;
    i++;
  }
  if (!node->getOutBuses().empty() || !node->getOutPorts().empty()) {
    myfile << "leaf" << i
           << " [rank=min, shape=record, style=bold, label=\"{ { ";
    node->setOutPortsLeaf(i);
    // if (dynamic_cast<const SNLTreeInstanceNode*>(node) != nullptr ||
    // dynamic_cast<const SNLTreeTopNode*>(node) != nullptr) {
    size_t totalOut = 0;  

    size_t count = 0;
    for (BusNodeID id : node->getOutBuses()) {
      if (totalOut >= 100) {
        // LCOV_EXCL_START
        totalOut = 0;
        count = 0;
        i++;
        myfile << " } }\"];" << std::endl;
        myfile << "leaf" << i
           << " [rank=min, shape=record, style=bold, label=\"{ { ";
        // LCOV_EXCL_STOP
      }
      if (count != 0) {
        myfile << " | ";
      }
      count++;
      BusNode<BusData, PortData>& bus = getBus(id);

      // IO [shape=record,label="{  IO | { <IO0>0|<IO>1|<IO>2} }"];
      myfile << "{ " << bus.getData().getName() << " | {";
      // i++;
      size_t bit = 0;
      for (PortNodeID id : bus.getPorts()) {
        PortNode<PortData>& port = getPort(id);
        port.setPortDotName(std::string("leaf") + std::to_string(i) +
                            std::string(":") + bus.getData().getName() +
                            port.getData().getName());
        // printf("----####%s\n", port->getData().getName().c_str());
        myfile << "<" << bus.getData().getName() << port.getData().getName()
               << ">" << port.getData().getName();
        if (bit != bus.getPortsCount() - 1)
          myfile << " | ";
        bit++;
        totalOut++;
      }
      myfile << " } } ";
    }
    size_t k = 0;
    if (!node->getOutBuses().empty() && !node->getOutPorts().empty()) {
      myfile << " | ";
    }
    for (PortNodeID id : node->getOutPorts()) {
      if (totalOut >= 100) {
        // LCOV_EXCL_START
        totalOut = 0;
        count = 0;
        i++;
        myfile << " } }\"];" << std::endl;
        myfile << "leaf" << i
           << " [rank=min, shape=record, style=bold, label=\"{ { ";
        // LCOV_EXCL_STOP
      }
      //printf("port %lu\n", id);
      PortNode<PortData>& port = getPort(id);
      port.setPortDotName(std::string("leaf") + std::to_string(i) +
                          std::string(":") + port.getData().getName());
      myfile << " "
             << "<" << port.getData().getName() << ">"
             << port.getData().getName();
      //printf("----$$$$%s\n", port.getData().getName().c_str());
      // i++;

      if (k < node->getOutPorts().size() - 1 && totalOut < 100) {
        myfile << "|";
      }
      k++;
      totalOut++;
    }
    // if (node != _top) {
    myfile << " } }\"];" << std::endl;
    i++;
  }
  myfile << "}" << std::endl;
  //}

  //printf("-------------------------------\n");
}

template <class InstData, class PortData, class WireData, class BusData>
void NetlistGraph<InstData, PortData, WireData, BusData>::addConnectivity(
    std::fstream& myfile) {
  for (const WireEdge<WireData>& wire : this->getWires()) {
    //printf("wire %lu %s\n", wire.getId(), wire.getData().getName().c_str());
    for (const auto driver : wire.getDrivers()) {
      //printf("driver %lu %s\n", driver,
            // this->getPort(driver).getData().getName().c_str());
      for (const auto reader : wire.getPorts()) {
        /*printf("reader %s\n",
               this->getPort(reader).getData().getName().c_str());*/
        myfile << this->getPort(driver).getPortDotName() << "->"
               << this->getPort(reader).getPortDotName();
        myfile << "[label =\"" << wire.getData().getName() << "\"];"
               << std::endl;
      }
    }
    if (wire.getDrivers().empty()) {
      for (const auto reader : wire.getPorts()) {
        myfile << this->getPort(reader).getPortDotName() << "->"
               << this->getPort(reader).getPortDotName();
        myfile << "[label =\"" << wire.getData().getName() << "\"];"
               << std::endl;
      }
    }
  }
}
#endif  // NETLIST_GTAPH_IMPL_H