// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"
#include "NetlistGraph.h"
#include "SNLBitNet.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTerm.h"
#include "SNLBitTermOccurrence.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLEquipotential.h"
#include "SNLException.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLUniverse.h"
#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja;

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
    SNLUniverse::get()->destroy();
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
TEST_F(SNLVisualTests, SNLDataAccessWith3levelsOfHierarchyAndIsoDB) {
  // Create one module snl with one input and one output
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, SNLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, SNLName("out"));
  // Create a sub module snl with one input and one output
  SNLDesign* submod = SNLDesign::create(library, SNLName("submod"));
  auto subinTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Input,
                                         SNLName("subin"));
  auto suboutTerm = SNLScalarTerm::create(submod, SNLTerm::Direction::Output,
                                          SNLName("subout"));
  SNLInstance* subinst = SNLInstance::create(mod, submod, SNLName("subinst"));
  // Create a sub module snl with one input and one output
  SNLDesign* subsubmod = SNLDesign::create(library, SNLName("subsubmod"));
  auto subsubinTerm = SNLScalarTerm::create(
      subsubmod, SNLTerm::Direction::Input, SNLName("subsubin"));
  auto subsuboutTerm = SNLScalarTerm::create(
      subsubmod, SNLTerm::Direction::Output, SNLName("subsubout"));
  SNLInstance* subsubinst =
      SNLInstance::create(submod, subsubmod, SNLName("subsubinst"));
  // Connect the output of the second sub module to the output of the first sub
  // module
  auto subOutNet = SNLScalarNet::create(submod, SNLName("submodnet"));
  suboutTerm->setNet(subOutNet);
  subsubinst->getInstTerm(subsuboutTerm)->setNet(subOutNet);
  // Connect the output of the first sub module to the input of the top module
  auto outNet = SNLScalarNet::create(mod, SNLName("modnet"));
  outTerm->setNet(outNet);
  subinst->getInstTerm(suboutTerm)->setNet(outNet);
  // Connect the input of the second sub module to the input of the first
  // submodule module
  auto subsInNet = SNLScalarNet::create(submod);
  subinTerm->setNet(subsInNet);
  subsubinst->getInstTerm(subsubinTerm)->setNet(subsInNet);
  // Connect the input of the first sub module to the input of the top module
  auto inNet = SNLScalarNet::create(mod);
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
  system(std::string(std::string("dot -Tsvg ") + dotFileName +
                     std::string(" -o ") + svgFileName)
             .c_str());
  // Destroy the DNL
  destroy();
}

// Same as previous test but with busterms instead of scalar terms
TEST_F(SNLVisualTests, SNLDataAccessWith3levelsOfHierarchyAndIsoDBBusTerms) {
  // Create one module snl with one input and one output
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
  SNLDesign* mod = SNLDesign::create(library, SNLName("mod"));
  univ->setTopDesign(mod);
  auto inTerm = SNLBusTerm::create(mod, SNLTerm::Direction::Input, 0, 2,
                                   SNLName("inBus0"));
  auto outTerm = SNLBusTerm::create(mod, SNLTerm::Direction::Output, 0, 2,
                                    SNLName("outBus0"));
  // Create a sub module snl with one input and one output
  SNLDesign* submod = SNLDesign::create(library, SNLName("submod"));
  auto subinTerm = SNLBusTerm::create(submod, SNLTerm::Direction::Input, 0, 2,
                                      SNLName("inBus0"));
  auto suboutTerm = SNLBusTerm::create(submod, SNLTerm::Direction::Output, 0, 2,
                                       SNLName("outBus0"));
  SNLInstance* subinst = SNLInstance::create(mod, submod, SNLName("subinst"));
  // Create a sub module snl with one input and one output
  SNLDesign* subsubmod = SNLDesign::create(library, SNLName("subsubmod"));
  auto subsubinTerm = SNLBusTerm::create(subsubmod, SNLTerm::Direction::Input,
                                         0, 2, SNLName("inBus0"));
  auto subsuboutTerm = SNLBusTerm::create(subsubmod, SNLTerm::Direction::Output,
                                          0, 2, SNLName("outBus0"));
  SNLInstance* subsubinst =
      SNLInstance::create(submod, subsubmod, SNLName("subsubinst"));
  // Connect the output of the second sub module to the output of the first sub
  // module
  auto subNetIn = SNLBusNet::create(subsubinst->getDesign(),
                                   subsubinTerm->getMSB(), subsubinTerm->getLSB());
  subsubinst->setTermNet(subsubinTerm, subNetIn);
  subinTerm->setNet(subNetIn);
  auto subNetOut = SNLBusNet::create(subsubinst->getDesign(),
                                   subsuboutTerm->getMSB(), subsuboutTerm->getLSB());
  subsubinst->setTermNet(subsuboutTerm, subNetIn);       
  suboutTerm->setNet(subNetOut); 

  auto netIn = SNLBusNet::create(subinst->getDesign(),
                                   subinTerm->getMSB(), subinTerm->getLSB());
  subinst->setTermNet(subinTerm, netIn);
  inTerm->setNet(netIn);
  auto netOut = SNLBusNet::create(subinst->getDesign(),
                                   suboutTerm->getMSB(), suboutTerm->getLSB());
  subinst->setTermNet(suboutTerm, netOut);       
  outTerm->setNet(netOut);                
  std::string dotFileName(
      std::string(std::string("./testBus") + std::string(".dot")));
  std::string svgFileName(
      std::string(std::string("./testBus") + std::string(".svg")));
  SnlVisualiser snl(mod);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  system(std::string(std::string("dot -Tsvg ") + dotFileName +
                     std::string(" -o ") + svgFileName)
             .c_str());
}
