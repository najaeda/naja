// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "NLUniverse.h"
#include "SNLBusNet.h"
#include "SNLBusTerm.h"
#include "SNLOccurrence.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "DNL.h"
#include "NetlistGraph.h"

using namespace naja;
using namespace naja::DNL;
using namespace naja::NL;

namespace {

void executeCommand(const std::string& command) {
  int result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Command execution failed." << std::endl;
  }
}

struct TestInstData : InstData {
  explicit TestInstData(std::string name = "node")
      : name_(std::move(name)), model_(name_), inst_(name_) {}

  std::string getName() const override { return name_; }
  std::string getModelName() const override { return model_; }
  std::string getInstName() const override { return inst_; }

  std::string name_;
  std::string model_;
  std::string inst_;
};

struct TestPortData : PortData {
  explicit TestPortData(std::string name) : name_(std::move(name)) {}

  std::string getName() const override { return name_; }
  std::string getFullName() const { return name_; }

  std::string name_;
};

struct TestWireData : WireData {
  explicit TestWireData(std::string name) : name_(std::move(name)) {}

  std::string getName() const override { return name_; }

  std::string name_;
};

struct TestBusData : BusData {
  explicit TestBusData(std::string name) : name_(std::move(name)) {}

  std::string getName() const override { return name_; }

  std::string name_;
};

using TestGraph = NetlistGraph<TestInstData, TestPortData, TestWireData, TestBusData>;

std::filesystem::path makeTempPath(const std::string& name) {
  auto path = std::filesystem::temp_directory_path() / name;
  std::error_code ec;
  std::filesystem::remove(path, ec);
  return path;
}

}

class SNLVisualTests : public ::testing::Test {
 protected:
  SNLVisualTests() {
    // You can do set-up work for each test here
  }
  ~SNLVisualTests() override {
    // You can do clean-up work that doesn't throw exceptions here
  }
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }
  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    // Destroy the SNL
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
  }
};

// Based on last test, create a DNL with 3 levels of hierarchy and validate iso
// db
//  1. Create a simple SNL netlist and then a DNL on top of it
//  2. Create a sub module snl with one input and one output
//  3. Create a sub module snl with one input and one output
//  4. connect the output of the second sub module to the output of the first
//  sub module and
//     the output of the first sub module to the input of the top module.
//  5. Connect the input of the second sub module to the input of the first
//  submodule module and
//     the input of the first sub module to the input of the top module.
//  6. Validate the iso db
TEST_F(SNLVisualTests, SimpleTest) {
  // Create one module snl with one input and one output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  // Create a sub module snl with one input and one output
  SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
  auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input,
                                         NLName("subin"));
  auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output,
                                          NLName("subout"));
  SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
  // Create a sub module snl with one input and one output
  SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
  auto subsubinTerm = SNLScalarTerm::create(
      subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
  auto subsuboutTerm = SNLScalarTerm::create(
      subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
  SNLInstance* subsubinst =
      SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
  // Connect the output of the second sub module to the output of the first sub
  // module
  auto subOutNet = SNLScalarNet::create(submod, NLName("subModOutNet"));
  suboutTerm->setNet(subOutNet);
  subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
  // Connect the output of the first sub module to the input of the top module
  auto outNet = SNLScalarNet::create(mod, NLName("modOutNet"));
  outTerm->setNet(outNet);
  subinst->getInstTerm(suboutTerm)->setNet(outNet);
  // Connect the input of the second sub module to the input of the first
  // submodule module
  auto subsInNet = SNLScalarNet::create(submod, NLName("subModInNet"));
  subinTerm->setNet(subsInNet);
  subsubinst->getInstTerm(subsubinTerm)->setNet(subsInNet);
  // Connect the input of the first sub module to the input of the top module
  auto inNet = SNLScalarNet::create(mod, NLName("modInNet"));
  inTerm->setNet(inNet);
  subinst->getInstTerm(subinTerm)->setNet(inNet);
  // Create a DNL on top of the SNL
  DNLFull* dnl = get();
  assert(dnl != nullptr);
  assert(dnl->getTop().getSNLModel() != nullptr);
  // Validate the access to the SNL data
  DNLID modID = dnl->getTop().getID();
  DNLID inTermID = dnl->getTop().getTerminalFromBitTerm(inTerm).getID();
  DNLID outTermID = dnl->getTop().getTerminalFromBitTerm(outTerm).getID();
  ;
  DNLID submodID = dnl->getTop().getChildInstance(subinst).getID();
  DNLID subinTermID = dnl->getTop()
                          .getChildInstance(subinst)
                          .getTerminalFromBitTerm(subinTerm)
                          .getID();
  DNLID suboutTermID = dnl->getTop()
                           .getChildInstance(subinst)
                           .getTerminalFromBitTerm(suboutTerm)
                           .getID();
  ;
  DNLID subsubmodID = dnl->getTop()
                          .getChildInstance(subinst)
                          .getChildInstance(subsubinst)
                          .getID();
  DNLID subsubinTermID = dnl->getTop()
                             .getChildInstance(subinst)
                             .getChildInstance(subsubinst)
                             .getTerminalFromBitTerm(subsubinTerm)
                             .getID();
  DNLID subsuboutTermID = dnl->getTop()
                              .getChildInstance(subinst)
                              .getChildInstance(subsubinst)
                              .getTerminalFromBitTerm(subsuboutTerm)
                              .getID();
  ;
  EXPECT_EQ(modID, 0);
  EXPECT_EQ(inTermID, 0);
  EXPECT_EQ(outTermID, 1);
  EXPECT_EQ(submodID, 1);
  EXPECT_EQ(subinTermID, 2);
  EXPECT_EQ(suboutTermID, 3);
  EXPECT_EQ(subsubmodID, 2);
  EXPECT_EQ(subsubinTermID, 4);
  EXPECT_EQ(subsuboutTermID, 5);
  // Validate the iso db
  EXPECT_EQ(dnl->getDNLIsoDB().getNumIsos(), 2);
  DNLID inIsoID = dnl->getTop().getTerminalFromBitTerm(inTerm).getIsoID();
  DNLID subinIsoID = dnl->getTop()
                         .getChildInstance(subinst)
                         .getTerminalFromBitTerm(subinTerm)
                         .getIsoID();
  DNLID subsubInIsoID = dnl->getTop()
                            .getChildInstance(subinst)
                            .getChildInstance(subsubinst)
                            .getTerminalFromBitTerm(subsubinTerm)
                            .getIsoID();
  DNLID outIsoID = dnl->getTop().getTerminalFromBitTerm(outTerm).getIsoID();
  DNLID suboutIsoID = dnl->getTop()
                          .getChildInstance(subinst)
                          .getTerminalFromBitTerm(suboutTerm)
                          .getIsoID();
  DNLID subsubOutIsoID = dnl->getTop()
                             .getChildInstance(subinst)
                             .getChildInstance(subsubinst)
                             .getTerminalFromBitTerm(subsuboutTerm)
                             .getIsoID();
  EXPECT_EQ(inIsoID, subinIsoID);
  EXPECT_EQ(subinIsoID, subsubInIsoID);
  EXPECT_EQ(outIsoID, suboutIsoID);
  EXPECT_EQ(suboutIsoID, subsubOutIsoID);
  const DNLIso& isoIn = dnl->getDNLIsoDB().getIsoFromIsoIDconst(inIsoID);
  const DNLIso& isoOut = dnl->getDNLIsoDB().getIsoFromIsoIDconst(outIsoID);
  EXPECT_EQ(isoIn.getDrivers().size(), 1);
  EXPECT_EQ(isoOut.getDrivers().size(), 1);
  EXPECT_EQ(isoIn.getReaders().size(), 1);
  EXPECT_EQ(isoOut.getReaders().size(), 1);
  EXPECT_EQ(dnl->isInstanceChild(0, 1), true);
  EXPECT_EQ(dnl->isInstanceChild(0, 2), true);
  EXPECT_EQ(dnl->isInstanceChild(1, 0), false);
  EXPECT_EQ(dnl->isInstanceChild(2, 0), false);
  EXPECT_EQ(dnl->isInstanceChild(2, 1), false);
  DNLComplexIso complexIso0;
  dnl->getCustomIso(inIsoID, complexIso0);
  EXPECT_EQ(complexIso0.getDrivers().size(), 1);
  EXPECT_EQ(complexIso0.getReaders().size(), 1);
  EXPECT_EQ(complexIso0.getHierTerms().size(), 1);
  DNLComplexIso complexIso1;
  dnl->getCustomIso(outIsoID, complexIso1);
  EXPECT_EQ(complexIso1.getDrivers().size(), 1);
  EXPECT_EQ(complexIso1.getReaders().size(), 1);
  EXPECT_EQ(complexIso1.getHierTerms().size(), 1);
  dnl->display();
  dnl->getTop().display();
  dnl->getTop().getChildInstance(subinst).display();
  EXPECT_EQ(dnl->getTop()
                .getChildInstance(subinst)
                .getTerminal(subsubinst->getInstTerm(subsubinTerm))
                .isNull(),
            true);
  EXPECT_EQ(dnl->getTop().getChildInstance(subsubinst).isNull(), true);
  EXPECT_EQ(dnl->getDNLNullInstance().isNull(), true);
  EXPECT_EQ(dnl->getDNLNullTerminal().isNull(), true);
  SnlVisualiser snl(mod);
  snl.process();
  std::string dotFileName(
      std::string(std::string("./test") + std::string(".dot")));
  std::string svgFileName(
      std::string(std::string("./test") + std::string(".svg")));
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                     std::string(" -o ") + svgFileName)
             .c_str());
  // Destroy the DNL
  destroy();
}

// Based on last test, create a DNL with 3 levels of hierarchy and validate iso
// db
//  1. Create a simple SNL netlist and then a DNL on top of it
//  2. Create a sub module snl with one input and one output
//  3. Create a sub module snl with one input and one output
//  4. connect the output of the second sub module to the output of the first
//  sub module and
//     the output of the first sub module to the input of the top module.
//  5. Connect the input of the second sub module to the input of the first
//  submodule module and
//     the input of the first sub module to the input of the top module.
//  6. Validate the iso db
TEST_F(SNLVisualTests, ForCoverageTest) {
  // Create one module snl with one input and one output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  // Create a sub module snl with one input and one output
  SNLDesign* leafmod = SNLDesign::create(library, NLName("leafmod"));
  SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
  auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input,
                                         NLName("subin"));
  auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output,
                                          NLName("subout"));
  SNLInstance* subinst = SNLInstance::create(mod, submod);
  SNLInstance* leafinst = SNLInstance::create(mod, leafmod);
  // Create a sub module snl with one input and one output
  SNLDesign* subsubmod = SNLDesign::create(library);
  auto subsubinTerm = SNLScalarTerm::create(
      subsubmod, SNLTerm::Direction::Input, NLName("subsubin"));
  auto subsuboutTerm = SNLScalarTerm::create(
      subsubmod, SNLTerm::Direction::Output, NLName("subsubout"));
  SNLInstance* subsubinst =
      SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
  // Connect the output of the second sub module to the output of the first sub
  // module
  auto subOutNet = SNLScalarNet::create(submod);//No name check
  suboutTerm->setNet(subOutNet);
  //subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);//No driver check
  // Connect the output of the first sub module to the input of the top module
  auto outNet = SNLScalarNet::create(mod, NLName("modOutNet"));
  subinst->getInstTerm(suboutTerm)->setNet(outNet);
  // Connect the input of the second sub module to the input of the first
  // submodule module
  auto subsInNet = SNLScalarNet::create(submod, NLName("subModInNet"));
  subinTerm->setNet(subsInNet);
  subsubinst->getInstTerm(subsubinTerm)->setNet(subsInNet);
  // Connect the input of the first sub module to the input of the top module
  auto inNet = SNLScalarNet::create(mod, NLName("modInNet"));
  inTerm->setNet(inNet);
  subinst->getInstTerm(subinTerm)->setNet(inNet);
  // Create a DNL on top of the SNL
  SnlVisualiser snl(mod);
  snl.process();
  std::string dotFileName(
      std::string(std::string("./testCov") + std::string(".dot")));
  std::string svgFileName(
      std::string(std::string("./testCov") + std::string(".svg")));
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                     std::string(" -o ") + svgFileName)
             .c_str());
}

// Same as previous test but with busterms instead of scalar terms
TEST_F(SNLVisualTests, BusTest) {
  // Create one module snl with one input and one output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm = SNLBusTerm::create(mod, SNLTerm::Direction::Input, 0, 2,
                                   NLName("inBus0"));
  auto outTerm = SNLBusTerm::create(mod, SNLTerm::Direction::Output, 0, 2,
                                    NLName("outBus0"));
  // Create a sub module snl with one input and one output
  SNLDesign* submod = SNLDesign::create(library, NLName("submod"));
  EXPECT_FALSE(submod->isPrimitive());
  auto subinTerm = SNLBusTerm::create(submod, SNLTerm::Direction::Input, 0, 2,
                                      NLName("inBus0"));
  auto suboutTerm = SNLBusTerm::create(submod, SNLTerm::Direction::Output, 0, 2,
                                       NLName("outBus0"));
  SNLInstance* subinst = SNLInstance::create(mod, submod, NLName("subinst"));
  // Create a sub module snl with one input and one output
  SNLDesign* subsubmod = SNLDesign::create(library, NLName("subsubmod"));
  auto subsubinTerm = SNLBusTerm::create(subsubmod, SNLTerm::Direction::Input,
                                         0, 2, NLName("inBus0"));
  auto subsuboutTerm = SNLBusTerm::create(subsubmod, SNLTerm::Direction::Output,
                                          0, 2, NLName("outBus0"));
  SNLInstance* subsubinst =
      SNLInstance::create(submod, subsubmod, NLName("subsubinst"));
  // Connect the output of the second sub module to the output of the first sub
  // module
  auto subNetIn = SNLBusNet::create(subsubinst->getDesign(),
                                   subsubinTerm->getMSB(), subsubinTerm->getLSB(), NLName("subNetIn"));
  subsubinst->setTermNet(subsubinTerm, subNetIn);
  subinTerm->setNet(subNetIn);
  auto subNetOut = SNLBusNet::create(subsubinst->getDesign(),
                                   subsuboutTerm->getMSB(), subsuboutTerm->getLSB(), NLName("subNetOut"));
  subsubinst->setTermNet(subsuboutTerm, subNetOut);       
  suboutTerm->setNet(subNetOut); 

  auto netIn = SNLBusNet::create(subinst->getDesign(),
                                   subinTerm->getMSB(), subinTerm->getLSB(), NLName("netIn"));
  subinst->setTermNet(subinTerm, netIn);
  inTerm->setNet(netIn);
  auto netOut = SNLBusNet::create(subinst->getDesign(),
                                   suboutTerm->getMSB(), suboutTerm->getLSB(), NLName("netOut"));
  subinst->setTermNet(suboutTerm, netOut);       
  outTerm->setNet(netOut);                
  std::string dotFileName(
      std::string(std::string("./testBus") + std::string(".dot")));
  std::string svgFileName(
      std::string(std::string("./testBus") + std::string(".svg")));
  SnlVisualiser snl(mod);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                     std::string(" -o ") + svgFileName)
             .c_str());
  std::vector<SNLEquipotential> equipotentials;
  DNLFull* dnl = get();
  for (const auto& term : dnl->getDNLTerms()) {
    if (term.isNull()) {
      continue;
    }
    printf("Term %s has equipotential %s\n", term.getSnlBitTerm()->getName().getString().c_str(),
           term.getEquipotential().getString().c_str());
    equipotentials.push_back(term.getEquipotential());
  }
  std::string dotFileNameEquis(
      std::string(std::string("./testBusEquis") + std::string(".dot")));
  std::string svgFileNameEquis(
      std::string(std::string("./testBusEquis") + std::string(".svg")));
  SnlVisualiser snl2(mod, equipotentials);
  snl2.process();
  snl2.getNetlistGraph().dumpDotFile(dotFileNameEquis.c_str());
  executeCommand(std::string(std::string("dot -Tsvg ") + dotFileNameEquis +
                     std::string(" -o ") + svgFileNameEquis)
             .c_str());
}

TEST_F(SNLVisualTests, ManualGraphCoverage) {
  TestGraph graph;
  graph.addInst(InstNode<TestInstData>(TestInstData("top"), 0, 0));
  graph.setTop(0);

  for (size_t index = 0; index < 101; ++index) {
    BusNode<TestBusData, TestPortData> inBus(
        TestBusData("in_bus_" + std::to_string(index)), graph.getBuses().size());
    PortNode<TestPortData> inBusPort(
        TestPortData("in_bus_bit_" + std::to_string(index)), graph.getPorts().size());
    graph.addPort(inBusPort);
    inBus.addPort(inBusPort.getId());
    graph.addBus(inBus);
    graph.getInst(0).addInBus(inBus.getId());
  }

  for (size_t index = 0; index < 101; ++index) {
    PortNode<TestPortData> inPort(
        TestPortData("in_port_" + std::to_string(index)), graph.getPorts().size());
    graph.addPort(inPort);
    graph.getInst(0).addInPort(inPort.getId());
  }

  for (size_t index = 0; index < 101; ++index) {
    BusNode<TestBusData, TestPortData> outBus(
        TestBusData("out_bus_" + std::to_string(index)), graph.getBuses().size());
    PortNode<TestPortData> outBusPort(
        TestPortData("out_bus_bit_" + std::to_string(index)), graph.getPorts().size());
    graph.addPort(outBusPort);
    outBus.addPort(outBusPort.getId());
    graph.addBus(outBus);
    graph.getInst(0).addOutBus(outBus.getId());
  }

  for (size_t index = 0; index < 101; ++index) {
    PortNode<TestPortData> outPort(
        TestPortData("out_port_" + std::to_string(index)), graph.getPorts().size());
    graph.addPort(outPort);
    graph.getInst(0).addOutPort(outPort.getId());
  }

  auto dumpPath = makeTempPath("naja-visual-manual-graph.dot");
  {
    std::fstream dumpFile(dumpPath, std::ios::out);
    ASSERT_TRUE(dumpFile.is_open());
    size_t nextLeafID = 0;
    graph.dumpDotFileRec(&graph.getInst(0), dumpFile, nextLeafID);
  }

  TestGraph alignGraph;
  alignGraph.addInst(InstNode<TestInstData>(TestInstData("align_top"), 0, 0));
  alignGraph.addInst(InstNode<TestInstData>(TestInstData("align_child"), 0, 1));
  alignGraph.setTop(0);
  alignGraph.getInst(0).addChild(1);
  alignGraph.getInst(1).setInPortsLeaf(10);
  alignGraph.getInst(1).setOutPortsLeaf(20);
  for (size_t leaf = 0; leaf < 205; ++leaf) {
    alignGraph.getInst(1).addLeafId(100 + leaf);
  }

  auto alignPath = makeTempPath("naja-visual-align-graph.dot");
  {
    std::fstream alignFile(alignPath, std::ios::out);
    ASSERT_TRUE(alignFile.is_open());
    int childCount = 101;
    alignGraph.alignRec(&alignGraph.getInst(1), alignFile, childCount);
    int parentCount = 101;
    alignGraph.alignRec(&alignGraph.getInst(0), alignFile, parentCount);
  }

  std::error_code ec;
  std::filesystem::remove(dumpPath, ec);
  std::filesystem::remove(alignPath, ec);
}

TEST_F(SNLVisualTests, EquipotentialFilteringCoverage) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  SNLDesign* top = SNLDesign::create(library, NLName("top"));
  univ->setTopDesign(top);
  auto* topBus =
      SNLBusTerm::create(top, SNLTerm::Direction::Input, 0, 1, NLName("TOPBUS"));
  auto* topFiltered =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("TOP_FILTERED"));

  SNLDesign* selected =
      SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("selected"));
  auto* selectedBus = SNLBusTerm::create(
      selected, SNLTerm::Direction::Input, 0, 1, NLName("BUS"));
  auto* selectedUnnamedOutput =
      SNLScalarTerm::create(selected, SNLTerm::Direction::Output);

  SNLDesign* skipped =
      SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("skipped"));
  SNLScalarTerm::create(skipped, SNLTerm::Direction::Input, NLName("I"));

  auto* selectedInst = SNLInstance::create(top, selected, NLName("selectedInst"));
  SNLInstance::create(top, skipped, NLName("skippedInst"));

  auto* selectedBusNet = SNLBusNet::create(top, 0, 1, NLName("selectedBusNet"));
  topBus->setNet(selectedBusNet);
  selectedInst->setTermNet(selectedBus, selectedBusNet);
  auto* filteredScalarNet =
      SNLScalarNet::create(top, NLName("filteredScalarNet"));
  topFiltered->setNet(filteredScalarNet);
  selectedInst->getInstTerm(selectedUnnamedOutput)->setNet(filteredScalarNet);

  std::vector<SNLEquipotential> equis{
      SNLEquipotential(topBus->getBit(0)), SNLEquipotential(topFiltered)};
  SnlVisualiser visualiser(top, equis);
  visualiser.process();

  auto& selectedNode = visualiser.getNetlistGraph().getInst(1);
  EXPECT_FALSE(selectedNode.getInBuses().empty());
  EXPECT_FALSE(selectedNode.getOutPorts().empty());
  EXPECT_EQ(2u, visualiser.getNetlistGraph().getInsts().size());
}
