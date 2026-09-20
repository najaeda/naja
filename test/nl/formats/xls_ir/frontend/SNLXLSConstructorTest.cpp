// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <kj/io.h>
#include <kj/std/iostream.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "NLDB.h"
#include "NLDB0.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLXLSConstructor.h"
#include "SNLXLSConstructorException.h"
#include "SNLXLSIRReader.h"
#include "xls_ir_bridge.capnp.h"

using namespace naja::NL;

namespace {

struct BridgePayloadOptions {
  uint32_t schemaVersion{SNLXLSIRReader::SchemaVersion};
  std::string xlsRevision{SNLXLSIRReader::XLSRevision};
  XLSIRBridge::EntityKind kind{XLSIRBridge::EntityKind::FUNCTION};
  bool tupleNode{false};
};

void writeBridgePayload(
  const std::filesystem::path& path,
  const BridgePayloadOptions& options = {}) {
  ::capnp::MallocMessageBuilder message;
  auto payload = message.initRoot<XLSIRBridge::BridgePayload>();
  payload.setSchemaVersion(options.schemaVersion);
  payload.setXlsRevision(options.xlsRevision);
  payload.setPackageName("bridge_test");

  auto entity = payload.initEntities(1)[0];
  entity.setKind(options.kind);
  entity.setName("bridge_add");
  entity.setIsTop(true);
  entity.setResult("sum");
  entity.setOutputName("result");

  auto parameters = entity.initParameters(2);
  parameters[0].setName("a");
  parameters[0].initType().setBits(8);
  parameters[1].setName("b");
  parameters[1].initType().setBits(8);

  auto node = entity.initNodes(1)[0];
  node.setId(3);
  node.setName("sum");
  node.setOp("add");
  if (options.tupleNode) {
    node.initType().initTuple(0);
  } else {
    node.initType().setBits(8);
  }
  auto operands = node.initOperands(2);
  operands.set(0, "a");
  operands.set(1, "b");
  node.setSource("bridge_test.ir:7");

  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  ASSERT_TRUE(output);
  kj::std::StdOutputStream rawOutput(output);
  kj::BufferedOutputStreamWrapper bufferedOutput(rawOutput);
  ::capnp::writePackedMessage(bufferedOutput, message);
  bufferedOutput.flush();
  output.flush();
  ASSERT_TRUE(output);
}

}  // namespace

class SNLXLSConstructorTest : public ::testing::Test {
  protected:
    void SetUp() override {
      auto* universe = NLUniverse::create();
      auto* db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("WORK"));
      const auto* testInfo =
        ::testing::UnitTest::GetInstance()->current_test_info();
      bridgePath_ = std::filesystem::temp_directory_path() /
        (std::string("naja_") + testInfo->test_suite_name() + "_" +
          testInfo->name() + ".capnp");
    }

    void TearDown() override {
      std::error_code error;
      std::filesystem::remove(bridgePath_, error);
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
      library_ = nullptr;
    }

    NLLibrary* library_{nullptr};
    std::filesystem::path bridgePath_{};
};

TEST_F(SNLXLSConstructorTest, ConstructsFromVersionedBridgePayload) {
  writeBridgePayload(bridgePath_);

  SNLXLSConstructor constructor(library_);
  auto* design = constructor.construct(bridgePath_);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(NLName("bridge_add"), design->getName());
  ASSERT_NE(nullptr, design->getBusTerm(NLName("a")));
  ASSERT_NE(nullptr, design->getBusTerm(NLName("b")));
  ASSERT_NE(nullptr, design->getBusTerm(NLName("result")));

  size_t fullAdders = 0;
  for (auto* instance : design->getInstances()) {
    if (instance->getModel() == NLDB0::getFA()) {
      ++fullAdders;
    }
  }
  EXPECT_EQ(8, fullAdders);
}

TEST_F(SNLXLSConstructorTest, RejectsIncompatibleBridgeSchema) {
  BridgePayloadOptions options;
  options.schemaVersion = SNLXLSIRReader::SchemaVersion + 1;
  writeBridgePayload(bridgePath_, options);

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(bridgePath_);
    FAIL() << "incompatible bridge schema was accepted";
  } catch (const SNLXLSConstructorException& exception) {
    EXPECT_NE(std::string::npos,
      std::string(exception.what()).find("incompatible schema version"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("bridge_add")));
}

TEST_F(SNLXLSConstructorTest, RejectsIncompatibleXLSRevision) {
  BridgePayloadOptions options;
  options.xlsRevision = "different-xls-revision";
  writeBridgePayload(bridgePath_, options);

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(bridgePath_);
    FAIL() << "incompatible XLS revision was accepted";
  } catch (const SNLXLSConstructorException& exception) {
    EXPECT_NE(std::string::npos,
      std::string(exception.what()).find("incompatible XLS revision"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("bridge_add")));
}

TEST_F(SNLXLSConstructorTest, RejectsNonFunctionBridgeTop) {
  BridgePayloadOptions options;
  options.kind = XLSIRBridge::EntityKind::BLOCK;
  writeBridgePayload(bridgePath_, options);

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(bridgePath_);
    FAIL() << "block top was accepted by the function importer";
  } catch (const SNLXLSConstructorException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(std::string::npos, message.find("is a block"));
    EXPECT_NE(std::string::npos, message.find("expected a function"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("bridge_add")));
}

TEST_F(SNLXLSConstructorTest, RejectsUnsupportedBridgeType) {
  BridgePayloadOptions options;
  options.tupleNode = true;
  writeBridgePayload(bridgePath_, options);

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(bridgePath_);
    FAIL() << "tuple node was accepted by the bits-only importer";
  } catch (const SNLXLSConstructorException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(std::string::npos, message.find("node 'sum'"));
    EXPECT_NE(std::string::npos, message.find("unsupported type tuple"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("bridge_add")));
}

TEST_F(SNLXLSConstructorTest, RejectsMalformedBridgePayload) {
  {
    std::ofstream output(bridgePath_, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output);
    output << "not a packed Cap'n Proto message";
  }

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(bridgePath_);
    FAIL() << "malformed bridge payload was accepted";
  } catch (const SNLXLSConstructorException& exception) {
    EXPECT_NE(std::string::npos,
      std::string(exception.what()).find("malformed Cap'n Proto message"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("bridge_add")));
}

TEST_F(SNLXLSConstructorTest, ConstructsBitsOnlyAddSubSelectFunction) {
  SNLXLSIRFunction function;
  function.name = "add_select";
  function.parameters = {{"a", 8}, {"b", 8}, {"select", 1}};
  function.nodes = {
    {6, "selected", "sel", 8, {"select", "sum", "difference"}, "fixture.ir:9"},
    {5, "difference", "sub", 8, {"a", "b"}, "fixture.ir:8"},
    {4, "sum", "add", 8, {"a", "b"}, "fixture.ir:7"},
  };
  function.result = "selected";
  function.outputName = "result";

  SNLXLSConstructor constructor(library_);
  auto* design = constructor.construct(function);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(NLName("add_select"), design->getName());

  auto* a = design->getBusTerm(NLName("a"));
  auto* b = design->getBusTerm(NLName("b"));
  auto* select = design->getScalarTerm(NLName("select"));
  auto* result = design->getBusTerm(NLName("result"));
  ASSERT_NE(nullptr, a);
  ASSERT_NE(nullptr, b);
  ASSERT_NE(nullptr, select);
  ASSERT_NE(nullptr, result);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, select->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, result->getDirection());
  EXPECT_EQ(8, a->getWidth());
  EXPECT_EQ(8, b->getWidth());
  EXPECT_EQ(8, result->getWidth());

  size_t fullAdders = 0;
  size_t notGates = 0;
  size_t muxes = 0;
  for (auto* instance : design->getInstances()) {
    if (instance->getModel() == NLDB0::getFA()) {
      ++fullAdders;
    } else if (NLDB0::isMux2(instance->getModel())) {
      ++muxes;
    } else if (NLDB0::getGateName(instance->getModel()) == "not") {
      ++notGates;
    }
  }
  EXPECT_EQ(16, fullAdders);
  EXPECT_EQ(8, notGates);
  EXPECT_EQ(1, muxes);
  for (auto* bit : result->getBits()) {
    EXPECT_NE(nullptr, bit->getNet());
  }
}

TEST_F(SNLXLSConstructorTest, RejectsUnsupportedOperationWithoutCreatingDesign) {
  SNLXLSIRFunction function;
  function.name = "unsupported";
  function.parameters = {{"a", 8}, {"b", 8}};
  function.nodes = {
    {3, "product", "umul", 8, {"a", "b"}, "unsupported.ir:7"},
  };
  function.result = "product";

  SNLXLSConstructor constructor(library_);
  try {
    constructor.construct(function);
    FAIL() << "unsupported operation was accepted";
  } catch (const SNLXLSConstructorException& exception) {
    const std::string message = exception.what();
    EXPECT_NE(std::string::npos, message.find("umul"));
    EXPECT_NE(std::string::npos, message.find("unsupported.ir:7"));
  }
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("unsupported")));
}

TEST_F(SNLXLSConstructorTest, RejectsWidthMismatchWithoutCreatingDesign) {
  SNLXLSIRFunction function;
  function.name = "width_mismatch";
  function.parameters = {{"a", 8}, {"b", 4}};
  function.nodes = {
    {3, "sum", "add", 8, {"a", "b"}, "width.ir:7"},
  };
  function.result = "sum";

  SNLXLSConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(function), SNLXLSConstructorException);
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("width_mismatch")));
}

TEST_F(SNLXLSConstructorTest, RejectsCyclicNodeGraphWithoutCreatingDesign) {
  SNLXLSIRFunction function;
  function.name = "cycle";
  function.parameters = {{"a", 8}};
  function.nodes = {
    {2, "left", "add", 8, {"a", "right"}, "cycle.ir:7"},
    {3, "right", "sub", 8, {"left", "a"}, "cycle.ir:8"},
  };
  function.result = "left";

  SNLXLSConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(function), SNLXLSConstructorException);
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("cycle")));
}
