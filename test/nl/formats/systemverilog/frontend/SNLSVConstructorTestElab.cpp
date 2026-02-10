// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>

#include "NLUniverse.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

class SNLSVConstructorTestElab: public ::testing::Test {
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
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestElab, elaborateParameterizedPorts) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"param_inst.sv");

  auto leaf = library_->getSNLDesign(NLName("leaf"));
  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(leaf, nullptr);
  ASSERT_NE(top, nullptr);

  auto leafA = leaf->getBusTerm(NLName("a"));
  auto leafY = leaf->getBusTerm(NLName("y"));
  ASSERT_NE(leafA, nullptr);
  ASSERT_NE(leafY, nullptr);
  EXPECT_EQ(4, leafA->getWidth());
  EXPECT_EQ(4, leafY->getWidth());

  auto topA = top->getBusNet(NLName("a"));
  auto topY = top->getBusNet(NLName("y"));
  ASSERT_NE(topA, nullptr);
  ASSERT_NE(topY, nullptr);
  EXPECT_EQ(4, topA->getWidth());
  EXPECT_EQ(4, topY->getWidth());

  auto inst = top->getInstance(NLName("u0"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(leaf, inst->getModel());

  auto leafABit0 = leafA->getBit(0);
  auto leafYBit0 = leafY->getBit(0);
  ASSERT_NE(leafABit0, nullptr);
  ASSERT_NE(leafYBit0, nullptr);

  auto instABit0 = inst->getInstTerm(leafABit0);
  auto instYBit0 = inst->getInstTerm(leafYBit0);
  ASSERT_NE(instABit0, nullptr);
  ASSERT_NE(instYBit0, nullptr);
  EXPECT_EQ(topA->getBit(0), instABit0->getNet());
  EXPECT_EQ(topY->getBit(0), instYBit0->getNet());

  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "elaborateParameterizedPorts";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
}
