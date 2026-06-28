// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLName.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLScalarNet.h"

#include "SNLSVConstructor.h"
#include "SNLSVIntent.h"

using namespace naja::NL;

#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

std::filesystem::path writeIntentMiniFixture(const std::string& testName) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / testName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directories(outPath);
  const auto svPath = outPath / "intent_mini.sv";
  std::ofstream svFile(svPath);
  svFile << R"(
package mini_pkg;
  typedef logic [7:0] byte_t;
  typedef enum logic [1:0] {
    ST_IDLE = 2'b00,
    ST_RUN  = 2'b01,
    ST_DONE = 2'b11
  } state_e;
  typedef struct packed {
    logic   valid;
    byte_t  data;
    state_e state;
  } payload_t;
  typedef union packed {
    logic [7:0] raw;
    byte_t      data;
  } overlay_t;
  localparam int PLEN = (32 == 32) ? 34 : 56;
endpackage

module intent_mini #(
    parameter int DEPTH = 4
) (
    input  logic clk,
    input  logic rst_n
);
  import mini_pkg::*;
  localparam int IDX_W = $clog2(DEPTH);

  mini_pkg::state_e state_q;
  byte_t             byte_q;
  logic [3:0]        plain_q;
  payload_t          payload_q;
  overlay_t          overlay_q;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) state_q <= ST_IDLE;
    else        state_q <= ST_RUN;
  end
endmodule
)";
  return svPath;
}

SNLInstance* findInstanceByModel(SNLDesign* design, const std::string& modelName) {
  for (auto* instance : design->getInstances()) {
    if (instance && instance->getModel() &&
        instance->getModel()->getName().getString() == modelName) {
      return instance;
    }
  }
  return nullptr;
}

}  // namespace

class SNLSVIntentTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }

    SNLDesign* loadIntentMini() {
      SNLSVConstructor constructor(library_);
      SNLSVConstructor::ConstructOptions options;
      options.keepASTLink = true;
      constructor.construct(writeIntentMiniFixture(::testing::UnitTest::GetInstance()
                                                     ->current_test_info()
                                                     ->name()),
                            options);
      auto* top = library_->getSNLDesign(NLName("intent_mini"));
      EXPECT_NE(nullptr, top);
      return top;
    }

    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVIntentTest, returnsEnumIntentForSourceAndBitBlastedObjects) {
  EXPECT_FALSE(SNLSVIntent::available());
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);
  ASSERT_TRUE(SNLSVIntent::available());

  auto* stateQ = top->getNet(NLName("state_q"));
  ASSERT_NE(nullptr, stateQ);

  const auto type = SNLSVIntent::typeOf(stateQ);
  ASSERT_TRUE(type.valid);
  EXPECT_EQ("mini_pkg::state_e", type.typeName);
  EXPECT_EQ("enum", type.canonicalKind);
  ASSERT_TRUE(type.isEnum);
  EXPECT_EQ(2u, type.enumWidth);
  ASSERT_EQ(3u, type.members.size());
  EXPECT_EQ("ST_IDLE", type.members[0].name);
  EXPECT_EQ("2'b00", type.members[0].encoding);
  EXPECT_EQ("ST_RUN", type.members[1].name);
  EXPECT_EQ("2'b01", type.members[1].encoding);
  EXPECT_EQ("ST_DONE", type.members[2].name);
  EXPECT_EQ("2'b11", type.members[2].encoding);
  EXPECT_NE(std::string::npos, type.enumDeclLoc.file.getString().find("intent_mini.sv"));
  EXPECT_EQ(4u, type.enumDeclLoc.line);

  auto* ff = findInstanceByModel(top, "naja_dffrn__w2");
  ASSERT_NE(nullptr, ff);
  const auto ffType = SNLSVIntent::typeOf(ff);
  ASSERT_TRUE(ffType.valid);
  EXPECT_EQ(type.typeName, ffType.typeName);
  ASSERT_TRUE(ffType.isEnum);
  EXPECT_EQ(type.members[2].encoding, ffType.members[2].encoding);
}

TEST_F(SNLSVIntentTest, returnsScalarAliasAndPackedAggregateIntent) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  auto* clk = top->getNet(NLName("clk"));
  ASSERT_NE(nullptr, clk);
  const auto clkType = SNLSVIntent::typeOf(clk);
  ASSERT_TRUE(clkType.valid);
  EXPECT_EQ("logic", clkType.typeName);
  EXPECT_EQ("scalar", clkType.canonicalKind);
  EXPECT_FALSE(clkType.isEnum);
  EXPECT_FALSE(clkType.isStruct);

  auto* byteQ = top->getNet(NLName("byte_q"));
  ASSERT_NE(nullptr, byteQ);
  const auto byteType = SNLSVIntent::typeOf(byteQ);
  ASSERT_TRUE(byteType.valid);
  EXPECT_EQ("mini_pkg::byte_t", byteType.typeName);
  EXPECT_EQ("packed_array", byteType.canonicalKind);
  EXPECT_FALSE(byteType.isEnum);
  EXPECT_FALSE(byteType.isStruct);

  auto* plainQ = top->getNet(NLName("plain_q"));
  ASSERT_NE(nullptr, plainQ);
  const auto plainType = SNLSVIntent::typeOf(plainQ);
  ASSERT_TRUE(plainType.valid);
  EXPECT_EQ("logic[3:0]", plainType.typeName);
  EXPECT_EQ("packed_array", plainType.canonicalKind);
  EXPECT_FALSE(plainType.isEnum);
  EXPECT_FALSE(plainType.isStruct);

  auto* payloadQ = top->getNet(NLName("payload_q"));
  ASSERT_NE(nullptr, payloadQ);
  const auto payloadType = SNLSVIntent::typeOf(payloadQ);
  ASSERT_TRUE(payloadType.valid);
  EXPECT_EQ("mini_pkg::payload_t", payloadType.typeName);
  EXPECT_EQ("packed_struct", payloadType.canonicalKind);
  EXPECT_FALSE(payloadType.isEnum);
  ASSERT_TRUE(payloadType.isStruct);
  EXPECT_EQ(11u, payloadType.structWidth);
  EXPECT_EQ(9u, payloadType.structDeclLoc.line);
  ASSERT_EQ(3u, payloadType.fields.size());
  EXPECT_EQ("valid", payloadType.fields[0].name);
  EXPECT_EQ("logic", payloadType.fields[0].typeName);
  EXPECT_EQ(10u, payloadType.fields[0].msb);
  EXPECT_EQ(10u, payloadType.fields[0].lsb);
  EXPECT_EQ("data", payloadType.fields[1].name);
  EXPECT_EQ("mini_pkg::byte_t", payloadType.fields[1].typeName);
  EXPECT_EQ(9u, payloadType.fields[1].msb);
  EXPECT_EQ(2u, payloadType.fields[1].lsb);
  EXPECT_EQ("state", payloadType.fields[2].name);
  EXPECT_EQ("mini_pkg::state_e", payloadType.fields[2].typeName);
  EXPECT_EQ(1u, payloadType.fields[2].msb);
  EXPECT_EQ(0u, payloadType.fields[2].lsb);

  auto* overlayQ = top->getNet(NLName("overlay_q"));
  ASSERT_NE(nullptr, overlayQ);
  const auto overlayType = SNLSVIntent::typeOf(overlayQ);
  ASSERT_TRUE(overlayType.valid);
  EXPECT_EQ("mini_pkg::overlay_t", overlayType.typeName);
  EXPECT_EQ("packed_union", overlayType.canonicalKind);
  ASSERT_TRUE(overlayType.isStruct);
  EXPECT_EQ(8u, overlayType.structWidth);
  ASSERT_EQ(2u, overlayType.fields.size());
  EXPECT_EQ(7u, overlayType.fields[0].msb);
  EXPECT_EQ(0u, overlayType.fields[0].lsb);
  EXPECT_EQ(7u, overlayType.fields[1].msb);
  EXPECT_EQ(0u, overlayType.fields[1].lsb);

  auto* synthetic = SNLScalarNet::create(top, NLName("synthetic"));
  EXPECT_FALSE(SNLSVIntent::typeOf(synthetic).valid);
}

TEST_F(SNLSVIntentTest, returnsParametersAndPackageMembers) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);

  const auto params = SNLSVIntent::parametersOf(top);
  ASSERT_TRUE(params.valid);
  EXPECT_EQ("intent_mini", params.module);
  EXPECT_EQ(2u, params.params.size());

  std::map<std::string, SNLSVIntentParam> byName;
  for (const auto& param : params.params) {
    byName.emplace(param.name, param);
  }
  ASSERT_TRUE(byName.contains("DEPTH"));
  ASSERT_TRUE(byName.contains("IDX_W"));
  EXPECT_FALSE(byName["DEPTH"].localParam);
  EXPECT_EQ("4", byName["DEPTH"].value);
  EXPECT_EQ("4", byName["DEPTH"].expr);
  EXPECT_TRUE(byName["IDX_W"].localParam);
  EXPECT_EQ("2", byName["IDX_W"].value);
  EXPECT_EQ("$clog2(DEPTH)", byName["IDX_W"].expr);

  const auto plen = SNLSVIntent::packageMember("mini_pkg", "PLEN");
  EXPECT_EQ("PLEN", plen.name);
  EXPECT_EQ("34", plen.value);
  EXPECT_EQ("(32 == 32) ? 34 : 56", plen.expr);
  EXPECT_TRUE(plen.localParam);

  const auto packageType = SNLSVIntent::packageMemberType("mini_pkg", "state_e");
  ASSERT_TRUE(packageType.valid);
  EXPECT_EQ("mini_pkg::state_e", packageType.typeName);
  EXPECT_TRUE(packageType.isEnum);
}

TEST_F(SNLSVIntentTest, returnsInvalidWithoutWarmLink) {
  auto* top = loadIntentMini();
  ASSERT_NE(nullptr, top);
  auto* stateQ = top->getNet(NLName("state_q"));
  ASSERT_NE(nullptr, stateQ);

  SNLSVLiveASTLinkRegistry::clear(library_->getDB());
  EXPECT_FALSE(SNLSVIntent::available());
  EXPECT_FALSE(SNLSVIntent::typeOf(stateQ).valid);
  EXPECT_FALSE(SNLSVIntent::parametersOf(top).valid);
  EXPECT_TRUE(SNLSVIntent::packageMember("mini_pkg", "PLEN").name.empty());
}
