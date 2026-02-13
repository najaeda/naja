// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <iterator>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLAttributes.h"
#include "SNLBitNet.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLUtils.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

bool hasAttribute(const NLObject* object, const std::string& name) {
  for (const auto& attribute : SNLAttributes::getAttributes(object)) {
    if (attribute.getName().getString() == name) {
      return true;
    }
  }
  return false;
}

}

class SNLSVConstructorTestSimple: public ::testing::Test {
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

TEST_F(SNLSVConstructorTestSimple, parseSimpleModule) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"simple.sv");

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  EXPECT_TRUE(hasAttribute(top, "sv_src_file"));
  EXPECT_TRUE(hasAttribute(top, "sv_src_line"));
  EXPECT_TRUE(hasAttribute(top, "sv_src_column"));
  EXPECT_EQ(3, top->getTerms().size());
  auto a = top->getTerm(NLName("a"));
  ASSERT_NE(a, nullptr);
  EXPECT_TRUE(hasAttribute(a, "sv_src_file"));
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto b = top->getTerm(NLName("b"));
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto y = top->getTerm(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Output, y->getDirection());

  EXPECT_NE(top->getNet(NLName("a")), nullptr);
  EXPECT_NE(top->getNet(NLName("b")), nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
  EXPECT_EQ(top, SNLUtils::findTop(library_));

  auto aNet = top->getNet(NLName("a"));
  auto bNet = top->getNet(NLName("b"));
  auto yNet = top->getNet(NLName("y"));
  ASSERT_NE(aNet, nullptr);
  ASSERT_NE(bNet, nullptr);
  ASSERT_NE(yNet, nullptr);

  SNLInstance* assignInst = nullptr;
  SNLInstance* andInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      assignInst = inst;
    } else if (NLDB0::isGate(inst->getModel()) &&
               NLDB0::getGateName(inst->getModel()) == "and") {
      andInst = inst;
    }
  }
  ASSERT_NE(assignInst, nullptr);
  ASSERT_NE(andInst, nullptr);
  EXPECT_TRUE(hasAttribute(assignInst, "sv_src_line"));
  EXPECT_TRUE(hasAttribute(andInst, "sv_src_line"));

  auto assignInput = NLDB0::getAssignInput();
  auto assignOutput = NLDB0::getAssignOutput();
  ASSERT_NE(assignInput, nullptr);
  ASSERT_NE(assignOutput, nullptr);
  auto assignInTerm = assignInst->getInstTerm(assignInput);
  auto assignOutTerm = assignInst->getInstTerm(assignOutput);
  ASSERT_NE(assignInTerm, nullptr);
  ASSERT_NE(assignOutTerm, nullptr);
  auto yBitNet = dynamic_cast<SNLBitNet*>(yNet);
  ASSERT_NE(yBitNet, nullptr);
  EXPECT_EQ(assignOutTerm->getNet(), yBitNet);

  auto andInputs = NLDB0::getGateNTerms(andInst->getModel());
  auto andOutput = NLDB0::getGateSingleTerm(andInst->getModel());
  ASSERT_NE(andInputs, nullptr);
  ASSERT_NE(andOutput, nullptr);
  auto andIn0 = andInst->getInstTerm(andInputs->getBitAtPosition(0));
  auto andIn1 = andInst->getInstTerm(andInputs->getBitAtPosition(1));
  auto andOut = andInst->getInstTerm(andOutput);
  ASSERT_NE(andIn0, nullptr);
  ASSERT_NE(andIn1, nullptr);
  ASSERT_NE(andOut, nullptr);
  auto aBitNet = dynamic_cast<SNLBitNet*>(aNet);
  auto bBitNet = dynamic_cast<SNLBitNet*>(bNet);
  ASSERT_NE(aBitNet, nullptr);
  ASSERT_NE(bBitNet, nullptr);
  EXPECT_EQ(andIn0->getNet(), aBitNet);
  EXPECT_EQ(andIn1->getNet(), bBitNet);
  EXPECT_EQ(assignInTerm->getNet(), andOut->getNet());
}

TEST_F(SNLSVConstructorTestSimple, parseUpCounter) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "up_counter";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  auto jsonPath = outPath / "up_counter_elaborated_ast.json";
  SNLSVConstructor::ConstructOptions options;
  options.elaboratedASTJsonPath = jsonPath;
  constructor.construct(benchmarksPath/"up_counter.sv", options);

  auto top = library_->getSNLDesign(NLName("up_counter"));
  ASSERT_NE(top, nullptr);

  auto outTerm = top->getBusTerm(NLName("out"));
  ASSERT_NE(outTerm, nullptr);
  EXPECT_EQ(8, outTerm->getWidth());
  EXPECT_EQ(SNLTerm::Direction::Output, outTerm->getDirection());

  auto enableTerm = top->getTerm(NLName("enable"));
  ASSERT_NE(enableTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Input, enableTerm->getDirection());

  auto clkTerm = top->getTerm(NLName("clk"));
  ASSERT_NE(clkTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Input, clkTerm->getDirection());

  auto resetTerm = top->getTerm(NLName("reset"));
  ASSERT_NE(resetTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Input, resetTerm->getDirection());

  auto outNet = top->getNet(NLName("out"));
  ASSERT_NE(outNet, nullptr);
  EXPECT_EQ(8, outNet->getWidth());
  EXPECT_NE(top->getNet(NLName("enable")), nullptr);
  EXPECT_NE(top->getNet(NLName("clk")), nullptr);
  EXPECT_NE(top->getNet(NLName("reset")), nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t gateCount = 0;
  bool dffHasSource = false;
  for (auto inst : top->getInstances()) {
    auto model = inst->getModel();
    if (model == dffModel) {
      ++dffCount;
      if (hasAttribute(inst, "sv_src_line")) {
        dffHasSource = true;
      }
      continue;
    }
    if (NLDB0::isGate(model) || NLDB0::isFA(model)) {
      ++gateCount;
    }
  }
  EXPECT_EQ(8, dffCount);
  EXPECT_TRUE(dffHasSource);
  EXPECT_GT(gateCount, 0u);

  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  ASSERT_TRUE(std::filesystem::exists(jsonPath));
  std::ifstream jsonFile(jsonPath);
  ASSERT_TRUE(jsonFile.good());
  std::string json{
    std::istreambuf_iterator<char>(jsonFile),
    std::istreambuf_iterator<char>()};
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("\"up_counter\""), std::string::npos);
}

TEST_F(SNLSVConstructorTestSimple, parseSimpleModuleDumpElaboratedASTJson) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "simple_ast_json";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  auto jsonPath = outPath / "simple_elaborated_ast.json";
  SNLSVConstructor::ConstructOptions options;
  options.elaboratedASTJsonPath = jsonPath;
  constructor.construct(benchmarksPath / "simple.sv", options);

  ASSERT_TRUE(std::filesystem::exists(jsonPath));
  std::ifstream jsonFile(jsonPath);
  ASSERT_TRUE(jsonFile.good());
  std::string json{
    std::istreambuf_iterator<char>(jsonFile),
    std::istreambuf_iterator<char>()};
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("\"top\""), std::string::npos);
}
