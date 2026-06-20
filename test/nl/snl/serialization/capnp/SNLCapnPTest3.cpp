// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"

using namespace naja::NL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

// Regression test for the naja-if snapshot reload bug: designs lowered from
// SystemVerilog instantiate NLDB0 primitives that are generated lazily
// (parameterized gates, parameterized flip-flops, ...). Those models live in the
// auto-managed DB0, not in the dumped user DB, and are absent from a freshly
// reset universe. Before the fix, reloading such a snapshot threw
//   "cannot deserialize instance N: model not found (reference dbID 0, ...)".
// This test builds a design referencing the same lazily-created DB0 primitives a
// SystemVerilog comparison + sequential block lowers to (an N-input gate and a
// width-parameterized DFFRN), plus an eager primitive (assign), then round-trips
// it through Cap'n Proto and checks the reloaded hierarchy/models/connectivity.
class SNLCapnPDB0PrimitivesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* designs = NLLibrary::create(db_, NLName("designs"));
      SNLDesign* top = SNLDesign::create(designs, NLName("m"));
      universe->setTopDesign(top);

      // Top interface: clk, rstn, d[7:0] inputs; tick output.
      auto clk = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("clk"));
      auto rstn = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("rstn"));
      auto d = SNLBusTerm::create(top, SNLTerm::Direction::Input, 7, 0, NLName("d"));
      auto tick = SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("tick"));

      auto clkNet = SNLScalarNet::create(top, NLName("clk"));
      clk->setNet(clkNet);
      auto rstnNet = SNLScalarNet::create(top, NLName("rstn"));
      rstn->setNet(rstnNet);
      auto dNet = SNLBusNet::create(top, 7, 0, NLName("d"));
      d->setNet(dNet);
      auto tickNet = SNLScalarNet::create(top, NLName("tick"));
      tick->setNet(tickNet);
      // Internal register output net.
      auto qNet = SNLBusNet::create(top, 7, 0, NLName("count"));

      // Lazily-created NLDB0 primitives (the crux of the bug).
      auto dffrn8 = NLDB0::getOrCreateDFFRN(8);  // naja_dffrn__w8, design id 8
      ASSERT_NE(nullptr, dffrn8);
      auto andGate = NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 8);  // and_8
      ASSERT_NE(nullptr, andGate);
      // Eager NLDB0 primitive, present even in a fresh universe.
      auto assign = NLDB0::getAssign();
      ASSERT_NE(nullptr, assign);

      // reg: DFFRN width 8. Terms by id: C(0), D(1 bus), RN(2), Q(3 bus).
      auto reg = SNLInstance::create(top, dffrn8, NLName("reg"));
      reg->getInstTerm(dffrn8->getScalarTerm(NLID::DesignObjectID(0)))->setNet(clkNet);
      reg->getInstTerm(dffrn8->getScalarTerm(NLID::DesignObjectID(2)))->setNet(rstnNet);
      connectBus(reg, dffrn8->getBusTerm(NLID::DesignObjectID(1)), dNet);
      connectBus(reg, dffrn8->getBusTerm(NLID::DesignObjectID(3)), qNet);

      // cmp: 8-input AND of the register bits driving an internal net.
      auto cmpNet = SNLScalarNet::create(top, NLName("cmp"));
      auto cmp = SNLInstance::create(top, andGate, NLName("cmp"));
      cmp->getInstTerm(NLDB0::getGateSingleTerm(andGate))->setNet(cmpNet);  // output scalar
      connectBus(cmp, NLDB0::getGateNTerms(andGate), qNet);

      // buf: assign cmp -> tick.
      auto buf = SNLInstance::create(top, assign, NLName("buf"));
      buf->getInstTerm(NLDB0::getAssignInput())->setNet(cmpNet);
      buf->getInstTerm(NLDB0::getAssignOutput())->setNet(tickNet);
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    static void connectBus(SNLInstance* instance, SNLBusTerm* term, SNLBusNet* net) {
      for (auto bitTerm: term->getBits()) {
        auto busTermBit = static_cast<SNLBusTermBit*>(bitTerm);
        instance->getInstTerm(bitTerm)->setNet(net->getBit(busTermBit->getBit()));
      }
    }
  protected:
    NLDB* db_ {nullptr};
};

TEST_F(SNLCapnPDB0PrimitivesTest, testRoundTrip) {
  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapnPDB0PrimitivesTest_testRoundTrip";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);

  // Reset the universe: DB0 is recreated with only its eager primitives.
  NLUniverse::get()->destroy();
  db_ = nullptr;

  // Before the fix this threw "model not found".
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);

  auto top = db_->getTopDesign();
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(NLName("m"), top->getName());
  EXPECT_EQ(3u, top->getInstances().size());

  // All three models resolved, and they are the canonical DB0 primitives that
  // lazy lookup would return (i.e. merged in place, not duplicated).
  auto reg = top->getInstance(NLName("reg"));
  ASSERT_NE(nullptr, reg);
  ASSERT_NE(nullptr, reg->getModel());
  EXPECT_EQ(NLDB0::getOrCreateDFFRN(8), reg->getModel());
  EXPECT_TRUE(NLDB0::isDFFRN(reg->getModel()));
  EXPECT_EQ(NLName("naja_dffrn__w8"), reg->getModel()->getName());

  auto cmp = top->getInstance(NLName("cmp"));
  ASSERT_NE(nullptr, cmp);
  ASSERT_NE(nullptr, cmp->getModel());
  EXPECT_EQ(NLDB0::getOrCreateNInputGate(NLDB0::GateType::And, 8), cmp->getModel());
  EXPECT_TRUE(NLDB0::isGate(cmp->getModel()));
  EXPECT_EQ(NLName("and_8"), cmp->getModel()->getName());

  auto buf = top->getInstance(NLName("buf"));
  ASSERT_NE(nullptr, buf);
  EXPECT_EQ(NLDB0::getAssign(), buf->getModel());

  // Connectivity survives: the register output net feeds both the register Q
  // bits and the comparison gate inputs (8 + 8 instterms).
  auto qNet = top->getBusNet(NLName("count"));
  ASSERT_NE(nullptr, qNet);
  size_t qInstTerms = 0;
  for (auto bit: qNet->getBits()) {
    qInstTerms += bit->getInstTerms().size();
  }
  EXPECT_EQ(16u, qInstTerms);

  // The assign drives tick from the comparison output.
  auto cmpNet = top->getScalarNet(NLName("cmp"));
  ASSERT_NE(nullptr, cmpNet);
  EXPECT_EQ(2u, cmpNet->getInstTerms().size());  // gate output + assign input
  auto tickNet = top->getScalarNet(NLName("tick"));
  ASSERT_NE(nullptr, tickNet);
  EXPECT_EQ(1u, tickNet->getInstTerms().size());  // assign output
}

// Covers the multi-parameter resolver branch: a memory primitive cannot encode
// its full signature in a single designID, so on load it is rebuilt from the
// instance parameters carried in the snapshot.
TEST_F(SNLCapnPDB0PrimitivesTest, testMemoryRoundTrip) {
  auto top = db_->getTopDesign();
  ASSERT_NE(nullptr, top);

  NLDB0::MemorySignature signature;
  signature.width = 4;
  signature.depth = 8;
  signature.abits = 3;
  signature.readPorts = 1;
  signature.writePorts = 1;
  signature.resetMode = NLDB0::MemoryResetMode::None;
  auto memory = NLDB0::getOrCreateMemory(signature);
  ASSERT_NE(nullptr, memory);

  auto mem = SNLInstance::create(top, memory, NLName("ram"));
  auto addInstParam = [&](const char* name, const std::string& value) {
    SNLInstParameter::create(mem, memory->getParameter(NLName(name)), value);
  };
  addInstParam("WIDTH", "4");
  addInstParam("DEPTH", "8");
  addInstParam("ABITS", "3");
  addInstParam("RD_PORTS", "1");
  addInstParam("WR_PORTS", "1");
  addInstParam("RST_ENABLE", "0");
  addInstParam("RST_ASYNC", "0");
  addInstParam("RST_ACTIVE_LOW", "0");
  mem->getInstTerm(NLDB0::getMemoryClock(memory))->setNet(top->getScalarNet(NLName("clk")));

  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapnPDB0PrimitivesTest_testMemoryRoundTrip";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  SNLCapnP::dump(db_, outPath);

  NLUniverse::get()->destroy();
  db_ = nullptr;

  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  auto reloadedTop = db_->getTopDesign();
  ASSERT_NE(nullptr, reloadedTop);

  auto reloadedMem = reloadedTop->getInstance(NLName("ram"));
  ASSERT_NE(nullptr, reloadedMem);
  ASSERT_NE(nullptr, reloadedMem->getModel());
  EXPECT_TRUE(NLDB0::isMemory(reloadedMem->getModel()));
  EXPECT_EQ(NLDB0::getOrCreateMemory(signature), reloadedMem->getModel());
  // The signature was rebuilt from the round-tripped instance parameters.
  EXPECT_EQ(signature, NLDB0::getMemorySignature(reloadedMem));
}
