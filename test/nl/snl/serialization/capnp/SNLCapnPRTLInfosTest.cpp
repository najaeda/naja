// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstance.h"
#include "SNLRTLInfos.h"

#include "SNLCapnP.h"

using namespace naja::NL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

// Round-trips RTL source metadata (SNLRTLInfos: source location + extra
// key/value infos) through Cap'n Proto for every object kind that carries it:
// design, scalar/bus terms, instances, scalar/bus nets and bus net bits.
class SNLCapnPRTLInfosTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* designs = NLLibrary::create(db_, NLName("designs"));

      // A small model to instantiate.
      SNLDesign* model = SNLDesign::create(designs, NLName("model"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));

      SNLDesign* top = SNLDesign::create(designs, NLName("top"));
      universe->setTopDesign(top);

      auto designInfos = SNLRTLInfos::create(top);
      designInfos->setSourceLoc(makeSourceLoc("top.sv", 1));
      designInfos->setInfo(NLName("module_kind"), "rtl");

      auto scalarTerm = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("a"));
      auto scalarTermInfos = SNLRTLInfos::create(scalarTerm);
      scalarTermInfos->setSourceLoc(makeSourceLoc("top.sv", 2));
      scalarTermInfos->setInfo(NLName("port_attr"), "keep");

      auto busTerm = SNLBusTerm::create(top, SNLTerm::Direction::Output, 3, 0, NLName("b"));
      auto busTermInfos = SNLRTLInfos::create(busTerm);
      busTermInfos->setSourceLoc(makeSourceLoc("top.sv", 3));

      auto scalarNet = SNLScalarNet::create(top, NLName("n"));
      auto scalarNetInfos = SNLRTLInfos::create(scalarNet);
      scalarNetInfos->setInfo(NLName("net_attr"), "wire");

      auto busNet = SNLBusNet::create(top, 3, 0, NLName("bn"));
      auto busNetInfos = SNLRTLInfos::create(busNet);
      busNetInfos->setSourceLoc(makeSourceLoc("top.sv", 4));
      auto busNetBit = busNet->getBit(2);
      auto busNetBitInfos = SNLRTLInfos::create(busNetBit);
      busNetBitInfos->setInfo(NLName("bit_attr"), "special");

      auto instance = SNLInstance::create(top, model, NLName("inst"));
      auto instanceInfos = SNLRTLInfos::create(instance);
      instanceInfos->setSourceLoc(makeSourceLoc("top.sv", 5));
      instanceInfos->setInfo(NLName("inst_attr"), "u_inst");
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    static SNLSourceLoc makeSourceLoc(const std::string& file, std::uint32_t line) {
      SNLSourceLoc sourceLoc;
      sourceLoc.file = NLName(file);
      sourceLoc.line = line;
      sourceLoc.endLine = line + 1;
      sourceLoc.column = 4;
      sourceLoc.endColumn = 8;
      return sourceLoc;
    }
    static void expectSourceLoc(
      const SNLRTLInfos* infos,
      const std::string& file,
      std::uint32_t line) {
      ASSERT_NE(nullptr, infos);
      ASSERT_TRUE(infos->hasSourceLoc());
      const auto& sourceLoc = infos->getSourceLoc();
      EXPECT_EQ(NLName(file), sourceLoc->file);
      EXPECT_EQ(line, sourceLoc->line);
      EXPECT_EQ(line + 1, sourceLoc->endLine);
      EXPECT_EQ(4u, sourceLoc->column);
      EXPECT_EQ(8u, sourceLoc->endColumn);
    }
  protected:
    NLDB* db_ {nullptr};
};

TEST_F(SNLCapnPRTLInfosTest, testRoundTrip) {
  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapnPRTLInfosTest_testRoundTrip";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);

  NLUniverse::get()->destroy();
  db_ = nullptr;

  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  auto top = db_->getTopDesign();
  ASSERT_NE(nullptr, top);

  // Design.
  ASSERT_TRUE(top->hasRTLInfos());
  expectSourceLoc(top->getRTLInfos(), "top.sv", 1);
  EXPECT_EQ("rtl", top->getRTLInfos()->getInfo(NLName("module_kind")));

  // Scalar term.
  auto scalarTerm = top->getScalarTerm(NLName("a"));
  ASSERT_NE(nullptr, scalarTerm);
  ASSERT_TRUE(scalarTerm->hasRTLInfos());
  expectSourceLoc(scalarTerm->getRTLInfos(), "top.sv", 2);
  EXPECT_EQ("keep", scalarTerm->getRTLInfos()->getInfo(NLName("port_attr")));

  // Bus term (source loc only, no extra infos).
  auto busTerm = top->getBusTerm(NLName("b"));
  ASSERT_NE(nullptr, busTerm);
  ASSERT_TRUE(busTerm->hasRTLInfos());
  expectSourceLoc(busTerm->getRTLInfos(), "top.sv", 3);
  EXPECT_TRUE(busTerm->getRTLInfos()->getInfos().empty());

  // Scalar net (extra info only, no source loc).
  auto scalarNet = top->getScalarNet(NLName("n"));
  ASSERT_NE(nullptr, scalarNet);
  ASSERT_TRUE(scalarNet->hasRTLInfos());
  EXPECT_FALSE(scalarNet->getRTLInfos()->hasSourceLoc());
  EXPECT_EQ("wire", scalarNet->getRTLInfos()->getInfo(NLName("net_attr")));

  // Bus net + bus net bit.
  auto busNet = top->getBusNet(NLName("bn"));
  ASSERT_NE(nullptr, busNet);
  ASSERT_TRUE(busNet->hasRTLInfos());
  expectSourceLoc(busNet->getRTLInfos(), "top.sv", 4);
  auto busNetBit = busNet->getBit(2);
  ASSERT_NE(nullptr, busNetBit);
  ASSERT_TRUE(busNetBit->hasRTLInfos());
  EXPECT_EQ("special", busNetBit->getRTLInfos()->getInfo(NLName("bit_attr")));

  // Instance.
  auto instance = top->getInstance(NLName("inst"));
  ASSERT_NE(nullptr, instance);
  ASSERT_TRUE(instance->hasRTLInfos());
  expectSourceLoc(instance->getRTLInfos(), "top.sv", 5);
  EXPECT_EQ("u_inst", instance->getRTLInfos()->getInfo(NLName("inst_attr")));
}
