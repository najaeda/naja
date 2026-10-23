// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <kj/io.h>

#include "naja_nl_implementation.capnp.h"

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
#include "SNLParameter.h"
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

TEST_F(SNLCapnPDB0PrimitivesTest, testAbsentInstanceParameters) {
  auto* library = db_->getTopDesign()->getLibrary();
  auto* design = SNLDesign::create(library, NLName("empty"));
  auto* model = NLDB0::getOrCreateDFFRN(3);
  const auto modelReference = model->getReference();
  const auto libraryID = library->getID();
  const auto designID = design->getID();

  // The normal writer initializes even empty parameter lists. Build a message
  // with the field absent to exercise the reader's optional-field handling.
  ::capnp::MallocMessageBuilder message;
  auto db = message.initRoot<DBImplementation>();
  db.setId(db_->getID());
  auto libraryImplementation = db.initLibraryImplementations(1)[0];
  libraryImplementation.setId(libraryID);
  auto designImplementation = libraryImplementation.initSnlDesignImplementations(1)[0];
  designImplementation.setId(designID);
  auto instance = designImplementation.initInstances(1)[0];
  instance.setId(0);
  instance.setName("reg");
  auto reference = instance.initModelReference();
  reference.setDbID(modelReference.dbID_);
  reference.setLibraryID(modelReference.getDBDesignReference().libraryID_);
  reference.setDesignID(modelReference.getDBDesignReference().designID_);
  ASSERT_FALSE(instance.hasInstParameters());

  const auto outPath = std::filesystem::path(SNL_CAPNP_TEST_PATH)
    / "absent_instance_parameters";
  std::filesystem::create_directories(outPath);
  SNLCapnP::dumpInterface(db_, outPath / SNLCapnP::InterfaceName);
  kj::VectorOutputStream output;
  ::capnp::writePackedMessage(output, message);
  auto bytes = output.getArray();
  {
    std::ofstream file(outPath / SNLCapnP::ImplementationName, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.begin()), bytes.size());
    ASSERT_TRUE(file.good());
  }

  // Recreate the universe so resolving the parameter-free model must also
  // lazily rebuild its canonical DB0 primitive.
  NLUniverse::get()->destroy();
  db_ = nullptr;
  NLUniverse::create();
  const SNLCapnP::LoadingConfiguration configuration;
  db_ = SNLCapnP::loadInterface(outPath / SNLCapnP::InterfaceName, configuration);
  ASSERT_EQ(db_, SNLCapnP::loadImplementation(
    outPath / SNLCapnP::ImplementationName, configuration));
  design = db_->getLibrary(libraryID)->getSNLDesign(designID);
  ASSERT_EQ(1u, design->getInstances().size());
  auto* loaded = design->getInstance(NLName("reg"));
  ASSERT_NE(nullptr, loaded);
  EXPECT_TRUE(loaded->getInstParameters().empty());
  EXPECT_EQ(NLDB0::getOrCreateDFFRN(3), loaded->getModel());
}

// Covers the multi-parameter resolver branch: a memory primitive cannot encode
// its full signature in a single designID, so on load it is rebuilt from the
// instance parameters carried in the snapshot.
TEST_F(SNLCapnPDB0PrimitivesTest, testReadOnlyMemoryContentsRoundTrip) {
  auto* top = db_->getTopDesign();
  const NLDB0::MemorySignature signature{4,3,2,1,0};
  auto* model = NLDB0::getOrCreateMemory(signature);
  for (const auto& [name, init] : std::vector<std::pair<std::string,std::string>>{{"first","12'h3a1"},{"second","12'b10xz00111100"}}) {
    auto* instance = SNLInstance::create(top, model, NLName(name));
    for (const auto* parameter : {"WIDTH","DEPTH","ABITS","RD_PORTS","WR_PORTS","RST_ENABLE","RST_ASYNC","RST_ACTIVE_LOW"}) {
      auto* p = model->getParameter(NLName(parameter));
      SNLInstParameter::create(instance, p, p->getValue());
    }
    SNLInstParameter::create(instance, model->getParameter(NLName("INIT_ENABLE")), "1");
    SNLInstParameter::create(instance, model->getParameter(NLName("INIT")), init);
  }
  const auto outPath = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "read_only_memory_round_trip";
  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy(); db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_NE(db_, nullptr);
  top = db_->getTopDesign();
  auto* first = top->getInstance(NLName("first"));
  auto* second = top->getInstance(NLName("second"));
  ASSERT_NE(first, nullptr); ASSERT_NE(second, nullptr);
  EXPECT_EQ(first->getModel(), second->getModel());
  EXPECT_EQ(NLDB0::getOrCreateMemory(signature), first->getModel());
  EXPECT_EQ(signature, NLDB0::getMemorySignature(first));
  for (const auto* name : {"RST_ENABLE", "RST_ASYNC", "RST_ACTIVE_LOW", "INIT_ENABLE"}) {
    EXPECT_EQ(SNLParameter::Type::Boolean,
        first->getModel()->getParameter(NLName(name))->getType());
  }
  EXPECT_EQ("12'h3a1", first->getInstParameter(NLName("INIT"))->getValue());
  EXPECT_EQ("12'b10xz00111100", second->getInstParameter(NLName("INIT"))->getValue());
  EXPECT_EQ(nullptr, NLDB0::getMemoryClock(first->getModel()));
  EXPECT_EQ(2u, first->getModel()->getTerms().size());
}

TEST_F(SNLCapnPDB0PrimitivesTest, testShiftRoundTrip) {
  const std::vector<NLDB0::ShiftSignature> signatures{
      {5, 80, false, false}, {5, 80, true, false}, {5, 80, true, true}};
  auto* top = db_->getTopDesign();
  for (size_t i = 0; i < signatures.size(); ++i) {
    auto* model = NLDB0::getOrCreateShift(signatures[i]);
    auto* instance = SNLInstance::create(top, model, NLName("shift" + std::to_string(i)));
    for (auto* parameter : model->getParameters()) {
      SNLInstParameter::create(instance, parameter, parameter->getValue());
    }
  }
  const auto outPath = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "shift_round_trip";
  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy();
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_NE(db_, nullptr);
  top = db_->getTopDesign();
  for (size_t i = 0; i < signatures.size(); ++i) {
    auto* instance = top->getInstance(NLName("shift" + std::to_string(i)));
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(NLDB0::getOrCreateShift(signatures[i]), instance->getModel());
    EXPECT_EQ(signatures[i], NLDB0::getShiftSignature(instance));
    for (const auto* name : {"RIGHT", "ARITHMETIC"}) {
      EXPECT_EQ(SNLParameter::Type::Boolean,
          instance->getModel()->getParameter(NLName(name))->getType());
    }
    EXPECT_EQ(4u, instance->getInstParameters().size());
    EXPECT_EQ(80u, NLDB0::getShiftCount(instance->getModel())->getWidth());
  }
}

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

TEST_F(SNLCapnPDB0PrimitivesTest, testMultiplyRoundTrip) {
  auto* top = db_->getTopDesign();
  for (size_t width : {1u, 7u, 96u}) {
    auto* model = NLDB0::getOrCreateMultiply(width);
    auto* instance = SNLInstance::create(top, model, NLName("mul" + std::to_string(width)));
    for (auto* term : {NLDB0::getMultiplyInputA(model), NLDB0::getMultiplyInputB(model),
                       NLDB0::getMultiplyOutput(model)}) {
      auto* net = SNLBusNet::create(top, width - 1, 0,
          NLName(term->getName().getString() + std::to_string(width)));
      connectBus(instance, term, net);
    }
  }
  const auto outPath = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "multiply_round_trip";
  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy();
  db_ = SNLCapnP::load(outPath);
  ASSERT_NE(db_, nullptr);
  top = db_->getTopDesign();
  for (size_t width : {1u, 7u, 96u}) {
    auto* instance = top->getInstance(NLName("mul" + std::to_string(width)));
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(NLDB0::getOrCreateMultiply(width), instance->getModel());
    EXPECT_EQ(width, NLDB0::getMultiplyWidth(instance));
    for (auto* term : instance->getInstTerms()) EXPECT_NE(nullptr, term->getNet());
  }
}

TEST_F(SNLCapnPDB0PrimitivesTest, testAddSubRoundTrip) {
  auto* top = db_->getTopDesign();
  for (bool subtract : {false, true}) {
    for (size_t width : {1u, 7u, 96u}) {
      auto* model = NLDB0::getOrCreateAddSub(width, subtract);
      auto* instance = SNLInstance::create(top, model, NLName(std::string(subtract ? "sub" : "add") + std::to_string(width)));
      for (auto* term : {NLDB0::getAddSubInputA(model), NLDB0::getAddSubInputB(model),
                         NLDB0::getAddSubOutput(model)}) {
        auto* net = SNLBusNet::create(top, width - 1, 0,
            NLName(std::string(subtract ? "sub" : "add") + term->getName().getString() + std::to_string(width)));
        connectBus(instance, term, net);
      }
    }
  }
  const auto outPath = std::filesystem::path(SNL_CAPNP_TEST_PATH) / "add_sub_round_trip";
  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy();
  db_ = SNLCapnP::load(outPath);
  ASSERT_NE(db_, nullptr);
  top = db_->getTopDesign();
  for (bool subtract : {false, true}) {
    for (size_t width : {1u, 7u, 96u}) {
      auto* instance = top->getInstance(NLName(std::string(subtract ? "sub" : "add") + std::to_string(width)));
      ASSERT_NE(instance, nullptr);
      EXPECT_EQ(NLDB0::getOrCreateAddSub(width, subtract), instance->getModel());
      EXPECT_EQ(width, NLDB0::getAddSubWidth(instance));
      EXPECT_EQ(subtract, NLDB0::isSubtract(instance->getModel()));
      for (auto* term : instance->getInstTerms()) EXPECT_NE(nullptr, term->getNet());
    }
  }
}

TEST_F(SNLCapnPDB0PrimitivesTest, testAllGateFamiliesRoundTrip) {
  auto* top = db_->getTopDesign();
  std::vector<std::pair<std::string, NLID::LibraryID>> expected;
  for (auto type : {NLDB0::GateType::And, NLDB0::GateType::Nand,
      NLDB0::GateType::Or, NLDB0::GateType::Nor, NLDB0::GateType::Xor,
      NLDB0::GateType::Xnor, NLDB0::GateType::Buf, NLDB0::GateType::Not}) {
    const NLDB0::GateType gateType(type);
    auto* model = gateType.isNInput()
      ? NLDB0::getOrCreateNInputGate(gateType, 3)
      : NLDB0::getOrCreateNOutputGate(gateType, 3);
    SNLInstance::create(top, model, model->getName());
    expected.emplace_back(model->getName().getString(), model->getLibrary()->getID());
  }
  const auto outPath = std::filesystem::path(SNL_CAPNP_TEST_PATH)
    / "allGateFamiliesRoundTrip";
  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy();
  db_ = SNLCapnP::load(outPath);
  for (const auto& [name, libraryID] : expected) {
    auto* instance = db_->getTopDesign()->getInstance(NLName(name));
    ASSERT_NE(nullptr, instance);
    auto* model = instance->getModel();
    EXPECT_TRUE(NLDB0::isGate(model));
    EXPECT_EQ(name, model->getName().getString());
    EXPECT_EQ(libraryID, model->getLibrary()->getID());
    EXPECT_EQ(model, NLDB0::getOrCreatePrimitive(libraryID, 3));
  }
  EXPECT_EQ(11u, db_->getTopDesign()->getInstances().size());
}
