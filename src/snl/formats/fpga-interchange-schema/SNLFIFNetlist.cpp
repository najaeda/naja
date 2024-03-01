#include "SNLFIFNetlist.h"

#include <fcntl.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <zlib.h>

#include "LogicalNetlist.capnp.h"

#include "SNLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"

namespace naja { namespace SNL {

using Strings = std::vector<std::string>;
using Designs = std::vector<SNLDesign*>;
struct Port {
  SNLName name;
  SNLTerm::Direction direction;
  bool isBus { false };
  uint start { 0 };
  uint end { 0 };
};
using Ports = std::vector<Port>;
struct CellInstance {
  SNLName name;
  uint modelID;
};
using CellInstances = std::vector<CellInstance>;

SNLTerm::Direction loadPortDirection(LogicalNetlist::Netlist::Direction direction) {
  switch (direction) {
    case LogicalNetlist::Netlist::Direction::INPUT:
      return SNLTerm::Direction::Input;
    case LogicalNetlist::Netlist::Direction::OUTPUT:
      return SNLTerm::Direction::Output;
    case LogicalNetlist::Netlist::Direction::INOUT:
    default:
      assert(false);
  }
}

void loadPort(
  Ports& ports,
  const Strings& strings,
  const LogicalNetlist::Netlist::Port::Reader& port) {
  auto nameID = port.getName();
  assert(nameID < strings.size());
  auto name = SNLName(strings[nameID]);
  auto dir = port.getDir();
  auto snlDirection = loadPortDirection(dir);
  if (port.isBus()) {
    auto start = port.getBus().getBusStart();
    auto end = port.getBus().getBusEnd();
    std::cerr << "Loading port bus " << name.getString()
      << "[" << start << ":" << end << "]"
      << std::endl;
    ports.push_back({name, snlDirection, true, start, end});
  } else {
    std::cerr << "Loading port " << name.getString() << std::endl;
    ports.push_back({name, snlDirection});
  }
}

void loadCellInstance(
  CellInstances& cellInstances,
  const Strings& strings,
  const LogicalNetlist::Netlist::CellInstance::Reader& inst) {
  auto nameID = inst.getName();
  assert(nameID < strings.size());
  auto name = SNLName(strings[nameID]);
  auto modelID = inst.getCell();
  std::cerr << "Loading cell instance " << nameID << " : " << name.getString() << std::endl;
  cellInstances.push_back({name, modelID});
}

void loadInst(
  SNLDesign* design,
  const Designs& designs,
  const Strings& strings,
  const CellInstance& cellInstance) { 
  auto name = cellInstance.name;
  auto modelID = cellInstance.modelID;
  assert(modelID < designs.size());
  auto model = designs[modelID];
  auto instance = SNLInstance::create(design, model, name);
  std::cerr << "Created instance " << instance->getString() << std::endl;
}

void createTerm(SNLDesign* design, const Port& port) {
  if (port.isBus) {
    SNLBusTerm::create(design, port.direction, port.start, port.end, port.name);
  } else {
    SNLScalarTerm::create(design, port.direction, port.name);
  }
}

void loadCellDecl(
  SNLDB* db,
  Designs& designs,
  const Strings& strings,
  const Ports& ports,
  const LogicalNetlist::Netlist::CellDeclaration::Reader& cellDecl) {
  auto nameID = cellDecl.getName();
  assert(nameID < strings.size());
  auto name = SNLName(strings[nameID]);
  auto viewID = cellDecl.getView();
  assert(viewID < strings.size());
  auto view = strings[viewID];
  auto libID = cellDecl.getLib();
  assert(libID < strings.size());
  auto libName = SNLName(strings[libID]);

  auto snlLib = db->getLibrary(libName);
  if (not snlLib) {
    auto libType = SNLLibrary::Type::Standard;
    if (libName.getString().find("primitives") != std::string::npos) {
      libType = SNLLibrary::Type::Primitives;
    }
    snlLib = SNLLibrary::create(db, libType, libName);
  }
  auto cellType = SNLDesign::Type::Standard;
  if (snlLib->getType() == SNLLibrary::Type::Primitives) {
    cellType = SNLDesign::Type::Primitive;
  }

  auto design = SNLDesign::create(snlLib, cellType, name);
  designs.push_back(design);
  for (auto portID: cellDecl.getPorts()) {
    assert(portID < ports.size());
    auto port = ports[portID];
    createTerm(design, port);
  }

  std::cerr << "Loading cell decl "
    << nameID << ": " << name.getString() << ", "
    << viewID << ": " << view << ", "
    << libID << ": " << libName.getString()
    << std::endl;
}

SNLBitTerm* getBitTerm(
  const SNLDesign* design,
  const Port& port,
  const LogicalNetlist::Netlist::PortInstance::Reader& portInst) {
  if (port.isBus) {
    auto busTerm = design->getBusTerm(port.name);
    assert(busTerm);
    assert(portInst.getBusIdx().isIdx());
    auto bit = portInst.getBusIdx().getIdx();
    assert(SNLDesign::isBetween(bit, busTerm->getMSB(), busTerm->getLSB()));
    auto bitTerm = busTerm->getBit(bit);
    assert(bitTerm);
    return bitTerm;
  } else {
    assert(portInst.getBusIdx().isSingleBit());
    auto scalarTerm = design->getScalarTerm(port.name);
    assert(scalarTerm);
    return scalarTerm;
  }
}

void loadNet(SNLDesign* design,
const LogicalNetlist::Netlist::Net::Reader& net,
const Strings& strings,
const Ports& ports,
const CellInstances& instances) {
  auto nameID = net.getName();
  assert(nameID < strings.size());
  auto name = SNLName(strings[nameID]);
  //auto name = design->getTerms()[nameID]->getName();
  std::cerr << "Loading net " << name.getString() << std::endl;
  auto snlNet = SNLScalarNet::create(design, name);
  for (auto portInst: net.getPortInsts()) {
    auto portID = portInst.getPort();
    assert(portID < ports.size());
    auto port = ports[portID];
    if (portInst.isInst()) {
      auto instID = portInst.getInst();
      assert(instID < instances.size());
      auto cellInstance = instances[instID];
      auto inst = design->getInstance(cellInstance.name);
      assert(inst);
      auto model = inst->getModel();
      auto bitTerm = getBitTerm(model, port, portInst);
      auto instTerm = inst->getInstTerm(bitTerm);
      assert(instTerm);
      instTerm->setNet(snlNet);
    } else if (portInst.isExtPort()) {
      auto bitTerm = getBitTerm(design, port, portInst);
      bitTerm->setNet(snlNet);
    }
  }
}

void loadCell(
  const Designs& designs,
  const Ports& ports,
  const CellInstances& cellInstances,
  const Strings& strings,
  const LogicalNetlist::Netlist::Cell::Reader& cell) {
  auto cellID = cell.getIndex();
  assert(cellID < designs.size());
  auto design = designs[cellID];
  std::cerr << "Loading cell " << design->getName().getString() << std::endl;
  for (auto instID: cell.getInsts()) {
    assert(instID < cellInstances.size());
    auto cellInstance = cellInstances[instID];
    loadInst(design, designs, strings, cellInstance);
  }
  for (auto net: cell.getNets()) {
    loadNet(design, net, strings, ports, cellInstances);
  }
}

SNLDB* SNLFIFNetlist::load(const std::filesystem::path& dumpPath) {
  // Open the file
  int fd = open(dumpPath.c_str(), O_RDONLY);
  if (fd == -1) {
    // Handle error
    return nullptr;
  }

  ::capnp::StreamFdMessageReader message(fd);

  LogicalNetlist::Netlist::Reader netlist = message.getRoot<LogicalNetlist::Netlist>();
  auto universe = SNLUniverse::get();
  if (not universe) {
    universe = SNLUniverse::create();
  }
  auto db = SNLDB::create(universe);
  auto library = SNLLibrary::create(db);
  //auto top = SNLDesign::create(library, SNLName(netlist.getName()));
  //db->setTopDesign(top);

  Strings strings;
  Designs designs;
  Ports ports;
  CellInstances cellInstances;

  for (auto str: netlist.getStrList()) {
    strings.push_back(str);
  }

  for (auto port: netlist.getPortList()) {
    loadPort(ports, strings, port);
  }

  for (auto cellInstance: netlist.getInstList()) {
    loadCellInstance(cellInstances, strings, cellInstance);
  }

  std::cerr << "Loading " << netlist.getCellDecls().size() << " cell decls" << std::endl;
  for (auto cellDecl: netlist.getCellDecls()) {
    loadCellDecl(db, designs, strings, ports, cellDecl);
  }

  for(auto cell: netlist.getCellList()) {
    loadCell(designs, ports, cellInstances, strings, cell);
  }

  auto topInst = netlist.getTopInst();
  auto topInstNameId = topInst.getName();
  assert(topInstNameId < strings.size());
  auto topInstName = SNLName(strings[topInstNameId]);
  std::cerr << "Top instance name: " << topInstName.getString() << std::endl;
  auto topRef = topInst.getCell();
  assert(topRef < designs.size());
  auto topDesign = designs[topRef];
  std::cerr << "Top is " << topDesign->getString() << std::endl;
  db->setTopDesign(topDesign);

  std::cerr << "Loading done" << std::endl;


  return db;
}

}} // namespace SNL // namespace naja