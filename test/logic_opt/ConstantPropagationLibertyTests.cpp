// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "ConstantPropagation.h"
#include "DNL.h"
#include "NLLibraryTruthTables.h"
#include "NLUniverse.h"
#include "SNLLibertyConstructor.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

using namespace naja::NL;
using namespace naja::NAJA_OPT;

namespace {

std::filesystem::path makeTemporaryDirectory() {
  auto dir = std::filesystem::temp_directory_path() /
      ("naja_cp_liberty_test_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(dir);
  return dir;
}

void writeFile(const std::filesystem::path& path, const std::string& contents) {
  std::ofstream stream(path, std::ios::out | std::ios::trunc);
  ASSERT_TRUE(stream.good()) << "failed to open " << path;
  stream << contents;
  ASSERT_TRUE(stream.good()) << "failed to write " << path;
}

}  // namespace

class ConstantPropagationLibertyTests : public ::testing::Test {
 protected:
  void SetUp() override { NLUniverse::create(); }

  void TearDown() override {
    naja::DNL::destroy();
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
  }
};

TEST_F(ConstantPropagationLibertyTests,
       TestConstantPropagationCanMaterializeLogic0FromLibertyPrimitives) {
  auto db = NLDB::create(NLUniverse::get());
  auto primitives = NLLibrary::create(
      db, NLLibrary::Type::Primitives, NLName("PRIMS"));

  auto tempDir = makeTemporaryDirectory();
  auto libertyPath = tempDir / "logic01_buf.lib";
  writeFile(libertyPath, R"lib(
library (logic01_buf) {
  cell (logic0) {
    pin(Z) {
      direction : output ;
      function : "0" ;
    }
  }
  cell (logic1) {
    pin(Z) {
      direction : output ;
      function : "1" ;
    }
  }
  cell (BUF1) {
    pin(A) {
      direction : input ;
    }
    pin(Z) {
      direction : output ;
      function : "A" ;
    }
  }
}
)lib");

  SNLLibertyConstructor libertyConstructor(primitives);
  libertyConstructor.construct(libertyPath);

  auto logic0 = primitives->getSNLDesign(NLName("logic0"));
  ASSERT_NE(nullptr, logic0);
  EXPECT_TRUE(SNLDesignModeling::isConst0(logic0));
  EXPECT_EQ(logic0, NLLibraryTruthTables::getDesignForTruthTable(
      primitives, SNLTruthTable::Logic0()).first);
  EXPECT_NO_THROW(NLLibraryTruthTables::construct(primitives));
  EXPECT_EQ(logic0, NLLibraryTruthTables::getDesignForTruthTable(
      primitives, SNLTruthTable::Logic0()).first);

  auto buffer = primitives->getSNLDesign(NLName("BUF1"));
  ASSERT_NE(nullptr, buffer);
  auto bufferInput = buffer->getScalarTerm(NLName("A"));
  auto bufferOutput = buffer->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, bufferInput);
  ASSERT_NE(nullptr, bufferOutput);

  auto designLibrary = NLLibrary::create(db, NLName("DESIGN"));
  auto top = SNLDesign::create(designLibrary, NLName("top"));
  NLUniverse::get()->setTopDesign(top);

  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("y"));

  auto constant0 = SNLScalarNet::create(top, NLName("constant0"));
  SNLDesignModeling::createConstantDriver(constant0, NLLogicValue::Zero, NLConstantDriverKind::Assign);
  auto firstBufferOutput = SNLScalarNet::create(top, NLName("first_buffer_output"));
  auto topOutputNet = SNLScalarNet::create(top, NLName("top_output"));

  auto firstBuffer = SNLInstance::create(top, buffer, NLName("first_buffer"));
  auto secondBuffer = SNLInstance::create(top, buffer, NLName("second_buffer"));

  firstBuffer->getInstTerm(bufferInput)->setNet(constant0);
  firstBuffer->getInstTerm(bufferOutput)->setNet(firstBufferOutput);
  secondBuffer->getInstTerm(bufferInput)->setNet(firstBufferOutput);
  secondBuffer->getInstTerm(bufferOutput)->setNet(topOutputNet);
  topOutput->setNet(topOutputNet);

  ConstantPropagation constantPropagation;
  constantPropagation.setTruthTableEngine(true);

  EXPECT_NO_THROW(constantPropagation.run());

  std::filesystem::remove_all(tempDir);
}
