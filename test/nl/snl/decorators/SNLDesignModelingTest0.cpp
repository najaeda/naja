// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

#include "NLUniverse.h"
#include "NLException.h"
#include "NajaDumpableProperty.h"

#include "SNLDesignModeling.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"
using namespace naja::NL;

class SNLDesignModelingTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignModelingTest0, testCombinatorial) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto lut = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("LUT"));
  auto luti0 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I0"));
  auto luti1 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I1"));
  auto luti2 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I2"));
  auto luti3 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I3"));
  auto luto = SNLScalarTerm::create(lut, SNLTerm::Direction::Output, NLName("O"));
  EXPECT_FALSE(SNLDesignModeling::hasModeling(lut));
  SNLDesignModeling::addCombinatorialArcs({luti0, luti1, luti2, luti3}, {luto});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(lut));
  EXPECT_FALSE(SNLDesignModeling::isSequential(lut));
  auto lutIns0 = SNLInstance::create(top, lut, NLName("ins0"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(luto).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(luto).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(luto).begin(),
      SNLDesignModeling::getCombinatorialInputs(luto).end()),
    ElementsAre(luti0, luti1, luti2, luti3));
  //lutIns0/luto
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luto)).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).begin(),
      SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).end()),
    ElementsAre(
      lutIns0->getInstTerm(luti0),
      lutIns0->getInstTerm(luti1),
      lutIns0->getInstTerm(luti2),
      lutIns0->getInstTerm(luti3)));
  
  //luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti0).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti0).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti0).end()),
    ElementsAre(luto));
  //lutIns0/luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luti0)).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).begin(),
      SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).end()),
    ElementsAre(lutIns0->getInstTerm(luto)));

  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti1).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti1).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti1).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti2).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti2).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti2).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti3).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti3).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti3).end()),
    ElementsAre(luto));
}

TEST_F(SNLDesignModelingTest0, testSequential) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("REG"));
  auto regD = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("D"));
  auto regQ = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("Q"));
  auto regC = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("C"));

  EXPECT_FALSE(SNLDesignModeling::hasModeling(reg));
  SNLDesignModeling::addInputsToClockArcs({regD}, regC);
  SNLDesignModeling::addClockToOutputsArcs(regC, {regQ});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(reg));
  EXPECT_TRUE(SNLDesignModeling::isSequential(reg));

  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(regD).empty());

  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedInputs(regC).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedInputs(regC).begin(),
      SNLDesignModeling::getClockRelatedInputs(regC).end()),
    ElementsAre(regD));
  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedOutputs(regC).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedOutputs(regC).begin(),
      SNLDesignModeling::getClockRelatedOutputs(regC).end()),
    ElementsAre(regQ));
  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(regD).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getInputRelatedClocks(regD).begin(),
      SNLDesignModeling::getInputRelatedClocks(regD).end()),
    ElementsAre(regC));
  EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(regQ).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getOutputRelatedClocks(regQ).begin(),
      SNLDesignModeling::getOutputRelatedClocks(regQ).end()),
    ElementsAre(regC));

  auto regIns = SNLInstance::create(top, reg, NLName("regIns"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regD)).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regQ)).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regD)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regQ)).empty());

  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).begin(),
      SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).end()),
    ElementsAre(regIns->getInstTerm(regD)));
  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).begin(),
      SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).end()),
    ElementsAre(regIns->getInstTerm(regQ)));
  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).begin(),
      SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).end()),
    ElementsAre(regIns->getInstTerm(regC)));
  EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).begin(),
      SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).end()),
    ElementsAre(regIns->getInstTerm(regC)));
}

TEST_F(SNLDesignModelingTest0, testManualMemoryInterfaceSurvivesOnInstances) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM1P"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto ce = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CE"));
  auto we = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE"));
  auto addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("ADDR"));
  auto wdata = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("WDATA"));
  auto wmask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("WMASK"));
  auto rdata = SNLBusTerm::create(mem, SNLTerm::Direction::Output, 3, 0, NLName("RDATA"));

  SNLDesignModeling::BitTerms readDataBits;
  SNLDesignModeling::BitTerms updateInputs;
  for (int bit = 0; bit <= 3; ++bit) {
    readDataBits.push_back(static_cast<SNLBitTerm*>(rdata->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(wdata->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(wmask->getBit(bit)));
    if (bit <= 1) {
      updateInputs.push_back(static_cast<SNLBitTerm*>(addr->getBit(bit)));
    }
  }
  updateInputs.push_back(ce);
  updateInputs.push_back(we);

  SNLDesignModeling::addClockToOutputsArcs(clk, readDataBits);
  SNLDesignModeling::addInputsToClockArcs(updateInputs, clk);

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 4;
  interface.depth = 4;
  interface.abits = 2;
  interface.clock = clk;
  SNLDesignModeling::MemoryReadPort readPort;
  readPort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(0)));
  readPort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(1)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(0)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(1)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(2)));
  readPort.data.push_back(static_cast<SNLBitTerm*>(rdata->getBit(3)));
  interface.readPorts.push_back(readPort);

  SNLDesignModeling::MemoryWritePort writePort;
  writePort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(0)));
  writePort.address.push_back(static_cast<SNLBitTerm*>(addr->getBit(1)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(0)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(1)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(2)));
  writePort.data.push_back(static_cast<SNLBitTerm*>(wdata->getBit(3)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(0)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(1)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(2)));
  writePort.mask.push_back(static_cast<SNLBitTerm*>(wmask->getBit(3)));
  writePort.enables.push_back(ce);
  writePort.enables.push_back(we);
  interface.writePorts.push_back(writePort);
  SNLDesignModeling::setMemoryInterface(mem, interface);

  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(mem));
  const auto modelInterface = SNLDesignModeling::getMemoryInterface(mem);
  EXPECT_EQ(4u, modelInterface.width);
  EXPECT_EQ(4u, modelInterface.depth);
  EXPECT_EQ(2u, modelInterface.abits);

  auto inst = SNLInstance::create(top, mem, NLName("mem0"));
  // Guard the connected-instance path: once a memory interface is projected to
  // a concrete instance, fully wired ports must remain visible.
  auto* clkNet = SNLScalarNet::create(top, NLName("clk"));
  auto* ceNet = SNLScalarNet::create(top, NLName("ce"));
  auto* weNet = SNLScalarNet::create(top, NLName("we"));
  inst->getInstTerm(clk)->setNet(clkNet);
  inst->getInstTerm(ce)->setNet(ceNet);
  inst->getInstTerm(we)->setNet(weNet);
  for (int bit = 0; bit <= 1; ++bit) {
    auto* addrNet =
        SNLScalarNet::create(top, NLName("addr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(addr->getBit(bit)))->setNet(addrNet);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* wdataNet =
        SNLScalarNet::create(top, NLName("wdata_" + std::to_string(bit)));
    auto* wmaskNet =
        SNLScalarNet::create(top, NLName("wmask_" + std::to_string(bit)));
    auto* rdataNet =
        SNLScalarNet::create(top, NLName("rdata_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(wdata->getBit(bit)))->setNet(wdataNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(wmask->getBit(bit)))->setNet(wmaskNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(rdata->getBit(bit)))->setNet(rdataNet);
  }
  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(inst->getModel()));
  const auto instInterface = SNLDesignModeling::getMemoryInterface(inst);
  EXPECT_EQ(1u, instInterface.readPorts.size());
  EXPECT_EQ(1u, instInterface.writePorts.size());
  EXPECT_EQ(4u, instInterface.readPorts.front().data.size());
  EXPECT_EQ(4u, instInterface.writePorts.front().mask.size());
  EXPECT_EQ(2u, instInterface.writePorts.front().enables.size());

  for (int bit = 0; bit <= 3; ++bit) {
    auto* instReadBit =
        inst->getInstTerm(static_cast<SNLBitTerm*>(rdata->getBit(bit)));
    EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(instReadBit).size());
    EXPECT_EQ(inst->getInstTerm(clk),
              *SNLDesignModeling::getOutputRelatedClocks(instReadBit).begin());
  }
}

TEST_F(SNLDesignModelingTest0,
       testMemoryInterfaceProvidesSequentialRelationsWithoutExplicitArcs) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM1P"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto rst = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("RST"));
  auto we = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE"));
  auto addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("ADDR"));
  auto wdata = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("WDATA"));
  auto wmask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("WMASK"));
  auto rdata = SNLBusTerm::create(mem, SNLTerm::Direction::Output, 1, 0, NLName("RDATA"));

  auto* addr0 = static_cast<SNLBitTerm*>(addr->getBit(0));
  auto* addr1 = static_cast<SNLBitTerm*>(addr->getBit(1));
  auto* wdata0 = static_cast<SNLBitTerm*>(wdata->getBit(0));
  auto* wdata1 = static_cast<SNLBitTerm*>(wdata->getBit(1));
  auto* wmask0 = static_cast<SNLBitTerm*>(wmask->getBit(0));
  auto* wmask1 = static_cast<SNLBitTerm*>(wmask->getBit(1));
  auto* rdata0 = static_cast<SNLBitTerm*>(rdata->getBit(0));
  auto* rdata1 = static_cast<SNLBitTerm*>(rdata->getBit(1));

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 2;
  interface.depth = 4;
  interface.abits = 2;
  interface.resetMode = SNLDesignModeling::MemoryResetMode::SyncLow;
  interface.clock = clk;
  interface.reset = rst;
  interface.readPorts.push_back(
      {.address = {addr0, addr1}, .data = {rdata0, rdata1}});
  interface.writePorts.push_back(
      {.address = {addr0, addr1},
       .data = {wdata0, wdata1},
       .mask = {wmask0, wmask1},
       .enables = {we}});
  SNLDesignModeling::setMemoryInterface(mem, interface);

  // The memory interface itself is enough to describe the sequential timing
  // surface. SEC relies on these generic relations so memory read data is a
  // state output and write/read controls are next-state dependencies without
  // hardcoded primitive pin names.
  EXPECT_TRUE(SNLDesignModeling::hasModeling(mem));
  EXPECT_TRUE(SNLDesignModeling::isSequential(mem));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getClockRelatedOutputs(clk).begin(),
                  SNLDesignModeling::getClockRelatedOutputs(clk).end()),
      UnorderedElementsAre(rdata0, rdata1));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getClockRelatedInputs(clk).begin(),
                  SNLDesignModeling::getClockRelatedInputs(clk).end()),
      UnorderedElementsAre(rst, we, addr0, addr1, wdata0, wdata1, wmask0, wmask1));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getOutputRelatedClocks(rdata0).begin(),
                  SNLDesignModeling::getOutputRelatedClocks(rdata0).end()),
      ElementsAre(clk));
  EXPECT_THAT(
      std::vector(SNLDesignModeling::getInputRelatedClocks(wdata1).begin(),
                  SNLDesignModeling::getInputRelatedClocks(wdata1).end()),
      ElementsAre(clk));

  auto inst = SNLInstance::create(top, mem, NLName("mem0"));
  size_t netIndex = 0;
  for (auto* term : std::vector<SNLBitTerm*>{
           clk, rst, we, addr0, addr1, wdata0, wdata1, wmask0, wmask1, rdata0,
           rdata1}) {
    auto* net = SNLScalarNet::create(
        top, NLName(std::string("n") + std::to_string(netIndex++)));
    inst->getInstTerm(term)->setNet(net);
  }

  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getClockRelatedOutputs(inst->getInstTerm(clk)).begin(),
          SNLDesignModeling::getClockRelatedOutputs(inst->getInstTerm(clk)).end()),
      UnorderedElementsAre(inst->getInstTerm(rdata0), inst->getInstTerm(rdata1)));
  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(wmask0)).begin(),
          SNLDesignModeling::getInputRelatedClocks(inst->getInstTerm(wmask0)).end()),
      ElementsAre(inst->getInstTerm(clk)));
  EXPECT_THAT(
      std::vector(
          SNLDesignModeling::getOutputRelatedClocks(inst->getInstTerm(rdata1)).begin(),
          SNLDesignModeling::getOutputRelatedClocks(inst->getInstTerm(rdata1)).end()),
      ElementsAre(inst->getInstTerm(clk)));
}

TEST_F(SNLDesignModelingTest0,
       testInstanceMemoryInterfaceDropsDisconnectedPorts) {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto mem = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("MEM2W"));

  auto clk = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("CLK"));
  auto raddr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("RADDR"));
  auto rdata = SNLBusTerm::create(mem, SNLTerm::Direction::Output, 3, 0, NLName("RDATA"));
  auto w0addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("W0_ADDR"));
  auto w0data = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W0_DATA"));
  auto w0mask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W0_MASK"));
  auto we0 = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE0"));
  auto w1addr = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 1, 0, NLName("W1_ADDR"));
  auto w1data = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W1_DATA"));
  auto w1mask = SNLBusTerm::create(mem, SNLTerm::Direction::Input, 3, 0, NLName("W1_MASK"));
  auto we1 = SNLScalarTerm::create(mem, SNLTerm::Direction::Input, NLName("WE1"));

  SNLDesignModeling::BitTerms readDataBits;
  SNLDesignModeling::BitTerms updateInputs;
  for (int bit = 0; bit <= 3; ++bit) {
    readDataBits.push_back(static_cast<SNLBitTerm*>(rdata->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w0data->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w0mask->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w1data->getBit(bit)));
    updateInputs.push_back(static_cast<SNLBitTerm*>(w1mask->getBit(bit)));
    if (bit <= 1) {
      updateInputs.push_back(static_cast<SNLBitTerm*>(raddr->getBit(bit)));
      updateInputs.push_back(static_cast<SNLBitTerm*>(w0addr->getBit(bit)));
      updateInputs.push_back(static_cast<SNLBitTerm*>(w1addr->getBit(bit)));
    }
  }
  updateInputs.push_back(we0);
  updateInputs.push_back(we1);

  SNLDesignModeling::addClockToOutputsArcs(clk, readDataBits);
  SNLDesignModeling::addInputsToClockArcs(updateInputs, clk);

  SNLDesignModeling::MemoryInterface interface;
  interface.width = 4;
  interface.depth = 4;
  interface.abits = 2;
  interface.clock = clk;
  interface.readPorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(raddr->getBit(0)),
                   static_cast<SNLBitTerm*>(raddr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(rdata->getBit(0)),
                static_cast<SNLBitTerm*>(rdata->getBit(1)),
                static_cast<SNLBitTerm*>(rdata->getBit(2)),
                static_cast<SNLBitTerm*>(rdata->getBit(3))}});
  interface.writePorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(w0addr->getBit(0)),
                   static_cast<SNLBitTerm*>(w0addr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(w0data->getBit(0)),
                static_cast<SNLBitTerm*>(w0data->getBit(1)),
                static_cast<SNLBitTerm*>(w0data->getBit(2)),
                static_cast<SNLBitTerm*>(w0data->getBit(3))},
       .mask = {static_cast<SNLBitTerm*>(w0mask->getBit(0)),
                static_cast<SNLBitTerm*>(w0mask->getBit(1)),
                static_cast<SNLBitTerm*>(w0mask->getBit(2)),
                static_cast<SNLBitTerm*>(w0mask->getBit(3))},
       .enables = {we0}});
  interface.writePorts.push_back(
      {.address = {static_cast<SNLBitTerm*>(w1addr->getBit(0)),
                   static_cast<SNLBitTerm*>(w1addr->getBit(1))},
       .data = {static_cast<SNLBitTerm*>(w1data->getBit(0)),
                static_cast<SNLBitTerm*>(w1data->getBit(1)),
                static_cast<SNLBitTerm*>(w1data->getBit(2)),
                static_cast<SNLBitTerm*>(w1data->getBit(3))},
       .mask = {static_cast<SNLBitTerm*>(w1mask->getBit(0)),
                static_cast<SNLBitTerm*>(w1mask->getBit(1)),
                static_cast<SNLBitTerm*>(w1mask->getBit(2)),
                static_cast<SNLBitTerm*>(w1mask->getBit(3))},
       .enables = {we1}});
  SNLDesignModeling::setMemoryInterface(mem, interface);

  auto inst = SNLInstance::create(top, mem, NLName("mem0"));

  auto* clkNet = SNLScalarNet::create(top, NLName("clk"));
  inst->getInstTerm(clk)->setNet(clkNet);
  for (int bit = 0; bit <= 1; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("raddr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(raddr->getBit(bit)))->setNet(net);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("rdata_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(rdata->getBit(bit)))->setNet(net);
  }

  for (int bit = 0; bit <= 1; ++bit) {
    auto* net = SNLScalarNet::create(top, NLName("w0addr_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(w0addr->getBit(bit)))->setNet(net);
  }
  for (int bit = 0; bit <= 3; ++bit) {
    auto* dataNet =
        SNLScalarNet::create(top, NLName("w0data_" + std::to_string(bit)));
    auto* maskNet =
        SNLScalarNet::create(top, NLName("w0mask_" + std::to_string(bit)));
    inst->getInstTerm(static_cast<SNLBitTerm*>(w0data->getBit(bit)))->setNet(dataNet);
    inst->getInstTerm(static_cast<SNLBitTerm*>(w0mask->getBit(bit)))->setNet(maskNet);
  }
  auto* we0Net = SNLScalarNet::create(top, NLName("we0"));
  inst->getInstTerm(we0)->setNet(we0Net);

  // Guard a real SEC regression: instance-level memory consumers should only
  // see connected ports, otherwise disconnected formal write ports become
  // fake no-driver dependencies downstream.
  const auto instInterface = SNLDesignModeling::getMemoryInterface(inst);
  ASSERT_EQ(1u, instInterface.readPorts.size());
  ASSERT_EQ(1u, instInterface.writePorts.size());
  EXPECT_EQ(1u, instInterface.writePorts.front().enables.size());
  EXPECT_EQ(we0, instInterface.writePorts.front().enables.front());
  EXPECT_THAT(
      std::vector(instInterface.writePorts.front().address.begin(),
                  instInterface.writePorts.front().address.end()),
      ElementsAre(static_cast<SNLBitTerm*>(w0addr->getBit(0)),
                  static_cast<SNLBitTerm*>(w0addr->getBit(1))));
}

TEST_F(SNLDesignModelingTest0, testCombiWithParameter) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto gate = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("LUT"));
  auto mode = SNLParameter::create(gate, NLName("MODE"), SNLParameter::Type::String, "NORMAL");
  auto luti0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("I0"));
  auto luti1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("I1"));
  auto luto0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, NLName("O0"));
  auto luto1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::setParameter(gate, "MODE", "NORMAL");
  //no parameter arg so default mode
  SNLDesignModeling::addCombinatorialArcs({luti0}, {luto0});
  SNLDesignModeling::addCombinatorialArcs({luti1}, {luto1});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti1}, {luto0});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti0}, {luto1});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(gate));

  //Default parameter
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti1).size());
  EXPECT_EQ(luto0, *SNLDesignModeling::getCombinatorialOutputs(luti0).begin());
  EXPECT_EQ(luto1, *SNLDesignModeling::getCombinatorialOutputs(luti1).begin());

  //instance with no parameter => default parameter
  auto ins0 = SNLInstance::create(top, gate, NLName("ins0")); 
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).size());
  EXPECT_EQ(ins0->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins0->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).begin());

  //instance with default parameter value
  auto ins1 = SNLInstance::create(top, gate, NLName("ins1")); 
  SNLInstParameter::create(ins1, mode, "NORMAL");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).size());
  EXPECT_EQ(ins1->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins1->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).begin());

  //instance with non default parameter value
  auto ins2 = SNLInstance::create(top, gate, NLName("ins2")); 
  SNLInstParameter::create(ins2, mode, "CROSS");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).size());
  EXPECT_EQ(ins2->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins2->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).begin());

  //Error for non existing parameter
  auto ins3 = SNLInstance::create(top, gate, NLName("ins3")); 
  SNLInstParameter::create(ins3, mode, "UNKNOWN");
  EXPECT_THROW(SNLDesignModeling::getCombinatorialOutputs(ins3->getInstTerm(luti0)), NLException);
}

TEST_F(SNLDesignModelingTest0, testErrors0) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto designs = NLLibrary::create(db);
  //not a primitive
  auto design = SNLDesign::create(designs, NLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("C"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({designD}, designC), NLException);
}

TEST_F(SNLDesignModelingTest0, testErrors1) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("C"));
  auto designA = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("A"));
  auto designB = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("B"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({}, designC), NLException);
  EXPECT_THROW(SNLDesignModeling::addClockToOutputsArcs(designC, {}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({}, {designB}), NLException);

  auto design1 = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design1"));
  auto design1A = SNLScalarTerm::create(design1, SNLTerm::Direction::Input, NLName("A"));
  auto design1B = SNLScalarTerm::create(design1, SNLTerm::Direction::Output, NLName("B"));
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {design1B}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA, design1A}, {design1B}), NLException);
}

TEST_F(SNLDesignModelingTest0, testNonExistingParameterError) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto prim = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("prim"));
  EXPECT_THROW(SNLDesignModeling::setParameter(prim, "MODE", "NORMAL"), NLException);
}

TEST_F(SNLDesignModelingTest0, testGetCombiDepsFromTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  //set truth table
  std::vector<u_int64_t> deps;
  deps.push_back(3);
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, deps));
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1, deps)), NLException);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_EQ(inputArcs.size(), 2);
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_EQ(outputArcs.size(), 1);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  auto insInputArcs = SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o));
  EXPECT_EQ(insInputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insInputArcs.begin(), insInputArcs.end()),
    ElementsAre(ins0->getInstTerm(i0), ins0->getInstTerm(i1)));
  auto insOutputArcs = SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0));
  EXPECT_EQ(insOutputArcs.size(), 1);
  EXPECT_THAT(
    std::vector(insOutputArcs.begin(), insOutputArcs.end()),
    ElementsAre(ins0->getInstTerm(o)));
}

TEST_F(SNLDesignModelingTest0, testGetCombiDepsFromTTs) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  auto o1 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O1"));
  //set truth table
  std::vector<u_int64_t> deps;
  deps.push_back(3);
  SNLDesignModeling::setTruthTables(design, { SNLTruthTable(2, 0x5, deps) , SNLTruthTable(2, 0x5, deps)});
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1, deps)), NLException);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_EQ(inputArcs.size(), 2);
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_EQ(outputArcs.size(), 2);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  auto insInputArcs = SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o));
  EXPECT_EQ(insInputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insInputArcs.begin(), insInputArcs.end()),
    ElementsAre(ins0->getInstTerm(i0), ins0->getInstTerm(i1)));
  auto insOutputArcs = SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0));
  EXPECT_EQ(insOutputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insOutputArcs.begin(), insOutputArcs.end()),
    ElementsAre(ins0->getInstTerm(o), ins0->getInstTerm(o1)));
}

TEST_F(SNLDesignModelingTest0, testNoDepsFromTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  auto o1 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O1"));
  // create a top
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  // create an instance of the design
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  std::vector<u_int64_t> deps;
  EXPECT_THROW(
      SNLDesignModeling::setTruthTables(
          design, {SNLTruthTable(2, 0x5, deps), SNLTruthTable(2, 0x5, deps)}),
      NLException);
  auto property = naja::NajaDumpableProperty::create(
      static_cast<naja::NajaObject*>(design), "SNLDesignTruthTableProperty");
  property->addUInt64Value(2);
  property->addUInt64Value(0x5);
  property->addUInt64Value(0);
  property->addUInt64Value(2);
  property->addUInt64Value(0x5);
  property->addUInt64Value(0);
  // Test all inputs are returned for combinatorial quarie for each output
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o1).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o1)).size(), 2);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 2);
}

TEST_F(SNLDesignModelingTest0, testNoDepsFromSingleTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  // create a top
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  // create an instance of the design
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  //set truth table
  std::vector<u_int64_t> deps;
  deps.push_back(1);
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, deps));
  // Test all inputs are returned for combinatorial quarie for each output
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 1);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 0);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 0);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_THAT(
    std::vector(inputArcs.begin(), inputArcs.end()),
    ElementsAre(i0));
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_THAT(
    std::vector(outputArcs.begin(), outputArcs.end()),
    ElementsAre(o));
  auto ouputArcsI1 = SNLDesignModeling::getCombinatorialOutputs(i1);
  EXPECT_TRUE(ouputArcsI1.empty());
}

TEST_F(SNLDesignModelingTest0, testNoDepsFromSingleTTWithEmptyDeps) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  // create a top
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  // create an instance of the design
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  //set truth table
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 2);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 1);
  std::vector<u_int64_t> deps;
  EXPECT_THROW(
      SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5, deps)),
      NLException);
  auto property = naja::NajaDumpableProperty::create(
      static_cast<naja::NajaObject*>(design), "SNLDesignTruthTableProperty");
  property->addUInt64Value(2);
  property->addUInt64Value(0x5);
  property->addUInt64Value(0);
  // Test all inputs are returned for combinatorial quarie for each output
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(o).size(), 2);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o)).size(), 2);
  // Test all outputs are returned for combinatorial quarie for each input
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i0).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(i1).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0)).size(), 1);
  EXPECT_EQ(SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i1)).size(), 1);
}
