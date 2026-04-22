// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <unordered_set>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBitNet.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLUtils.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"
#include "SNLSVConstructorTestDetail.h"
#include "SNLSVConstructorException.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

const SNLRTLInfos* getRTLInfos(const NLObject* object) {
  if (auto design = dynamic_cast<const SNLDesign*>(object)) {
    return design->getRTLInfos();
  }
  if (auto designObject = dynamic_cast<const SNLDesignObject*>(object)) {
    return designObject->getRTLInfos();
  }
  return nullptr;
}

bool hasRTLInfo(const NLObject* object, const std::string& name) {
  auto rtlInfos = getRTLInfos(object);
  return rtlInfos && rtlInfos->hasInfo(NLName(name));
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::filesystem::path dumpTopAndGetVerilogPath(const SNLDesign* top,
                                               const std::string& outDirName,
                                               bool dumpRTLInfosAsAttributes = false) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= outDirName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  auto fileName = top->getName().getString() + ".v";
  dumper.setTopFileName(fileName);
  dumper.setSingleFile(true);
  dumper.setDumpRTLInfosAsAttributes(dumpRTLInfosAsAttributes);
  dumper.dumpDesign(top, outPath);
  return outPath / fileName;
}

size_t getMux2Width(const SNLInstance* instance) {
  if (!instance || !NLDB0::isMux2(instance->getModel())) {
    return 0;
  }
  if (auto* widthInstParam = instance->getInstParameter(NLName("WIDTH"))) {
    return static_cast<size_t>(std::stoull(widthInstParam->getValue()));
  }
  auto* widthParam = instance->getModel()->getParameter(NLName("WIDTH"));
  if (!widthParam) {
    return 0;
  }
  return static_cast<size_t>(std::stoull(widthParam->getValue()));
}

size_t countMux2Instances(const SNLDesign* design, size_t width = 0) {
  size_t count = 0;
  for (auto inst : design->getInstances()) {
    if (!NLDB0::isMux2(inst->getModel())) {
      continue;
    }
    if (width != 0 && getMux2Width(inst) != width) {
      continue;
    }
    ++count;
  }
  return count;
}

size_t countFAInstances(const SNLDesign* design) {
  size_t count = 0;
  for (auto inst : design->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++count;
    }
  }
  return count;
}

void expectUnsupportedConstruct(
  SNLSVConstructor& constructor,
  const std::filesystem::path& svPath,
  std::initializer_list<const char*> expectedSubstrings) {
  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    for (const auto* expected : expectedSubstrings) {
      EXPECT_NE(std::string::npos, reason.find(expected)) << reason;
    }
  }
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

TEST(SNLSVConstructorStandaloneTest, constructRequiresValidLibrary) {
  SNLSVConstructor constructor(nullptr);
  SNLSVConstructor::Paths paths;
  try {
    constructor.construct(paths);
    FAIL() << "Expected null library rejection";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SNLSVConstructor requires a valid NLLibrary"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSimpleModule) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"simple"/"simple.sv");

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  EXPECT_TRUE(hasRTLInfo(top, "sv_src_file"));
  EXPECT_TRUE(hasRTLInfo(top, "sv_src_line"));
  EXPECT_TRUE(hasRTLInfo(top, "sv_src_column"));
  ASSERT_TRUE(top->hasRTLInfos());
  EXPECT_TRUE(top->getRTLInfos()->hasInfo(NLName("sv_src_file")));
  EXPECT_TRUE(top->getRTLInfos()->hasInfo(NLName("sv_src_line")));
  EXPECT_FALSE(top->getRTLInfos()->getInfo(NLName("sv_src_line")).empty());
  EXPECT_EQ(3, top->getTerms().size());
  auto a = top->getTerm(NLName("a"));
  ASSERT_NE(a, nullptr);
  EXPECT_TRUE(hasRTLInfo(a, "sv_src_file"));
  ASSERT_TRUE(a->hasRTLInfos());
  EXPECT_TRUE(a->getRTLInfos()->hasInfo(NLName("sv_src_file")));
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
  EXPECT_TRUE(hasRTLInfo(assignInst, "sv_src_line"));
  EXPECT_TRUE(hasRTLInfo(andInst, "sv_src_line"));
  ASSERT_TRUE(assignInst->hasRTLInfos());
  EXPECT_TRUE(assignInst->getRTLInfos()->hasInfo(NLName("sv_src_line")));

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

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "simple_module");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
  std::ifstream dumpedFile(dumpedVerilog);
  ASSERT_TRUE(dumpedFile.good());
  std::string dumpedText{
    std::istreambuf_iterator<char>(dumpedFile),
    std::istreambuf_iterator<char>()};
  EXPECT_EQ(dumpedText.find("sv_src_file"), std::string::npos);

  auto dumpedVerilogWithRTLInfos =
    dumpTopAndGetVerilogPath(top, "simple_module_with_rtl_infos", true);
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilogWithRTLInfos));
  std::ifstream dumpedWithRTLInfosFile(dumpedVerilogWithRTLInfos);
  ASSERT_TRUE(dumpedWithRTLInfosFile.good());
  std::string dumpedWithRTLInfosText{
    std::istreambuf_iterator<char>(dumpedWithRTLInfosFile),
    std::istreambuf_iterator<char>()};
  EXPECT_NE(dumpedWithRTLInfosText.find("sv_src_file"), std::string::npos);
}

TEST_F(SNLSVConstructorTestSimple, parseDistinctParameterizedInstanceBodiesUseDistinctModels) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "distinct_parameterized_instance_bodies";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "distinct_parameterized_instance_bodies.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module leaf #(\n"
         << "  parameter int W = 1\n"
         << ") (\n"
         << "  input logic [W-1:0] a,\n"
         << "  output logic [W-1:0] y\n"
         << ");\n"
         << "  assign y = a;\n"
         << "endmodule\n"
         << "\n"
         << "module top(\n"
         << "  input logic [1:0] a2,\n"
         << "  output logic [1:0] y2,\n"
         << "  input logic [3:0] a4,\n"
         << "  output logic [3:0] y4\n"
         << ");\n"
         << "  leaf #(.W(2)) u2 (.a(a2), .y(y2));\n"
         << "  leaf #(.W(4)) u4 (.a(a4), .y(y4));\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  auto* u2 = top->getInstance(NLName("u2"));
  auto* u4 = top->getInstance(NLName("u4"));
  ASSERT_NE(u2, nullptr);
  ASSERT_NE(u4, nullptr);

  auto* u2Model = u2->getModel();
  auto* u4Model = u4->getModel();
  ASSERT_NE(u2Model, nullptr);
  ASSERT_NE(u4Model, nullptr);
  EXPECT_NE(u2Model, u4Model);
  EXPECT_EQ(std::string("leaf"), u2Model->getName().getString());
  EXPECT_NE(std::string("leaf"), u4Model->getName().getString());
  EXPECT_EQ(u2Model, library_->getSNLDesign(NLName("leaf")));

  auto* u2A = u2Model->getBusTerm(NLName("a"));
  auto* u2Y = u2Model->getBusTerm(NLName("y"));
  auto* u4A = u4Model->getBusTerm(NLName("a"));
  auto* u4Y = u4Model->getBusTerm(NLName("y"));
  ASSERT_NE(u2A, nullptr);
  ASSERT_NE(u2Y, nullptr);
  ASSERT_NE(u4A, nullptr);
  ASSERT_NE(u4Y, nullptr);
  EXPECT_EQ(2, u2A->getWidth());
  EXPECT_EQ(2, u2Y->getWidth());
  EXPECT_EQ(4, u4A->getWidth());
  EXPECT_EQ(4, u4Y->getWidth());

  auto* topA2 = top->getBusNet(NLName("a2"));
  auto* topY2 = top->getBusNet(NLName("y2"));
  auto* topA4 = top->getBusNet(NLName("a4"));
  auto* topY4 = top->getBusNet(NLName("y4"));
  ASSERT_NE(topA2, nullptr);
  ASSERT_NE(topY2, nullptr);
  ASSERT_NE(topA4, nullptr);
  ASSERT_NE(topY4, nullptr);
  auto* topA2Bit0 = dynamic_cast<SNLBitNet*>(topA2->getBit(0));
  auto* topY2Bit1 = dynamic_cast<SNLBitNet*>(topY2->getBit(1));
  auto* topA4Bit3 = dynamic_cast<SNLBitNet*>(topA4->getBit(3));
  auto* topY4Bit2 = dynamic_cast<SNLBitNet*>(topY4->getBit(2));
  auto* u2ATerm0 = u2->getInstTerm(u2A->getBit(0));
  auto* u2YTerm1 = u2->getInstTerm(u2Y->getBit(1));
  auto* u4ATerm3 = u4->getInstTerm(u4A->getBit(3));
  auto* u4YTerm2 = u4->getInstTerm(u4Y->getBit(2));
  ASSERT_NE(topA2Bit0, nullptr);
  ASSERT_NE(topY2Bit1, nullptr);
  ASSERT_NE(topA4Bit3, nullptr);
  ASSERT_NE(topY4Bit2, nullptr);
  ASSERT_NE(u2ATerm0, nullptr);
  ASSERT_NE(u2YTerm1, nullptr);
  ASSERT_NE(u4ATerm3, nullptr);
  ASSERT_NE(u4YTerm2, nullptr);
  EXPECT_EQ(topA2Bit0, u2ATerm0->getNet());
  EXPECT_EQ(topY2Bit1, u2YTerm1->getNet());
  EXPECT_EQ(topA4Bit3, u4ATerm3->getNet());
  EXPECT_EQ(topY4Bit2, u4YTerm2->getNet());
}

TEST_F(SNLSVConstructorTestSimple, parseEmptyPortOnlyModuleDetectedAsUserBlackBox) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "empty_port_only_blackbox";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "empty_port_only_blackbox.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module unread(\n"
         << "  input logic d_i\n"
         << ");\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto unread = library_->getSNLDesign(NLName("unread"));
  ASSERT_NE(unread, nullptr);
  EXPECT_TRUE(unread->isUserBlackBox());
  EXPECT_TRUE(unread->isBlackBox());
  EXPECT_TRUE(unread->isLeaf());
  EXPECT_EQ(1, unread->getTerms().size());
  EXPECT_EQ(1, unread->getNets().size());
  EXPECT_TRUE(unread->getInstances().empty());
  EXPECT_NE(unread->getScalarTerm(NLName("d_i")), nullptr);
  EXPECT_NE(unread->getScalarNet(NLName("d_i")), nullptr);
  EXPECT_EQ(unread, SNLUtils::findTop(library_));
}

TEST_F(SNLSVConstructorTestSimple, parseEmptyPortOnlyModuleBlackboxDetectionCanBeDisabled) {
  SNLSVConstructor constructor(library_);
  constructor.config_.blackboxDetection_ = false;
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "empty_port_only_standard";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "empty_port_only_standard.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module unread(\n"
         << "  input logic d_i\n"
         << ");\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto unread = library_->getSNLDesign(NLName("unread"));
  ASSERT_NE(unread, nullptr);
  EXPECT_TRUE(unread->isStandard());
  EXPECT_FALSE(unread->isBlackBox());
  EXPECT_FALSE(unread->isLeaf());
  EXPECT_EQ(1, unread->getTerms().size());
  EXPECT_EQ(1, unread->getNets().size());
  EXPECT_TRUE(unread->getInstances().empty());
  EXPECT_NE(unread->getScalarTerm(NLName("d_i")), nullptr);
  EXPECT_NE(unread->getScalarNet(NLName("d_i")), nullptr);
  EXPECT_EQ(unread, SNLUtils::findTop(library_));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignsInsideNestedGenerateLoops) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "nested_generate_continuous_assigns";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "nested_generate_continuous_assigns.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module top(\n"
         << "  input logic [1:0][1:0] i,\n"
         << "  output logic [1:0][1:0] o\n"
         << ");\n"
         << "  genvar r, c;\n"
         << "  for (r = 0; r < 2; r = r + 1) begin: rows\n"
         << "    for (c = 0; c < 2; c = c + 1) begin: cols\n"
         << "      assign o[r][c] = i[r][c];\n"
         << "    end\n"
         << "  end\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_FALSE(top->isLeaf());
  EXPECT_EQ(2, top->getTerms().size());

  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(4, assignCount);

  const auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "nested_generate_continuous_assigns_dump");
  const auto dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("assign"));
}

TEST_F(SNLSVConstructorTestSimple, parseGeneratedNetAndInstancesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "generated_net_and_instances_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "generated_net_and_instances_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module child(input logic a, output logic y);\n"
         << "  assign y = a;\n"
         << "endmodule\n"
         << "module top(input logic [1:0] a, output logic [1:0] y);\n"
         << "  genvar i;\n"
         << "  for (i = 0; i < 2; i = i + 1) begin: g\n"
         << "    wire local_li = a[i];\n"
         << "    child u(.a(local_li), .y(y[i]));\n"
         << "  end\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());

  size_t childCount = 0;
  std::unordered_set<std::string> childNames;
  std::unordered_set<std::string> localNetNames;
  for (auto inst : top->getInstances()) {
    if (inst->getModel()->getName().getString() == "child") {
      ++childCount;
      childNames.insert(inst->getName().getString());
    }
  }
  for (auto net : top->getNets()) {
    const auto name = net->getName().getString();
    if (name.find("local_li") != std::string::npos) {
      localNetNames.insert(name);
    }
  }
  EXPECT_EQ(2, childCount);
  EXPECT_EQ(2, childNames.size());
  EXPECT_EQ(2, localNetNames.size());
}

TEST_F(SNLSVConstructorTestSimple,
       parseGeneratedProceduralBlockSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "generated_procedural_block_lowering_coverage";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "generated_procedural_block_lowering_coverage.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module top(input logic a, output logic y);\n"
         << "  if (1) begin : g\n"
         << "    always_comb y = a;\n"
         << "  end\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);

  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(1u, assignCount);
}

TEST_F(SNLSVConstructorTestSimple,
       parseTypeAliasAndLocalparamDoNotReportLoweringCoverageErrors) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "type_alias_and_localparam_lowering_coverage";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "type_alias_and_localparam_lowering_coverage.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module top(input logic [3:0] i, output logic y);\n"
         << "  typedef logic [3:0] nibble_t;\n"
         << "  localparam int IDX = 1;\n"
         << "  nibble_t tmp;\n"
         << "  assign tmp = i;\n"
         << "  assign y = tmp[IDX];\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());

  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(5, assignCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSimpleModuleViaPathsOverload) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  SNLSVConstructor::Paths paths { benchmarksPath / "simple" / "simple.sv" };
  constructor.construct(paths);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "simple_module_via_paths");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSimpleModuleViaFList) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "simple_module_via_flist";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto flistPath = outPath / "sources.f";
  std::ofstream flist(flistPath);
  ASSERT_TRUE(flist.good());
  flist << (benchmarksPath / "simple" / "simple.sv").string() << "\n";
  flist.close();

  SNLSVConstructor::Paths paths { std::filesystem::path("-f"), flistPath };
  constructor.construct(paths);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "simple_module_via_flist_dump");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseFListTopOptionRestrictsElaboration) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "flist_top_option_restricts_elaboration";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto genericPath = outPath / "generic.sv";
  std::ofstream generic(genericPath);
  ASSERT_TRUE(generic.good());
  generic << "module generic #(parameter type T = logic) (input T i, output logic y);\n"
          << "  assign y = i.foo;\n"
          << "endmodule\n";
  generic.close();

  const auto topPath = outPath / "top2.sv";
  std::ofstream top(topPath);
  ASSERT_TRUE(top.good());
  top << "module top2(input logic a, output logic y);\n"
      << "  assign y = a;\n"
      << "endmodule\n";
  top.close();

  const auto flistPath = outPath / "sources.f";
  std::ofstream flist(flistPath);
  ASSERT_TRUE(flist.good());
  flist << "--top top2\n";
  flist << genericPath.string() << "\n";
  flist << topPath.string() << "\n";
  flist.close();

  SNLSVConstructor::Paths paths { std::filesystem::path("-f"), flistPath };
  constructor.construct(paths);

  auto top2 = library_->getSNLDesign(NLName("top2"));
  ASSERT_NE(top2, nullptr);
  EXPECT_EQ(nullptr, library_->getSNLDesign(NLName("generic")));
}

TEST_F(SNLSVConstructorTestSimple, parseMissingFileThrowsLoadError) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  auto missingPath = benchmarksPath / "does_not_exist" / "missing.sv";
  ASSERT_FALSE(std::filesystem::exists(missingPath));

  try {
    constructor.construct(missingPath);
    FAIL() << "Expected load failure exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find(missingPath.string()));
    const bool hasLoadPrefix =
      reason.find("Failed to load SystemVerilog file: ") != std::string::npos;
    const bool hasCompilationPrefix =
      reason.find("SystemVerilog compilation failed") != std::string::npos;
    const bool hasFileSystemDetail =
      reason.find("No such file or directory") != std::string::npos;
    EXPECT_TRUE(hasLoadPrefix || hasCompilationPrefix || hasFileSystemDetail);
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSyntaxErrorThrowsCompilationError) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "syntax_error";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "syntax_error.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module syntax_error(input logic a, output logic y)\n"
         << "  assign y = a;\n"
         << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected SystemVerilog compilation failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SystemVerilog compilation failed"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseCommandFileSyntaxErrorIncludesDriverFailureDetails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "syntax_error_command_file";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "syntax_error_command_file.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module syntax_error_command_file(input logic a, output logic y)\n"
         << "  assign y = a;\n"
         << "endmodule\n";
  svFile.close();

  const auto flistPath = outPath / "syntax_error_command_file.f";
  std::ofstream flistFile(flistPath);
  ASSERT_TRUE(flistFile.good());
  flistFile << svPath.string() << "\n";
  flistFile.close();

  SNLSVConstructor::Paths paths{std::filesystem::path("-f"), flistPath};
  try {
    constructor.construct(paths);
    FAIL() << "Expected SystemVerilog compilation failure through driver path";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SystemVerilog compilation failed:\n"));
    EXPECT_NE(std::string::npos, reason.find(svPath.filename().string()));
    EXPECT_NE(std::string::npos, reason.find("error"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseCommandFileMissingSourceIncludesDriverFailureDetails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "missing_source_command_file";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto missingPath = outPath / "does_not_exist.sv";
  ASSERT_FALSE(std::filesystem::exists(missingPath));

  const auto flistPath = outPath / "missing_source_command_file.f";
  std::ofstream flistFile(flistPath);
  ASSERT_TRUE(flistFile.good());
  flistFile << missingPath.string() << "\n";
  flistFile.close();

  SNLSVConstructor::Paths paths{std::filesystem::path("-f"), flistPath};
  try {
    constructor.construct(paths);
    FAIL() << "Expected missing source failure through driver path";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SystemVerilog compilation failed:\n"));
    EXPECT_NE(std::string::npos, reason.find(missingPath.filename().string()));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseCommandFileMixedValidAndMissingSourceIncludesDriverFailureDetails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "mixed_source_command_file";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto validPath = outPath / "mixed_source_valid.sv";
  std::ofstream validFile(validPath);
  ASSERT_TRUE(validFile.good());
  validFile << "module mixed_source_valid(input logic a, output logic y);\n"
            << "  assign y = a;\n"
            << "endmodule\n";
  validFile.close();

  const auto missingPath = outPath / "mixed_source_missing.sv";
  ASSERT_FALSE(std::filesystem::exists(missingPath));

  const auto flistPath = outPath / "mixed_source_command_file.f";
  std::ofstream flistFile(flistPath);
  ASSERT_TRUE(flistFile.good());
  flistFile << validPath.string() << "\n";
  flistFile << missingPath.string() << "\n";
  flistFile.close();

  SNLSVConstructor::Paths paths{std::filesystem::path("-f"), flistPath};
  try {
    constructor.construct(paths);
    FAIL() << "Expected mixed valid/missing source failure through driver path";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SystemVerilog compilation failed:\n"));
    EXPECT_NE(std::string::npos, reason.find(missingPath.filename().string()));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseCommandFileOptionMissingArgumentFallsBackToGenericFailure) {
  SNLSVConstructor constructor(library_);
  SNLSVConstructor::Paths paths{std::filesystem::path("-f")};

  try {
    constructor.construct(paths);
    FAIL() << "Expected command-file option parsing failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SystemVerilog compilation failed"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectIndexVariants) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_index_variants";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_index_variants.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_index_variants_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  input logic idx,\n"
    << "  output logic [3:0] y_ok,\n"
    << "  output logic [3:0] y_dyn,\n"
    << "  output logic [3:0] y_big\n"
    << ");\n"
    << "  localparam int IDX_OK = $clog2(2) - 1;\n"
    << "  localparam longint IDX_BIG = 64'd2147483648;\n"
    << "\n"
    << "  assign y_ok = a[IDX_OK];\n"
    << "  assign y_dyn = a[idx];\n"
    << "  assign y_big = a[IDX_BIG];\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_index_variants_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_ok")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_dyn")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_big")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectDynamicIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_dynamic_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_dynamic_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_dynamic_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  input logic idx,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  assign y = a[idx] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_dynamic_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectConstVariableIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_const_variable_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_const_variable_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_const_variable_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  const int IDX = $clog2(2) - 1;\n"
    << "  assign y = a[IDX] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("element_select_const_variable_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectFunctionIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_function_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_function_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_function_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  function automatic int idx_fn();\n"
    << "    idx_fn = 1;\n"
    << "  endfunction\n"
    << "  assign y = a[idx_fn()] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_function_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("a")), nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectBitOfConstIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_bit_of_const_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_bit_of_const_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_bit_of_const_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  localparam logic [0:0] IDX = 1'b1;\n"
    << "  assign y = a[IDX[0]] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectMaskedZeroIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_masked_zero_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_masked_zero_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_masked_zero_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  input logic idx,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  assign y = a[(idx & 1'b0)] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectLiteralOutOfInt32IndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_literal_out_of_int32_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_literal_out_of_int32_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_literal_out_of_int32_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  assign y = a[32'd2147483648] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_literal_out_of_int32_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectConstLongintOutOfInt32IndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_const_longint_out_of_int32_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_const_longint_out_of_int32_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_const_longint_out_of_int32_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  const longint IDX = 64'd2147483648;\n"
    << "  assign y = a[IDX] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("element_select_const_longint_out_of_int32_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectConstTooWideIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_const_too_wide_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_const_too_wide_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_const_too_wide_index_under_add_top(\n"
    << "  input logic [1:0][3:0] a,\n"
    << "  output logic [3:0] y\n"
    << ");\n"
    << "  const logic [127:0] IDX = 128'h00000000000000000000000100000000;\n"
    << "  assign y = a[IDX] + 4'h1;\n"
    << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_const_too_wide_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectBinaryBaseUnderAddUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_binary_base_under_add_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_binary_base_under_add_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module element_select_binary_base_under_add_unsupported_top(\n"
    << "  input logic [3:0] a,\n"
    << "  input logic [3:0] b,\n"
    << "  output logic y\n"
    << ");\n"
    << "  assign y = ((a & b)[0]) + 1'b1;\n"
    << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported select-on-binary-base under add failure";
  } catch (const SNLSVConstructorException&) {
  }
}


TEST_F(SNLSVConstructorTestSimple, parseElementSelectNegativeOutOfInt32IndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_negative_out_of_int32_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_negative_out_of_int32_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_negative_out_of_int32_index_under_add_top(
  input logic [1:0][3:0] a,
  output logic [3:0] y
);
  const longint signed IDX = -64'sd2147483649;
  assign y = a[IDX] + 4'h1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("element_select_negative_out_of_int32_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectUnknownConstIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_unknown_const_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_unknown_const_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_unknown_const_index_under_add_top(
  input logic [1:0][3:0] a,
  output logic [3:0] y
);
  const logic [3:0] IDX = 4'b1x00;
  assign y = a[IDX] + 4'h1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_unknown_const_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectConcatBaseUnderAddUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_concat_base_under_add_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_concat_base_under_add_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_concat_base_under_add_unsupported_top(
  input logic [3:0] a,
  input logic [3:0] b,
  output logic y
);
  assign y = ({a, b})[0] + 1'b1;
endmodule
)";
  svFile.close();

  EXPECT_THROW(constructor.construct(svPath), SNLSVConstructorException);
}

TEST_F(SNLSVConstructorTestSimple, parseScalarBitPortsGetScalarTerms) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "scalar_bit_ports_get_scalar_terms";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "scalar_bit_ports_get_scalar_terms.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module scalar_bit_ports_get_scalar_terms_top(
  input bit a,
  output bit y
);
  assign y = a;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("scalar_bit_ports_get_scalar_terms_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarTerm(NLName("a")), nullptr);
  EXPECT_NE(top->getScalarTerm(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectHugeLiteralIndexUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_huge_literal_index_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_huge_literal_index_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_huge_literal_index_under_add_top(
  input logic [1:0][3:0] a,
  output logic [3:0] y
);
  assign y = a[128'd18446744073709551616] + 4'h1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_huge_literal_index_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectNegativeLiteralOutOfInt32UnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_negative_literal_out_of_int32_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_negative_literal_out_of_int32_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_negative_literal_out_of_int32_under_add_top(
  input logic [1:0][3:0] a,
  output logic [3:0] y
);
  assign y = a[-64'sd2147483649] + 4'h1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("element_select_negative_literal_out_of_int32_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectFunctionCallBaseUnderAddSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_function_call_base_under_add_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_function_call_base_under_add_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_function_call_base_under_add_supported_top(
  input logic [3:0] a,
  output logic y
);
  function automatic logic [3:0] id(input logic [3:0] x);
    id = x;
  endfunction
  assign y = id(a)[0] + 1'b1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("element_select_function_call_base_under_add_supported_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousGateAssignBitwiseNotUnknownOperandReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_gate_assign_bitwise_not_unknown_operand_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_gate_assign_bitwise_not_unknown_operand_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_gate_assign_bitwise_not_unknown_operand_unsupported(
  input  logic a,
  output logic y
);
  assign y = a & ~1'bx;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {
      "Unsupported operand in continuous gate assign",
      "gate=and",
      "failed to resolve bitwise-not operand bits"
    });
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousBitwiseGateAssignBitwiseNotUnknownOperandReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_bitwise_gate_assign_bitwise_not_unknown_operand_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_bitwise_gate_assign_bitwise_not_unknown_operand_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_bitwise_gate_assign_bitwise_not_unknown_operand_unsupported(
  input  logic [3:0] a,
  output logic [3:0] y
);
  assign y = a & ~4'bxxxx;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {
      "Unsupported gate construction in continuous assign",
      "gate=and",
      "lhs_width=4",
      "failed to resolve bitwise-not operand bits"
    });
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousGateAssignGenericOperandResolveFailureReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_gate_assign_generic_operand_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_gate_assign_generic_operand_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_gate_assign_generic_operand_resolve_failure_unsupported(
  input  logic       a,
  input  logic [3:0] b,
  output logic       y
);
  function automatic logic bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 1'b1;
      default:          bad_fn = 1'b0;
    endcase
  endfunction
  assign y = a & bad_fn(b);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {
      "Unsupported operand in continuous gate assign",
      "gate=and",
      "failed to resolve operand bits"
    });
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideAssignmentBodyUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_assignment_body_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_assignment_body_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_assignment_body_unsupported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: is_amo = 1'b1;
      default:        is_amo = 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideAbsoluteToleranceUnsupportedLanguage) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_absolute_tolerance_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_absolute_tolerance_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_absolute_tolerance_unsupported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd8 +/- 6'd1]: return 1'b1;
      default:       return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL();
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("language feature is not supported in version 1800-2017"));
  } catch (...) {
    FAIL();
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideUnknownLowerBoundUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_unknown_lower_bound_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_unknown_lower_bound_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_unknown_lower_bound_unsupported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'bxxxxxx : 6'd17]: return 1'b1;
      default:             return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideUnknownUpperBoundUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_unknown_upper_bound_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_unknown_upper_bound_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_unknown_upper_bound_unsupported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'bxxxxxx]: return 1'b1;
      default:            return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignBitSliceResolveExpressionBitsFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_bit_slice_resolve_expression_bits_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_bit_slice_resolve_expression_bits_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bit_slice_resolve_expression_bits_failure_unsupported(
  input  logic [3:0] a,
  output logic [7:0] y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y[3:0] = bad_fn(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_bit_slice_resolve_expression_bits_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConditionalResolveExpressionBitsFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_conditional_resolve_expression_bits_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_resolve_expression_bits_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_resolve_expression_bits_failure_unsupported(
  input  logic [3:0] a,
  input  logic       sel,
  output logic [3:0] y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = sel ? bad_fn(a) : 4'h0;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_conditional_resolve_expression_bits_failure_unsupported'"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_conditional_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_shortcut_supported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  input  logic       sel,
  output logic [3:0] y_true,
  output logic [3:0] y_false
);
  assign y_true  = (sel || 1'b1) ? a : b;
  assign y_false = (sel && 1'b0) ? a : b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_assign_conditional_shortcut_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_true")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_false")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConditionalUnknownLiteralKnownSideSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_unknown_literal_known_side_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_unknown_literal_known_side_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_unknown_literal_known_side_supported #(
  parameter int width_p = 8
) (
  output logic [width_p-1:0] y
);
  assign y = (width_p == 1) ? '0 : 'x;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_conditional_unknown_literal_known_side_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConditionalUnknownLiteralMuxSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_unknown_literal_mux_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_unknown_literal_mux_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_unknown_literal_mux_supported(
  input  logic [3:0] a,
  input  logic       sel,
  output logic [3:0] y
);
  assign y = sel ? a : {4{1'bx}};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_conditional_unknown_literal_mux_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConditionalUnknownLiteralShortcutBranchesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_conditional_unknown_literal_shortcut_branches_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_unknown_literal_shortcut_branches_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_unknown_literal_shortcut_branches_supported(
  input  logic [1:0] a,
  input  logic       sel,
  output logic [1:0] y_true,
  output logic [1:0] y_false
);
  assign y_true  = (sel || 1'b1) ? a : {1'b0, {0{1'b1}}, 1'bx};
  assign y_false = (sel && 1'b0) ? {1'b1, {0{1'b0}}, 1'bx} : a;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_conditional_unknown_literal_shortcut_branches_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_true")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_false")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalIdenticalBranchesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_identical_branches_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_conditional_identical_branches_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_identical_branches_supported(
  input  logic [3:0] a,
  input  logic       sel,
  output logic [3:0] y
);
  assign y = sel ? a : a;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_conditional_identical_branches_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalScalarMuxSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_scalar_mux_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_conditional_scalar_mux_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_scalar_mux_supported(
  input  logic a,
  input  logic b,
  input  logic sel,
  output logic y
);
  assign y = sel ? a : b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_assign_conditional_scalar_mux_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalLogicalOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_logical_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_conditional_logical_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_logical_or_supported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  input  logic       sel0,
  input  logic       sel1,
  output logic [3:0] y
);
  assign y = (sel0 || sel1) ? a : b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_conditional_logical_or_supported"));
  ASSERT_NE(top, nullptr);

  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());

  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConditionalConditionResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_conditional_condition_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_condition_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_condition_resolve_failure_unsupported(
  input  logic [3:0] a,
  output logic [3:0] y
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction
  assign y = bad_cond(a) ? 4'hf : 4'h0;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_conditional_condition_resolve_failure_unsupported'"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignInsideItemResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_inside_item_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_inside_item_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_inside_item_resolve_failure_unsupported(
  input  logic [3:0] a,
  output logic       y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = a inside {bad_fn(a)};
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_inside_item_resolve_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignBitwiseNotResolveExpressionBitsFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_bitwise_not_resolve_expression_bits_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_bitwise_not_resolve_expression_bits_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bitwise_not_resolve_expression_bits_failure_unsupported(
  input  logic [3:0] a,
  output logic [3:0] y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = {~bad_fn(a)};
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_bitwise_not_resolve_expression_bits_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignUnaryPlusResolveExpressionBitsFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_unary_plus_resolve_expression_bits_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_unary_plus_resolve_expression_bits_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_unary_plus_resolve_expression_bits_failure_unsupported(
  input  logic [3:0] a,
  output logic [3:0] y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = {+bad_fn(a)};
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_unary_plus_resolve_expression_bits_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignElementSelectUnsupportedReportsDetails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_element_select_unsupported_reports_details";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_element_select_unsupported_reports_details.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_element_select_unsupported_reports_details(
  input  logic [3:0] a,
  output logic       y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = bad_fn(a)[0];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_element_select_unsupported_reports_details'",
     "ElementSelect"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignGeneratedComplexConstantSelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_generated_complex_constant_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_generated_complex_constant_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_generated_complex_constant_selector_supported
  #(parameter logic [4:0] pattern_p = 5'b10101)
(
  input  logic [2:0] i,
  output logic [4:0] o
);
  assign o[0] = i[0];

  genvar j;
  for (j = 1; j < $bits(pattern_p); j = j + 1)
    begin : gen_unconcentrate
      if (pattern_p[j])
        assign o[j] = i[((($bits(pattern_p[j-1:0]) < 65) ? 1'b0 : 1'b0)
                         + (((pattern_p[j-1:0] >> 0) & 1'b1)
                         +  ((pattern_p[j-1:0] >> 1) & 1'b1)
                         +  ((pattern_p[j-1:0] >> 2) & 1'b1)
                         +  ((pattern_p[j-1:0] >> 3) & 1'b1)))];
      else
        assign o[j] = 1'b0;
    end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_generated_complex_constant_selector_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignNamedParameterUnknownBitSliceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_named_parameter_unknown_bit_slice_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_named_parameter_unknown_bit_slice_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_named_parameter_unknown_bit_slice_supported
  #(parameter unconnected_val_p = 'z)
(
  output logic [1:0] o
);
  assign o[0] = unconnected_val_p;

  genvar j;
  for (j = 1; j < 2; j = j + 1)
    begin : gen_unconnected
      assign o[j] = unconnected_val_p;
    end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_named_parameter_unknown_bit_slice_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("o")), nullptr);
  EXPECT_EQ(top->getNet(NLName("unconnected_val_p")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignBitSliceWidthMismatchUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_bit_slice_width_mismatch_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_bit_slice_width_mismatch_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bit_slice_width_mismatch_unsupported(
  input  logic [2:0] a,
  output logic [7:0] y
);
  assign y[3:0] = a;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported net compatibility in continuous assign"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignBitSliceTruncateSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_bit_slice_truncate_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_bit_slice_truncate_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bit_slice_truncate_supported #(
  parameter int expWidth = 5
) (
  input  logic [expWidth:0] adjusted_i,
  output logic [(expWidth - 3):0] lo_o
);
  logic [expWidth:0] exp;

  assign exp[(expWidth - 3):0] = adjusted_i;
  assign lo_o = exp[(expWidth - 3):0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_assign_bit_slice_truncate_supported"));
  ASSERT_NE(top, nullptr);
  auto lo = top->getBusNet(NLName("lo_o"));
  ASSERT_NE(lo, nullptr);
  EXPECT_EQ(3, lo->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignBitSliceUnsizedZeroSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_bit_slice_unsized_zero_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_bit_slice_unsized_zero_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bit_slice_unsized_zero_supported(
  output logic [39:0] lo_o
);
  logic signed [40:0] c;

  assign c[39:0] = 0;
  assign lo_o = c[39:0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_bit_slice_unsized_zero_supported"));
  ASSERT_NE(top, nullptr);
  auto lo = top->getBusNet(NLName("lo_o"));
  ASSERT_NE(lo, nullptr);
  EXPECT_EQ(40, lo->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignBitSliceUnknownLiteralSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_bit_slice_unknown_literal_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_bit_slice_unknown_literal_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bit_slice_unknown_literal_supported(
  output logic [3:0] y
);
  logic [7:0] c;

  assign c[3:0] = 4'bxxxx;
  assign y = c[3:0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_bit_slice_unknown_literal_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignNamedLHSUnknownLiteralSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_named_lhs_unknown_literal_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_named_lhs_unknown_literal_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_named_lhs_unknown_literal_supported(
  output logic [3:0] y
);
  assign y = 4'bxxxx;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_named_lhs_unknown_literal_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalBitSliceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_bit_slice_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_conditional_bit_slice_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_bit_slice_supported(
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  input  logic       sel_i,
  output logic [3:0] y_o
);
  logic [7:0] tmp;
  assign tmp[3:0] = sel_i ? a_i : b_i;
  assign y_o = tmp[3:0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_conditional_bit_slice_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y_o"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignPackedArrayElementWidenSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_packed_array_element_widen_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_packed_array_element_widen_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_packed_array_element_widen_supported(
  input  logic [7:0] a,
  output logic [63:0] y
);
  logic [1:0][63:0] data;

  assign data[0] = a;
  assign y = data[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_packed_array_element_widen_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(64, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignBitSliceStreamingSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_bit_slice_streaming_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_bit_slice_streaming_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_bit_slice_streaming_supported(
  input  logic [7:0] i,
  output logic [7:0] o
);
  wire [0:0][7:0] t;

  assign t[0] = {<<{i}};
  assign o = t[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_assign_bit_slice_streaming_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getBusNet(NLName("o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideConstantCallBelowRangeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_constant_call_below_range_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_constant_call_below_range_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_constant_call_below_range_supported(
  output logic y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: return 1'b1;
      default:        return 1'b0;
    endcase
  endfunction
  assign y = is_amo(6'd3);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_constant_call_below_range_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideConstantCallInRangeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_constant_call_in_range_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_constant_call_in_range_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_constant_call_in_range_supported(
  output logic y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: return 1'b1;
      default:        return 1'b0;
    endcase
  endfunction
  assign y = is_amo(6'd8);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_constant_call_in_range_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideUpperBoundMaxSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_function_case_inside_upper_bound_max_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_upper_bound_max_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_upper_bound_max_supported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd63]: return 1'b1;
      default:        return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_upper_bound_max_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignFunctionCaseInsideRangeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_case_inside_range_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_function_case_inside_range_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_range_supported(
  input logic [5:0] op,
  output logic y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: begin
        return 1'b1;
      end
      default: return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_function_case_inside_range_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("op")), nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top,
    "continuous_assign_function_case_inside_range_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideMultipleItemExpressionsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_case_inside_multiple_item_expressions_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_multiple_item_expressions_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_multiple_item_expressions_supported(
  input logic [5:0] op,
  output logic y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      6'd4, 6'd17: begin
        return 1'b1;
      end
      default: return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_multiple_item_expressions_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("op")), nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);

  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignSimpleReturnFunctionScalarMaterializedArgSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_simple_return_function_scalar_materialized_arg_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_simple_return_function_scalar_materialized_arg_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_simple_return_function_scalar_materialized_arg_supported(
  input  logic a,
  input  logic b,
  output logic y
);
  function automatic logic id(input logic x);
    return x;
  endfunction
  assign y = id(a ^ b);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_simple_return_function_scalar_materialized_arg_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignSimpleReturnFunctionRecursiveUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_simple_return_function_recursive_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_simple_return_function_recursive_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_simple_return_function_recursive_unsupported(
  input  logic a,
  output logic y
);
  function automatic logic recurse(input logic x);
    return recurse(x);
  endfunction
  assign y = recurse(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_simple_return_function_recursive_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignSimpleReturnFunctionNonIntegralFormalUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_simple_return_function_non_integral_formal_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_simple_return_function_non_integral_formal_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_simple_return_function_non_integral_formal_unsupported(
  output logic y
);
  function automatic logic is_empty(input string s);
    return (s.len() == 0);
  endfunction
  assign y = is_empty("abc");
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_simple_return_function_non_integral_formal_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignSimpleReturnFunctionArgumentResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_simple_return_function_argument_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_simple_return_function_argument_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_simple_return_function_argument_resolve_failure_unsupported(
  output logic y
);
  function automatic logic id(input logic x);
    return x;
  endfunction
  assign y = id(1'bx);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_simple_return_function_argument_resolve_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionMaterializedArgsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_materialized_args_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_materialized_args_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_materialized_args_supported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  input  logic       sel,
  input  logic       en,
  output logic [3:0] y_bus,
  output logic       y_scalar
);
  function automatic logic [3:0] pass_bus(input logic [3:0] x);
    logic [3:0] tmp;
    tmp = x;
    return tmp;
  endfunction

  function automatic logic pass_scalar(input logic x);
    logic tmp;
    tmp = x;
    return tmp;
  endfunction

  assign y_bus = pass_bus(a ^ b);
  assign y_scalar = pass_scalar(sel & en);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_procedural_return_function_materialized_args_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_bus")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_scalar")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignNestedCaseReturnFunctionTerminalDefaultSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_nested_case_return_function_terminal_default_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_nested_case_return_function_terminal_default_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_nested_case_return_function_terminal_default_supported(
  input  logic [2:0] addr_i,
  input  logic [1:0] size_i,
  output logic [7:0] be_o
);
  function automatic logic [7:0] be_gen(input logic [2:0] addr, input logic [1:0] size);
    case (size)
      2'b10: begin
        case (addr)
          3'b000: return 8'b0000_1111;
          3'b001: return 8'b0001_1110;
          default: ;
        endcase
      end
      2'b01: begin
        case (addr)
          3'b000: return 8'b0000_0011;
          3'b001: return 8'b0000_0110;
          default: ;
        endcase
      end
      default: ;
    endcase
    return 8'b0;
  endfunction

  assign be_o = be_gen(addr_i, size_i);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_nested_case_return_function_terminal_default_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("be_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionUnchangedBitsSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_unchanged_bits_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_unchanged_bits_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_unchanged_bits_skip_supported(
  input  logic [3:0] a,
  output logic [3:0] y
);
  function automatic logic [3:0] passthrough(input logic [3:0] x);
    logic unused;
    unused = 1'b0;
    return x;
  endfunction

  assign y = passthrough(a);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_procedural_return_function_unchanged_bits_skip_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionIgnoresAssertionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_procedural_return_function_ignores_assertion_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_ignores_assertion_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_ignores_assertion_supported(
  input  logic [3:0] a,
  output logic [3:0] y
);
  function automatic logic [3:0] passthrough(input logic [3:0] x);
    assert (1'b1);
    return x;
  endfunction

  assign y = passthrough(a);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_procedural_return_function_ignores_assertion_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionDynamicElementSelectScalarSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_procedural_return_function_dynamic_element_select_scalar_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_procedural_return_function_dynamic_element_select_scalar_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_dynamic_element_select_scalar_supported(
  input  logic [1:0] a,
  input  logic       sel,
  input  logic       bit_i,
  output logic [1:0] y
);
  function automatic logic [1:0] update_bit(
    input logic [1:0] x,
    input logic       idx,
    input logic       v
  );
    x[idx] = v;
    return x;
  endfunction

  assign y = update_bit(a, sel, bit_i);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_procedural_return_function_dynamic_element_select_scalar_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionUnsupportedStatementKindUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_procedural_return_function_unsupported_statement_kind_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_procedural_return_function_unsupported_statement_kind_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_unsupported_statement_kind_unsupported(
  input  logic a,
  output logic y
);
  function automatic logic passthrough(input logic x);
    while (x) begin
      x = 1'b0;
    end
    return x;
  endfunction

  assign y = passthrough(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_unsupported_statement_kind_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionImportedBodyUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_imported_body_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_imported_body_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_imported_body_unsupported(
  input  logic a,
  output logic y
);
  import "DPI-C" function logic dpi_passthrough(input logic x);
  assign y = dpi_passthrough(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_imported_body_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionEmptyBlockBodyUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_empty_block_body_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_empty_block_body_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_empty_block_body_unsupported(
  input  logic a,
  output logic y
);
  function automatic logic passthrough(input logic x);
    begin
    end
  endfunction

  assign y = passthrough(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_empty_block_body_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionNamedResultNoReturnUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_named_result_no_return_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_named_result_no_return_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_named_result_no_return_unsupported(
  input  logic a,
  output logic y
);
  function automatic logic passthrough(input logic x);
    logic tmp;
    tmp = x;
  endfunction

  assign y = passthrough(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_named_result_no_return_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralNamedResultAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_named_result_assignment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_named_result_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_named_result_assignment_supported(
  input  logic [7:0] a,
  input  logic [7:0] b,
  output logic [15:0] y
);
  function automatic logic [7:0] passthrough(input logic [7:0] x);
    passthrough = {x[6:0], 1'b0};
  endfunction

  assign y = {passthrough(a), passthrough(b)};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_procedural_named_result_assignment_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionPowerReturnUnderAddSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_power_return_under_add_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_function_power_return_under_add_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package func_power_pkg;
  typedef logic [1:0] fmt_t;
  function automatic int unsigned bias(fmt_t fmt);
    return unsigned'(2**(fmt+2)-1);
  endfunction
endpackage

module continuous_assign_function_power_return_under_add_supported(
  input  logic signed [7:0] in_i,
  input  logic [1:0]        fmt_i,
  output logic signed [7:0] bias_o,
  output logic signed [7:0] sum_o
);
  import func_power_pkg::*;
  assign bias_o = signed'(bias(fmt_i));
  assign sum_o  = in_i + signed'(bias(fmt_i));
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_function_power_return_under_add_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("bias_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("sum_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionElementSelectMemberBaseUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_procedural_return_function_element_select_member_base_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_procedural_return_function_element_select_member_base_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_element_select_member_base_unsupported(
  input  logic [1:0] p_bits,
  output logic y
);
  typedef struct packed {
    logic [1:0] bits;
  } pair_t;

  function automatic logic pick(input pair_t p);
    logic tmp;
    tmp = p.bits[1];
    return p.bits[0];
  endfunction

  assign y = pick(pair_t'(p_bits));
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_element_select_member_base_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionConstRefFormalUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_const_ref_formal_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_const_ref_formal_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_const_ref_formal_unsupported(
  input  logic a,
  output logic y
);
  function automatic logic passthrough(const ref logic x);
    logic tmp;
    tmp = x;
    return tmp;
  endfunction

  assign y = passthrough(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_const_ref_formal_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionNonIntegralFormalUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_non_integral_formal_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_non_integral_formal_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_non_integral_formal_unsupported(
  output logic y
);
  function automatic logic passthrough(input string x);
    logic tmp;
    tmp = 1'b0;
    return tmp;
  endfunction

  assign y = passthrough("abc");
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_non_integral_formal_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignProceduralReturnFunctionArgumentResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_procedural_return_function_argument_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_procedural_return_function_argument_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_procedural_return_function_argument_resolve_failure_unsupported(
  output logic y
);
  function automatic logic passthrough(input logic x);
    logic tmp;
    tmp = x;
    return tmp;
  endfunction

  assign y = passthrough(1'bx);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_procedural_return_function_argument_resolve_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideDefaultOneSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_case_inside_default_one_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_default_one_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_default_one_supported(
  input  logic [5:0] op,
  output logic       y_dyn,
  output logic       y_below,
  output logic       y_in
);
  function automatic logic is_not_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: return 1'b0;
      default:        return 1'b1;
    endcase
  endfunction
  assign y_dyn   = is_not_amo(op);
  assign y_below = is_not_amo(6'd3);
  assign y_in    = is_not_amo(6'd8);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_default_one_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_dyn")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_below")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_in")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideNoDefaultUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_case_inside_no_default_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_no_default_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_no_default_unsupported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: return 1'b1;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseReturnNoDefaultUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_case_return_no_default_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_return_no_default_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_return_no_default_unsupported(
  input  logic [1:0] op,
  output logic       y
);
  function automatic logic decode(input logic [1:0] op_i);
    case (op_i)
      2'd1: return 1'b1;
    endcase
  endfunction
  assign y = decode(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_function_case_return_no_default_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseReturnDefaultResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_function_case_return_default_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_return_default_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_return_default_resolve_failure_unsupported(
  input  logic [1:0] op,
  output logic       y
);
  function automatic logic decode(input logic [1:0] op_i);
    case (op_i)
      2'd1:    return 1'b1;
      default: return 1'bx;
    endcase
  endfunction
  assign y = decode(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_function_case_return_default_resolve_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseReturnItemNonReturnSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_function_case_return_item_non_return_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_return_item_non_return_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_return_item_non_return_unsupported(
  input  logic [1:0] op,
  output logic       y
);
  function automatic logic decode(input logic [1:0] op_i);
    case (op_i)
      2'd1:    decode = 1'b1;
      default: return 1'b0;
    endcase
  endfunction
  assign y = decode(op);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_return_item_non_return_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseReturnItemResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_function_case_return_item_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_return_item_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_return_item_resolve_failure_unsupported(
  input  logic [1:0] op,
  output logic       y
);
  function automatic logic decode(input logic [1:0] op_i);
    case (op_i)
      2'd1:    return 1'bx;
      default: return 1'b0;
    endcase
  endfunction
  assign y = decode(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_function_case_return_item_resolve_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseReturnItemEqualityResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_function_case_return_item_equality_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_function_case_return_item_equality_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_return_item_equality_resolve_failure_unsupported(
  input  logic [1:0] op,
  output logic       y
);
  function automatic logic decode(input logic [1:0] op_i);
    case (op_i)
      2'bx1:   return 1'b1;
      default: return 1'b0;
    endcase
  endfunction
  assign y = decode(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_function_case_return_item_equality_resolve_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideNonConstantItemReturnUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_function_case_inside_non_constant_item_return_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_non_constant_item_return_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_non_constant_item_return_unsupported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      [6'd4 : 6'd17]: return op_i[0];
      default:        return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideSameAsDefaultItemSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_function_case_inside_same_as_default_item_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_same_as_default_item_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_same_as_default_item_supported(
  input  logic [5:0] op,
  output logic       y_zero,
  output logic       y_range
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      6'd1:           return 1'b0;
      [6'd4 : 6'd17]: return 1'b1;
      default:        return 1'b0;
    endcase
  endfunction
  assign y_zero  = is_amo(6'd1);
  assign y_range = is_amo(op);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_same_as_default_item_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_zero")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_range")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignFunctionCaseInsideAllItemsSameAsDefaultSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_function_case_inside_all_items_same_as_default_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_function_case_inside_all_items_same_as_default_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_function_case_inside_all_items_same_as_default_supported(
  input  logic [5:0] op,
  output logic       y
);
  function automatic logic is_amo(input logic [5:0] op_i);
    case (op_i) inside
      6'd1:           return 1'b0;
      [6'd4 : 6'd17]: return 1'b0;
      default:        return 1'b0;
    endcase
  endfunction
  assign y = is_amo(op);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_function_case_inside_all_items_same_as_default_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCaseAssignmentFunctionCallSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_assignment_function_call_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_assignment_function_call_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_assignment_function_call_unsupported(
  input  logic [3:0] op_i,
  output logic [1:0] data_size_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: extract_transfer_size = 2'b00;
      4'h1: extract_transfer_size = 2'b01;
      default: extract_transfer_size = 2'b11;
    endcase
  endfunction

  always_comb begin
    data_size_o = extract_transfer_size(op_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_assignment_function_call_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_size_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConcatenationLHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_concatenation_lhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_concatenation_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concatenation_lhs_supported(
  input  logic [1:0] d_i,
  output logic       a_o,
  output logic       b_o
);
  always_comb begin
    {a_o, b_o} = d_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_concatenation_lhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("a_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("b_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombWhileStatementUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_while_statement_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_while_statement_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_while_statement_unsupported(
  input  logic en_i,
  input  logic d_i,
  output logic y_o
);
  always_comb begin
    while (en_i) begin
      y_o = d_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombUnpackedStructMemberLHSBitsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_unpacked_struct_member_lhs_bits_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_unpacked_struct_member_lhs_bits_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_struct_member_lhs_bits_unsupported(
  input  logic [1:0] d_i,
  output logic [7:0] q_o
);
  typedef struct {
    logic [1:0] a;
  } unpacked_s_t;
  unpacked_s_t s;

  always_comb begin
    s.a = d_i;
    q_o = {6'b0, s.a};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to resolve always_comb assignment LHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombMemberAccessNonIntegralLHSWidthUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_member_access_non_integral_lhs_width_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_member_access_non_integral_lhs_width_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_member_access_non_integral_lhs_width_unsupported;
  typedef struct {
    logic [1:0] a [0:1];
  } unpacked_s_t;

  unpacked_s_t s_q;
  unpacked_s_t s_n;

  always_comb begin
    s_n.a = s_q.a;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to resolve always_comb assignment LHS bits"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalWithFunctionReturnExprSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_with_function_return_expr_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_with_function_return_expr_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_with_function_return_expr_supported #(
  parameter bit IS_XLEN64 = 1'b1
) (
  input logic [63:0] result,
  input logic        word_op_q,
  output logic [63:0] div_result
);
  function automatic logic [63:0] sext32to64(logic [63:0] operand);
    return {{32{operand[31]}}, operand[31:0]};
  endfunction

  assign div_result = (IS_XLEN64 && word_op_q) ? sext32to64(result) : result;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_conditional_with_function_return_expr_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("result")), nullptr);
  EXPECT_NE(top->getNet(NLName("word_op_q")), nullptr);
  EXPECT_NE(top->getNet(NLName("div_result")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConditionalConcatNestedFunctionCallsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_conditional_concat_nested_function_calls_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_concat_nested_function_calls_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package aes_pkg;
  function automatic logic [31:0] aes_subword_fwd(input logic [31:0] word);
    aes_subword_fwd = {word[23:0], word[31:24]};
  endfunction

  function automatic logic [31:0] aes_decode_rcon(input logic [3:0] r);
    case (r)
      4'h0: aes_decode_rcon = 32'h00000001;
      4'hA: aes_decode_rcon = 32'h00000000;
      default: aes_decode_rcon = 32'h00000002;
    endcase
  endfunction
endpackage

module continuous_assign_conditional_concat_nested_function_calls_supported(
  input  logic [63:0] operand_a_i,
  input  logic [3:0]  round_i,
  output logic [63:0] y_o
);
  import aes_pkg::*;

  assign y_o = (round_i <= 4'hA) ? {
    (aes_subword_fwd(
      (round_i == 4'hA) ? operand_a_i[63:32]
                        : ((operand_a_i[63:32] >> 8) | (operand_a_i[63:32] << 24))
    ) ^ aes_decode_rcon(round_i)),
    (aes_subword_fwd(
      (round_i == 4'hA) ? operand_a_i[63:32]
                        : ((operand_a_i[63:32] >> 8) | (operand_a_i[63:32] << 24))
    ) ^ aes_decode_rcon(round_i))
  } : 64'h0;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_conditional_concat_nested_function_calls_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConfigRangeCheckFunctionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_config_range_check_function_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_supported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 2,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_config_range_check_function_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("addr")), nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionLoopBodyListSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_loop_body_list_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_loop_body_list_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = 1'b0;
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_loop_body_list_supported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 2,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_config_range_check_function_loop_body_list_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("addr")), nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionConstantAddressBelowBaseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_config_range_check_function_constant_address_below_base_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_constant_address_below_base_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_constant_address_below_base_supported(
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, 64'h0);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_config_range_check_function_constant_address_below_base_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionReplicatedAddressShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_config_range_check_function_replicated_address_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_replicated_address_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_replicated_address_shortcut_supported(
  input  logic addr_bit_i,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'hC000000000000000},
    NonIdempotentLength: '{64'h0000000000000001}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, {64{addr_bit_i}});
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_config_range_check_function_replicated_address_shortcut_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("addr_bit_i")), nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionLoopBodyFallbackPatternsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_config_range_check_function_loop_body_fallback_patterns_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_loop_body_fallback_patterns_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(
    logic [63:0] base,
    logic [63:0] len,
    logic [63:0] address = 64'd0);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic range_check_alt(
    logic [63:0] base,
    logic [63:0] len,
    logic [63:0] address);
    return range_check(base, len, address);
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
      pass[k] =
        range_check_alt(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
      pass[k] = range_check(
        .base(Cfg.NonIdempotentAddrBase[k]),
        .len(Cfg.NonIdempotentLength[k]),
        .address());
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_loop_body_fallback_patterns_supported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 2,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_config_range_check_function_loop_body_fallback_patterns_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("addr")), nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionWrongArityUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_wrong_arity_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_wrong_arity_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(
    cfg_t Cfg,
    logic [63:0] address,
    logic extra_i);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return extra_i ? 1'b0 : |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_wrong_arity_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 2,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr, 1'b0);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_wrong_arity_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionRefFormalUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_config_range_check_function_ref_formal_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_config_range_check_function_ref_formal_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(
    input cfg_t Cfg,
    ref logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module always_comb_config_range_check_function_ref_formal_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 2,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  always_comb begin
    inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module "
     "'always_comb_config_range_check_function_ref_formal_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionNonConstantDefaultWhenNoRulesUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_config_range_check_function_non_constant_default_when_no_rules_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_non_constant_default_when_no_rules_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    if (Cfg.NrNonIdempotentRules != 0) begin
      logic [NrMaxRules-1:0] pass;
      pass = '0;
      for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
        pass[k] =
          range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
      end
      return |pass;
    end else begin
      return address[0];
    end
  endfunction
endpackage

module continuous_assign_config_range_check_function_non_constant_default_when_no_rules_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 0,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_non_constant_default_when_no_rules_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionConstantDefaultWhenNoRulesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "continuous_assign_config_range_check_function_constant_default_when_no_rules_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_constant_default_when_no_rules_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    if (Cfg.NrNonIdempotentRules != 0) begin
      logic [NrMaxRules-1:0] pass;
      pass = '0;
      for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
        pass[k] =
          range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
      end
      return |pass;
    end else begin
      return 1'b1;
    end
  endfunction
endpackage

module continuous_assign_config_range_check_function_constant_default_when_no_rules_supported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 0,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "continuous_assign_config_range_check_function_constant_default_when_no_rules_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("addr")), nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionNonBinaryStopExprUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_non_binary_stop_expr_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_non_binary_stop_expr_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_non_binary_stop_expr_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_non_binary_stop_expr_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionNonLessThanStopExprUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_non_less_than_stop_expr_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_non_less_than_stop_expr_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k <= Cfg.NrNonIdempotentRules; k++) begin
      pass[0] = range_check(Cfg.NonIdempotentAddrBase[0], Cfg.NonIdempotentLength[0], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_non_less_than_stop_expr_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_non_less_than_stop_expr_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionNonIsolatedLoopIndexUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_non_isolated_loop_index_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_non_isolated_loop_index_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k + 1 < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_non_isolated_loop_index_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 2,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000002000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_non_isolated_loop_index_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionNoRangeCheckInLoopUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_no_range_check_in_loop_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_no_range_check_in_loop_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = address[0];
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_no_range_check_in_loop_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_no_range_check_in_loop_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionUnknownRuleCountUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_unknown_rule_count_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_unknown_rule_count_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    logic [31:0]                 NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_unknown_rule_count_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 32'h0000_000x,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_unknown_rule_count_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionOversizedRuleCountUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_oversized_rule_count_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_oversized_rule_count_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    logic [31:0]                 NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_oversized_rule_count_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 32'd4097,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_oversized_rule_count_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionUnsupportedAddressExprUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "continuous_assign_config_range_check_function_unsupported_address_expr_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "continuous_assign_config_range_check_function_unsupported_address_expr_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_unsupported_address_expr_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000100}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr << 1'bx);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_unsupported_address_expr_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionUnknownLengthUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_unknown_length_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_unknown_length_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_unknown_length_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h00000000000000x0}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_unknown_length_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionOversizedBaseUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_oversized_base_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_oversized_base_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 1;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [64:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] =
        range_check(65'h1_0000_0000_0000_0000, Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_oversized_base_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t Cfg = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000},
    NonIdempotentLength: '{64'h0000000000000010}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(Cfg, addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_oversized_base_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionLiteralConfigUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_literal_config_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_literal_config_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_literal_config_unsupported(
  input logic [63:0] addr,
  output logic inside_o
);
  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(
    cfg_pkg::cfg_t'('{
      NrNonIdempotentRules: 2,
      NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
      NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
    }),
    addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_literal_config_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionContextualLiteralConfigUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_range_check_function_contextual_literal_config_probe";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_contextual_literal_config_probe.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_contextual_literal_config_probe(
  input logic [63:0] addr,
  output logic inside_o
);
  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(
    '{
      NrNonIdempotentRules: 2,
      NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
      NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000080}
    },
    addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_contextual_literal_config_probe'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionConfigTableElementSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_config_range_check_function_config_table_element_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_config_table_element_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_config_table_element_supported(
  input logic [63:0] addr,
  output logic inside_o
);
  localparam cfg_pkg::cfg_t CfgTable [0:0] = '{
    '{
      NrNonIdempotentRules: 1,
      NonIdempotentAddrBase: '{64'h0000000000002000, 64'h0000000000000000},
      NonIdempotentLength: '{64'h0000000000000040, 64'h0000000000000000}
    }
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(CfgTable[0], addr);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_config_range_check_function_config_table_element_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("addr")), nullptr);
  EXPECT_NE(top->getNet(NLName("inside_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConfigRangeCheckFunctionConditionalConfigUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "continuous_assign_config_range_check_function_conditional_config_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_config_range_check_function_conditional_config_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  NrNonIdempotentRules;
    logic [NrMaxRules-1:0][63:0] NonIdempotentAddrBase;
    logic [NrMaxRules-1:0][63:0] NonIdempotentLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_nonidempotent_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    pass = '0;
    for (int unsigned k = 0; k < Cfg.NrNonIdempotentRules; k++) begin
      pass[k] = range_check(Cfg.NonIdempotentAddrBase[k], Cfg.NonIdempotentLength[k], address);
    end
    return |pass;
  endfunction
endpackage

module continuous_assign_config_range_check_function_conditional_config_unsupported(
  input logic [63:0] addr,
  input logic        select_cfg,
  output logic       inside_o
);
  localparam cfg_pkg::cfg_t Cfg0 = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000001000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000100, 64'h0000000000000000}
  };
  localparam cfg_pkg::cfg_t Cfg1 = '{
    NrNonIdempotentRules: 1,
    NonIdempotentAddrBase: '{64'h0000000000002000, 64'h0000000000000000},
    NonIdempotentLength: '{64'h0000000000000080, 64'h0000000000000000}
  };

  assign inside_o = cfg_pkg::is_inside_nonidempotent_regions(
    select_cfg ? Cfg0 : Cfg1,
    addr);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_assign_config_range_check_function_conditional_config_unsupported'"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConfigExecuteRegionsFunctionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_config_execute_regions_function_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_config_execute_regions_function_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package cfg_exec_pkg;
  localparam int unsigned NrMaxRules = 2;

  typedef struct packed {
    int unsigned                  PLEN;
    int unsigned                  NrExecuteRegionRules;
    logic [NrMaxRules-1:0][63:0] ExecuteRegionAddrBase;
    logic [NrMaxRules-1:0][63:0] ExecuteRegionLength;
  } cfg_t;

  function automatic logic range_check(logic [63:0] base, logic [63:0] len, logic [63:0] address);
    return (address >= base) && (({1'b0, address}) < (65'(base) + len));
  endfunction

  function automatic logic is_inside_execute_regions(cfg_t Cfg, logic [63:0] address);
    logic [NrMaxRules-1:0] pass;
    if (Cfg.NrExecuteRegionRules != 0) begin
      pass = '0;
      for (int unsigned k = 0; k < Cfg.NrExecuteRegionRules; k++) begin
        pass[k] = range_check(Cfg.ExecuteRegionAddrBase[k], Cfg.ExecuteRegionLength[k], address);
      end
      return |pass;
    end else begin
      return 1'b1;
    end
  endfunction
endpackage

module continuous_assign_config_execute_regions_function_supported(
  input logic [55:0] fetch_paddr_i,
  output logic       match_o
);
  typedef struct packed {
    logic [55:0] fetch_paddr;
  } icache_areq_t;

  icache_areq_t icache_areq_i;
  assign icache_areq_i.fetch_paddr = fetch_paddr_i;

  localparam cfg_exec_pkg::cfg_t Cfg = '{
    PLEN: 56,
    NrExecuteRegionRules: 1,
    ExecuteRegionAddrBase: '{64'h0000000000000000, 64'h0000000000000000},
    ExecuteRegionLength: '{64'h00000000FFFFFFFF, 64'h0000000000000000}
  };

  assign match_o = cfg_exec_pkg::is_inside_execute_regions(
      Cfg, {{64 - Cfg.PLEN{1'b0}}, icache_areq_i.fetch_paddr}
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_config_execute_regions_function_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("fetch_paddr_i")), nullptr);
  EXPECT_NE(top->getNet(NLName("match_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConditionalUnaryMinusSignedSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_conditional_unary_minus_signed_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_conditional_unary_minus_signed_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_conditional_unary_minus_signed_supported(
  input logic        res_inv_q,
  input logic [63:0] out_mux,
  output logic [63:0] res_o
);
  assign res_o = (res_inv_q) ? -$signed(out_mux) : out_mux;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_assign_conditional_unary_minus_signed_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("res_inv_q")), nullptr);
  EXPECT_NE(top->getNet(NLName("out_mux")), nullptr);
  EXPECT_NE(top->getNet(NLName("res_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseBytePortsInferRangeFromWidth) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "byte_ports" / "byte_ports.sv");

  auto top = library_->getSNLDesign(NLName("byte_ports_top"));
  ASSERT_NE(top, nullptr);

  auto aTerm = top->getBusTerm(NLName("a"));
  auto yTerm = top->getBusTerm(NLName("y"));
  ASSERT_NE(aTerm, nullptr);
  ASSERT_NE(yTerm, nullptr);
  EXPECT_EQ(8, aTerm->getWidth());
  EXPECT_EQ(8, yTerm->getWidth());
  EXPECT_EQ(7, aTerm->getMSB());
  EXPECT_EQ(0, aTerm->getLSB());
  EXPECT_EQ(7, yTerm->getMSB());
  EXPECT_EQ(0, yTerm->getLSB());

  auto aNet = top->getBusNet(NLName("a"));
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(aNet, nullptr);
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(8, aNet->getWidth());
  EXPECT_EQ(8, yNet->getWidth());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "byte_ports");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseImplicitWidthPortsInferRangeFromWidth) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "implicit_width_ports" / "implicit_width_ports.sv");

  auto top = library_->getSNLDesign(NLName("implicit_width_ports_top"));
  ASSERT_NE(top, nullptr);

  auto wideInput = top->getBusTerm(NLName("wide_i"));
  auto wideOutput = top->getBusTerm(NLName("wide_o"));
  ASSERT_NE(wideInput, nullptr);
  ASSERT_NE(wideOutput, nullptr);
  EXPECT_EQ(32, wideInput->getWidth());
  EXPECT_EQ(32, wideOutput->getWidth());
  EXPECT_EQ(31, wideInput->getMSB());
  EXPECT_EQ(0, wideInput->getLSB());
  EXPECT_EQ(31, wideOutput->getMSB());
  EXPECT_EQ(0, wideOutput->getLSB());

  auto bitTerm = top->getTerm(NLName("bit_i"));
  ASSERT_NE(bitTerm, nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarTerm*>(bitTerm), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "implicit_width_ports");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parsePackedStructAndEnumPortsInferRangeFromWidth) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "packed_struct_enum_ports";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "packed_struct_enum_ports.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "typedef struct packed {\n"
         << "  logic [3:0] a;\n"
         << "  logic b;\n"
         << "} st_t;\n"
         << "typedef enum logic [1:0] {\n"
         << "  IDLE = 2'b00,\n"
         << "  BUSY = 2'b01\n"
         << "} state_t;\n"
         << "module packed_struct_enum_ports(\n"
         << "  input st_t in_s,\n"
         << "  input state_t st_i,\n"
         << "  output st_t out_s,\n"
         << "  output state_t st_o\n"
         << ");\n"
         << "  assign out_s = in_s;\n"
         << "  assign st_o = st_i;\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("packed_struct_enum_ports"));
  ASSERT_NE(top, nullptr);

  auto inStruct = top->getBusTerm(NLName("in_s"));
  auto outStruct = top->getBusTerm(NLName("out_s"));
  auto inEnum = top->getBusTerm(NLName("st_i"));
  auto outEnum = top->getBusTerm(NLName("st_o"));
  ASSERT_NE(inStruct, nullptr);
  ASSERT_NE(outStruct, nullptr);
  ASSERT_NE(inEnum, nullptr);
  ASSERT_NE(outEnum, nullptr);

  EXPECT_EQ(5, inStruct->getWidth());
  EXPECT_EQ(5, outStruct->getWidth());
  EXPECT_EQ(2, inEnum->getWidth());
  EXPECT_EQ(2, outEnum->getWidth());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "packed_struct_enum_ports_dump");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedNestedFixedDynamicPortTypeReportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unsupported_nested_fixed_dynamic_port_type";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "unsupported_nested_fixed_dynamic_port_type.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module unsupported_nested_fixed_dynamic_port_type(\n"
    << "  input logic [3:0] arr [0:1] [],\n"
    << "  output logic y\n"
    << ");\n"
    << "  assign y = 1'b0;\n"
    << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported nested fixed/dynamic unpacked port type exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog type not representable in SNL"));
    EXPECT_NE(std::string::npos, reason.find("arr"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedPortTypesReportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "unsupported_port_types" / "unsupported_port_types.sv");
    FAIL() << "Expected unsupported port type exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog floating-point type"));
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog type 'string'"));
    EXPECT_NE(std::string::npos, reason.find("wide_f"));
    EXPECT_NE(std::string::npos, reason.find("str_i"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignUnsupportedLHSTypeReportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_assign_unsupported_lhs" / "continuous_assign_unsupported_lhs.sv");
    FAIL() << "Expected unsupported continuous-assign LHS type exception";
  } catch (const SNLSVConstructorException& e) {
    (void)e;
  }
}

TEST_F(SNLSVConstructorTestSimple, parseFixedUnpackedArrayPortSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "unsupported_generic_type" / "unsupported_generic_type.sv");

  auto top = library_->getSNLDesign(NLName("unsupported_generic_type"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getBusTerm(NLName("arr")), nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseFixedUnpackedArrayVariableSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "fixed_unpacked_array_variable";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "fixed_unpacked_array_variable.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module fixed_unpacked_array_variable(\n"
         << "  input logic [31:0] in0,\n"
         << "  input logic [31:0] in1,\n"
         << "  output logic [31:0] y\n"
         << ");\n"
         << "  logic [31:0] fetch_instructions [1:0];\n"
         << "  assign fetch_instructions[0] = in0;\n"
         << "  assign fetch_instructions[1] = in1;\n"
         << "  assign y = in0;\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("fixed_unpacked_array_variable"));
  ASSERT_NE(top, nullptr);

  auto fetchInstructions = top->getBusNet(NLName("fetch_instructions"));
  ASSERT_NE(fetchInstructions, nullptr);
  EXPECT_EQ(64, fetchInstructions->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseDynamicUnpackedVariablesIgnoredInNetCreation) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "dynamic_unpacked_variables_ignored" /
      "dynamic_unpacked_variables_ignored.sv",
    {"dynamic unpacked array/queue/associative array"});
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedSymbolInExpressionReportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "unsupported_symbol_expr" / "unsupported_symbol_expr.sv");
    FAIL() << "Expected unsupported symbol type exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"));
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog floating-point type"));
    EXPECT_NE(std::string::npos, reason.find("for net/variable: wide_f"));
    EXPECT_NE(std::string::npos, reason.find("for symbol: wide_f"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseInoutPortDirection) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "port_directions" / "port_directions.sv");

  auto top = library_->getSNLDesign(NLName("port_directions_top"));
  ASSERT_NE(top, nullptr);

  auto inputTerm = top->getTerm(NLName("i"));
  ASSERT_NE(inputTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Input, inputTerm->getDirection());

  auto outputTerm = top->getTerm(NLName("o"));
  ASSERT_NE(outputTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::Output, outputTerm->getDirection());

  auto inoutTerm = top->getTerm(NLName("io"));
  ASSERT_NE(inoutTerm, nullptr);
  EXPECT_EQ(SNLTerm::Direction::InOut, inoutTerm->getDirection());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "port_directions");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseNonAnsiPortsCreateTermNets) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "non_ansi_ports" / "non_ansi_ports.sv");

  auto top = library_->getSNLDesign(NLName("non_ansi_ports_top"));
  ASSERT_NE(top, nullptr);

  auto aTerm = top->getTerm(NLName("a"));
  auto bTerm = top->getBusTerm(NLName("b"));
  auto yTerm = top->getTerm(NLName("y"));
  auto zTerm = top->getBusTerm(NLName("z"));
  ASSERT_NE(aTerm, nullptr);
  ASSERT_NE(bTerm, nullptr);
  ASSERT_NE(yTerm, nullptr);
  ASSERT_NE(zTerm, nullptr);

  auto aNet = top->getNet(NLName("a"));
  auto bNet = top->getBusNet(NLName("b"));
  auto yNet = top->getNet(NLName("y"));
  auto zNet = top->getBusNet(NLName("z"));
  ASSERT_NE(aNet, nullptr);
  ASSERT_NE(bNet, nullptr);
  ASSERT_NE(yNet, nullptr);
  ASSERT_NE(zNet, nullptr);

  EXPECT_NE(dynamic_cast<SNLScalarNet*>(aNet), nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarNet*>(yNet), nullptr);
  EXPECT_EQ(4, bNet->getWidth());
  EXPECT_EQ(4, zNet->getWidth());

  EXPECT_TRUE(hasRTLInfo(aNet, "sv_src_file"));
  EXPECT_TRUE(hasRTLInfo(bNet, "sv_src_file"));
  EXPECT_TRUE(hasRTLInfo(yNet, "sv_src_file"));
  EXPECT_TRUE(hasRTLInfo(zNet, "sv_src_file"));

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "non_ansi_ports");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseModelReuseBuildDesignCache) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "model_reuse" / "model_reuse.sv");

  auto top = library_->getSNLDesign(NLName("model_reuse_top"));
  auto child = library_->getSNLDesign(NLName("child"));
  ASSERT_NE(top, nullptr);
  ASSERT_NE(child, nullptr);

  auto u0 = top->getInstance(NLName("u0"));
  auto u1 = top->getInstance(NLName("u1"));
  ASSERT_NE(u0, nullptr);
  ASSERT_NE(u1, nullptr);
  EXPECT_EQ(child, u0->getModel());
  EXPECT_EQ(child, u1->getModel());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "model_reuse");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseModelReuseRepresentativeBodyCacheAfterDefparam) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "model_reuse_representative_body_cache_after_defparam";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "model_reuse_representative_body_cache_after_defparam.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module child #(parameter int W = 1) (\n"
         << "  input logic [W-1:0] a,\n"
         << "  output logic [W-1:0] y\n"
         << ");\n"
         << "  assign y = a;\n"
         << "endmodule\n"
         << "\n"
         << "module top(\n"
         << "  input logic [1:0] a0,\n"
         << "  input logic [1:0] a1,\n"
         << "  output logic [1:0] y0,\n"
         << "  output logic [1:0] y1\n"
         << ");\n"
         << "  child u0(.a(a0), .y(y0));\n"
         << "  child u1(.a(a1), .y(y1));\n"
         << "  defparam u0.W = 2;\n"
         << "  defparam u1.W = 2;\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  auto* u0 = top->getInstance(NLName("u0"));
  auto* u1 = top->getInstance(NLName("u1"));
  ASSERT_NE(u0, nullptr);
  ASSERT_NE(u1, nullptr);
  EXPECT_EQ(u0->getModel(), u1->getModel());
  ASSERT_NE(u0->getModel(), nullptr);

  auto* aTerm = u0->getModel()->getBusTerm(NLName("a"));
  auto* yTerm = u0->getModel()->getBusTerm(NLName("y"));
  ASSERT_NE(aTerm, nullptr);
  ASSERT_NE(yTerm, nullptr);
  EXPECT_EQ(2, aTerm->getWidth());
  EXPECT_EQ(2, yTerm->getWidth());

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "model_reuse_representative_body_cache_after_defparam");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionEdgeCasesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "instance_connection_edge_cases" / "instance_connection_edge_cases.sv");

  auto top = library_->getSNLDesign(NLName("instance_connection_edge_cases"));
  ASSERT_NE(top, nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "instance_connection_edge_cases");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionInputBusWidthResizeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "instance_connection_input_bus_width_resize_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_input_bus_width_resize_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module child(input logic [3:0] a, output logic y);\n"
         << "  assign y = a[0];\n"
         << "endmodule\n"
         << "module top(input logic [1:0] a, output logic y);\n"
         << "  child u(.a(a), .y(y));\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("u"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "instance_connection_input_bus_width_resize_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionScalarInputWidthResizeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "instance_connection_scalar_input_width_resize_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_scalar_input_width_resize_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module child(input logic a, output logic y);\n"
         << "  assign y = a;\n"
         << "endmodule\n"
         << "module top(input logic [1:0] a, output logic y);\n"
         << "  child u(.a(a), .y(y));\n"
         << "endmodule\n";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("u"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "instance_connection_scalar_input_width_resize_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionScalarOutputWidthMismatchReportsRTL) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= "instance_connection_scalar_output_width_mismatch_reports_rtl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_scalar_output_width_mismatch_reports_rtl.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module child(output logic a);\n"
         << "endmodule\n"
         << "module top(output logic [1:0] a);\n"
         << "  child u(.a(a));\n"
         << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    EXPECT_NE(std::string::npos, reason.find(svPath.filename().string())) << reason;
    EXPECT_NE(std::string::npos, reason.find("Unsupported instance connection for port 'a'"))
      << reason;
    EXPECT_NE(std::string::npos, reason.find("cannot set u/a to a[1:0], bus width is 2"))
      << reason;
    EXPECT_NE(std::string::npos, reason.find("[RTL: a]")) << reason;
  }
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic kill_s2;
  logic req;
} dreq_t;

module child_flush(
  input  logic flush_i,
  output logic y
);
  assign y = flush_i;
endmodule

module instance_connection_member_access_supported(
  input  dreq_t icache_dreq_o,
  output logic y
);
  child_flush i_instr_realign(
    .flush_i(icache_dreq_o.kill_s2),
    .y(y)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("instance_connection_member_access_supported"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child_flush"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("i_instr_realign"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "instance_connection_member_access_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionScalarConstSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_scalar_const_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_scalar_const_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module child_testmode(
  input  logic testmode_i,
  output logic y
);
  assign y = testmode_i;
endmodule

module instance_connection_scalar_const_supported(
  output logic y
);
  child_testmode i_fifo_address (
    .testmode_i(1'b0),
    .y(y)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("instance_connection_scalar_const_supported"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child_testmode"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("i_fifo_address"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto testmodeTerm = child->getScalarTerm(NLName("testmode_i"));
  ASSERT_NE(testmodeTerm, nullptr);
  auto instTerm = inst->getInstTerm(testmodeTerm);
  ASSERT_NE(instTerm, nullptr);
  auto net = instTerm->getNet();
  ASSERT_NE(net, nullptr);
  EXPECT_EQ(SNLNet::Type::Assign0, net->getType());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseInstanceConnectionScalarUnsupportedExpressionReported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_scalar_unsupported_expression_reported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_scalar_unsupported_expression_reported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module child_scalar(
  input logic a_i
);
endmodule

module instance_connection_scalar_unsupported_expression_reported(
  input logic [3:0] op_i
);
  function automatic logic bad_fn(input logic [3:0] op);
    case (op) inside
      [4'bxxxx : 4'd7]: bad_fn = 1'b1;
      default:          bad_fn = 1'b0;
    endcase
  endfunction

  child_scalar u_child(
    .a_i(bad_fn(op_i))
  );
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported instance connection expression for port 'a_i'"))
      << reason;
  }
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionTypeParamConcatToBusSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_type_param_concat_to_bus_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_type_param_concat_to_bus_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module shift_reg #(
  parameter type dtype = logic
) (
  input  dtype d_i,
  output dtype d_o
);
  assign d_o = d_i;
endmodule

module instance_connection_type_param_concat_to_bus_supported(
  input  logic a,
  input  logic b,
  input  logic c,
  output logic [2:0] y
);
  shift_reg #(
    .dtype(logic [2:0])
  ) i_pipe_reg_load (
    .d_i({a, b, c}),
    .d_o(y)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("instance_connection_type_param_concat_to_bus_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());

  auto child = library_->getSNLDesign(NLName("shift_reg"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("i_pipe_reg_load"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionOutputElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_output_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_output_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module child_output_slice(
  output logic [8:0] d_o
);
endmodule

module instance_connection_output_element_select_supported(
  output logic [2:0][8:0] y
);
  child_output_slice i_load_unit (
    .d_o(y[1])
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("instance_connection_output_element_select_supported"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child_output_slice"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("i_load_unit"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(27, yNet->getWidth());

  auto dTerm = child->getBusTerm(NLName("d_o"));
  ASSERT_NE(dTerm, nullptr);
  auto dTermBit0 = dTerm->getBit(0);
  ASSERT_NE(dTermBit0, nullptr);
  auto dBit0InstTerm = inst->getInstTerm(dTermBit0);
  ASSERT_NE(dBit0InstTerm, nullptr);
  ASSERT_NE(dBit0InstTerm->getNet(), nullptr);
  EXPECT_EQ(static_cast<void*>(yNet->getBit(9)), static_cast<void*>(dBit0InstTerm->getNet()));
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionOutputPackedToFixedUnpackedSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_output_packed_to_fixed_unpacked_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "instance_connection_output_packed_to_fixed_unpacked_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef logic [7:0] link_t;

module child_output_mesh(
  output logic [1:0][1:0][7:0] d_o
);
endmodule

module instance_connection_output_packed_to_fixed_unpacked_supported(
  output link_t [1:0][1:0] y
);
  child_output_mesh u_mesh (
    .d_o(y)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("instance_connection_output_packed_to_fixed_unpacked_supported"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child_output_mesh"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("u_mesh"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(32, yNet->getWidth());

  auto dTerm = child->getBusTerm(NLName("d_o"));
  ASSERT_NE(dTerm, nullptr);
  auto dTermBit0 = dTerm->getBit(0);
  ASSERT_NE(dTermBit0, nullptr);
  auto dBit0InstTerm = inst->getInstTerm(dTermBit0);
  ASSERT_NE(dBit0InstTerm, nullptr);
  ASSERT_NE(dBit0InstTerm->getNet(), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionOutputPackedTruncateSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_output_packed_truncate_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "instance_connection_output_packed_truncate_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module child_output_mesh(
  output logic [1:0][1:0][7:0] d_o
);
endmodule

module instance_connection_output_packed_truncate_supported(
  output logic [0:0][1:0][7:0] y
);
  child_output_mesh u_mesh (
    .d_o(y)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("instance_connection_output_packed_truncate_supported"));
  ASSERT_NE(top, nullptr);
  auto child = library_->getSNLDesign(NLName("child_output_mesh"));
  ASSERT_NE(child, nullptr);
  auto inst = top->getInstance(NLName("u_mesh"));
  ASSERT_NE(inst, nullptr);
  EXPECT_EQ(child, inst->getModel());

  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(16, yNet->getWidth());

  auto dTerm = child->getBusTerm(NLName("d_o"));
  ASSERT_NE(dTerm, nullptr);
  auto dTermBit0 = dTerm->getBit(0);
  auto dTermBit15 = dTerm->getBit(15);
  auto dTermBit16 = dTerm->getBit(16);
  ASSERT_NE(dTermBit0, nullptr);
  ASSERT_NE(dTermBit15, nullptr);
  ASSERT_NE(dTermBit16, nullptr);

  auto dBit0InstTerm = inst->getInstTerm(dTermBit0);
  auto dBit15InstTerm = inst->getInstTerm(dTermBit15);
  auto dBit16InstTerm = inst->getInstTerm(dTermBit16);
  ASSERT_NE(dBit0InstTerm, nullptr);
  ASSERT_NE(dBit15InstTerm, nullptr);
  ASSERT_NE(dBit16InstTerm, nullptr);
  EXPECT_EQ(static_cast<void*>(yNet->getBit(0)), static_cast<void*>(dBit0InstTerm->getNet()));
  EXPECT_EQ(static_cast<void*>(yNet->getBit(15)), static_cast<void*>(dBit15InstTerm->getNet()));
  EXPECT_EQ(nullptr, dBit16InstTerm->getNet());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseInstanceConnectionBusUnsupportedExpressionNetCompatibilityReported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_bus_unsupported_expression_net_compatibility_reported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "instance_connection_bus_unsupported_expression_net_compatibility_reported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module child_bus(
  input logic [1:0] a_i
);
endmodule

module instance_connection_bus_unsupported_expression_net_compatibility_reported(
  input logic [3:0] op_i
);
  function automatic logic [1:0] bad_fn(input logic [3:0] op);
    case (op) inside
      [4'bxxxx : 4'd7]: bad_fn = 2'b11;
      default:          bad_fn = 2'b00;
    endcase
  endfunction

  child_bus u_child(
    .a_i(bad_fn(op_i))
  );
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported instance connection net/term compatibility for port 'a_i'"))
      << reason;
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseInstanceConnectionOutputPackedWiderActualReportsWidths) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "instance_connection_output_packed_wider_actual_reports_widths";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "instance_connection_output_packed_wider_actual_reports_widths.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module child_output_pair(
  output logic [1:0] d_o
);
endmodule

module instance_connection_output_packed_wider_actual_reports_widths(
  output logic [2:0] y
);
  child_output_pair u_pair (
    .d_o(y)
  );
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported instance connection for port 'd_o'"))
      << reason;
    EXPECT_NE(
      std::string::npos,
      reason.find("term width is 2, actual width is 3, direction is output"))
      << reason;
  }
}

TEST_F(SNLSVConstructorTestSimple, parseInterfacePortReportedUnsupportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "interface_port_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "interface_port_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(interface data_if;
  logic sig;
endinterface

module interface_port_child(
  data_if bus,
  input logic i,
  output logic o
);
  assign o = i;
endmodule

module interface_port_reported_unsupported_top(
  input logic i,
  output logic o
);
  data_if bus();
  interface_port_child u_child(
    .bus(bus),
    .i(i),
    .o(o)
  );
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported interface port exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported SystemVerilog interface port declaration for port: bus"))
      << reason;
  }
}

TEST_F(SNLSVConstructorTestSimple, parseNonAnsiUnnamedMultiPortReportedUnsupportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "non_ansi_unnamed_multiport" / "non_ansi_unnamed_multiport.sv");
    FAIL() << "Expected unsupported unnamed multi-port exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported SystemVerilog multi-port declaration for port: <anonymous>"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedElementsReportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unsupported_elements";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "unsupported_elements.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module unsupported_leaf(ref logic r0, input logic a, output logic y);\n"
    << "  assign y = a;\n"
    << "endmodule\n"
    << "module unsupported_elements(input logic a, output logic y);\n"
    << "  logic r0;\n"
    << "  unsupported_leaf u_leaf(.r0(r0), .a(a), .y(y));\n"
    << "endmodule\n";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported SystemVerilog port direction 'ref' for port: r0"});
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedModuleBodyCoverageScopeLabelReported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unsupported_module_body_coverage_scope";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "unsupported_module_body_coverage_scope.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module top(input logic clk);\n"
         << "  clocking cb @(posedge clk);\n"
         << "  endclocking\n"
         << "endmodule\n";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported elaborated member in module body"});
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedGenerateBodyCoverageScopeLabelReported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unsupported_generate_body_coverage_scope";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "unsupported_generate_body_coverage_scope.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module top(input logic clk);\n"
         << "  if (1) begin : g\n"
         << "    clocking cb @(posedge clk);\n"
         << "    endclocking\n"
         << "  end\n"
         << "endmodule\n";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported elaborated member in generate block"});
}

TEST_F(SNLSVConstructorTestSimple, parseUnsupportedModuleBodyCoverageScopeLabelUnnamedMemberReported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "unsupported_module_body_coverage_scope_unnamed";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "unsupported_module_body_coverage_scope_unnamed.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module top(input logic clk);\n"
         << "  default clocking @(posedge clk);\n"
         << "  endclocking\n"
         << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported elaborated member in module body of module 'top': ClockingBlock"))
      << reason;
    EXPECT_EQ(std::string::npos, reason.find("ClockingBlock '")) << reason;
  }
}

TEST_F(SNLSVConstructorTestSimple, formatReasonWithSourceExcerptNoRangeFallsBackToReason) {
  const std::string reason = "unsupported connection";
  EXPECT_EQ(reason, detail::testSVConstructorFormatReasonWithSourceExcerptNoRange(reason));
}

TEST_F(SNLSVConstructorTestSimple, sourceExcerptHandlesMalformedRanges) {
  detail::SourceExcerptTestOptions options;
  options.sourceText = "abc";
  options.startOffset = 1;
  options.endOffset.reset();
  EXPECT_EQ(
    std::make_optional<std::string>("b"),
    detail::testSVConstructorGetSourceExcerpt(options));

  options = {};
  options.sourceText = "abc";
  options.startOffset.reset();
  options.endOffset = 1;
  EXPECT_EQ(std::nullopt, detail::testSVConstructorGetSourceExcerpt(options));

  options = {};
  options.sourceText = "abc";
  options.startOffset = 1;
  options.endOffset = 2;
  options.useAlternateEndBuffer = true;
  EXPECT_EQ(
    std::make_optional<std::string>("b"),
    detail::testSVConstructorGetSourceExcerpt(options));

  options = {};
  options.sourceText = "abc";
  options.startOffset = 1;
  options.endOffset = 1;
  EXPECT_EQ(
    std::make_optional<std::string>("b"),
    detail::testSVConstructorGetSourceExcerpt(options));

  options = {};
  options.sourceText = "";
  options.startOffset = 0;
  options.endOffset.reset();
  options.preserveRawBufferSize = true;
  EXPECT_EQ(std::nullopt, detail::testSVConstructorGetSourceExcerpt(options));

  options = {};
  options.sourceText = "abc";
  options.startOffset = 3;
  options.endOffset = 4;
  options.preserveRawBufferSize = true;
  EXPECT_EQ(std::nullopt, detail::testSVConstructorGetSourceExcerpt(options));
}

TEST_F(SNLSVConstructorTestSimple, sourceExcerptNormalizesWhitespace) {
  detail::SourceExcerptTestOptions options;
  options.sourceText = "a \n\t b  ";
  options.startOffset = 0;
  options.endOffset = options.sourceText.size();
  EXPECT_EQ(
    std::make_optional<std::string>("a b"),
    detail::testSVConstructorGetSourceExcerpt(options));
}

TEST_F(SNLSVConstructorTestSimple, sourceExcerptWhitespaceOnlyReturnsNullopt) {
  detail::SourceExcerptTestOptions options;
  options.sourceText = " \n\t ";
  options.startOffset = 0;
  options.endOffset = options.sourceText.size();
  EXPECT_EQ(std::nullopt, detail::testSVConstructorGetSourceExcerpt(options));
}

TEST_F(SNLSVConstructorTestSimple, sourceExcerptTruncatesLongExcerpt) {
  detail::SourceExcerptTestOptions options;
  options.sourceText = "abcdef";
  options.startOffset = 0;
  options.endOffset = options.sourceText.size();
  options.maxLength = 5;
  EXPECT_EQ(
    std::make_optional<std::string>("ab..."),
    detail::testSVConstructorGetSourceExcerpt(options));
}

TEST_F(SNLSVConstructorTestSimple, tryPowerInt64ComputesExponentiationBySquaring) {
  int64_t value = 0;
  ASSERT_TRUE(detail::testSVConstructorTryPowerInt64(3, 5, value));
  EXPECT_EQ(243, value);
}

TEST_F(SNLSVConstructorTestSimple, defaultInferredMemoryGuardUsesExpectedDefaults) {
  const auto defaults = detail::testSVConstructorDefaultInferredMemoryGuard();
  EXPECT_EQ(0, defaults.kind);
  EXPECT_TRUE(defaults.polarity);
  EXPECT_TRUE(defaults.exprNull);
  EXPECT_TRUE(defaults.caseExprNull);
  EXPECT_TRUE(defaults.caseStmtNull);
  EXPECT_TRUE(defaults.caseItemNull);
  EXPECT_FALSE(defaults.hasSourceRange);
}

TEST_F(SNLSVConstructorTestSimple, collectIndexedRangeElementIndicesHandlesIndexedDownAndOverflow) {
  const auto indices =
    detail::testSVConstructorCollectIndexedRangeElementIndices(3, 4, true);
  ASSERT_TRUE(indices.has_value());
  EXPECT_EQ((std::vector<int32_t> {0, 1, 2, 3}), *indices);

  EXPECT_EQ(
    std::nullopt,
    detail::testSVConstructorCollectIndexedRangeElementIndices(
      std::numeric_limits<int32_t>::min(),
      2,
      true));
}

TEST_F(SNLSVConstructorTestSimple, encodeUnsignedProductBitsExpandsConstantProduct) {
  const auto encodedBits = detail::testSVConstructorEncodeUnsignedProductBits(3, 5, 6);
  const std::vector<bool> expectedBits {true, true, true, true, false, false};
  EXPECT_EQ(expectedBits, encodedBits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  createPowerOfTwoBitsHandlesConstantExponentInAndOutOfRange) {
  const auto inRange = detail::testSVConstructorCreatePowerOfTwoBitsFromAssignRhs(
    R"(module detail_test(output logic [4:0] y);
  assign y = 3;
endmodule
)",
    5);
  ASSERT_TRUE(inRange.has_value());
  EXPECT_EQ("00010", *inRange);

  const auto outOfRange = detail::testSVConstructorCreatePowerOfTwoBitsFromAssignRhs(
    R"(module detail_test(output logic [3:0] y);
  assign y = 7;
endmodule
)",
    4);
  ASSERT_TRUE(outOfRange.has_value());
  EXPECT_EQ("0000", *outOfRange);
}

TEST_F(
  SNLSVConstructorTestSimple,
  createPowerOfTwoBitsHandlesEnumTypedConstantExponent) {
  const auto bits = detail::testSVConstructorCreatePowerOfTwoBitsFromAssignRhs(
    R"(module detail_test(output logic [3:0] y);
  typedef enum logic [1:0] { ShiftOne = 2'd1 } shift_t;
  localparam shift_t SHIFT = ShiftOne;
  assign y = SHIFT;
endmodule
)",
    4);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("0100", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveFixedUnpackedArraySelectionBitCountHandlesElementAndFailures) {
  const auto elementWidth =
    detail::testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
      R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [1:0] y
);
  assign y = arr[2];
endmodule
)");
  ASSERT_TRUE(elementWidth.has_value());
  EXPECT_EQ(2u, *elementWidth);

  const auto outOfRangeWidth =
    detail::testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
      R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [1:0] y
);
  assign y = arr[5];
endmodule
)");
  EXPECT_EQ(std::nullopt, outOfRangeWidth);

  const auto nonConstantIndexWidth =
    detail::testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
      R"(module detail_test(
  input logic [1:0] idx,
  input logic [1:0] arr [0:3],
  output logic [1:0] y
);
  assign y = arr[idx];
endmodule
)");
  EXPECT_EQ(std::nullopt, nonConstantIndexWidth);

  const auto indexedUpWidth =
    detail::testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
      R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [3:0] y
);
  assign y = arr[1+:2];
endmodule
)");
  EXPECT_EQ(std::nullopt, indexedUpWidth);

  const auto indexedDownWidth =
    detail::testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
      R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [3:0] y
);
  assign y = arr[2-:2];
endmodule
)");
  EXPECT_EQ(std::nullopt, indexedDownWidth);

  const auto outOfRangeIndexedUpWidth =
    detail::testSVConstructorResolveFixedUnpackedArraySelectionBitCountFromAssignRhs(
      R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [3:0] y
);
  assign y = arr[3+:2];
endmodule
)");
  EXPECT_EQ(std::nullopt, outOfRangeIndexedUpWidth);
}

TEST_F(SNLSVConstructorTestSimple, resolveWildcardPatternWidthFallbackUsesCanonicalWidth) {
  EXPECT_EQ(
    std::make_optional<size_t>(3),
    detail::testSVConstructorResolveWildcardPatternWidthFallback(std::nullopt, true, 3));
  EXPECT_EQ(
    std::nullopt,
    detail::testSVConstructorResolveWildcardPatternWidthFallback(std::nullopt, false, 3));
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveWildcardCaseItemPatternPackedStructLocalparamSupported) {
  const auto bits = detail::testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef struct packed {
    logic [2:0] bits;
  } packed_t;
  localparam packed_t P = '{bits: 3'b10x};
  assign y = P;
endmodule
)",
    3,
    false);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("x01", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveWildcardCaseItemPatternReplicatedPackedStructWidthFallbackSupported) {
  const auto bits = detail::testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
    R"(module detail_test(output logic [3:0] y);
  typedef struct packed {
    logic [1:0] bits;
  } packed_t;
  localparam packed_t P = '{bits: 2'b1x};
  assign y = {2{P}};
endmodule
)",
    4,
    false);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("x1x1", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveWildcardCaseItemPatternZeroRepeatNonIntegralOperandIgnored) {
  const auto bits = detail::testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
    R"(module detail_test(output logic y);
  localparam string P = "z";
  assign y = {1'b1, {0{P}}};
endmodule
)",
    1,
    false);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("1", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveWildcardCaseItemPatternEvaluatesLocalparamArrayElementSupported) {
  const auto bits = detail::testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef logic [2:0] pat_t;
  localparam pat_t PATTERNS [0:0] = '{3'b10x};
  localparam int IDX = 0;
  assign y = PATTERNS[IDX];
endmodule
)",
    3,
    false);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("x01", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveWildcardCaseItemPatternEnumTypedLocalparamSupported) {
  const auto bits = detail::testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef enum logic [2:0] { Value = 3'b10x } value_t;
  localparam value_t P = Value;
  assign y = P;
endmodule
)",
    3,
    false);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("x01", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveWildcardCaseItemPatternRealLocalparamConvertToIntSupported) {
  const auto bits = detail::testSVConstructorResolveWildcardCaseItemPatternFromAssignRhs(
    R"(module detail_test(output logic [3:0] y);
  localparam real P = 3.0;
  assign y = P;
endmodule
)",
    4,
    false);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("1100", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveUnknownLiteralBitsAsZeroPackedStructAndConcatZeroRepeatSupported) {
  const auto packedBits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef struct packed {
    logic [2:0] bits;
  } packed_t;
  localparam packed_t P = '{bits: 3'b10x};
  assign y = P;
endmodule
)",
    3);
  ASSERT_TRUE(packedBits.has_value());
  EXPECT_EQ("001", *packedBits);

  const auto concatBits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(output logic [1:0] y);
  typedef struct packed {
    logic [1:0] bits;
  } packed_t;
  localparam packed_t P = '{bits: 2'b1x};
  assign y = {P, {0{P}}};
endmodule
)",
    2);
  ASSERT_TRUE(concatBits.has_value());
  EXPECT_EQ("01", *concatBits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveUnknownLiteralBitsAsZeroConcatNamedZeroRepeatPackedStructSupported) {
  const auto bits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(output logic [1:0] y);
  localparam int ZERO = 0;
  typedef struct packed {
    logic [1:0] bits;
  } packed_t;
  localparam packed_t P = '{bits: 2'b1x};
  assign y = {P, {ZERO{P}}};
endmodule
)",
    2);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("01", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveUnknownLiteralBitsAsZeroSkipsZeroRepeatNonIntegralOperand) {
  const auto bits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(output logic y);
  localparam string P = "z";
  assign y = {1'bx, {0{P}}};
endmodule
)",
    1);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("0", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveUnknownLiteralBitsAsZeroConditionalShortcutUsesConstantConditionNet) {
  const auto constTrueBits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(input logic sel, output logic [1:0] y);
  assign y = (sel || 1'b1) ? 2'b1x : 2'b00;
endmodule
)",
    2);
  ASSERT_TRUE(constTrueBits.has_value());
  EXPECT_EQ("01", *constTrueBits);

  const auto constFalseBits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(input logic sel, output logic [1:0] y);
  assign y = (sel && 1'b0) ? 2'b00 : 2'bx1;
endmodule
)",
    2);
  ASSERT_TRUE(constFalseBits.has_value());
  EXPECT_EQ("10", *constFalseBits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveUnknownLiteralBitsAsZeroEnumTypedLocalparamSupported) {
  const auto bits = detail::testSVConstructorResolveUnknownLiteralBitsAsZeroFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef enum logic [2:0] { Value = 3'b10x } value_t;
  localparam value_t P = Value;
  assign y = P;
endmodule
)",
    3);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("001", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveExpressionBitsHandlesCompareAndPowerShortcuts) {
  const auto caseCompareBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(input logic [1:0] a, input logic [1:0] b, output logic y);
  assign y = (a === b);
endmodule
)",
    1);
  ASSERT_TRUE(caseCompareBits.has_value());
  EXPECT_EQ("?", *caseCompareBits);

  const auto relationalBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(input logic [1:0] a, input logic [1:0] b, output logic y);
  assign y = (a > b);
endmodule
)",
    1);
  ASSERT_TRUE(relationalBits.has_value());
  EXPECT_EQ("?", *relationalBits);

  const auto powerOneBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(input logic [1:0] sh, output logic [3:0] y);
  assign y = (1 ** sh);
endmodule
)",
    4);
  ASSERT_TRUE(powerOneBits.has_value());
  EXPECT_EQ("1000", *powerOneBits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveExpressionBitsHandlesInequalityAndFixedUnpackedSelections) {
  const auto inequalityBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(input logic [1:0] a, input logic [1:0] b, output logic y);
  assign y = (a !== b);
endmodule
)",
    1);
  ASSERT_TRUE(inequalityBits.has_value());
  EXPECT_EQ("?", *inequalityBits);

  const auto elementBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [1:0] y
);
  assign y = arr[2];
endmodule
)",
    2);
  ASSERT_TRUE(elementBits.has_value());
  EXPECT_EQ("??", *elementBits);

  const auto outOfRangeBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(
  input logic [1:0] arr [0:3],
  output logic [1:0] y
);
  assign y = arr[5];
endmodule
)",
    2);
  ASSERT_TRUE(outOfRangeBits.has_value());
  EXPECT_EQ("00", *outOfRangeBits);

  const auto nonConstantIndexBits = detail::testSVConstructorResolveExpressionBitsFromAssignRhs(
    R"(module detail_test(
  input logic [1:0] idx,
  input logic [1:0] arr [0:3],
  output logic [1:0] y
);
  assign y = arr[idx];
endmodule
)",
    2);
  ASSERT_TRUE(nonConstantIndexBits.has_value());
  EXPECT_EQ("??", *nonConstantIndexBits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveAssignmentLHSBitsHandlesFixedUnpackedSelections) {
  const auto elementWidth = detail::testSVConstructorResolveAssignmentLHSBitsFromAssignLhs(
    R"(module detail_test(
  input logic [1:0] d,
  output logic [1:0] arr [0:3]
);
  assign arr[2] = d;
endmodule
)");
  ASSERT_TRUE(elementWidth.has_value());
  EXPECT_EQ(2u, *elementWidth);

  const auto nonConstantIndex = detail::testSVConstructorResolveAssignmentLHSBitsFromAssignLhs(
    R"(module detail_test(
  input logic [1:0] idx,
  output logic [1:0] arr [0:3]
);
  assign arr[idx] = 2'b0;
endmodule
)");
  EXPECT_EQ(std::nullopt, nonConstantIndex);

  const auto outOfRangeIndexedDown =
    detail::testSVConstructorResolveAssignmentLHSBitsFromAssignLhs(
      R"(module detail_test(output logic [1:0] arr [0:3]);
  assign arr[0-:2] = 4'b0;
endmodule
)");
  EXPECT_EQ(std::nullopt, outOfRangeIndexedDown);

  const auto indexedUpRange =
    detail::testSVConstructorResolveAssignmentLHSBitsFromAssignLhs(
      R"(module detail_test(output logic [1:0] arr [0:3]);
  assign arr[1+:2] = 4'b0;
endmodule
)");
  EXPECT_EQ(std::nullopt, indexedUpRange);

  const auto dynamicIndexedUpRange =
    detail::testSVConstructorResolveAssignmentLHSBitsFromAssignLhs(
      R"(module detail_test(input logic [1:0] idx, output logic arr [0:3]);
  assign arr[idx+:1] = 1'b0;
endmodule
)");
  EXPECT_EQ(std::nullopt, dynamicIndexedUpRange);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveAssignmentLHSBitsReportsUnsupportedConcatAndIndexedSliceFailures) {
  const auto unsupportedConcat =
    detail::testSVConstructorResolveAssignmentLHSBitsResultFromAssignLhs(
      R"(module detail_test(output logic a, output logic b);
  assign {a, b} = 2'b0;
endmodule
)");
  ASSERT_TRUE(unsupportedConcat.has_value());
  EXPECT_FALSE(unsupportedConcat->success);
  EXPECT_NE(
    std::string::npos,
    unsupportedConcat->failureReason.find("unsupported always_comb assignment LHS"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveAssignmentLHSBitsReportedFailuresIncludeFormattedContext) {
  const auto concatFailure =
    detail::testSVConstructorResolveAssignmentLHSBitsReportedFailureFromAssignLhs(
      R"(module detail_test(output logic a, output logic b);
  assign {a, b} = 2'b0;
endmodule
)");
  ASSERT_TRUE(concatFailure.has_value());
  EXPECT_FALSE(concatFailure->success);
  EXPECT_FALSE(concatFailure->failureReason.empty());
  EXPECT_NE(std::string::npos, concatFailure->failureReason.find("Concatenation"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveConstantExpressionBitsPackedStructConvertToIntSupported) {
  const auto packedBits = detail::testSVConstructorResolveConstantExpressionBitsFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef struct packed {
    logic [2:0] bits;
  } packed_t;
  localparam packed_t P = '{bits: 3'b101};
  assign y = P;
endmodule
)",
    3);
  ASSERT_TRUE(packedBits.has_value());
  EXPECT_EQ("101", *packedBits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  convertConstantToIntegerFromAssignRhsRealLocalparamSupported) {
  const auto converted = detail::testSVConstructorConvertConstantToIntegerFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  localparam real P = 3.0;
  assign y = P;
endmodule
)");
  ASSERT_TRUE(converted.has_value());
  EXPECT_TRUE(*converted);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveConstantExpressionBitsEnumTypedLocalparamSupported) {
  const auto bits = detail::testSVConstructorResolveConstantExpressionBitsFromAssignRhs(
    R"(module detail_test(output logic [2:0] y);
  typedef enum logic [2:0] { Value = 3'b101 } value_t;
  localparam value_t P = Value;
  assign y = P;
endmodule
)",
    3);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("101", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  resolveConstantExpressionBitsSignedEnumTypedLocalparamSupported) {
  const auto bits = detail::testSVConstructorResolveConstantExpressionBitsFromAssignRhs(
    R"(module detail_test(output logic [3:0] y);
  typedef enum logic signed [3:0] { NegTwo = -2 } value_t;
  localparam value_t P = NegTwo;
  assign y = P;
endmodule
)",
    4);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("0111", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  appendSignedConstantBitsEncodesNegativeValuesToConstNets) {
  const auto bits = detail::testSVConstructorAppendSignedConstantBits(-2, 4);
  ASSERT_TRUE(bits.has_value());
  EXPECT_EQ("0111", *bits);
}

TEST_F(
  SNLSVConstructorTestSimple,
  findTimedStatementSkipsIgnorableListMembersAndReportsUnsupportedShapes) {
  const auto skippedIgnorable =
    detail::testSVConstructorFindTimedStatementFromProceduralBlock(
      R"(module detail_test(input logic clk, input logic d, output logic q);
  always begin
    @(posedge clk) q = d;
    $display("ignored");
  end
endmodule
)");
  ASSERT_TRUE(skippedIgnorable.has_value());
  EXPECT_TRUE(skippedIgnorable->foundTimed);
  EXPECT_FALSE(skippedIgnorable->reportedUnsupported);

  const auto unsupportedList =
    detail::testSVConstructorFindTimedStatementFromProceduralBlock(
      R"(module detail_test(input logic clk, input logic d, output logic q);
  always begin
    @(posedge clk) q = d;
    q = d;
  end
endmodule
)");
  ASSERT_TRUE(unsupportedList.has_value());
  EXPECT_FALSE(unsupportedList->foundTimed);
  EXPECT_TRUE(unsupportedList->reportedUnsupported);
  EXPECT_NE(
    std::string::npos,
    unsupportedList->unsupportedReason.find(
      "Unsupported statement list while extracting sequential timing control"));

  const auto unsupportedPlain =
    detail::testSVConstructorFindTimedStatementFromProceduralBlock(
      R"(module detail_test(input logic d, output logic q);
  always q = d;
endmodule
)");
  ASSERT_TRUE(unsupportedPlain.has_value());
  EXPECT_FALSE(unsupportedPlain->foundTimed);
  EXPECT_TRUE(unsupportedPlain->reportedUnsupported);
  EXPECT_NE(
    std::string::npos,
    unsupportedPlain->unsupportedReason.find(
      "Unsupported statement while extracting sequential timing control"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  collectDirectAssignmentsTraversesAndRejectsUnsupportedStatementKinds) {
  const auto traversed =
    detail::testSVConstructorCollectDirectAssignmentsFromProceduralBlock(
      R"(module detail_test(input logic d, output logic q);
  always begin
    ;
    $display("ignored");
    begin
      q = d;
    end
  end
endmodule
)");
  ASSERT_TRUE(traversed.has_value());
  EXPECT_TRUE(traversed->success);
  EXPECT_EQ(1u, traversed->assignmentCount);
  EXPECT_TRUE(traversed->failureReason.empty());

  const auto traversedNamedBlock =
    detail::testSVConstructorCollectDirectAssignmentsFromProceduralBlock(
      R"(module detail_test(input logic d, output logic q);
  always begin
    begin : named_blk
      q = d;
    end
  end
endmodule
)");
  ASSERT_TRUE(traversedNamedBlock.has_value());
  EXPECT_TRUE(traversedNamedBlock->success);
  EXPECT_EQ(1u, traversedNamedBlock->assignmentCount);
  EXPECT_TRUE(traversedNamedBlock->failureReason.empty());

  const auto rejected =
    detail::testSVConstructorCollectDirectAssignmentsFromProceduralBlock(
      R"(module detail_test(input logic en, input logic d, output logic q);
  always begin
    if (en) q = d;
  end
endmodule
)");
  ASSERT_TRUE(rejected.has_value());
  EXPECT_FALSE(rejected->success);
  EXPECT_EQ(0u, rejected->assignmentCount);
  EXPECT_NE(
    std::string::npos,
    rejected->failureReason.find("expected direct assignment statement in reset branch"));

  const auto rejectedNestedBlockInList =
    detail::testSVConstructorCollectDirectAssignmentsFromProceduralBlock(
      R"(module detail_test(input logic d, output logic q);
  always begin
    q = d;
    begin : bad_blk
      logic tmp;
    end
  end
endmodule
)");
  ASSERT_TRUE(rejectedNestedBlockInList.has_value());
  EXPECT_FALSE(rejectedNestedBlockInList->success);
  EXPECT_EQ(1u, rejectedNestedBlockInList->assignmentCount);
  EXPECT_NE(
    std::string::npos,
    rejectedNestedBlockInList->failureReason.find(
      "expected direct assignment statement in reset branch"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  getSingleLHSFallbackPathAssignmentMaxTraversesBlocksAndRejectsMultipleAssignments) {
  const auto traversed =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic clk, input logic d, output logic q);
  always_comb begin
    begin
      q = d;
    end
  end
endmodule
)");
  ASSERT_TRUE(traversed.has_value());
  EXPECT_TRUE(traversed->success);
  EXPECT_EQ(1u, traversed->maxAssignments);
  EXPECT_TRUE(traversed->failureReason.empty());

  const auto variableDeclarationPrefix =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic d, output logic q);
  always_comb begin
    logic tmp;
    q = d;
  end
endmodule
)");
  ASSERT_TRUE(variableDeclarationPrefix.has_value());
  EXPECT_TRUE(variableDeclarationPrefix->success);
  EXPECT_EQ(1u, variableDeclarationPrefix->maxAssignments);

  const auto rejected =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic clk, input logic d, input logic e, output logic q);
  always_comb begin
    begin
      q = d;
      q = e;
    end
  end
endmodule
)");
  ASSERT_TRUE(rejected.has_value());
  EXPECT_FALSE(rejected->success);
  EXPECT_EQ(0u, rejected->maxAssignments);
  EXPECT_NE(
    std::string::npos,
    rejected->failureReason.find("single-LHS fallback path contains multiple direct assignments"));

  const auto conditionalBlocks =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic en, input logic d, output logic q);
  always_comb begin
    if (en) begin
      q = d;
    end else begin
      q = d;
    end
  end
endmodule
)");
  ASSERT_TRUE(conditionalBlocks.has_value());
  EXPECT_TRUE(conditionalBlocks->success);
  EXPECT_EQ(1u, conditionalBlocks->maxAssignments);

  const auto rejectedListItem =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic d, output logic q);
  always_comb begin
    q = d;
    case (d)
      1'b1: q = d;
      default: q = d;
    endcase
  end
endmodule
)");
  ASSERT_TRUE(rejectedListItem.has_value());
  EXPECT_FALSE(rejectedListItem->success);
  EXPECT_EQ(0u, rejectedListItem->maxAssignments);
  EXPECT_FALSE(rejectedListItem->failureReason.empty());

  const auto rejectedConditionalTrue =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic en, input logic d, output logic q);
  always_comb begin
    if (en) begin
      case (d)
        1'b1: q = d;
        default: q = d;
      endcase
    end else begin
      q = d;
    end
  end
endmodule
)");
  ASSERT_TRUE(rejectedConditionalTrue.has_value());
  EXPECT_FALSE(rejectedConditionalTrue->success);
  EXPECT_EQ(0u, rejectedConditionalTrue->maxAssignments);

  const auto rejectedConditionalFalse =
    detail::testSVConstructorGetSingleLHSFallbackPathAssignmentMaxFromProceduralBlock(
      R"(module detail_test(input logic en, input logic d, output logic q);
  always_comb begin
    if (en) begin
      q = d;
    end else begin
      case (d)
        1'b1: q = d;
        default: q = d;
      endcase
    end
  end
endmodule
)");
  ASSERT_TRUE(rejectedConditionalFalse.has_value());
  EXPECT_FALSE(rejectedConditionalFalse->success);
  EXPECT_EQ(0u, rejectedConditionalFalse->maxAssignments);
}

TEST_F(
  SNLSVConstructorTestSimple,
  applyForLoopStepExpressionHandlesOperandConstantAndBinaryRhsForms) {
  const auto sameValue =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  int other;
  always_comb begin
    for (int i = 0; i < 4; i = i) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(sameValue.has_value());
  EXPECT_TRUE(sameValue->success);
  EXPECT_EQ(3, sameValue->loopValue);

  const auto constantValue =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  always_comb begin
    for (int i = 0; i < 4; i = 7) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(constantValue.has_value());
  EXPECT_TRUE(constantValue->success);
  EXPECT_EQ(7, constantValue->loopValue);

  const auto addRight =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  always_comb begin
    for (int i = 0; i < 4; i = 2 + i) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(addRight.has_value());
  EXPECT_TRUE(addRight->success);
  EXPECT_EQ(5, addRight->loopValue);

  const auto subtractRight =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  always_comb begin
    for (int i = 0; i < 4; i = 2 - i) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(subtractRight.has_value());
  EXPECT_TRUE(subtractRight->success);
  EXPECT_EQ(-1, subtractRight->loopValue);

  const auto unsupported =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  int other;
  always_comb begin
    for (int i = 0; i < 4; i = other) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(unsupported.has_value());
  EXPECT_FALSE(unsupported->success);
  EXPECT_EQ(3, unsupported->loopValue);
  EXPECT_NE(
    std::string::npos,
    unsupported->failureReason.find("unsupported for-loop step expression"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  applyForLoopStepExpressionHandlesCompoundAssignmentFastPaths) {
  const auto addConstant =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  always_comb begin
    for (int i = 0; i < 4; i += 2) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(addConstant.has_value());
  EXPECT_TRUE(addConstant->success);
  EXPECT_EQ(5, addConstant->loopValue);

  const auto multiplyConstant =
    detail::testSVConstructorApplyForLoopStepExpressionFromForLoop(
      R"(module detail_test;
  always_comb begin
    for (int i = 0; i < 4; i *= 2) begin end
  end
endmodule
)",
      3);
  ASSERT_TRUE(multiplyConstant.has_value());
  EXPECT_FALSE(multiplyConstant->success);
  EXPECT_EQ(3, multiplyConstant->loopValue);
  EXPECT_FALSE(multiplyConstant->failureReason.empty());

  const auto proceduralMultiply =
    detail::testSVConstructorApplyForLoopStepExpressionFromProceduralStatement(
      R"(module detail_test;
  int i;
  always_comb begin
    i *= 2;
  end
endmodule
)",
      3);
  ASSERT_TRUE(proceduralMultiply.has_value());
  EXPECT_FALSE(proceduralMultiply->success);
  EXPECT_EQ(3, proceduralMultiply->loopValue);
  EXPECT_FALSE(proceduralMultiply->failureReason.empty());
}

TEST_F(
  SNLSVConstructorTestSimple,
  applyConstantCompoundForLoopOperatorHandlesSupportedAndUnsupportedOps) {
  const auto add =
    detail::testSVConstructorApplyConstantCompoundForLoopOperator("+", 3, 2);
  ASSERT_TRUE(add.has_value());
  EXPECT_TRUE(add->success);
  EXPECT_EQ(5, add->loopValue);

  const auto subtract =
    detail::testSVConstructorApplyConstantCompoundForLoopOperator("-", 3, 2);
  ASSERT_TRUE(subtract.has_value());
  EXPECT_TRUE(subtract->success);
  EXPECT_EQ(1, subtract->loopValue);

  const auto multiply =
    detail::testSVConstructorApplyConstantCompoundForLoopOperator("*", 3, 2);
  ASSERT_TRUE(multiply.has_value());
  EXPECT_FALSE(multiply->success);
  EXPECT_EQ(3, multiply->loopValue);
  EXPECT_NE(
    std::string::npos,
    multiply->failureReason.find("unsupported compound for-loop assignment operator: *"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  sameExpressionStructureHandlesUnaryBinaryConcatAndConditionalForms) {
  const auto unarySame =
    detail::testSVConstructorSameExpressionStructureFromContinuousAssignRhsPair(
      R"(module detail_test(input logic sel, output logic a, output logic b);
  assign a = ~sel;
  assign b = ~sel;
endmodule
)");
  ASSERT_TRUE(unarySame.has_value());
  EXPECT_TRUE(*unarySame);

  const auto binarySame =
    detail::testSVConstructorSameExpressionStructureFromContinuousAssignRhsPair(
      R"(module detail_test(input logic [3:0] x, output logic [3:0] a, output logic [3:0] b);
  assign a = x + 4'd1;
  assign b = x + 4'd1;
endmodule
)");
  ASSERT_TRUE(binarySame.has_value());
  EXPECT_TRUE(*binarySame);

  const auto concatDifferent =
    detail::testSVConstructorSameExpressionStructureFromContinuousAssignRhsPair(
      R"(module detail_test(input logic x, input logic y, input logic z,
                           output logic [1:0] a, output logic [1:0] b);
  assign a = {x, y};
  assign b = {x, z};
endmodule
)");
  ASSERT_TRUE(concatDifferent.has_value());
  EXPECT_FALSE(*concatDifferent);

  const auto conditionalSame =
    detail::testSVConstructorSameExpressionStructureFromContinuousAssignRhsPair(
      R"(module detail_test(input logic sel, input logic x, input logic y,
                           output logic a, output logic b);
  assign a = sel ? x : y;
  assign b = sel ? x : y;
endmodule
)");
  ASSERT_TRUE(conditionalSame.has_value());
  EXPECT_TRUE(*conditionalSame);
}

TEST_F(SNLSVConstructorTestSimple, encodeSignedInt64ConstantBitsSignExtendsBeyond64Bits) {
  const auto encodedBits = detail::testSVConstructorEncodeSignedInt64ConstantBits(-2, 70);
  ASSERT_EQ(70u, encodedBits.size());
  EXPECT_FALSE(encodedBits[0]);
  for (size_t i = 1; i < encodedBits.size(); ++i) {
    EXPECT_TRUE(encodedBits[i]) << i;
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  formatAssignmentLHSResolutionFailureReasonPrefersDetailedContext) {
  EXPECT_EQ(
    "unable to resolve assignment LHS net for 'Concatenation'",
    detail::testSVConstructorFormatAssignmentLHSResolutionFailureReason(
      false,
      "Concatenation",
      ""));
  EXPECT_EQ(
    "unable to resolve assignment LHS bits for 'RangeSelect'",
    detail::testSVConstructorFormatAssignmentLHSResolutionFailureReason(
      true,
      "RangeSelect",
      ""));
  EXPECT_EQ(
    "custom failure",
    detail::testSVConstructorFormatAssignmentLHSResolutionFailureReason(
      true,
      "RangeSelect",
      "custom failure"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  formatSequentialConcatLeafFailureReasonAppendsNestedFailureWhenPresent) {
  EXPECT_EQ(
    "unable to resolve sequential concatenation assignment leaf 'MemberAccess'",
    detail::testSVConstructorFormatSequentialConcatLeafFailureReason(
      "MemberAccess",
      ""));
  EXPECT_EQ(
    "unable to resolve sequential concatenation assignment leaf 'MemberAccess' "
    "(leaf failure)",
    detail::testSVConstructorFormatSequentialConcatLeafFailureReason(
      "MemberAccess",
      "leaf failure"));
}

TEST_F(SNLSVConstructorTestSimple, formatDescribedFailureConcatenatesPrefixAndDescription) {
  EXPECT_EQ(
    "unable to resolve always_comb case item match for Concatenation width=5",
    detail::testSVConstructorFormatDescribedFailure(
      "unable to resolve always_comb case item match for ",
      "Concatenation width=5"));
}

TEST_F(SNLSVConstructorTestSimple, formatQuotedDescriptionFailureWrapsDescriptionInQuotes) {
  EXPECT_EQ(
    "unable to build reset mux for 'RangeSelect sel=Simple'",
    detail::testSVConstructorFormatQuotedDescriptionFailure(
      "unable to build reset mux for ",
      "RangeSelect sel=Simple"));
}

TEST_F(SNLSVConstructorTestSimple, parseBinaryOperatorsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "binary_ops_supported" / "binary_ops_supported.sv");

  auto top = library_->getSNLDesign(NLName("binary_ops_supported_top"));
  ASSERT_NE(top, nullptr);
  const std::array<const char*, 6> supportedBinaryOpOutputs{
    "y_and",
    "y_land",
    "y_or",
    "y_lor",
    "y_xor",
    "y_xnor"};
  for (const auto* output : supportedBinaryOpOutputs) {
    auto net = top->getNet(NLName(output));
    ASSERT_NE(net, nullptr);
    auto bitNet = dynamic_cast<SNLBitNet*>(net);
    ASSERT_NE(bitNet, nullptr);
    EXPECT_FALSE(bitNet->getInstTerms().empty());
  }
  auto yShr = top->getBusNet(NLName("y_shr"));
  ASSERT_NE(yShr, nullptr);
  EXPECT_EQ(4, yShr->getWidth());

  size_t andGateCount = 0;
  size_t orGateCount = 0;
  size_t xorGateCount = 0;
  size_t xnorGateCount = 0;
  size_t otherGateCount = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "or") {
      ++orGateCount;
    } else if (gateName == "xor") {
      ++xorGateCount;
    } else if (gateName == "xnor") {
      ++xnorGateCount;
    } else {
      ++otherGateCount;
    }
  }

  EXPECT_EQ(2u, andGateCount);
  EXPECT_EQ(2u, orGateCount);
  EXPECT_EQ(1u, xorGateCount);
  EXPECT_EQ(1u, xnorGateCount);
  EXPECT_EQ(0u, otherGateCount);
  EXPECT_EQ(10u, assignCount);
  EXPECT_EQ(16u, top->getInstances().size());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "binary_ops_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAddSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "continuous_add_supported" / "continuous_add_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_add_supported_top"));
  ASSERT_NE(top, nullptr);

  auto ySum = top->getBusNet(NLName("y_sum"));
  ASSERT_NE(ySum, nullptr);
  EXPECT_EQ(4, ySum->getWidth());

  auto yInc = top->getBusNet(NLName("y_inc"));
  ASSERT_NE(yInc, nullptr);
  EXPECT_EQ(4, yInc->getWidth());

  auto yCount = top->getBusNet(NLName("y_count"));
  ASSERT_NE(yCount, nullptr);
  EXPECT_EQ(2, yCount->getWidth());

  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(10u, faCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_add_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_mul_supported" / "continuous_mul_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_mul_supported_top"));
  ASSERT_NE(top, nullptr);

  auto yMul = top->getBusNet(NLName("y_mul"));
  ASSERT_NE(yMul, nullptr);
  EXPECT_EQ(8, yMul->getWidth());

  auto ySignedMul = top->getBusNet(NLName("y_signed_mul"));
  ASSERT_NE(ySignedMul, nullptr);
  EXPECT_EQ(8, ySignedMul->getWidth());

  size_t faCount = 0;
  size_t andGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    }
  }
  EXPECT_GT(faCount, 0u);
  EXPECT_GT(andGateCount, 0u);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_mul_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyZeroSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_mul_zero_supported" / "continuous_mul_zero_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_mul_zero_supported_top"));
  ASSERT_NE(top, nullptr);

  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());

  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(4u, assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_mul_zero_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyRightConstOneBitShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_mul_right_const_one_bit_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_mul_right_const_one_bit_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_mul_right_const_one_bit_shortcut_supported_top(
  input logic [3:0] a,
  input logic [1:0] s,
  output logic [3:0] y
);
  // Exercises rowEnableBit == const1 shortcut in createMultiplyAssign.
  assign y = a * $signed({1'b1, s});
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_mul_right_const_one_bit_shortcut_supported_top"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyLeftConstOneBitShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_mul_left_const_one_bit_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_mul_left_const_one_bit_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_mul_left_const_one_bit_shortcut_supported_top(
  input logic [1:0] s,
  input logic [3:0] b,
  output logic [3:0] y
);
  // Exercises multiplicandBit == const1 shortcut in createMultiplyAssign.
  assign y = $signed({1'b1, s}) * b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_mul_left_const_one_bit_shortcut_supported_top"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyZeroPartialRowShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_mul_zero_partial_row_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_mul_zero_partial_row_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_mul_zero_partial_row_shortcut_supported_top(
  output logic [3:0] y
);
  // Exercises the rowHasSignal == false shortcut in createMultiplyAssign.
  assign y = 4'b1000 * 4'b1111;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_mul_zero_partial_row_shortcut_supported_top"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyLeftConstZeroFactorShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_mul_left_const_zero_factor_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_mul_left_const_zero_factor_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_mul_left_const_zero_factor_shortcut_supported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  // Exercises leftIsConst + factor == 0 scaled-multiply shortcut.
  always_comb begin
    y = 4'b0000 * a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_mul_left_const_zero_factor_shortcut_supported_top"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyLeftConstOneFactorShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_mul_left_const_one_factor_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_mul_left_const_one_factor_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_mul_left_const_one_factor_shortcut_supported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  // Exercises leftIsConst + factor == 1 scaled-multiply shortcut.
  always_comb begin
    y = 4'b0001 * a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_mul_left_const_one_factor_shortcut_supported_top"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombMultiplyEvaluatedConstOperandsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_mul_evaluated_const_operands_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_mul_evaluated_const_operands_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_mul_evaluated_const_operands_supported(
  output logic [7:0] y
);
  localparam logic [3:0] A = 4'd3;
  localparam logic [3:0] B = 4'd5;
  always_comb begin
    y = A * B;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_mul_evaluated_const_operands_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(8, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombMultiplyLiteralOperandsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_mul_literal_operands_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_mul_literal_operands_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_mul_literal_operands_supported(
  output logic [7:0] y
);
  always_comb begin
    y = 4'd3 * 4'd5;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_mul_literal_operands_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(8, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombNestedMultiplyLiteralOperandsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_nested_mul_literal_operands_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_nested_mul_literal_operands_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_nested_mul_literal_operands_supported(
  output logic [7:0] y
);
  always_comb begin
    y = (4'd3 * 4'd5) + 8'd1;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_nested_mul_literal_operands_supported"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(8, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousMultiplyUnknownOperandUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_mul_unknown_operand_unsupported" /
      "continuous_mul_unknown_operand_unsupported.sv");
    FAIL() << "Expected unsupported multiply operand expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: *"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousSubSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "continuous_sub_supported" / "continuous_sub_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_sub_supported_top"));
  ASSERT_NE(top, nullptr);

  auto ySub = top->getBusNet(NLName("y_sub"));
  ASSERT_NE(ySub, nullptr);
  EXPECT_EQ(4, ySub->getWidth());

  auto yDec = top->getBusNet(NLName("y_dec"));
  ASSERT_NE(yDec, nullptr);
  EXPECT_EQ(4, yDec->getWidth());

  auto ySmall = top->getBusNet(NLName("y_small"));
  ASSERT_NE(ySmall, nullptr);
  EXPECT_EQ(2, ySmall->getWidth());

  auto yConcatSub = top->getBusNet(NLName("y_concat_sub"));
  ASSERT_NE(yConcatSub, nullptr);
  EXPECT_EQ(6, yConcatSub->getWidth());

  size_t faCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(16u, faCount);
  EXPECT_EQ(16u, notGateCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_sub_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousSubUnknownOperandUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_sub_unknown_operand_unsupported" /
      "continuous_sub_unknown_operand_unsupported.sv");
    FAIL() << "Expected unsupported subtraction operand expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: -"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousSubBitSliceLHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_sub_bitslice_lhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_sub_bitslice_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_sub_bitslice_lhs_supported(
  input  logic [12:0] a_i,
  input  logic [12:0] b_i,
  output logic [12:0] y_o
);
  assign y_o[12:0] = a_i[12:0] - b_i[12:0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_sub_bitslice_lhs_supported"));
  ASSERT_NE(top, nullptr);

  auto y = top->getBusNet(NLName("y_o"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(13, y->getWidth());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_sub_bitslice_lhs_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftLeftSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_shift_left_supported" / "continuous_shift_left_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_shift_left_supported_top"));
  ASSERT_NE(top, nullptr);

  auto yConst = top->getBusNet(NLName("y_const"));
  ASSERT_NE(yConst, nullptr);
  EXPECT_EQ(8, yConst->getWidth());

  auto yVar = top->getBusNet(NLName("y_var"));
  ASSERT_NE(yVar, nullptr);
  EXPECT_EQ(8, yVar->getWidth());

  auto yLogical = top->getBusNet(NLName("y_logical"));
  ASSERT_NE(yLogical, nullptr);
  EXPECT_EQ(8, yLogical->getWidth());

  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(mux2Model, nullptr);

  size_t mux2Count = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == mux2Model) {
      ++mux2Count;
    } else if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(48u, mux2Count);
  EXPECT_EQ(8u, assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_shift_left_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftRightDynamicElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_shift_right_unresolved_skipped" /
      "continuous_shift_right_unresolved_skipped.sv");

  auto top = library_->getSNLDesign(NLName("continuous_shift_right_unresolved_skipped_top"));
  ASSERT_NE(top, nullptr);
  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(4, y->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftRightSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_shift_right_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_shift_right_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_shift_right_supported(
  input logic [31:0] data,
  input logic [1:0] shamt,
  output logic [31:0] y_const,
  output logic [31:0] y_var
);
  assign y_const = data >> 3;
  assign y_var = data >> {shamt, 2'b0};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_shift_right_supported"));
  ASSERT_NE(top, nullptr);

  auto yConst = top->getBusNet(NLName("y_const"));
  ASSERT_NE(yConst, nullptr);
  EXPECT_EQ(32, yConst->getWidth());

  auto yVar = top->getBusNet(NLName("y_var"));
  ASSERT_NE(yVar, nullptr);
  EXPECT_EQ(32, yVar->getWidth());

  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(mux2Model, nullptr);

  size_t mux2Count = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == mux2Model) {
      ++mux2Count;
    } else if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(128u, mux2Count);
  EXPECT_EQ(32u, assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_shift_right_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftRightCastWrappedArithmeticSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_shift_right_cast_wrapped_arithmetic_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_shift_right_cast_wrapped_arithmetic_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_shift_right_cast_wrapped_arithmetic_supported(
  input logic sign_i,
  input logic [7:0] data_i,
  input logic [2:0] shamt_i,
  output logic [8:0] y_var,
  output logic [8:0] y_const
);
  logic [8:0] data_ext;
  assign data_ext = {sign_i, data_i};
  assign y_var = $unsigned($signed(data_ext) >>> shamt_i);
  assign y_const = $unsigned($signed(data_ext) >>> 3'd2);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_shift_right_cast_wrapped_arithmetic_supported"));
  ASSERT_NE(top, nullptr);

  auto yVar = top->getBusNet(NLName("y_var"));
  ASSERT_NE(yVar, nullptr);
  EXPECT_EQ(9, yVar->getWidth());

  auto yConst = top->getBusNet(NLName("y_const"));
  ASSERT_NE(yConst, nullptr);
  EXPECT_EQ(9, yConst->getWidth());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top,
    "continuous_shift_right_cast_wrapped_arithmetic_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousShiftRightConcatZeroWidthOperandSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_shift_right_concat_zero_width_operand_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_shift_right_concat_zero_width_operand_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_shift_right_concat_zero_width_operand_supported(
  input logic [31:0] data,
  output logic [31:0] y
);
  logic [$clog2(1)-1:0] shamt;
  assign y = data >> {shamt, 4'b0};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_shift_right_concat_zero_width_operand_supported"));
  ASSERT_NE(top, nullptr);

  auto y = top->getBusNet(NLName("y"));
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(32, y->getWidth());

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "continuous_shift_right_concat_zero_width_operand_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftRightMemberConcatSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_shift_right_member_concat_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_shift_right_member_concat_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic [31:0] data;
  logic        valid;
} icache_req_t;

module continuous_shift_right_member_concat_supported(
  input  icache_req_t  icache_dreq_i,
  input  logic [1:0]   shamt,
  output logic [31:0]  icache_data
);
  assign icache_data = icache_dreq_i.data >> {shamt, 2'b0};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_shift_right_member_concat_supported"));
  ASSERT_NE(top, nullptr);

  auto icacheData = top->getBusNet(NLName("icache_data"));
  ASSERT_NE(icacheData, nullptr);
  EXPECT_EQ(32, icacheData->getWidth());

  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(mux2Model, nullptr);
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(128u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousLHSElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_lhs_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_lhs_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_lhs_element_select_supported(
  input  logic [3:0] in0,
  input  logic [3:0] in1,
  output logic [1:0][3:0] out_bus
);
  localparam int SLOT0 = 0;
  localparam int SLOT1 = 1;

  assign out_bus[SLOT0] = in0;
  assign out_bus[SLOT1] = in1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_lhs_element_select_supported"));
  ASSERT_NE(top, nullptr);

  auto outBus = top->getBusNet(NLName("out_bus"));
  ASSERT_NE(outBus, nullptr);
  EXPECT_EQ(8, outBus->getWidth());

  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_EQ(8u, assignCount);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousConditionalMemberSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_conditional_member_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_conditional_member_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic [7:0] data;
  logic data_req;
} dcache_req_o_t;

module continuous_conditional_member_select_supported(
  input  dcache_req_o_t [0:3] req_ports_ex,
  input  dcache_req_o_t       req_ports_acc,
  output dcache_req_o_t       req_out
);
  assign req_out = req_ports_ex[2].data_req ? req_ports_ex[2] : req_ports_acc;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_conditional_member_select_supported"));
  ASSERT_NE(top, nullptr);

  auto reqOut = top->getBusNet(NLName("req_out"));
  ASSERT_NE(reqOut, nullptr);
  EXPECT_EQ(9, reqOut->getWidth());

  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 9));
  EXPECT_EQ(0u, countMux2Instances(top, 1));

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "continuous_conditional_member_select_supported");
  const auto dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mux2 #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(9)"));
  EXPECT_EQ(std::string::npos, dumpedText.find("naja_mux2__w9"));
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseVectorUsesWideMuxPrimitive) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_vector_wide_mux_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_vector_wide_mux_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_vector_wide_mux_supported(
  input  logic       sel,
  input  logic [7:0] a,
  input  logic [7:0] b,
  output logic [7:0] y
);
  always_comb begin
    case (sel)
      1'b0: y = a;
      default: y = b;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_vector_wide_mux_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 8));
  EXPECT_EQ(0u, countMux2Instances(top, 1));

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "always_comb_case_vector_wide_mux_supported");
  const auto dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mux2 #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(8)"));
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombDynamicPackedReadWriteUsesWideMuxPrimitive) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_dynamic_packed_read_write_wide_mux_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_dynamic_packed_read_write_wide_mux_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_dynamic_packed_read_write_wide_mux_supported(
  input  logic       idx,
  input  logic [7:0] data_i,
  input  logic [1:0][7:0] state_i,
  output logic [7:0] selected_o,
  output logic [1:0][7:0] updated_o
);
  always_comb begin
    updated_o = state_i;
    updated_o[idx] = data_i;
    selected_o = state_i[idx];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_dynamic_packed_read_write_wide_mux_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(4u, countMux2Instances(top));
  EXPECT_EQ(4u, countMux2Instances(top, 8));
  EXPECT_EQ(0u, countMux2Instances(top, 1));

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "always_comb_dynamic_packed_read_write_wide_mux_supported");
  const auto dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mux2 #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(8)"));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftLeftUnknownAmountUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_shift_left_unknown_amount_unsupported" /
      "continuous_shift_left_unknown_amount_unsupported.sv");
    FAIL() << "Expected unsupported unknown left-shift amount expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: <<<"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftRightUnknownAmountUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_shift_right_unknown_amount_unsupported" /
      "continuous_shift_right_unknown_amount_unsupported.sv");
    FAIL() << "Expected unsupported unknown right-shift amount expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: >>>"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousShiftRightUnknownValueUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_shift_right_unknown_value_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_shift_right_unknown_value_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_shift_right_unknown_value_unsupported(
  input logic [3:0] a,
  output logic [7:0] y
);
  localparam logic [3:0] BAD_CONST = 4'bx001;
  assign y = $signed({a, BAD_CONST}) >>> 1'd1;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported arithmetic right-shift value expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: >>>"));
    EXPECT_NE(std::string::npos, reason.find("failed to resolve value bits"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousLogicalShiftRightUnknownValueUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_logical_shift_right_unknown_value_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_logical_shift_right_unknown_value_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_logical_shift_right_unknown_value_unsupported(
  input logic [3:0] a,
  output logic [7:0] y
);
  localparam logic [3:0] BAD_CONST = 4'bx001;
  assign y = {a, BAD_CONST} >> 1'd1;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported logical right-shift value expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: >>"));
    EXPECT_NE(std::string::npos, reason.find("failed to resolve value bits"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousLogicalShiftRightUnknownAmountUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_logical_shift_right_unknown_amount_unsupported" /
      "continuous_logical_shift_right_unknown_amount_unsupported.sv");
    FAIL() << "Expected unsupported unknown logical right-shift amount expression";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: >>"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_eq_supported" / "continuous_eq_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_eq_supported_top"));
  ASSERT_NE(top, nullptr);

  const std::array<const char*, 4> outputs{
    "y_eq1",
    "y_eq4",
    "y_eq2",
    "y_eq_enum"};
  for (const auto* output : outputs) {
    auto net = top->getNet(NLName(output));
    ASSERT_NE(net, nullptr);
    auto bitNet = dynamic_cast<SNLBitNet*>(net);
    ASSERT_NE(bitNet, nullptr);
    EXPECT_FALSE(bitNet->getInstTerms().empty());
  }

  size_t andGateCount = 0;
  size_t xnorGateCount = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "xnor") {
      ++xnorGateCount;
    }
  }

  EXPECT_EQ(3u, andGateCount);
  EXPECT_EQ(9u, xnorGateCount);
  EXPECT_EQ(1u, assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_eq_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousInequalitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_ne_supported" / "continuous_ne_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_ne_supported_top"));
  ASSERT_NE(top, nullptr);

  const std::array<const char*, 4> outputs{
    "y_ne1",
    "y_ne4",
    "y_ne2",
    "y_ne_enum"};
  for (const auto* output : outputs) {
    auto net = top->getNet(NLName(output));
    ASSERT_NE(net, nullptr);
    auto bitNet = dynamic_cast<SNLBitNet*>(net);
    ASSERT_NE(bitNet, nullptr);
    EXPECT_FALSE(bitNet->getInstTerms().empty());
  }

  size_t andGateCount = 0;
  size_t xnorGateCount = 0;
  size_t notGateCount = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "xnor") {
      ++xnorGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }

  EXPECT_EQ(3u, andGateCount);
  EXPECT_EQ(9u, xnorGateCount);
  EXPECT_EQ(4u, notGateCount);
  EXPECT_EQ(1u, assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_ne_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousEqNeFailurePathsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "continuous_eq_ne_failure_paths_unsupported" /
      "continuous_eq_ne_failure_paths_unsupported.sv");
    FAIL() << "Expected unsupported equality/inequality failure paths";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: =="));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: !="));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousEqualityMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_eq_member_access_supported" /
    "continuous_eq_member_access_supported.sv");

  auto top = library_->getSNLDesign(NLName("continuous_eq_member_access_supported_top"));
  ASSERT_NE(top, nullptr);

  auto net = top->getNet(NLName("cvxif_req_allowed"));
  ASSERT_NE(net, nullptr);
  auto bitNet = dynamic_cast<SNLBitNet*>(net);
  ASSERT_NE(bitNet, nullptr);
  EXPECT_FALSE(bitNet->getInstTerms().empty());

  size_t andGateCount = 0;
  size_t xnorGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "xnor") {
      ++xnorGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
  EXPECT_EQ(2u, xnorGateCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "continuous_eq_member_access_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseCompatibleNetScalarReuse) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "compatible_net_scalar_reuse" / "compatible_net_scalar_reuse.sv");

  auto top = library_->getSNLDesign(NLName("compatible_net_scalar_reuse_top"));
  ASSERT_NE(top, nullptr);

  auto andY = top->getNet(NLName("and_y"));
  ASSERT_NE(andY, nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarNet*>(andY), nullptr);
  EXPECT_EQ(top->getNet(NLName("and_y_0")), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "compatible_net_scalar_reuse");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseCompatibleNetScalarMismatch) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "compatible_net_scalar_mismatch" / "compatible_net_scalar_mismatch.sv");

  auto top = library_->getSNLDesign(NLName("compatible_net_scalar_mismatch_top"));
  ASSERT_NE(top, nullptr);

  auto andY = top->getNet(NLName("and_y"));
  ASSERT_NE(andY, nullptr);
  EXPECT_NE(dynamic_cast<SNLBusNet*>(andY), nullptr);
  auto andYSuffixed = top->getNet(NLName("and_y_0"));
  ASSERT_NE(andYSuffixed, nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarNet*>(andYSuffixed), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "compatible_net_scalar_mismatch");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseCompatibleNetBusReuse) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "compatible_net_bus_reuse" / "compatible_net_bus_reuse.sv");

  auto top = library_->getSNLDesign(NLName("compatible_net_bus_reuse_top"));
  ASSERT_NE(top, nullptr);

  auto incOut = top->getNet(NLName("inc_out"));
  ASSERT_NE(incOut, nullptr);
  auto incOutBus = dynamic_cast<SNLBusNet*>(incOut);
  ASSERT_NE(incOutBus, nullptr);
  EXPECT_EQ(4, incOutBus->getWidth());
  EXPECT_EQ(top->getNet(NLName("inc_out_0")), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "compatible_net_bus_reuse");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseCompatibleNetBusMismatch) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "compatible_net_bus_mismatch" / "compatible_net_bus_mismatch.sv");

  auto top = library_->getSNLDesign(NLName("compatible_net_bus_mismatch_top"));
  ASSERT_NE(top, nullptr);

  auto incOut = top->getNet(NLName("inc_out"));
  ASSERT_NE(incOut, nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarNet*>(incOut), nullptr);
  auto incOutSuffixed = top->getNet(NLName("inc_out_0"));
  ASSERT_NE(incOutSuffixed, nullptr);
  EXPECT_NE(dynamic_cast<SNLBusNet*>(incOutSuffixed), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "compatible_net_bus_mismatch");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseCompatibleNetNullLikeFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "compatible_net_null_like" / "compatible_net_null_like.sv");

  auto top = library_->getSNLDesign(NLName("compatible_net_null_like_top"));
  ASSERT_NE(top, nullptr);

  auto notY = top->getNet(NLName("not_y"));
  ASSERT_NE(notY, nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarNet*>(notY), nullptr);
  auto notYSuffixed = top->getNet(NLName("not_y_0"));
  ASSERT_NE(notYSuffixed, nullptr);
  EXPECT_NE(dynamic_cast<SNLScalarNet*>(notYSuffixed), nullptr);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "compatible_net_null_like");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseGateOnBusLHSIsSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "gate_lhs_bus_skip" / "gate_lhs_bus_skip.sv");

  auto top = library_->getSNLDesign(NLName("gate_lhs_bus_skip_top"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isGate(inst->getModel()) &&
        NLDB0::getGateName(inst->getModel()) == "and") {
      ++andGateCount;
    }
  }
  EXPECT_EQ(2u, andGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseGateOnBusLHSUnaryOperandSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_lhs_bus_unary_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_lhs_bus_unary_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_lhs_bus_unary_supported(
  input logic [3:0] a,
  input logic [3:0] b,
  output logic [3:0] y
);
  assign y = a & ~b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("gate_lhs_bus_unary_supported"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(4u, andGateCount);
  EXPECT_EQ(4u, notGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseGateOnBusLHSUnaryOperandConstShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_lhs_bus_unary_const_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_lhs_bus_unary_const_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_lhs_bus_unary_const_shortcuts_supported(
  input logic [3:0] a,
  input logic [1:0] b,
  output logic [3:0] y
);
  assign y = a & ~{b[1], 1'b0, 1'b1, b[0]};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("gate_lhs_bus_unary_const_shortcuts_supported"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(4u, andGateCount);
  EXPECT_EQ(2u, notGateCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousUnaryPlusMixedConstAndVariableBitsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_unary_plus_mixed_const_and_variable_bits_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_unary_plus_mixed_const_and_variable_bits_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_unary_plus_mixed_const_and_variable_bits_supported(
  input  logic a,
  output logic [2:0] y
);
  assign y = {+{a, 1'b0, 1'b1}};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_unary_plus_mixed_const_and_variable_bits_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousBitwiseNotMixedConstAndVariableBitsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_bitwise_not_mixed_const_and_variable_bits_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_bitwise_not_mixed_const_and_variable_bits_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_bitwise_not_mixed_const_and_variable_bits_supported(
  input  logic a,
  output logic [2:0] y
);
  assign y = {~{a, 1'b0, 1'b1}};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_bitwise_not_mixed_const_and_variable_bits_supported"));
  ASSERT_NE(top, nullptr);

  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());

  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(1u, notGateCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousUnaryMinusMixedConstAndVariableBitsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_unary_minus_mixed_const_and_variable_bits_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_unary_minus_mixed_const_and_variable_bits_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_unary_minus_mixed_const_and_variable_bits_supported(
  input  logic a,
  output logic [2:0] y
);
  assign y = {-{a, 1'b0, 1'b1}};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_unary_minus_mixed_const_and_variable_bits_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseGateOperandLiteralSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "gate_operand_unresolved_skip" / "gate_operand_unresolved_skip.sv");

  auto top = library_->getSNLDesign(NLName("gate_operand_unresolved_skip_top"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isGate(inst->getModel()) &&
        NLDB0::getGateName(inst->getModel()) == "and") {
      ++andGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseGateMixedBinaryTreeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "gate_mixed_binary_tree_skip" / "gate_mixed_binary_tree_skip.sv");

  auto top = library_->getSNLDesign(NLName("gate_mixed_binary_tree_skip"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "or") {
      ++orGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
  EXPECT_EQ(1u, orGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseGateLogicalNotOperandTreeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_logical_not_operand_tree_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_logical_not_operand_tree_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_logical_not_operand_tree_supported(
  input logic a,
  input logic b,
  input logic c,
  input logic d,
  output logic y
);
  assign y = (a && !b || c) && !d;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("gate_logical_not_operand_tree_supported"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  size_t orGateCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "or") {
      ++orGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(2u, andGateCount);
  EXPECT_EQ(1u, orGateCount);
  EXPECT_EQ(2u, notGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseGateLogicalShortcutBranchesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_logical_shortcut_branches_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_logical_shortcut_branches_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_logical_shortcut_branches_supported(
  input logic a,
  input logic b,
  output logic y_lhs_zero_and,
  output logic y_lhs_one_or,
  output logic y_lhs_one_and_rhs,
  output logic y_same_and,
  output logic y_rhs_one_or,
  output logic y_lhs_zero_or_rhs,
  output logic y_same_or
);
  always_comb begin
    y_lhs_zero_and    = (a & 1'b0) && b;
    y_lhs_one_or      = (a | 1'b1) || b;
    y_lhs_one_and_rhs = (a | 1'b1) && b;
    y_same_and        = a && (b | 1'b1);
    y_rhs_one_or      = a || (b | 1'b1);
    y_lhs_zero_or_rhs = (a & 1'b0) || b;
    y_same_or         = a || (b & 1'b0);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("gate_logical_shortcut_branches_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_lhs_zero_and")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_lhs_one_or")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_lhs_one_and_rhs")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_same_and")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_rhs_one_or")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_lhs_zero_or_rhs")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_same_or")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseGateReductionOrOperandTreeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_reduction_or_operand_tree_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_reduction_or_operand_tree_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_reduction_or_operand_tree_supported(
  input logic a,
  input logic b,
  input logic [2:0] is_branch,
  input logic [1:0] is_return,
  output logic y
);
  assign y = (a || |is_branch || |is_return) && !b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("gate_reduction_or_operand_tree_supported"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  size_t orGateCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "or") {
      ++orGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
  EXPECT_GE(orGateCount, 3u);
  EXPECT_EQ(1u, notGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousReductionOrRHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_reduction_or_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_reduction_or_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_reduction_or_rhs_supported(
  input logic [3:0] in,
  output logic y
);
  assign y = |in;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_reduction_or_rhs_supported"));
  ASSERT_NE(top, nullptr);

  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousReductionOrAndConstantShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_reduction_or_and_constant_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_reduction_or_and_constant_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_reduction_or_and_constant_shortcuts_supported(
  input  logic a,
  output logic y_or_skip_zero,
  output logic y_or_break_one,
  output logic y_and_skip_one,
  output logic y_and_break_zero
);
  assign y_or_skip_zero  = |{a, 1'b0};
  assign y_or_break_one  = |{a, 1'b1};
  assign y_and_skip_one  = &{a, 1'b1};
  assign y_and_break_zero = &{a, 1'b0};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_reduction_or_and_constant_shortcuts_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_or_skip_zero")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_or_break_one")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_and_skip_one")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_and_break_zero")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousReductionOperandResolveBitsFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_reduction_operand_resolve_bits_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_reduction_operand_resolve_bits_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_reduction_operand_resolve_bits_failure_unsupported(
  input  logic [3:0] a,
  output logic       y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = |bad_fn(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_reduction_operand_resolve_bits_failure_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousLogicalNotResolveBitsFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_logical_not_resolve_bits_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_logical_not_resolve_bits_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_logical_not_resolve_bits_failure_unsupported(
  input  logic [3:0] a,
  output logic       y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  assign y = !bad_fn(a);
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign in module "
     "'continuous_logical_not_resolve_bits_failure_unsupported'"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousReductionXorXnorMixedRHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_reduction_xor_xnor_mixed_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_reduction_xor_xnor_mixed_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_reduction_xor_xnor_mixed_rhs_supported(
  input  logic a,
  input  logic b,
  input  logic c,
  output logic y_xor_chain,
  output logic y_xor_flip_left,
  output logic y_xor_flip_right,
  output logic y_xor_skip_zero,
  output logic y_xor_double_one_a,
  output logic y_xor_double_one_b,
  output logic y_xnor_chain
);
  assign y_xor_chain        = ^{a, b, c};
  assign y_xor_flip_left    = ^{1'b1, a};
  assign y_xor_flip_right   = ^{a, 1'b1};
  assign y_xor_skip_zero    = ^{a, 1'b0};
  assign y_xor_double_one_a = ^{a, 1'b1, 1'b1};
  assign y_xor_double_one_b = ^{1'b1, 1'b1, a};
  assign y_xnor_chain       = ~^{a, b};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_reduction_xor_xnor_mixed_rhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_xor_chain")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_xor_flip_left")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_xor_flip_right")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_xor_skip_zero")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_xor_double_one_a")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_xor_double_one_b")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_xnor_chain")), nullptr);

  size_t xorGateCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "xor") {
      ++xorGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_GE(xorGateCount, 1u);
  EXPECT_GE(notGateCount, 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousReductionNorNandConstantFlipShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_reduction_nor_nand_constant_flip_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_reduction_nor_nand_constant_flip_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_reduction_nor_nand_constant_flip_shortcuts_supported(
  input  logic a,
  output logic y_nor_const0,
  output logic y_nand_const1
);
  assign y_nor_const0  = ~|{a, 1'b1};
  assign y_nand_const1 = ~&{a, 1'b0};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_reduction_nor_nand_constant_flip_shortcuts_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_nor_const0")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_nand_const1")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousLogicalNotReductionOrChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_logical_not_reduction_or_chain_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_logical_not_reduction_or_chain_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_logical_not_reduction_or_chain_supported(
  input  logic a,
  input  logic b,
  output logic y
);
  assign y = !{a, b};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_logical_not_reduction_or_chain_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);

  size_t orGateCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "or") {
      ++orGateCount;
    } else if (gateName == "not") {
      ++notGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
  EXPECT_GE(notGateCount, 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousLogicalNotConstantShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_logical_not_constant_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_logical_not_constant_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_logical_not_constant_shortcuts_supported(
  input  logic a,
  output logic y_break_one,
  output logic y_skip_zero,
  output logic y_all_zero
);
  assign y_break_one = !{a, 1'b1};
  assign y_skip_zero = !{a, 1'b0};
  assign y_all_zero  = !{a && 1'b0, 1'b0};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_logical_not_constant_shortcuts_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_break_one")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_skip_zero")), nullptr);
  EXPECT_NE(top->getNet(NLName("y_all_zero")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousConstantBinaryAndWideLiteralFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_constant_binary_and_wide_literal_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_constant_binary_and_wide_literal_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_constant_binary_and_wide_literal_fallback_supported(
  output logic [1:0]  sum_o,
  output logic [64:0] wide_o
);
  assign sum_o  = 2'd1 + 2'd1;
  assign wide_o = 65'h1_0000_0000_0000_0000;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_constant_binary_and_wide_literal_fallback_supported"));
  ASSERT_NE(top, nullptr);
  auto sumNet = top->getBusNet(NLName("sum_o"));
  ASSERT_NE(sumNet, nullptr);
  EXPECT_EQ(sumNet->getWidth(), 2);
  auto wideNet = top->getBusNet(NLName("wide_o"));
  ASSERT_NE(wideNet, nullptr);
  EXPECT_EQ(wideNet->getWidth(), 65);
}

TEST_F(SNLSVConstructorTestSimple, parseGateOperandCaseEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_operand_case_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_operand_case_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_operand_case_equality_supported(
  input logic a,
  input logic b,
  input logic c,
  output logic y
);
  assign y = a && (b === c);
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  constructor.construct(svPath);
  const std::string stdoutOutput = testing::internal::GetCapturedStdout();
  EXPECT_NE(
    std::string::npos,
    stdoutOutput.find("Case comparison operator '===' lowered as 2-state comparison in SNL"));

  auto top = library_->getSNLDesign(NLName("gate_operand_case_equality_supported"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  size_t xnorGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    const auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "and") {
      ++andGateCount;
    } else if (gateName == "xnor") {
      ++xnorGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
  EXPECT_EQ(1u, xnorGateCount);
}

TEST_F(SNLSVConstructorTestSimple, parseGateOperandInsideSetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_operand_inside_set_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_operand_inside_set_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_operand_inside_set_supported(
  input logic a,
  input logic [2:0] op,
  output logic y
);
  assign y = a && (op inside {3'd1, 3'd2, 3'd5});
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("gate_operand_inside_set_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseGateLogicalRHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "gate_logical_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "gate_logical_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module gate_logical_rhs_resolve_failure_unsupported(
  input logic a,
  input logic [3:0] b,
  output logic y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction
  always_comb begin
    y = a && bad_fn(b);
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb RHS bits"});
}

TEST_F(SNLSVConstructorTestSimple, parseDirectAssignMismatchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "direct_assign_mismatch" / "direct_assign_mismatch.sv");

  auto top = library_->getSNLDesign(NLName("direct_assign_mismatch"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_s")), nullptr);
  EXPECT_NE(top->getNet(NLName("out_b2")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignPackedMemberWidthResizeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_packed_member_width_resize_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "continuous_assign_packed_member_width_resize_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_packed_member_width_resize_supported(
  input  logic [7:0] wider_i,
  output logic [2:0] field_o
);
  typedef struct packed {
    logic [2:0] field;
  } packed_s;

  packed_s payload;
  assign payload.field = wider_i;
  assign field_o = payload.field;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_packed_member_width_resize_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("field_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConcatLHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "continuous_assign_concat_lhs_skip" / "continuous_assign_concat_lhs_skip.sv");

  auto top = library_->getSNLDesign(NLName("continuous_assign_concat_lhs_skip"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y0")), nullptr);
  EXPECT_NE(top->getNet(NLName("y1")), nullptr);

  size_t instanceCount = 0;
  for (auto inst : top->getInstances()) {
    (void)inst;
    ++instanceCount;
  }
  EXPECT_GT(instanceCount, 0u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignConcatLHSNestedAddSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_concat_lhs_nested_add_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_concat_lhs_nested_add_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_concat_lhs_nested_add_supported(
  input  logic [3:0] src1_i,
  input  logic [3:0] src2_i,
  input  logic       cin_i,
  output logic       carry_o,
  output logic [4:0] sum_o
);
  assign {carry_o, sum_o} =
    {src1_i[3], src1_i} + {src2_i[3], src2_i} + cin_i;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_concat_lhs_nested_add_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("carry_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("sum_o")), nullptr);

  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(12u, faCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousAssignPackedMemberShiftLeftSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_assign_packed_member_shift_left_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_assign_packed_member_shift_left_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_assign_packed_member_shift_left_supported(
  input  logic [4:0] data_i,
  output logic [4:0] field_o
);
  typedef struct packed {
    logic [4:0] field;
  } packed_s;

  packed_s payload;
  assign payload.field = data_i << 2'b10;
  assign field_o = payload.field;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("continuous_assign_packed_member_shift_left_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("field_o")), nullptr);

  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    }
  }
  EXPECT_GT(assignCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "always_comb_ignored" / "always_comb_ignored.sv");

  auto top = library_->getSNLDesign(NLName("always_comb_ignored"));
  ASSERT_NE(top, nullptr);

  size_t andGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "and") {
      ++andGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRhsDivideAndModByOneSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_divide_and_mod_by_one_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_rhs_divide_and_mod_by_one_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_divide_and_mod_by_one_supported(
  input  logic       cce_id_i,
  output logic [1:0] cce_cord_o
);
  localparam int coh_noc_x_cord_width_p = 1;
  localparam int coh_noc_y_cord_width_p = 1;
  localparam int sac_x_dim_p = 0;
  localparam int ic_y_dim_p = 0;
  localparam int cc_x_dim_p = 1;

  always_comb begin
    cce_cord_o = '0;
    cce_cord_o[0+:coh_noc_x_cord_width_p] = sac_x_dim_p + (cce_id_i % cc_x_dim_p);
    cce_cord_o[coh_noc_x_cord_width_p+:coh_noc_y_cord_width_p] =
      ic_y_dim_p + (cce_id_i / cc_x_dim_p);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_rhs_divide_and_mod_by_one_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("cce_cord_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombAutomaticVariableDeclarationsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_automatic_variable_declarations_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_automatic_variable_declarations_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_automatic_variable_declarations_supported(
  input  logic a,
  input  logic b,
  output logic y
);
  always_comb begin
    automatic logic tmp_a;
    automatic logic tmp_b;
    tmp_a = a;
    tmp_b = b;
    y = tmp_a & tmp_b;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_automatic_variable_declarations_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);

  size_t andGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "and") {
      ++andGateCount;
    }
  }
  EXPECT_EQ(1u, andGateCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombAutomaticVariableDeclarationInitializerUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_automatic_variable_declaration_initializer_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_automatic_variable_declaration_initializer_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_automatic_variable_declaration_initializer_unsupported(
  input  logic [3:0] a,
  output logic       y
);
  function automatic logic bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      4'b1???: bad_fn = 1'b1;
      default: bad_fn = 1'b0;
    endcase
  endfunction

  always_comb begin
    automatic logic tmp = bad_fn(a);
    tmp = a[0];
    y = tmp;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb initializer bits for local 'tmp'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombUnrelatedLocalVariableDeclarationIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_unrelated_local_variable_declaration_ignored_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_unrelated_local_variable_declaration_ignored_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unrelated_local_variable_declaration_ignored_supported(
  input  logic a,
  output logic y
);
  always_comb begin
    logic tmp = ~a;
    y = a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_unrelated_local_variable_declaration_ignored_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombSimpleAssignmentPatternPackedStructSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_simple_assignment_pattern_packed_struct_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_simple_assignment_pattern_packed_struct_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_simple_assignment_pattern_packed_struct_supported(
  input  logic       valid_i,
  input  logic [3:0] op_i,
  input  logic [7:0] data_i,
  input  logic       ctrl_i,
  output logic [13:0] out_o
);
  typedef struct packed {
    logic       valid;
    logic [3:0] op;
    logic [7:0] data;
    logic       ctrl;
  } packed_t;

  packed_t tmp;

  assign out_o = tmp;

  always_comb begin
    tmp = '{valid_i, op_i, data_i, ctrl_i};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_simple_assignment_pattern_packed_struct_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("valid_i")), nullptr);
  EXPECT_NE(top->getNet(NLName("op_i")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombStructuredAssignmentPatternDefaultSignalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_structured_assignment_pattern_default_signal_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_structured_assignment_pattern_default_signal_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_structured_assignment_pattern_default_signal_supported(
  input  logic stall_i,
  output logic [3:0] stall_raw
);
  always_comb begin
    stall_raw = '{default: stall_i};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_structured_assignment_pattern_default_signal_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("stall_raw")), nullptr);
  EXPECT_NE(top->getNet(NLName("stall_i")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombStructuredAssignmentPatternDefaultExactWidthSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_structured_assignment_pattern_default_exact_width_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_structured_assignment_pattern_default_exact_width_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_structured_assignment_pattern_default_exact_width_supported(
  input  logic [3:0] stall_i,
  output logic [3:0] stall_raw
);
  always_comb begin
    stall_raw = '{default: stall_i};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_structured_assignment_pattern_default_exact_width_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("stall_raw")), nullptr);
  EXPECT_NE(top->getNet(NLName("stall_i")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombStructuredAssignmentPatternMemberSettersNoDefaultSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_structured_assignment_pattern_member_setters_no_default_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_structured_assignment_pattern_member_setters_no_default_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_structured_assignment_pattern_member_setters_no_default_supported(
  input  logic       fp_present_i,
  input  logic [3:0] op_i,
  input  logic [3:0] data_i,
  output logic [6:0] out_o
);
  typedef struct packed {
    logic       issued;
    logic       cancelled;
    logic       is_rd_fpr_flag;
    logic [3:0] sbe;
  } sb_mem_t;

  function automatic logic is_rd_fpr(input logic [3:0] op);
    case (op)
      4'd1: return 1'b1;
      default: return 1'b0;
    endcase
  endfunction

  sb_mem_t mem_n;
  assign out_o = mem_n;

  always_comb begin
    mem_n = '{
      issued: 1'b1,
      cancelled: 1'b0,
      is_rd_fpr_flag: fp_present_i && is_rd_fpr(op_i),
      sbe: data_i
    };
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_structured_assignment_pattern_member_setters_no_default_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombStructuredAssignmentPatternDefaultWithMemberSettersUnresolvedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_structured_assignment_pattern_default_with_member_setters_unresolved_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_structured_assignment_pattern_default_with_member_setters_unresolved_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_structured_assignment_pattern_default_with_member_setters_unresolved_unsupported(
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  output logic [7:0] out_o
);
  typedef struct packed {
    logic [3:0] a;
    logic [3:0] b;
  } pair_t;

  pair_t pair_n;
  assign out_o = pair_n;

  always_comb begin
    pair_n = '{default: $countones(a_i), a: a_i, b: b_i};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb RHS bits for StructuredAssignmentPattern"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombStructuredAssignmentPatternMemberSetterExprResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_structured_assignment_pattern_member_setter_expr_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_structured_assignment_pattern_member_setter_expr_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_structured_assignment_pattern_member_setter_expr_resolve_failure_unsupported(
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  output logic [7:0] out_o
);
  typedef struct packed {
    logic [3:0] a;
    logic [3:0] b;
  } pair_t;

  pair_t pair_n;
  assign out_o = pair_n;

  always_comb begin
    pair_n = '{a: $countones(a_i), b: b_i};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb RHS bits for StructuredAssignmentPattern"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialStructuredAssignmentPatternMemberSetterOffsetOverflowUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "sequential_structured_assignment_pattern_member_setter_offset_overflow_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "sequential_structured_assignment_pattern_member_setter_offset_overflow_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_structured_assignment_pattern_member_setter_offset_overflow_unsupported(
  input  logic       clk,
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  output logic [3:0] q_o
);
  typedef struct packed {
    logic [3:0] lo;
    logic [3:0] hi;
  } wide_t;

  always_ff @(posedge clk) begin
    q_o <= wide_t'('{lo: a_i, hi: b_i});
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombAutomaticVariableIncDecSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_automatic_variable_inc_dec_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_automatic_variable_inc_dec_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_automatic_variable_inc_dec_supported(
  input  logic [3:0] in,
  input  logic       inc,
  input  logic       dec,
  output logic [3:0] y
);
  always_comb begin
    automatic logic [3:0] cnt;
    cnt = in;
    if (inc) cnt++;
    if (dec) cnt--;
    y = cnt;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_automatic_variable_inc_dec_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopIntInitializerSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_int_initializer_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_int_initializer_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_int_initializer_supported(
  input  logic [3:0] in,
  output logic [3:0] y
);
  always_comb begin
    y = 4'b0;
    for (int i = 0; i < 4; i++) begin
      if (in[i]) begin
        y[i] = 1'b1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_int_initializer_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(4, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopExternalControlInitializerSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_external_control_initializer_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_external_control_initializer_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_external_control_initializer_supported(
  input  logic [3:0] in,
  output logic [3:0] y
);
  always_comb begin
    automatic int i;
    y = 4'b0;
    for (i = 0; i < 4; i++) begin
      y[i] = in[i];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_for_loop_external_control_initializer_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(4, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopExternalControlNonConstantInitializerReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_external_control_non_constant_initializer_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_external_control_non_constant_initializer_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_external_control_non_constant_initializer_reported_unsupported(
  input  logic [3:0] in,
  input  logic [1:0] start_i,
  output logic       y
);
  always_comb begin
    automatic int i;
    y = 1'b0;
    for (i = start_i; i < 1; i++) begin
      y = in[i];
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported non-constant for-loop initializer RHS expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopExternalControlInitializerNonSymbolLHSReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_external_control_initializer_non_symbol_lhs_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_external_control_initializer_non_symbol_lhs_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_external_control_initializer_non_symbol_lhs_reported_unsupported(
  input  logic [3:0] in,
  output logic       y
);
  always_comb begin
    automatic int i;
    y = 1'b0;
    for ({i} = 0; i < 1; i++) begin
      y = in[i];
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported non-constant for-loop initializer RHS expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopDeclaredControlNonConstantInitializerReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_declared_control_non_constant_initializer_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_declared_control_non_constant_initializer_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_declared_control_non_constant_initializer_reported_unsupported(
  input  logic [1:0] start_i,
  output logic       y
);
  always_comb begin
    y = 1'b0;
    for (int i = start_i; i < 1; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported non-constant for-loop initializer for control variable 'i'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopDeclaredControlInitializerAssignmentToDifferentSymbolReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_for_loop_declared_control_initializer_assignment_to_different_symbol_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_declared_control_initializer_assignment_to_different_symbol_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_declared_control_initializer_assignment_to_different_symbol_reported_unsupported(
  output logic y
);
  always_comb begin
    automatic int j;
    y = 1'b0;
    for (int i = (j = 0); i < 1; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported non-constant for-loop initializer for control variable 'i'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopInitializerStructureMultipleDeclaredControlsReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_initializer_structure_multiple_declared_controls_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_initializer_structure_multiple_declared_controls_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_initializer_structure_multiple_declared_controls_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0, j = 0; i < 1; i++) begin
      y = j[0];
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop initializer structure"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopCompoundOrAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_compound_or_assignment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_compound_or_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_compound_or_assignment_supported(
  input  logic [3:0] push_instr,
  output logic       push_address
);
  always_comb begin
    push_address = 1'b0;
    for (int i = 0; i < 4; i++) begin
      push_address |= push_instr[i];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_compound_or_assignment_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("push_address")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCompoundAddAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_compound_add_assignment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_compound_add_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_compound_add_assignment_supported(
  input  logic [3:0] seed_i,
  input  logic [1:0] delta_i,
  output logic [3:0] sum_o
);
  always_comb begin
    sum_o = seed_i;
    sum_o += {delta_i, 1'b1, 1'b0};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_compound_add_assignment_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("sum_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCompoundSubtractAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_compound_subtract_assignment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_compound_subtract_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_compound_subtract_assignment_supported(
  input  logic [3:0] seed_i,
  input  logic [1:0] delta_i,
  output logic [3:0] diff_o
);
  always_comb begin
    diff_o = seed_i;
    diff_o -= {delta_i, 1'b1, 1'b0};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_compound_subtract_assignment_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("diff_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCompoundAddAssignmentUnknownRHSUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_compound_add_assignment_unknown_rhs_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_compound_add_assignment_unknown_rhs_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_compound_add_assignment_unknown_rhs_unsupported(
  input  logic [3:0] seed_i,
  output logic [3:0] sum_o
);
  always_comb begin
    sum_o = seed_i;
    sum_o += 4'bx001;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb compound assignment RHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCompoundAddAssignmentRecoveredLeftOperandUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_compound_add_assignment_recovered_left_operand_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_compound_add_assignment_recovered_left_operand_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_compound_add_assignment_recovered_left_operand_unsupported(
  input  logic [3:0] seed_i,
  input  logic [3:0] op_i,
  output logic [3:0] sum_o
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op);
    case (op) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction

  always_comb begin
    sum_o = seed_i;
    sum_o += sum_o + bad_fn(op_i);
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb compound assignment RHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCompoundAddAssignmentRecoveredRightOperandUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_compound_add_assignment_recovered_right_operand_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_compound_add_assignment_recovered_right_operand_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_compound_add_assignment_recovered_right_operand_unsupported(
  input  logic [3:0] seed_i,
  input  logic [3:0] op_i,
  output logic [3:0] sum_o
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op);
    case (op) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction

  always_comb begin
    sum_o = seed_i;
    sum_o += bad_fn(op_i) + sum_o;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb compound assignment RHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCompoundMultiplyAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_compound_multiply_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_compound_multiply_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_compound_multiply_assignment_unsupported(
  input  logic [3:0] seed_i,
  input  logic [3:0] factor_i,
  output logic [3:0] prod_o
);
  always_comb begin
    prod_o = seed_i;
    prod_o *= factor_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported compound assignment operator in always_comb: *"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalConditionResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_condition_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_conditional_condition_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_condition_resolve_failure_unsupported(
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  output logic [3:0] y_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction

  always_comb begin
    if (bad_cond(a_i)) begin
      y_o = a_i;
    end else begin
      y_o = b_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb condition bit"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalUnaryConditionResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_unary_condition_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_conditional_unary_condition_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_unary_condition_resolve_failure_unsupported(
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  output logic [3:0] y_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction

  always_comb begin
    if (!bad_cond(a_i)) begin
      y_o = a_i;
    end else begin
      y_o = b_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb condition bit"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalLogicalAndLHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_logical_and_lhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_conditional_logical_and_lhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_logical_and_lhs_resolve_failure_unsupported(
  input  logic [3:0] a_i,
  input  logic       b_i,
  output logic [3:0] y_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction

  always_comb begin
    if (bad_cond(a_i) && b_i) begin
      y_o = a_i;
    end else begin
      y_o = '0;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb condition bit"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalLogicalAndRHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_logical_and_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_conditional_logical_and_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_logical_and_rhs_resolve_failure_unsupported(
  input  logic       a_i,
  input  logic [3:0] b_i,
  output logic [3:0] y_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction

  always_comb begin
    if (a_i && bad_cond(b_i)) begin
      y_o = b_i;
    end else begin
      y_o = '0;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb condition bit"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalLogicalShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_logical_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_conditional_logical_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_logical_shortcuts_supported(
  input  logic a_i,
  input  logic b_i,
  output logic y0_o,
  output logic y1_o,
  output logic y2_o,
  output logic y3_o,
  output logic y4_o,
  output logic y5_o
);
  always_comb begin
    y0_o = 1'b0;
    if (!(a_i && 1'b0)) begin
      y0_o = 1'b1;
    end
  end

  always_comb begin
    y1_o = 1'b0;
    if (!(a_i || 1'b1)) begin
      y1_o = 1'b1;
    end
  end

  always_comb begin
    y2_o = 1'b0;
    if ((a_i && 1'b0) && b_i) begin
      y2_o = 1'b1;
    end
  end

  always_comb begin
    y3_o = 1'b0;
    if ((a_i || 1'b1) || b_i) begin
      y3_o = 1'b1;
    end
  end

  always_comb begin
    y4_o = 1'b0;
    if ((a_i && 1'b0) || b_i) begin
      y4_o = 1'b1;
    end
  end

  always_comb begin
    y5_o = 1'b0;
    if (a_i || 1'b0) begin
      y5_o = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_conditional_logical_shortcuts_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y0_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("y5_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalUnknownScalarMuxSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_unknown_scalar_mux_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_conditional_unknown_scalar_mux_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_unknown_scalar_mux_supported(
  input  logic sel_i,
  output logic y_o
);
  always_comb begin
    y_o = sel_i ? 1'bx : 1'b1;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_conditional_unknown_scalar_mux_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(1u, countMux2Instances(top, 1));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionalCaseFunctionSelectorMismatchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_case_function_selector_mismatch_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_conditional_case_function_selector_mismatch_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_case_function_selector_mismatch_unsupported(
  input  logic [3:0] a_i,
  input  logic [3:0] b_i,
  output logic [3:0] y_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (a_i)
      4'd1:   bad_cond = 1'b1;
      default: bad_cond = 1'b0;
    endcase
  endfunction

  always_comb begin
    if (bad_cond(a_i)) begin
      y_o = a_i;
    end else begin
      y_o = b_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_conditional_case_function_selector_mismatch_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopBreakSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_break_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_break_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_break_supported(
  input  logic [3:0] in,
  output logic       hit
);
  always_comb begin
    hit = 1'b0;
    for (int i = 0; i < 4; i++) begin
      if (in[i]) begin
        hit = 1'b1;
        break;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_break_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("hit")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConstantTrueConditionalSelectedBranchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_constant_true_conditional_selected_branch_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_constant_true_conditional_selected_branch_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_true_conditional_selected_branch_supported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    if (1'b1) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_constant_true_conditional_selected_branch_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopConstantFalseConditionalWithoutElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_constant_false_conditional_without_else_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_constant_false_conditional_without_else_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_constant_false_conditional_without_else_supported(
  output logic y_o
);
  always_comb begin
    y_o = 1'b0;
    for (int i = 0; i < 1; i++) begin
      if (1'b0) begin
        y_o = 1'b1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_constant_false_conditional_without_else_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopConstantTrueConditionalBreakSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_constant_true_conditional_break_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_constant_true_conditional_break_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_constant_true_conditional_break_supported(
  output logic hit
);
  always_comb begin
    hit = 1'b0;
    for (int i = 0; i < 2; i++) begin
      if (1'b1) begin
        hit = 1'b1;
        break;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_constant_true_conditional_break_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("hit")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConstantTrueConditionalSelectedBranchUnsupportedPropagated) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_constant_true_conditional_selected_branch_unsupported_propagated";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_constant_true_conditional_selected_branch_unsupported_propagated.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_true_conditional_selected_branch_unsupported_propagated(
  output logic [3:0] y
);
  always_comb begin
    y = 4'b0000;
    if (1'b1) begin
      y = '{1'b1, 1'b0, 1'b1, 1'b0};
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb RHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnarySelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unary_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_unary_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unary_selector_supported(
  output logic [2:0] y
);
  always_comb begin
    y = 3'b000;
    for (int i = 0; i < 1; i++) begin
      y[!1'b1] = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_unary_selector_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopNamedValueSelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_named_value_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_named_value_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_named_value_selector_supported(
  output logic [2:0] y
);
  localparam int IDX = 2;
  always_comb begin
    y = 3'b000;
    for (int i = 0; i < 1; i++) begin
      y[IDX] = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_named_value_selector_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnsignedBoundWithinInt64Supported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unsigned_bound_within_int64_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_unsigned_bound_within_int64_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unsigned_bound_within_int64_supported(
  output logic hit
);
  always_comb begin
    hit = 1'b0;
    for (int i = 0; i < 64'h7FFF_FFFF_FFFF_FFFF; i++) begin
      hit = 1'b1;
      break;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_unsigned_bound_within_int64_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("hit")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnsignedBoundAboveInt64ReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unsigned_bound_above_int64_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unsigned_bound_above_int64_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unsigned_bound_above_int64_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 64'hFFFF_FFFF_FFFF_FFFF; i++) begin
      y = 1'b1;
      break;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"unsupported non-constant for-loop bound expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopActiveLongintElementIndexAboveInt32Supported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_active_longint_element_index_above_int32_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_active_longint_element_index_above_int32_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_active_longint_element_index_above_int32_supported(
  input  logic [3:0] a,
  output logic       y
);
  always_comb begin
    y = 1'b0;
    for (longint i = 64'd2147483648; i < 64'd2147483649; i++) begin
      y = a[i];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_active_longint_element_index_above_int32_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopSignedBoundAboveInt64ReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_signed_bound_above_int64_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_signed_bound_above_int64_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_signed_bound_above_int64_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 65'sh1_0000_0000_0000_0000; i++) begin
      y = 1'b1;
      break;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"unsupported non-constant for-loop bound expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnsignedParameterBoundAboveInt64ReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unsigned_parameter_bound_above_int64_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unsigned_parameter_bound_above_int64_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unsigned_parameter_bound_above_int64_reported_unsupported(
  output logic y
);
  localparam longint unsigned BOUND = 64'hFFFF_FFFF_FFFF_FFFF;
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < BOUND; i++) begin
      y = 1'b1;
      break;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"unsupported non-constant for-loop bound expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopStructParameterDivisionBoundSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_struct_parameter_division_bound_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_struct_parameter_division_bound_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_struct_parameter_division_bound_supported(
  input  logic [63:0] in,
  output logic [63:0] y
);
  typedef struct packed {
    int unsigned XLEN;
  } cfg_t;

  parameter cfg_t Cfg = '{XLEN: 64};

  always_comb begin
    y = '0;
    for (int i = 0; i < Cfg.XLEN / 8; i++) begin
      y[i] = in[i];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_struct_parameter_division_bound_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(64, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopConstantInt64BinaryOpsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_constant_int64_binary_ops_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_constant_int64_binary_ops_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_constant_int64_binary_ops_supported(
  input  logic [31:0] in,
  output logic [0:0]  y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[i + 0];
      y[i] = in[i - 0];
      y[i] = in[i * 1];
      y[i] = in[(i + 1) / 1];
      y[i] = in[(i + 1) % 2];
      y[i] = in[i << 0];
      y[i] = in[2 >> i];
      y[i] = in[$signed(i) >>> 0];
      y[i] = in[i & 0];
      y[i] = in[i | 1];
      y[i] = in[i ^ 1];
      y[i] = in[i < 1];
      y[i] = in[i <= 0];
      y[i] = in[i > 0];
      y[i] = in[i >= 0];
      y[i] = in[i == 0];
      y[i] = in[i != 0];
      y[i] = in[i && 1];
      y[i] = in[i || 0];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_constant_int64_binary_ops_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopConstantInt64UnaryOpsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_constant_int64_unary_ops_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_constant_int64_unary_ops_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic DebugEn;
} cfg_t;

module always_comb_for_loop_constant_int64_unary_ops_supported #(
  parameter cfg_t Cfg = '{DebugEn: 1'b0}
) (
  output logic [3:0] y
);
  localparam int ONE = 1;
  localparam int ZERO = 0;
  always_comb begin
    y = '0;
    for (int i = +ONE; i < 2; i++) begin
      y[0] = 1'b1;
      break;
    end
    for (int i = ~ZERO; i < 0; i++) begin
      y[1] = 1'b1;
      break;
    end
    for (int k = 0; k < 1; k++) begin
      for (int i = !k; i < 2; i++) begin
        y[2] = 1'b1;
        break;
      end
    end
    for (int i = -ONE; i < 0; i++) begin
      y[3] = 1'b1;
      break;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_constant_int64_unary_ops_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(4, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnaryNonConstantInitializerReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_unary_non_constant_initializer_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unary_non_constant_initializer_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unary_non_constant_initializer_reported_unsupported(
  input  logic start_i,
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = !start_i; i < 1; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported non-constant for-loop initializer for control variable 'i'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnaryUnsupportedInitializerOperatorReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "always_comb_for_loop_unary_unsupported_initializer_operator_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_unary_unsupported_initializer_operator_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unary_unsupported_initializer_operator_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int k = 0; k < 1; k++) begin
      for (int i = ++k; i < 1; i++) begin
        y = 1'b1;
      end
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module "
     "'always_comb_for_loop_unary_unsupported_initializer_operator_reported_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopConstantMultiplyLiteralRHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_constant_multiply_literal_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_constant_multiply_literal_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_constant_multiply_literal_rhs_supported(
  output logic [2:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 3; i++) begin
      y[i] = i * 3;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_for_loop_constant_multiply_literal_rhs_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(3, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopConstantMultiplyOverflowReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_constant_multiply_overflow_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_constant_multiply_overflow_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_constant_multiply_overflow_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 2; i < 3; i++) begin
      y = i * 64'hFFFFFFFFFFFFFFFF;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module 'always_comb_for_loop_constant_multiply_overflow_reported_unsupported'",
     "unable to resolve always_comb RHS bits for BinaryOp op=*"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopDivideByZeroBoundReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_divide_by_zero_selector_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_divide_by_zero_selector_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_divide_by_zero_selector_reported_unsupported(
  input  logic [3:0] in,
  output logic [0:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[(i + 1) / 0];
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module 'always_comb_for_loop_divide_by_zero_selector_reported_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopModByZeroBoundReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_mod_by_zero_selector_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_for_loop_mod_by_zero_selector_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_mod_by_zero_selector_reported_unsupported(
  input  logic [3:0] in,
  output logic [0:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[(i + 1) % 0];
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module 'always_comb_for_loop_mod_by_zero_selector_reported_unsupported'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopShiftLeftOutOfRangeSelectorFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_shift_left_out_of_range_selector_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_shift_left_out_of_range_selector_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_shift_left_out_of_range_selector_fallback_supported(
  input  logic [3:0] in,
  output logic [0:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[i << 64];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top =
    library_->getSNLDesign(NLName("always_comb_for_loop_shift_left_out_of_range_selector_fallback_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopShiftRightOutOfRangeSelectorFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_shift_right_out_of_range_selector_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_shift_right_out_of_range_selector_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_shift_right_out_of_range_selector_fallback_supported(
  input  logic [3:0] in,
  output logic [0:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[i >> 64];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top =
    library_->getSNLDesign(NLName("always_comb_for_loop_shift_right_out_of_range_selector_fallback_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopSelectorAboveInt32FallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_selector_above_int32_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_selector_above_int32_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_selector_above_int32_fallback_supported(
  input  logic [3:0] in,
  output logic [0:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[i + 33'd2147483648];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top =
    library_->getSNLDesign(NLName("always_comb_for_loop_selector_above_int32_fallback_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopArithmeticShiftRightOutOfRangeSelectorFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_arithmetic_shift_right_out_of_range_selector_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_arithmetic_shift_right_out_of_range_selector_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_arithmetic_shift_right_out_of_range_selector_fallback_supported(
  input  logic [3:0] in,
  output logic [0:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; i++) begin
      y[i] = in[i >>> 64];
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_arithmetic_shift_right_out_of_range_selector_fallback_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopDirectShiftAmountLoopVariableSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_direct_shift_amount_loop_variable_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_direct_shift_amount_loop_variable_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_direct_shift_amount_loop_variable_supported(
  input  logic [3:0] in,
  output logic [3:0] y_pos,
  output logic [3:0] y_neg
);
  always_comb begin
    y_pos = '0;
    y_neg = '0;
    for (int i = 1; i < 2; i++) begin
      y_pos = in >> i;
    end
    for (int j = -1; j < 0; j++) begin
      y_neg = in >> j;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);
  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_direct_shift_amount_loop_variable_supported"));
  ASSERT_NE(top, nullptr);
  auto yPosNet = top->getBusNet(NLName("y_pos"));
  ASSERT_NE(yPosNet, nullptr);
  EXPECT_EQ(yPosNet->getWidth(), 4);
  auto yNegNet = top->getBusNet(NLName("y_neg"));
  ASSERT_NE(yNegNet, nullptr);
  EXPECT_EQ(yNegNet->getWidth(), 4);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopStopConditionsAndAssignmentStepRHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_stop_conditions_and_assignment_step_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_stop_conditions_and_assignment_step_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_stop_conditions_and_assignment_step_rhs_supported(
  output logic [5:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i <= 0; i = 3) begin
      y[0] = 1'b1;
    end
    for (int i = 1; i > 0; i = i - 1) begin
      y[1] = 1'b1;
    end
    for (int i = 0; i >= 0; i = -1) begin
      y[2] = 1'b1;
    end
    for (int i = 0; 0 == i; i = i + 1) begin
      y[3] = 1'b1;
    end
    for (int i = 0; 1 != i; i = 1 + i) begin
      y[4] = 1'b1;
    end
    for (int i = 1; i < 2; i = 3 - i) begin
      y[5] = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_stop_conditions_and_assignment_step_rhs_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(6, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnaryIncrementStepsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unary_increment_steps_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unary_increment_steps_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unary_increment_steps_supported(
  output logic [1:0] y
);
  always_comb begin
    y = '0;
    for (int i = 0; i < 1; ++i) begin
      y[0] = 1'b1;
    end
    for (int i = 0; i < 1; i++) begin
      y[1] = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_unary_increment_steps_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(2, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnaryDecrementStepsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unary_decrement_steps_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unary_decrement_steps_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unary_decrement_steps_supported(
  output logic [1:0] y
);
  always_comb begin
    y = '0;
    for (int i = 1; i > 0; --i) begin
      y[0] = 1'b1;
    end
    for (int i = 1; i > 0; i--) begin
      y[1] = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_unary_decrement_steps_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getBusNet(NLName("y"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_EQ(2, yNet->getWidth());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnaryStepOnNonControlVariableReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_unary_step_on_non_control_variable_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unary_step_on_non_control_variable_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unary_step_on_non_control_variable_reported_unsupported(
  output logic y
);
  always_comb begin
    automatic int j;
    j = 0;
    y = 1'b0;
    for (int i = 0; i < 1; ++j) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop unary step on non-control variable"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopAssignmentStepOnNonControlVariableReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_assignment_step_on_non_control_variable_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_assignment_step_on_non_control_variable_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_assignment_step_on_non_control_variable_reported_unsupported(
  output logic y
);
  always_comb begin
    automatic int j;
    j = 0;
    y = 1'b0;
    for (int i = 0; i < 1; j = j + 1) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop assignment step on non-control variable"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopCompoundAssignmentStepAddNonConstantOperandReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_compound_assignment_step_add_non_constant_operand_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_compound_assignment_step_add_non_constant_operand_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_compound_assignment_step_add_non_constant_operand_reported_unsupported(
  input  logic step_i,
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 1; i += step_i) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported non-constant compound for-loop assignment step"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopCompoundAssignmentStepAddConstantSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_compound_assignment_step_add_constant_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_compound_assignment_step_add_constant_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_compound_assignment_step_add_constant_supported(
  output logic [2:0] y
);
  always_comb begin
    y = 3'b000;
    for (int i = 0; i < 3; i += 1) begin
      y[i] = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_for_loop_compound_assignment_step_add_constant_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopUnsupportedStopComparisonOperatorReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_unsupported_stop_comparison_operator_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_unsupported_stop_comparison_operator_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_unsupported_stop_comparison_operator_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i && 1'b1; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop comparison operator in stop expression: &&"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopNonBinaryStopExpressionReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_non_binary_stop_expression_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_non_binary_stop_expression_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_non_binary_stop_expression_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop stop expression (expected binary comparison)"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopStopExpressionLoopVariableNotIsolatedReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_stop_expression_loop_variable_not_isolated_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_stop_expression_loop_variable_not_isolated_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_stop_expression_loop_variable_not_isolated_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < i; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop stop expression (control variable not isolated)"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopMissingStopConditionReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_missing_stop_condition_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_missing_stop_condition_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_missing_stop_condition_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; ; i++) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(constructor, svPath, {"unsupported for-loop without stop condition"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopMissingStepExpressionReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_missing_step_expression_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_missing_step_expression_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_missing_step_expression_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 1; ) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported for-loop step count (only one step expression is supported)"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopAssignmentStepNonBinaryRHSReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_for_loop_assignment_step_non_binary_rhs_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_assignment_step_non_binary_rhs_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_assignment_step_non_binary_rhs_reported_unsupported(
  input  logic start_i,
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 1; i = start_i) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(constructor, svPath, {"unsupported for-loop step expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopAssignmentStepAddNonConstantOperandReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_assignment_step_add_non_constant_operand_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_for_loop_assignment_step_add_non_constant_operand_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_assignment_step_add_non_constant_operand_reported_unsupported(
  input  logic step_i,
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 1; i = i + step_i) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(constructor, svPath, {"unsupported for-loop step expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopAssignmentStepSubtractNonConstantOperandReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_assignment_step_subtract_non_constant_operand_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_assignment_step_subtract_non_constant_operand_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_assignment_step_subtract_non_constant_operand_reported_unsupported(
  input  logic step_i,
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i < 1; i = i - step_i) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(constructor, svPath, {"unsupported for-loop step expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopAssignmentStepUnsupportedBinaryOperatorReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_assignment_step_unsupported_binary_operator_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_assignment_step_unsupported_binary_operator_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_assignment_step_unsupported_binary_operator_reported_unsupported(
  input  logic scale_i,
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 1; i < 2; i = i * scale_i) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(constructor, svPath, {"unsupported for-loop step expression"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombForLoopAssignmentStepSelfReferenceIterationLimitReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_for_loop_assignment_step_self_reference_iteration_limit_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_for_loop_assignment_step_self_reference_iteration_limit_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_for_loop_assignment_step_self_reference_iteration_limit_reported_unsupported(
  output logic y
);
  always_comb begin
    y = 1'b0;
    for (int i = 0; i == 0; i = i) begin
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"for-loop unroll iteration limit exceeded"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_supported(
  input  logic [1:0] sel,
  input  logic       a,
  input  logic       b,
  input  logic       c,
  input  logic       d,
  output logic       y,
  output logic       z
);
  always_comb begin
    y = 1'b0;
    z = 1'b0;
    case (sel)
      2'b00: y = a;
      2'b01, 2'b10: begin
        y = b;
        z = c;
      end
      default: z = d;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_supported"));
  ASSERT_NE(top, nullptr);

  size_t xnorGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "xnor") {
      ++xnorGateCount;
    }
  }
  EXPECT_GT(xnorGateCount, 0u);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "always_comb_case_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseConstantSelectorShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_constant_selector_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_constant_selector_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_constant_selector_shortcuts_supported(
  input  logic a,
  input  logic b,
  input  logic c,
  output logic y
);
  always_comb begin
    y = 1'b0;
    case (2'b01)
      2'b00: y = a;
      2'b01: y = b;
      default: y = c;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_constant_selector_shortcuts_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseItemEqualBitShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_item_equal_bit_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_item_equal_bit_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_item_equal_bit_shortcut_supported(
  input  logic [1:0] sel,
  output logic [1:0] y
);
  always_comb begin
    y = 2'b10;
    case (sel)
      2'b01: y = 2'b11;
      default: y = 2'b10;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_item_equal_bit_shortcut_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseItemEqualMergedBitsShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_item_equal_merged_bits_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_item_equal_merged_bits_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_item_equal_merged_bits_shortcut_supported(
  input  logic [1:0] sel,
  output logic [1:0] y
);
  always_comb begin
    y = 2'b10;
    case (sel)
      2'b01: y = 2'b10;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_item_equal_merged_bits_shortcut_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseEmptyItemSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_empty_item_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_empty_item_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_empty_item_supported(
  input  logic [1:0] sel,
  output logic       y
);
  always_comb begin
    y = 1'b0;
    case (sel)
      2'b00: ;
      2'b01: y = 1'b1;
      default: ;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_empty_item_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseInsideSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_inside_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_inside_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_inside_supported(
  input  logic [4:0] sel,
  input  logic [1:0] a,
  output logic [1:0] y
);
  always_comb begin
    y = 2'b00;
    case (sel) inside
      5'b001??: y = a;
      [5'b10000:5'b10111]: y = 2'b10;
      default: y = 2'b11;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_inside_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCasezSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_casez_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_supported(
  input  logic [2:0] sel,
  input  logic [1:0] a,
  output logic [1:0] y
);
  always_comb begin
    unique casez (sel)
      3'b1?0: y = a;
      3'b0??: y = 2'b10;
      default: y = 2'b01;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_casez_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getBusNet(NLName("y")), nullptr);
  EXPECT_GE(countMux2Instances(top), 1u);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCasezConcatenationItemSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_concatenation_item_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_casez_concatenation_item_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_concatenation_item_supported(
  input  logic [3:0] sel,
  output logic [1:0] y
);
  always_comb begin
    unique casez (sel)
      {2'b10, 2'b?1}: y = 2'b11;
      4'b0???: y = 2'b10;
      default: y = 2'b01;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_casez_concatenation_item_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getBusNet(NLName("y")), nullptr);
  EXPECT_GE(countMux2Instances(top), 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezConcatenationZeroWidthReplicationItemSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_concat_zero_width_replication_item_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_concat_zero_width_replication_item_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_concat_zero_width_replication_item_supported(
  input  logic [1:0] sel,
  output logic       y
);
  always_comb begin
    unique casez (sel)
      {1'b1, {0{1'b0}}, 1'b?}: y = 1'b1;
      default: y = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_casez_concat_zero_width_replication_item_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y")), nullptr);
  EXPECT_GE(countMux2Instances(top), 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezSignedItemSignExtendSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_signed_item_signextend_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_casez_signed_item_signextend_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_signed_item_signextend_supported(
  input  logic signed [3:0] sel_i,
  output logic             y_o
);
  always_comb begin
    unique casez (sel_i)
      2'sb11: y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_casez_signed_item_signextend_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezUnbasedWildcardLiteralSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_unbased_wildcard_literal_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_unbased_wildcard_literal_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_unbased_wildcard_literal_supported(
  input  logic [1:0] sel_i,
  output logic       y_o
);
  always_comb begin
    unique casez (sel_i)
      'z:      y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_casez_unbased_wildcard_literal_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezConstantExpressionBitFlipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_constant_expression_bitflip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_constant_expression_bitflip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_constant_expression_bitflip_supported(
  output logic y_o
);
  always_comb begin
    unique casez (2'b10)
      2'b00:   y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_casez_constant_expression_bitflip_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezWildcardMatcherShapeVariantsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_wildcard_matcher_shape_variants_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_wildcard_matcher_shape_variants_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_wildcard_matcher_shape_variants_supported(
  output logic y_all_o,
  output logic y_single_o,
  output logic y_zero_o,
  output logic y_one_o
);
  always_comb begin
    unique casez (2'b10)
      2'b??:   y_all_o = 1'b1;
      default: y_all_o = 1'b0;
    endcase

    unique casez (2'b01)
      2'b?1:      y_single_o = 1'b1;
      default:    y_single_o = 1'b0;
    endcase

    unique casez (2'b00)
      2'b?0:    y_zero_o = 1'b1;
      default:  y_zero_o = 1'b0;
    endcase

    unique casez (2'b01)
      2'b?0:   y_one_o = 1'b1;
      default: y_one_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_casez_wildcard_matcher_shape_variants_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_all_o")), nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_single_o")), nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_zero_o")), nullptr);
  EXPECT_NE(top->getScalarNet(NLName("y_one_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCaseInsideVariableItemUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_inside_variable_item_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_inside_variable_item_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_inside_variable_item_unsupported(
  input  logic [2:0] sel_i,
  output logic       y_o
);
  always_comb begin
    case (sel_i) inside
      $urandom_range(3, 0): y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb case inside item match for"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezExpressionBitsResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_expression_bits_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_expression_bits_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_expression_bits_resolve_failure_unsupported(
  output logic y_o
);
  always_comb begin
    unique casez ($urandom_range(3, 0))
      2'b0?:   y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb case expression bits for"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezVariableItemConstantUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_variable_item_constant_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_variable_item_constant_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_variable_item_constant_unsupported(
  input  logic [1:0] sel_i,
  input  logic [1:0] item_i,
  output logic       y_o
);
  always_comb begin
    unique casez (sel_i)
      item_i:  y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb wildcard case item constant for"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCasezUnknownXBitMatchUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_casez_unknown_xbit_match_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_casez_unknown_xbit_match_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_casez_unknown_xbit_match_unsupported(
  input  logic [1:0] sel_i,
  output logic       y_o
);
  always_comb begin
    unique casez (sel_i)
      2'bx1:   y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb wildcard case item match for"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseDefaultBranchFunctionCallSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_default_branch_function_call_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_default_branch_function_call_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_default_branch_failure_unsupported(
  input  logic [1:0] sel_i,
  input  logic [3:0] op_i,
  output logic [1:0] y_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: extract_transfer_size = 2'b00;
      4'h1: extract_transfer_size = 2'b01;
      default: extract_transfer_size = 2'b11;
    endcase
  endfunction

  always_comb begin
    y_o = 2'b00;
    case (sel_i)
      2'b00: y_o = 2'b01;
      2'b01: y_o = 2'b10;
      default: y_o = extract_transfer_size(op_i);
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_default_branch_failure_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseItemBranchFunctionCallSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_item_branch_function_call_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_item_branch_function_call_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_item_branch_failure_unsupported(
  input  logic [1:0] sel_i,
  input  logic [3:0] op_i,
  output logic [1:0] y_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: extract_transfer_size = 2'b00;
      4'h1: extract_transfer_size = 2'b01;
      default: extract_transfer_size = 2'b11;
    endcase
  endfunction

  always_comb begin
    y_o = 2'b00;
    case (sel_i)
      2'b00: y_o = 2'b01;
      2'b01: y_o = extract_transfer_size(op_i);
      default: y_o = 2'b11;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_item_branch_failure_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombCaseItemFunctionMatchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_item_function_match_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_item_function_match_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_item_match_failure_unsupported(
  input  logic [1:0] sel_i,
  input  logic [3:0] op_i,
  output logic [1:0] y_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: extract_transfer_size = 2'b00;
      4'h1: extract_transfer_size = 2'b01;
      default: extract_transfer_size = 2'b11;
    endcase
  endfunction

  always_comb begin
    y_o = 2'b00;
    case (sel_i)
      extract_transfer_size(op_i): y_o = 2'b01;
      default: y_o = 2'b10;
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_item_match_failure_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConcatenationReplicationSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_concat_replication_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_concat_replication_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concat_replication_supported(
  input  logic [63:0] fetch_address,
  input  logic        if_ready,
  output logic [63:0] npc_d
);
  localparam int ALIGN = 3;
  always_comb begin
    npc_d = fetch_address;
    if (if_ready) begin
      npc_d = {fetch_address[63:ALIGN] + 1, {ALIGN{1'b0}}};
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_concat_replication_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("npc_d")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConcatenationZeroWidthReplicationSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_concat_zero_width_replication_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_concat_zero_width_replication_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concat_zero_width_replication_supported(
  input  logic [63:0] branch_result,
  output logic [63:0] flu_result_o
);
  typedef struct packed {
    int unsigned XLEN;
    int unsigned VLEN;
  } cfg_t;

  localparam cfg_t Cfg = '{XLEN: 64, VLEN: 64};

  always_comb begin
    flu_result_o = {{Cfg.XLEN - Cfg.VLEN{1'b0}}, branch_result};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_concat_zero_width_replication_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("branch_result")), nullptr);
  EXPECT_NE(top->getNet(NLName("flu_result_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombReplicationConcatResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_replication_concat_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_replication_concat_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_replication_concat_resolve_failure_unsupported(
  input  logic [3:0] a,
  output logic [7:0] y
);
  function automatic logic [3:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 4'hf;
      default:          bad_fn = 4'h0;
    endcase
  endfunction

  always_comb begin
    y = {2{bad_fn(a)}};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb RHS bits for Replication"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombReplicationUnknownLiteralSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_replication_unknown_literal_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_replication_unknown_literal_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_replication_unknown_literal_supported(
  input  logic       sel_i,
  input  logic [1:0] d_i,
  output logic [1:0] y_o
);
  always_comb begin
    if (sel_i)
      y_o = d_i;
    else
      y_o = {2{1'bx}};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_replication_unknown_literal_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getBusNet(NLName("y_o")), nullptr);
  EXPECT_EQ(1u, countMux2Instances(top));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConcatenationFallbackCanonicalWidthReportedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_concat_fallback_canonical_width_reported_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_concat_fallback_canonical_width_reported_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concat_fallback_canonical_width_reported_unsupported(
  input  logic        lsb_i,
  output logic [32:0] y_o
);
  always_comb begin
    y_o = {int'(1.5), lsb_i};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module 'always_comb_concat_fallback_canonical_width_reported_unsupported'",
     "unable to resolve always_comb RHS bits for Concatenation"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConcatenationZeroWidthTypedOperandSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_concat_zero_width_typed_operand_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_concat_zero_width_typed_operand_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concat_zero_width_typed_operand_supported(
  input  logic       lsb_i,
  output logic [0:0] y_o
);
  localparam int unsigned PAD_W = 0;
  typedef logic [PAD_W-1:0] pad_t;
  pad_t pad_q;

  always_comb begin
    y_o = {pad_q, lsb_i};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_concat_zero_width_typed_operand_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getNet(NLName("y_o"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_NE(dynamic_cast<SNLBitNet*>(yNet), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConcatenationZeroWidthSignalOperandSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_concat_zero_width_signal_operand_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_concat_zero_width_signal_operand_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concat_zero_width_signal_operand_supported(
  input  logic       lsb_i,
  output logic [0:0] y_o
);
  logic [$clog2(1)-1:0] shamt_q;

  always_comb begin
    y_o = {shamt_q, lsb_i};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_concat_zero_width_signal_operand_supported"));
  ASSERT_NE(top, nullptr);
  auto yNet = top->getNet(NLName("y_o"));
  ASSERT_NE(yNet, nullptr);
  EXPECT_NE(dynamic_cast<SNLBitNet*>(yNet), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConstantConditionSkipsOutOfRangeElementSelectBranch) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_const_cond_skips_oob_element_select_branch";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_const_cond_skips_oob_element_select_branch.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_const_cond_skips_oob_element_select_branch(
  output logic y
);
  typedef struct packed {
    logic none, load, store, alu, alu2, ctrl_flow, mult, csr, fpu, fpu_vec, cvxif, accel, aes;
  } fus_busy_t;

  localparam bit SUPER = 1'b0;
  localparam int unsigned N = SUPER ? 2 : 1;
  fus_busy_t [N-1:0] fus_busy;

  always_comb begin
    fus_busy = '0;
    if (SUPER) begin
      fus_busy[1] = '1;
    end
    y = fus_busy[0].none;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_const_cond_skips_oob_element_select_branch"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCaseSimpleReturnFunctionRangeSelectArgSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_simple_return_function_range_select_arg_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_case_simple_return_function_range_select_arg_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_simple_return_function_range_select_arg_supported(
  input  logic [1:0]   operator_q,
  input  logic [127:0] mult_result_q,
  input  logic [63:0]  clmul_q,
  input  logic [63:0]  clmulr_q,
  output logic [63:0]  result_o
);
  localparam bit IS_XLEN64 = 1'b1;
  localparam logic [1:0] MULW = 2'b01;
  localparam logic [1:0] CLMUL = 2'b10;
  localparam logic [1:0] CLMULH = 2'b11;

  function automatic logic [63:0] sext32to64(logic [31:0] operand);
    return {{32{operand[31]}}, operand[31:0]};
  endfunction

  always_comb begin
    unique case (operator_q)
      CLMUL:               result_o = clmul_q;
      CLMULH:              result_o = clmulr_q >> 1;
      default: begin
        if (operator_q == MULW && IS_XLEN64) result_o = sext32to64(mult_result_q[31:0]);
        else result_o = mult_result_q[63:0];
      end
    endcase
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_simple_return_function_range_select_arg_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("result_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCaseReturnFunctionCallSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_return_function_call_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_case_return_function_call_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_return_function_call_supported(
  input  logic [3:0] op_i,
  output logic [1:0] data_size_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: return 2'b00;
      4'h1: return 2'b01;
      4'h2: return 2'b10;
      default: return 2'b11;
    endcase
  endfunction

  always_comb begin
    data_size_o = extract_transfer_size(op_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_case_return_function_call_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_size_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombCaseReturnFunctionCallMultiSelectorMergeOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_case_return_function_call_multi_selector_merge_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_case_return_function_call_multi_selector_merge_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_return_function_call_multi_selector_merge_or_supported(
  input  logic [3:0] op_i,
  output logic [1:0] data_size_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0, 4'h1: return 2'b00;
      4'h2: return 2'b01;
      default: return 2'b10;
    endcase
  endfunction

  always_comb begin
    data_size_o = extract_transfer_size(op_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_return_function_call_multi_selector_merge_or_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_size_o")), nullptr);

  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombShiftLeftSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_shift_left_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_shift_left_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_shift_left_supported(
  input  logic [63:0] in,
  input  logic [5:0]  shamt,
  output logic [63:0] y
);
  always_comb begin
    y = in << shamt;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_shift_left_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombScalarShiftLeftSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_scalar_shift_left_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_scalar_shift_left_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_scalar_shift_left_supported(
  input  logic in,
  input  logic shamt,
  output logic y
);
  always_comb begin
    y = in << shamt;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_scalar_shift_left_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombShiftRightScaledLocalparamSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_shift_right_scaled_localparam_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_shift_right_scaled_localparam_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_shift_right_scaled_localparam_supported(
  input  logic [63:0] in,
  input  logic [4:0]  shamt,
  output logic [63:0] y
);
  localparam int unsigned LANE = 3;
  always_comb begin
    y = in >> (LANE * shamt);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_shift_right_scaled_localparam_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombSubtractFunctionReturnSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_subtract_function_return_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_subtract_function_return_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package always_comb_subtract_function_return_pkg;
  typedef logic [1:0] fmt_t;
  function automatic int unsigned man_bits(fmt_t fmt);
    return unsigned'(fmt + 3);
  endfunction
endpackage

module always_comb_subtract_function_return_supported(
  input  logic [1:0]  fmt_i,
  output logic [11:0] y
);
  import always_comb_subtract_function_return_pkg::*;
  localparam int unsigned SUPER_MAN_BITS = 16;
  always_comb begin
    y = SUPER_MAN_BITS - man_bits(fmt_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_subtract_function_return_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombEnumCaseNo2StateWarning) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_enum_case_no_2state_warning";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_enum_case_no_2state_warning.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_enum_case_no_2state_warning(
  input  logic trig_i,
  output logic y_o
);
  typedef enum logic [1:0] {
    IDLE = 2'b00,
    RUN  = 2'b01,
    DONE = 2'b10
  } state_t;

  state_t state_q;
  assign state_q = trig_i ? RUN : IDLE;

  always_comb begin
    y_o = 1'b0;
    unique case (state_q)
      IDLE: y_o = 1'b0;
      RUN:  y_o = 1'b1;
      DONE: y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  constructor.construct(svPath);
  const std::string stdoutOutput = testing::internal::GetCapturedStdout();
  EXPECT_EQ(
    std::string::npos,
    stdoutOutput.find("Case comparison operator '===' lowered as 2-state comparison in SNL"));

  auto top = library_->getSNLDesign(NLName("always_comb_enum_case_no_2state_warning"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombEnumCaseLiteralItemWarns) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_enum_case_literal_item_warns";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_enum_case_literal_item_warns.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_enum_case_literal_item_warns(
  input  logic trig_i,
  output logic y_o
);
  typedef enum logic [1:0] {
    IDLE = 2'b00,
    RUN  = 2'b01,
    DONE = 2'b10
  } state_t;

  state_t state_q;
  assign state_q = trig_i ? RUN : IDLE;

  always_comb begin
    y_o = 1'b0;
    unique case (state_q)
      state_t'(2'b00): y_o = 1'b0;
      RUN:   y_o = 1'b1;
      DONE:  y_o = 1'b1;
      default: y_o = 1'b0;
    endcase
  end
endmodule
)";
  svFile.close();

  testing::internal::CaptureStdout();
  constructor.construct(svPath);
  const std::string stdoutOutput = testing::internal::GetCapturedStdout();
  EXPECT_NE(
    std::string::npos,
    stdoutOutput.find("Case comparison operator '===' lowered as 2-state comparison in SNL"));

  auto top = library_->getSNLDesign(NLName("always_comb_enum_case_literal_item_warns"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConditionSubtractSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_condition_subtract_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_condition_subtract_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_subtract_supported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  output logic       y
);
  always_comb begin
    y = 1'b0;
    if (a == (b - 4'd1)) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_condition_subtract_supported"));
  ASSERT_NE(top, nullptr);

  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_GT(faCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConditionSubtractVariableRHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_condition_subtract_variable_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_condition_subtract_variable_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_subtract_variable_rhs_supported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  input  logic [3:0] c,
  output logic       y
);
  always_comb begin
    y = 1'b0;
    if (a == (b - c)) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_condition_subtract_variable_rhs_supported"));
  ASSERT_NE(top, nullptr);

  size_t faCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "not") {
      ++notGateCount;
    }
  }
  EXPECT_GT(faCount, 0u);
  EXPECT_GT(notGateCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConditionUnaryNotLogicalAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_condition_unary_not_logical_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_condition_unary_not_logical_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_unary_not_logical_and_supported #(
  parameter bit DBG_EN = 1'b1
) (
  input  logic dbg_mode,
  output logic y
);
  always_comb begin
    y = 1'b0;
    if (!(DBG_EN && dbg_mode)) begin
      y = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_condition_unary_not_logical_and_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConditionInsideRangeBitwiseOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_condition_inside_range_bitwise_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_condition_inside_range_bitwise_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_inside_range_bitwise_or_supported(
  input  logic [11:0] csr_addr_i,
  output logic        hit
);
  always_comb begin
    hit = 1'b0;
    if (csr_addr_i inside {[12'hC03:12'hC1F]} |
        csr_addr_i inside {[12'hC83:12'hC9F]}) begin
      hit = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_condition_inside_range_bitwise_or_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("hit")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConditionInsideWildcardSetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_condition_inside_wildcard_set_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_condition_inside_wildcard_set_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_inside_wildcard_set_supported(
  input  logic [3:0] op_i,
  output logic       hit_o
);
  always_comb begin
    hit_o = 1'b0;
    if (op_i inside {{2'b10, 2'b?1}, 4'b0??0}) begin
      hit_o = 1'b1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_condition_inside_wildcard_set_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("hit_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombConditionalFunctionCallRHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_conditional_function_call_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_conditional_function_call_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_conditional_function_call_rhs_supported #(
  parameter bit RVA = 1'b1
) (
  input  logic [2:0]  addr_i,
  input  logic        instr_is_amo_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  function automatic [63:0] data_align(logic [2:0] addr, logic [63:0] data);
    logic [63:0] data_tmp = {64{1'b0}};
    case (addr)
      3'b000: data_tmp[63:0] = data[63:0];
      3'b001: data_tmp[63:0] = {data[55:0], data[63:56]};
      default: data_tmp = data;
    endcase
    return data_tmp[63:0];
  endfunction

  always_comb begin
    data_o = ((RVA && instr_is_amo_i) ? data_i : data_align(addr_i, data_i));
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_conditional_function_call_rhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombRHSConditionalParamEqSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_conditional_param_eq_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_rhs_conditional_param_eq_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_conditional_param_eq_supported #(
  parameter int unsigned DEPTH = 4
) (
  input  logic [63:0] a,
  input  logic [63:0] b,
  output logic [63:0] y
);
  always_comb begin
    y = (DEPTH == 0) ? a : b;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_rhs_conditional_param_eq_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionNegatedParamStructMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_condition_negated_param_struct_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_condition_negated_param_struct_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic DebugEn;
} cfg_t;

module always_comb_condition_negated_param_struct_member_access_supported #(
  parameter cfg_t Cfg = '{DebugEn: 1'b1}
) (
  input  logic       debug_mode_q,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  output logic [7:0] y_o
);
  always_comb begin
    y_o = a_i;
    if (!(Cfg.DebugEn && debug_mode_q)) begin
      y_o = b_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_condition_negated_param_struct_member_access_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombConditionMemberAccessDynamicIndexMulSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_condition_member_access_dynamic_index_mul_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_condition_member_access_dynamic_index_mul_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic       locked;
  logic [6:0] pad;
} pmpcfg_t;

module always_comb_condition_member_access_dynamic_index_mul_supported(
  input  logic [11:0] idx_i,
  output logic        y_o
);
  pmpcfg_t [63:0] pmpcfg_q;

  always_comb begin
    automatic logic [11:0] index = idx_i;
    y_o = 1'b0;
    for (int i = 0; i < 2; i++) begin
      if (!pmpcfg_q[index*4+i].locked) begin
        y_o = 1'b1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_condition_member_access_dynamic_index_mul_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectLoopConstBaseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_loop_const_base_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_loop_const_base_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic       locked;
  logic [6:0] pad;
} pmpcfg_t;

module always_comb_rhs_indexed_range_select_loop_const_base_supported(
  input  logic [11:0] idx_i,
  input  logic [63:0] csr_wdata_i,
  output logic [7:0]  y_o
);
  pmpcfg_t [63:0] pmpcfg_q, pmpcfg_d;

  always_comb begin
    automatic logic [11:0] index = idx_i;
    pmpcfg_d = pmpcfg_q;
    y_o = '0;
    for (int i = 0; i < 2; i++) begin
      if (!pmpcfg_q[index*4+i].locked) begin
        pmpcfg_d[index*4+i] = csr_wdata_i[i*8+:8];
        y_o = csr_wdata_i[i*8+:8];
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_rhs_indexed_range_select_loop_const_base_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectMemberBaseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_member_base_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_member_base_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(typedef struct packed {
  logic [63:0] data;
} data_wrap_t;

module always_comb_rhs_indexed_range_select_member_base_supported(
  input  logic [3:0] idx_i,
  input  logic [63:0] data_i,
  output logic [7:0] y_o
);
  data_wrap_t wrap_q;
  always_comb begin
    wrap_q.data = data_i;
    y_o = wrap_q.data[idx_i*8+:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_rhs_indexed_range_select_member_base_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectConstantBaseHasFixedRangeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_constant_base_has_fixed_range_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_constant_base_has_fixed_range_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_constant_base_has_fixed_range_supported(
  input  logic [63:0] in0_i,
  input  logic [63:0] in1_i,
  output logic [7:0]  y_o
);
  logic [63:0] arr [0:1];
  always_comb begin
    arr[0] = in0_i;
    arr[1] = in1_i;
    y_o = arr[1][16+:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_rhs_indexed_range_select_constant_base_has_fixed_range_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectConstantConcatFallbackIndexedDownSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_constant_concat_fallback_indexed_down_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_constant_concat_fallback_indexed_down_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_constant_concat_fallback_indexed_down_supported(
  input  logic [31:0] hi_i,
  input  logic [31:0] lo_i,
  output logic [7:0]  y_o
);
  always_comb begin
    y_o = {hi_i, lo_i}[23-:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_constant_concat_fallback_indexed_down_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectConstantSelectorFixedRangeFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "always_comb_rhs_indexed_range_select_constant_selector_fixed_range_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_constant_selector_fixed_range_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_constant_selector_fixed_range_fallback_supported(
  output logic [15:0] y_o
);
  logic [63:0][7:0] arr;
  always_comb begin
    y_o[7:0]  = arr[12+:8];
    y_o[15:8] = arr[19-:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_constant_selector_fixed_range_fallback_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectLargeSelectorMultiplyFallbackIndexedDownSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_down_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_down_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_down_supported(
  input  logic [127:0] data_i,
  input  logic [63:0]  idx_i,
  output logic [15:0]  y_o
);
  always_comb begin
    y_o[7:0]   = data_i[idx_i*8-:8];
    y_o[15:8]  = data_i[8*idx_i-:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_down_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectLargeSelectorMultiplyFallbackIndexedUpSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_up_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_up_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_up_supported(
  input  logic [127:0] data_i,
  input  logic [63:0]  idx_i,
  output logic [15:0]  y_o
);
  always_comb begin
    y_o[7:0]   = data_i[idx_i*8+:8];
    y_o[15:8]  = data_i[8*idx_i+:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_large_selector_multiply_fallback_indexed_up_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectScalarLateConstantSelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_scalar_late_constant_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_scalar_late_constant_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_scalar_late_constant_selector_supported(
  input  logic [3:0] data_i,
  input  logic       sel_i,
  output logic       y_o
);
  always_comb begin
    y_o = data_i[(sel_i ? 1'b1 : 1'b1)*1+:1];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_scalar_late_constant_selector_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectScalarSymbolicSelectorMergeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_scalar_symbolic_selector_merge_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_scalar_symbolic_selector_merge_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_scalar_symbolic_selector_merge_supported(
  input  logic a_i,
  input  logic idx_i,
  output logic y_o
);
  always_comb begin
    y_o = {a_i, 1'b0}[idx_i*1+:1];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_scalar_symbolic_selector_merge_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSIndexedRangeSelectLoopNonPowerConstantFactorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath /
            "always_comb_rhs_indexed_range_select_loop_non_power_constant_factor_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_indexed_range_select_loop_non_power_constant_factor_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_indexed_range_select_loop_non_power_constant_factor_supported(
  input  logic [63:0] data_i,
  output logic [7:0]  y_o
);
  logic [7:0] tmp [0:3];

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      tmp[i] = data_i[i*8+:8];
    end
    y_o = tmp[3];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_rhs_indexed_range_select_loop_non_power_constant_factor_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombRHSConditionalUnpackedDynamicSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_conditional_unpacked_dynamic_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_rhs_conditional_unpacked_dynamic_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_conditional_unpacked_dynamic_select_supported(
  input  logic [63:0] in0,
  input  logic [63:0] in1,
  input  logic        idx,
  input  logic        sel,
  output logic [63:0] y
);
  logic [63:0] mem [0:1];
  always_comb begin
    mem[0] = in0;
    mem[1] = in1;
    y = sel ? mem[idx] : in0;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_rhs_conditional_unpacked_dynamic_select_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSElementSelectUnpackedSingleBitElementWidthFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_rhs_element_select_unpacked_single_bit_element_width_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_element_select_unpacked_single_bit_element_width_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_element_select_unpacked_single_bit_element_width_fallback_supported(
  input  logic mem [0:1][0:0],
  input  logic idx,
  output logic y [0:0]
);
  always_comb begin
    y = mem[idx];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_rhs_element_select_unpacked_single_bit_element_width_fallback_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombRHSUnpackedIndexedRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_unpacked_indexed_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_rhs_unpacked_indexed_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_unpacked_indexed_range_select_supported(
  input  logic [3:0] idx,
  output logic [63:0] y
);
  logic [63:0][7:0] arr;
  always_comb begin
    y = arr[idx*4+:8];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_rhs_unpacked_indexed_range_select_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombRHSStreamingConcatenationSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_streaming_concatenation_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_rhs_streaming_concatenation_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_supported(
  input  logic [15:0] data_i,
  output logic [15:0] data_o
);
  always_comb begin
    data_o = {<<8{data_i}};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_rhs_streaming_concatenation_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombRHSStreamingConcatenationWithClauseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_streaming_concatenation_with_clause_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_rhs_streaming_concatenation_with_clause_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_with_clause_supported(
  input  logic [7:0] data_i [0:3],
  output logic [15:0] data_o
);
  always_comb begin
    data_o = {<<8{data_i with [1+:2]}};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_rhs_streaming_concatenation_with_clause_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSStreamingConcatenationImplicitSliceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_streaming_concatenation_implicit_slice_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_rhs_streaming_concatenation_implicit_slice_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_implicit_slice_supported(
  input  logic [15:0] data_i,
  output logic [15:0] data_o
);
  always_comb begin
    data_o = {>>{data_i}};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_rhs_streaming_concatenation_implicit_slice_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSStreamingConcatenationWithClauseSubArraySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_streaming_concatenation_with_clause_sub_array_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_rhs_streaming_concatenation_with_clause_sub_array_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_with_clause_sub_array_supported(
  input  logic [7:0] data_i [0:3][0:1],
  output logic [31:0] data_o
);
  always_comb begin
    data_o = {<<8{data_i with [1+:2]}};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_rhs_streaming_concatenation_with_clause_sub_array_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSStreamingConcatenationDynamicWithWidthUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_streaming_concatenation_dynamic_with_width_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_rhs_streaming_concatenation_dynamic_with_width_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_dynamic_with_width_unsupported(
  input  logic [7:0] data_i [0:3],
  input  logic [1:0] len_i,
  output logic [31:0] data_o
);
  always_comb begin
    data_o = {<<{data_i with [0+:len_i]}};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module "
     "'always_comb_rhs_streaming_concatenation_dynamic_with_width_unsupported'",
     "unable to resolve always_comb RHS bits for Streaming"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSStreamingConcatenationUnpackedOperandUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_rhs_streaming_concatenation_unpacked_operand_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_rhs_streaming_concatenation_unpacked_operand_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_unpacked_operand_unsupported(
  input  logic [7:0] data_i [0:3],
  output logic [31:0] data_o
);
  always_comb begin
    data_o = {<<{data_i}};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module "
     "'always_comb_rhs_streaming_concatenation_unpacked_operand_unsupported'",
     "unable to resolve always_comb RHS bits for Streaming"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSStreamingConcatenationFunctionOperandResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "always_comb_rhs_streaming_concatenation_function_operand_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_rhs_streaming_concatenation_function_operand_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_streaming_concatenation_function_operand_resolve_failure_unsupported(
  input  logic [15:0] data_i,
  output logic [15:0] data_o
);
  function automatic logic [15:0] bad_fn(input string x);
    logic [15:0] tmp;
    tmp = 16'h0000;
    return tmp;
  endfunction

  always_comb begin
    data_o = {<<8{bad_fn("abc")}};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module "
     "'always_comb_rhs_streaming_concatenation_function_operand_resolve_failure_unsupported'",
     "unable to resolve always_comb RHS bits for Streaming"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSConditionalUnpackedDynamicSelectTypeParamSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_rhs_conditional_unpacked_dynamic_select_type_param_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_rhs_conditional_unpacked_dynamic_select_type_param_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_conditional_unpacked_dynamic_select_type_param_supported #(
  parameter int unsigned DATA_WIDTH = 32,
  parameter int unsigned DEPTH = 8,
  parameter type dtype = logic [DATA_WIDTH-1:0],
  parameter int unsigned ADDR_DEPTH = (DEPTH > 1) ? $clog2(DEPTH) : 1
) (
  input  dtype                     data_i,
  input  logic [ADDR_DEPTH-1:0]    read_pointer_q,
  output dtype                     data_o
);
  dtype [((DEPTH > 0) ? DEPTH : 1) - 1:0] mem_q;
  always_comb begin
    data_o = (DEPTH == 0) ? data_i : mem_q[read_pointer_q];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_rhs_conditional_unpacked_dynamic_select_type_param_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicElementSelectTypeParamSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_dynamic_element_select_type_param_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_dynamic_element_select_type_param_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_element_select_type_param_supported #(
  parameter int unsigned DATA_WIDTH = 64,
  parameter int unsigned DEPTH = 8,
  parameter type dtype = logic [DATA_WIDTH-1:0],
  parameter int unsigned ADDR_DEPTH = (DEPTH > 1) ? $clog2(DEPTH) : 1
) (
  input  dtype                     data_i,
  input  logic [ADDR_DEPTH-1:0]    write_pointer_q
);
  dtype [((DEPTH > 0) ? DEPTH : 1) - 1:0] mem_q;
  dtype [((DEPTH > 0) ? DEPTH : 1) - 1:0] mem_n;
  always_comb begin
    mem_n = mem_q;
    mem_n[write_pointer_q] = data_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_lhs_dynamic_element_select_type_param_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicElementSelectMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_dynamic_element_select_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_dynamic_element_select_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_element_select_member_access_supported #(
  parameter int unsigned DATA_WIDTH = 64
) (
  input  logic [DATA_WIDTH-1:0] data_i,
  input  logic                  sel_i,
  output logic [DATA_WIDTH-1:0] out_o
);
  typedef struct packed {
    logic [1:0][DATA_WIDTH-1:0] slot;
  } state_t;

  state_t q;
  state_t d;

  always_comb begin
    d = q;
    for (int i = 0; i < 2; i++) begin
      if (sel_i)
        d.slot[i] = data_i;
    end
    out_o = d.slot[0];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_lhs_dynamic_element_select_member_access_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicUnpackedArrayElementSelectMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_dynamic_unpacked_array_element_select_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_lhs_dynamic_unpacked_array_element_select_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_unpacked_array_element_select_member_access_supported #(
  parameter int unsigned NR_ROWS = 4,
  parameter int unsigned INSTR_PER_FETCH = 2
) (
  input  logic [$clog2(NR_ROWS)-1:0]         row_i,
  input  logic [$clog2(INSTR_PER_FETCH)-1:0] col_i,
  input  logic                               en_i,
  output logic                               valid_o
);
  typedef struct packed {
    logic        valid;
    logic [31:0] target_address;
  } pred_t;

  pred_t table_q[NR_ROWS-1:0][INSTR_PER_FETCH-1:0];
  pred_t table_d[NR_ROWS-1:0][INSTR_PER_FETCH-1:0];

  always_comb begin
    table_d = table_q;
    if (en_i) begin
      table_d[row_i][col_i].valid = 1'b1;
    end
    valid_o = table_d[row_i][col_i].valid;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_lhs_dynamic_unpacked_array_element_select_member_access_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("valid_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSAutomaticLocalSimpleRangeSelectConstBoundsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_automatic_local_simple_range_select_const_bounds_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_lhs_automatic_local_simple_range_select_const_bounds_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_automatic_local_simple_range_select_const_bounds_supported(
  input  logic        op_mod_i,
  input  logic        negate_i,
  output logic [63:0] out_o
);
  localparam int unsigned INT_WIDTH = 64;
  logic [INT_WIDTH-1:0] out_q;

  always_comb begin
    automatic logic [INT_WIDTH-1:0] special_res;
    special_res[INT_WIDTH-2:0] = '1;
    special_res[INT_WIDTH-1]   = op_mod_i;
    if (negate_i)
      special_res = ~special_res;
    out_q = special_res;
  end

  assign out_o = out_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_lhs_automatic_local_simple_range_select_const_bounds_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombGeneratedPackageLocalparamSpecialResultSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_generated_package_localparam_special_result_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_generated_package_localparam_special_result_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package fp_width_pkg;
  typedef enum logic [0:0] {W32 = 1'b0, W64 = 1'b1} int_format_e;
  function automatic int unsigned int_width(input int_format_e fmt);
    case (fmt)
      W32: return 32;
      default: return 64;
    endcase
  endfunction
endpackage

module always_comb_generated_package_localparam_special_result_supported(
  input  logic            op_mod_i,
  input  logic            negate_i,
  output logic [1:0][63:0] out_o
);
  import fp_width_pkg::*;

  localparam int unsigned NUM_INT_FORMATS = 2;
  logic [NUM_INT_FORMATS-1:0][63:0] ifmt_special_result;

  for (genvar ifmt = 0; ifmt < int'(NUM_INT_FORMATS); ifmt++) begin : gen_special_results_int
    localparam int unsigned INT_WIDTH = fp_width_pkg::int_width(int_format_e'(ifmt));

    always_comb begin
      automatic logic [INT_WIDTH-1:0] special_res;
      special_res[INT_WIDTH-2:0] = '1;
      special_res[INT_WIDTH-1] = op_mod_i;
      if (negate_i)
        special_res = ~special_res;
      ifmt_special_result[ifmt] = '{default: special_res[INT_WIDTH-1]};
      ifmt_special_result[ifmt][INT_WIDTH-1:0] = special_res;
    end
  end

  assign out_o = ifmt_special_result;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_generated_package_localparam_special_result_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parsePackageConstantNetCacheIsPerDesignSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "package_constant_net_cache_is_per_design_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "package_constant_net_cache_is_per_design_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(package fpnew_pkg;
  typedef struct packed {
    logic [4:0] exp_bits;
    logic [5:0] man_bits;
  } fp_encoding_t;
  localparam fp_encoding_t [0:1] FP_ENCODINGS = '{
    '{5'd8, 6'd23},
    '{5'd11, 6'd52}
  };
endpackage

module package_constant_net_cache_is_per_design_leaf(
  input  logic        sel_i,
  input  logic        idx_i,
  output logic [4:0]  y_o
);
  import fpnew_pkg::*;
  always_comb begin
    if (sel_i)
      y_o = FP_ENCODINGS[idx_i].exp_bits;
    else
      y_o = 5'd0;
  end
endmodule

module package_constant_net_cache_is_per_design_supported(
  input  logic        sel_i,
  input  logic        idx_i,
  output logic [4:0]  a_o,
  output logic [4:0]  b_o
);
  import fpnew_pkg::*;
  logic [4:0] parent_q;
  always_comb begin
    if (sel_i)
      parent_q = FP_ENCODINGS[idx_i].exp_bits;
    else
      parent_q = 5'd0;
  end
  assign a_o = parent_q;
  package_constant_net_cache_is_per_design_leaf u_leaf(
    .sel_i(sel_i),
    .idx_i(idx_i),
    .y_o(b_o)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("package_constant_net_cache_is_per_design_supported"));
  ASSERT_NE(top, nullptr);
  auto leaf =
    library_->getSNLDesign(NLName("package_constant_net_cache_is_per_design_leaf"));
  ASSERT_NE(leaf, nullptr);

  auto* topPkgNet = top->getNet(NLName("fpnew_pkg_FP_ENCODINGS"));
  auto* leafPkgNet = leaf->getNet(NLName("fpnew_pkg_FP_ENCODINGS"));
  ASSERT_NE(topPkgNet, nullptr);
  ASSERT_NE(leafPkgNet, nullptr);
  EXPECT_NE(topPkgNet, leafPkgNet);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombUnpackedRangeAssignmentPatternSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_unpacked_range_assignment_pattern_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_unpacked_range_assignment_pattern_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_range_assignment_pattern_supported(
  output logic [4:0] out_o
);
  logic events[5:1];

  always_comb begin
    events[5:1] = '{default: 0};
    out_o = {events[5], events[4], events[3], events[2], events[1]};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_unpacked_range_assignment_pattern_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombUnpackedRangeAssignmentPatternSymbolicBoundSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_unpacked_range_assignment_pattern_symbolic_bound_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_unpacked_range_assignment_pattern_symbolic_bound_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_range_assignment_pattern_symbolic_bound_supported(
  output logic [5:0] out_o
);
  localparam int MHPMCounterNum = 6;
  logic events[MHPMCounterNum:1];

  always_comb begin
    events[MHPMCounterNum:1] = '{default: 0};
    out_o = {events[6], events[5], events[4], events[3], events[2], events[1]};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_unpacked_range_assignment_pattern_symbolic_bound_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSIndexedRangeSelectDynamicBaseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_indexed_range_select_dynamic_base_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_indexed_range_select_dynamic_base_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_indexed_range_select_dynamic_base_supported(
  input  logic [2:0] idx_i,
  output logic [15:0] out_o
);
  logic [1:0][7:0] lanes;

  always_comb begin
    lanes = '0;
    lanes[0][idx_i+:4] = 4'hF;
    out_o = lanes;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("always_comb_lhs_indexed_range_select_dynamic_base_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSIndexedDownRangeSelectDynamicBaseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_indexed_down_range_select_dynamic_base_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_indexed_down_range_select_dynamic_base_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_indexed_down_range_select_dynamic_base_supported(
  input  logic [2:0] idx_i,
  output logic [15:0] out_o
);
  logic [1:0][7:0] lanes;

  always_comb begin
    lanes = '0;
    lanes[1][idx_i-:4] = 4'hF;
    out_o = lanes;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_lhs_indexed_down_range_select_dynamic_base_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSIndexedRangeSelectDynamicBaseBitsResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_indexed_range_select_dynamic_base_bits_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_lhs_indexed_range_select_dynamic_base_bits_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_indexed_range_select_dynamic_base_bits_resolve_failure_unsupported;
  logic [7:0] data_o;
  always_comb begin
    data_o = '0;
    data_o[$urandom_range(3, 0)+:1] = 1'b1;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve dynamic indexed range-select base bits in always_comb assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSIndexedRangeSelectDynamicBaseOutOfRangeUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_indexed_range_select_dynamic_base_out_of_range_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_lhs_indexed_range_select_dynamic_base_out_of_range_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_indexed_range_select_dynamic_base_out_of_range_unsupported(
  input  logic [3:0] idx_i,
  output logic [3:0] data_o
);
  always_comb begin
    data_o = '0;
    data_o[idx_i+:5] = 5'b11111;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"dynamic indexed range-select out of range in always_comb assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSSimpleRangeSelectOutOfRangeUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_simple_range_select_out_of_range_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_simple_range_select_out_of_range_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_simple_range_select_out_of_range_unsupported(
  input  logic [4:0] data_i,
  output logic [3:0] data_o
);
  always_comb begin
    data_o = '0;
    data_o[5:1] = data_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"constant range-select out of range in always_comb assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicElementSelectScalarSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_dynamic_element_select_scalar_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_dynamic_element_select_scalar_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_element_select_scalar_supported(
  input  logic sel_i,
  input  logic data_i,
  output logic [1:0] out_o
);
  logic [1:0] mem_q;
  logic [1:0] mem_n;
  always_comb begin
    mem_n = mem_q;
    mem_n[sel_i] = data_i;
    out_o = mem_n;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_lhs_dynamic_element_select_scalar_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSIndexedRangeSelectLoopNonPowerConstantFactorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_lhs_indexed_range_select_loop_non_power_constant_factor_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "always_comb_lhs_indexed_range_select_loop_non_power_constant_factor_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_indexed_range_select_loop_non_power_constant_factor_supported(
  input  logic [1:0] i,
  output logic [3:0] o
);
  localparam int safe_expand_lp = 2;
  always_comb
    for (integer k = 0; k < 2; k++)
      o[safe_expand_lp*k+:safe_expand_lp] = {safe_expand_lp{i[k]}};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName(
      "always_comb_lhs_indexed_range_select_loop_non_power_constant_factor_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSConstantElementSelectOutOfRangeUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_constant_element_select_out_of_range_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_constant_element_select_out_of_range_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_constant_element_select_out_of_range_unsupported(
  input logic [63:0] data_i
);
  logic [63:0] mem_q [0:1];
  logic [63:0] mem_n [0:1];
  always_comb begin
    mem_n = mem_q;
    mem_n[2] = data_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"constant element-select index out of range in always_comb assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicElementSelectUnknownBitsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_dynamic_element_select_unknown_bits_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_dynamic_element_select_unknown_bits_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_element_select_unknown_bits_unsupported(
  input  logic        sel_i,
  input  logic [63:0] data_i
);
  logic [63:0] mem_q [0:1];
  logic [63:0] mem_n [0:1];
  always_comb begin
    mem_n = mem_q;
    mem_n[sel_i ? 1'b0 : 1'bx] = data_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve dynamic index bits in always_comb assignment LHS"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicElementSelectCompoundAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_dynamic_element_select_compound_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_dynamic_element_select_compound_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_element_select_compound_assignment_unsupported(
  input  logic        sel_i,
  input  logic [63:0] data_i
);
  logic [63:0] mem_q [0:1];
  logic [63:0] mem_n [0:1];
  always_comb begin
    mem_n = mem_q;
    mem_n[sel_i] += data_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported compound assignment in always_comb without current LHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSDynamicElementSelectIncrementUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_dynamic_element_select_increment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_dynamic_element_select_increment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_dynamic_element_select_increment_unsupported(
  input logic sel_i
);
  logic [63:0] mem_q [0:1];
  logic [63:0] mem_n [0:1];
  always_comb begin
    mem_n = mem_q;
    mem_n[sel_i]++;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported increment/decrement assignment in always_comb without current LHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSFullWidthIndexedDownRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_full_width_indexed_down_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_full_width_indexed_down_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_full_width_indexed_down_range_select_supported(
  input  logic [3:0] data_i,
  output logic [3:0] data_o
);
  always_comb begin
    data_o = 4'h0;
    data_o[3 -: 4] = data_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_lhs_full_width_indexed_down_range_select_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSFullWidthRangeSelectRhsResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_full_width_range_select_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_full_width_range_select_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_full_width_range_select_rhs_resolve_failure_unsupported(
  output logic [3:0] data_o
);
  always_comb begin
    data_o = 4'h0;
    data_o[3:0] = $urandom_range(4'hF, 4'h0);
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported combinational block in module "
     "'always_comb_lhs_full_width_range_select_rhs_resolve_failure_unsupported'",
     "unable to resolve always_comb RHS bits for"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombLHSElementSelectNonIntegralSliceUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_lhs_element_select_non_integral_slice_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_lhs_element_select_non_integral_slice_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_lhs_element_select_non_integral_slice_unsupported;
  logic [7:0] mem_q [0:1][0:1];
  logic [7:0] mem_n [0:1][0:1];
  always_comb begin
    mem_n = mem_q;
    mem_n[0] = mem_q[1];
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve always_comb element-select assignment width"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseAlwaysCombRHSMemberAccessElementSelectUnderBinarySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "always_comb_rhs_member_access_element_select_under_binary_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_comb_rhs_member_access_element_select_under_binary_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_member_access_element_select_under_binary_supported(
  input  logic [63:0] data_i,
  input  logic        en_i,
  output logic        out_o
);
  typedef struct packed {
    logic [1:0][63:0] slot;
  } state_t;

  state_t d;

  always_comb begin
    d.slot[0] = data_i;
    d.slot[1] = '0;
    out_o = d.slot[0][0] & en_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "always_comb_rhs_member_access_element_select_under_binary_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("out_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetBranchNestedConditionalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "sequential_reset_branch_nested_conditional_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "sequential_reset_branch_nested_conditional_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_reset_branch_nested_conditional_supported(
  input  logic       clk,
  input  logic       rst_n,
  input  logic       guard,
  input  logic [7:0] next_a,
  input  logic [7:0] next_b,
  output logic [7:0] a,
  output logic [7:0] b,
  output logic [7:0] c
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      a <= '0;
      b <= '0;
      if (guard) c <= '0;
    end else begin
      a <= next_a;
      b <= next_b;
      if (guard) c <= next_a;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("sequential_reset_branch_nested_conditional_supported"));
  ASSERT_NE(top, nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialAsyncResetUnaryPlusConditionUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "sequential_async_reset_unary_plus_condition_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "sequential_async_reset_unary_plus_condition_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_async_reset_unary_plus_condition_unsupported(
  input  logic clk,
  input  logic rst_n,
  input  logic d,
  output logic q
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (+rst_n)
      q <= 1'b0;
    else
      q <= d;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported sequential block in module "
     "'sequential_async_reset_unary_plus_condition_unsupported'",
     "unable to resolve reset condition net"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialAsyncResetConditionalConditionUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "sequential_async_reset_conditional_condition_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "sequential_async_reset_conditional_condition_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_async_reset_conditional_condition_unsupported(
  input  logic clk,
  input  logic rst_n,
  input  logic sel,
  input  logic d0,
  input  logic d1,
  output logic q0,
  output logic q1
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (sel ? rst_n : 1'b0)
    begin
      q0 <= 1'b0;
      q1 <= 1'b0;
    end else begin
      q0 <= d0;
      q1 <= d1;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported sequential block in module "
     "'sequential_async_reset_conditional_condition_unsupported'",
     "unable to resolve reset condition net"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysNoTimingNoClkSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "always_no_timing_no_clk_skipped" / "always_no_timing_no_clk_skipped.sv");
    FAIL() << "Expected unsupported untimed sequential statement exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported statement while extracting sequential timing control"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialListSingleEmptyStatementUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_list_single_empty_stmt_unsupported" /
      "seq_list_single_empty_stmt_unsupported.sv");
    FAIL() << "Expected unsupported single-list empty statement exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported statement while extracting sequential timing control"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialMultiAssignmentResetEmptyStatementSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_empty_statement_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_assignment_reset_empty_statement_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_empty_statement_supported(
  input  logic clk,
  input  logic rst,
  input  logic d0,
  input  logic d1,
  output logic q0,
  output logic q1
);
  always_ff @(posedge clk or posedge rst) begin
    if (rst) begin
      ;
      q0 <= 1'b0;
      q1 <= 1'b0;
    end else begin
      q0 <= d0;
      q1 <= d1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_multi_assignment_reset_empty_statement_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffreModel = NLDB0::getDFFRE();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffreModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);
  size_t dffCount = 0;
  size_t dffreCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffreModel) {
      ++dffreCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(2u, dffreCount);
  EXPECT_EQ(0u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseUnaryNotCreatesNOutputGate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "unary_not" / "unary_not.sv");

  auto top = library_->getSNLDesign(NLName("unary_not_top"));
  ASSERT_NE(top, nullptr);

  size_t notGateCount = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
      continue;
    }
    if (NLDB0::isGate(inst->getModel()) &&
        NLDB0::getGateName(inst->getModel()) == "not") {
      ++notGateCount;
    }
  }

  EXPECT_EQ(1u, notGateCount);
  EXPECT_EQ(1u, assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "unary_not");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseUnaryNotOnBinaryOperatorsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "unary_not_binary_supported" / "unary_not_binary_supported.sv");

  auto top = library_->getSNLDesign(NLName("unary_not_binary_supported_top"));
  ASSERT_NE(top, nullptr);
  const std::array<const char*, 3> outputs{
    "y_nand",
    "y_nor",
    "y_xnor"};
  for (const auto* output : outputs) {
    auto net = top->getNet(NLName(output));
    ASSERT_NE(net, nullptr);
    auto bitNet = dynamic_cast<SNLBitNet*>(net);
    ASSERT_NE(bitNet, nullptr);
    EXPECT_FALSE(bitNet->getInstTerms().empty());
  }

  size_t nandGateCount = 0;
  size_t norGateCount = 0;
  size_t xnorGateCount = 0;
  size_t otherGateCount = 0;
  size_t assignCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
      continue;
    }
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    auto gateName = NLDB0::getGateName(inst->getModel());
    if (gateName == "nand") {
      ++nandGateCount;
    } else if (gateName == "nor") {
      ++norGateCount;
    } else if (gateName == "xnor") {
      ++xnorGateCount;
    } else {
      ++otherGateCount;
    }
  }

  EXPECT_EQ(1u, nandGateCount);
  EXPECT_EQ(1u, norGateCount);
  EXPECT_EQ(1u, xnorGateCount);
  EXPECT_EQ(0u, otherGateCount);
  EXPECT_EQ(outputs.size(), assignCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "unary_not_binary_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseUnaryNotOnUnsupportedBinaryOperatorFails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "unary_not_binary_unsupported" / "unary_not_binary_unsupported.sv");
    FAIL() << "Expected unsupported unary-not binary operator exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary operator under bitwise not in continuous assign: +"));

    auto top = library_->getSNLDesign(NLName("unary_not_binary_unsupported_top"));
    ASSERT_NE(top, nullptr);
    auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "unary_not_binary_unsupported");
    EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseBinaryOperatorsUnsupportedFails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "binary_ops_unsupported" / "binary_ops_unsupported.sv");
    FAIL() << "Expected unsupported binary operator exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary operator in continuous assign"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousRelationalOpsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_relational_ops_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_relational_ops_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_relational_ops_supported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  input  logic       inv,
  output logic       gt,
  output logic       ge,
  output logic       lt,
  output logic       le,
  output logic       mix
);
  assign gt = a > b;
  assign ge = a >= b;
  assign lt = a < b;
  assign le = a <= b;
  assign mix = ((a == b) | ((a > b) ^ inv)) & ((|a) | (b == 4'b0));
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_relational_ops_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("gt")), nullptr);
  EXPECT_NE(top->getNet(NLName("ge")), nullptr);
  EXPECT_NE(top->getNet(NLName("lt")), nullptr);
  EXPECT_NE(top->getNet(NLName("le")), nullptr);
  EXPECT_NE(top->getNet(NLName("mix")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousRelationalGeneratedPowerThresholdSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_relational_generated_power_threshold_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_relational_generated_power_threshold_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_relational_generated_power_threshold_supported #(
  parameter int width_lp = 4
) (
  input  logic [width_lp-1:0] size_i,
  output logic [width_lp-1:0] wrap_sel_o
);
  for (genvar i = 0; i < width_lp; i++) begin : gen_sel
    assign wrap_sel_o[i] = size_i >= 2**i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("continuous_relational_generated_power_threshold_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("wrap_sel_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousRelationalOpsSignedSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_relational_ops_signed_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_relational_ops_signed_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_relational_ops_signed_supported(
  input  logic signed [3:0] a,
  input  logic signed [3:0] b,
  output logic              gt,
  output logic              ge,
  output logic              lt,
  output logic              le
);
  assign gt = a > b;
  assign ge = a >= b;
  assign lt = a < b;
  assign le = a <= b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("continuous_relational_ops_signed_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("gt")), nullptr);
  EXPECT_NE(top->getNet(NLName("ge")), nullptr);
  EXPECT_NE(top->getNet(NLName("lt")), nullptr);
  EXPECT_NE(top->getNet(NLName("le")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousRelationalBusLHSUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_relational_bus_lhs_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_relational_bus_lhs_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_relational_bus_lhs_unsupported(
  input  logic [3:0] a,
  input  logic [3:0] b,
  output logic [3:0] y
);
  assign y = a < b;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported relational expression with bus LHS";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: <"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousRelationalNonIntegralOperandUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "continuous_relational_non_integral_operand_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "continuous_relational_non_integral_operand_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module continuous_relational_non_integral_operand_unsupported(
  input  logic [3:0] a,
  output logic       y
);
  assign y = real'(a) < 1.0;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported binary expression in continuous assign: <"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousBinaryExpressionFallbacksUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(
      benchmarksPath /
      "continuous_binary_expression_fallbacks_unsupported" /
      "continuous_binary_expression_fallbacks_unsupported.sv"),
    SNLSVConstructorException);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousResolveExpressionBitsFailurePathsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(
      benchmarksPath /
      "continuous_resolve_expression_bits_failure_paths_unsupported" /
      "continuous_resolve_expression_bits_failure_paths_unsupported.sv"),
    SNLSVConstructorException);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousUnbasedUnsizedLiteralPathsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath /
      "continuous_unbased_unsized_literals_unsupported" /
      "continuous_unbased_unsized_literals_unsupported.sv");
    FAIL() << "Expected unsupported unbased unsized literal in continuous assign";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectPackedScalarUnderAdd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_packed_scalar_under_add";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_packed_scalar_under_add.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_packed_scalar_under_add_top(
  input logic [3:0] a,
  output logic y
);
  assign y = a[0] + 1'b1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("element_select_packed_scalar_under_add_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseMultiplyRightOperandResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "multiply_right_operand_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "multiply_right_operand_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module multiply_right_operand_resolve_failure_unsupported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  assign y = a * int'(1.5);
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("multiply_right_operand_resolve_failure_unsupported_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseMultiplyLeftOperandResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "multiply_left_operand_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "multiply_left_operand_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module multiply_left_operand_resolve_failure_unsupported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  assign y = 4'bx001 * a;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported binary expression in continuous assign: *"});
}

TEST_F(SNLSVConstructorTestSimple, parseIntegralCastOfRealUnderAddUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "integral_cast_of_real_under_add_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "integral_cast_of_real_under_add_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module integral_cast_of_real_under_add_unsupported_top(
  input logic [7:0] b,
  output logic [7:0] y
);
  assign y = int'(1.5) + b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("integral_cast_of_real_under_add_unsupported_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseNonIntegralCastUnderAddSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "non_integral_cast_under_add_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "non_integral_cast_under_add_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module non_integral_cast_under_add_supported_top(
  input logic [7:0] a,
  input logic [7:0] b,
  output logic [7:0] y
);
  assign y = shortreal'(a) + b;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("non_integral_cast_under_add_supported_top"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
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
  constructor.construct(benchmarksPath/"up_counter"/"up_counter.sv", options);

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
      if (hasRTLInfo(inst, "sv_src_line")) {
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

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "up_counter_dump");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));

  ASSERT_TRUE(std::filesystem::exists(jsonPath));
  std::ifstream jsonFile(jsonPath);
  ASSERT_TRUE(jsonFile.good());
  std::string json{
    std::istreambuf_iterator<char>(jsonFile),
    std::istreambuf_iterator<char>()};
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("\"up_counter\""), std::string::npos);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddOutPlusOne) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_add_out_plus_one" / "seq_add_out_plus_one.sv");

  auto top = library_->getSNLDesign(NLName("seq_add_out_plus_one"));
  ASSERT_NE(top, nullptr);

  size_t faCount = 0;
  size_t dffCount = 0;
  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_GT(faCount, 0u);
  EXPECT_EQ(8u, dffCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_add_out_plus_one");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddOnePlusOut) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_add_one_plus_out" / "seq_add_one_plus_out.sv");

  auto top = library_->getSNLDesign(NLName("seq_add_one_plus_out"));
  ASSERT_NE(top, nullptr);

  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_GT(faCount, 0u);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_add_one_plus_out");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiResetAddIncrementFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_reset_add_increment_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_reset_add_increment_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_reset_add_increment_fallback_supported(
  input  logic       clk,
  input  logic       rst,
  output logic [7:0] q_left,
  output logic [7:0] q_right
);
  always_ff @(posedge clk) begin
    if (rst) begin
      q_left <= 8'h00;
      q_right <= 8'h00;
    end else begin
      q_left <= q_left + 8'h01;
      q_right <= 8'h01 + q_right;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_multi_reset_add_increment_fallback_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(16u, dffCount);
  EXPECT_GT(faCount, 0u);

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "seq_multi_reset_add_increment_fallback_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiResetUnaryIncrementFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_reset_unary_increment_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_reset_unary_increment_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_reset_unary_increment_fallback_supported(
  input  logic       clk,
  input  logic       rst,
  output logic [7:0] q_left,
  output logic [7:0] q_right
);
  always_ff @(posedge clk) begin
    if (rst) begin
      q_left <= 8'h00;
      q_right <= 8'h00;
    end else begin
      q_left++;
      ++q_right;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_multi_reset_unary_increment_fallback_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(16u, dffCount);
  EXPECT_GT(faCount, 0u);

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "seq_multi_reset_unary_increment_fallback_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiResetSubtractFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_reset_subtract_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_reset_subtract_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_reset_subtract_fallback_supported(
  input  logic       clk,
  input  logic       rst,
  output logic [7:0] q_bad,
  output logic [7:0] q_hold
);
  always_ff @(posedge clk) begin
    if (rst) begin
      q_bad <= 8'h00;
      q_hold <= 8'h00;
    end else begin
      q_bad <= q_bad - 8'h01;
      q_hold <= q_hold;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_reset_subtract_fallback_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffreModel = NLDB0::getDFFRE();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffreModel, nullptr);
  size_t faCount = 0;
  size_t flopCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel || inst->getModel() == dffreModel) {
      ++flopCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(16u, flopCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiResetAddNonIncrementFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_reset_add_non_increment_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_reset_add_non_increment_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_reset_add_non_increment_fallback_supported(
  input  logic       clk,
  input  logic       rst,
  output logic [7:0] q_bad,
  output logic [7:0] q_hold
);
  always_ff @(posedge clk) begin
    if (rst) begin
      q_bad <= 8'h00;
      q_hold <= 8'h00;
    end else begin
      q_bad <= q_bad + 8'h02;
      q_hold <= q_hold;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_reset_add_non_increment_fallback_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffreModel = NLDB0::getDFFRE();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffreModel, nullptr);
  size_t faCount = 0;
  size_t flopCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel || inst->getModel() == dffreModel) {
      ++flopCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(16u, flopCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialAddSubtractAccumulatorFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_add_subtract_accumulator_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_add_subtract_accumulator_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_add_subtract_accumulator_fallback_supported(
  input  logic       clk,
  input  logic       rst,
  input  logic       v_i,
  input  logic       ready_i,
  input  logic       yumi_i,
  output logic [1:0] count_o
);
  always_ff @(posedge clk) begin
    if (rst) begin
      count_o <= 2'b00;
    end else begin
      count_o <= count_o + v_i - (ready_i & yumi_i);
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_add_subtract_accumulator_fallback_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(2u, dffCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableElseDefaultSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_enable_else_default" / "seq_enable_else_default.sv");

  auto top = library_->getSNLDesign(NLName("seq_enable_else_default"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_GT(faCount, 0u);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_enable_else_default");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialMultiAssignmentElseBlockSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_block_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_assignment_else_block_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_block_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       valid_i,
  input  logic       flush_i,
  input  logic       q0_d,
  input  logic [7:0] q1_d,
  input  logic [7:0] q2_d,
  output logic       q0,
  output logic [7:0] q1,
  output logic [7:0] q2
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0 <= 1'b0;
      q1 <= '0;
      q2 <= '0;
    end else begin
      if (valid_i) begin
        q1 <= q1_d;
        q2 <= q2_d;
      end
      if (flush_i) begin
        q0 <= 1'b0;
      end else if (valid_i) begin
        q0 <= q0_d;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_multi_assignment_else_block_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  size_t ffCount = 0;
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(17u, ffCount);
  EXPECT_EQ(9u, countMux2Instances(top));
  EXPECT_EQ(3u, countMux2Instances(top, 1));
  EXPECT_EQ(6u, countMux2Instances(top, 8));

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "seq_multi_assignment_else_block_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseIfConstantFalseWithoutElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_if_constant_false_without_else_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_else_if_constant_false_without_else_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_if_constant_false_without_else_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else if (1'b0) begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_else_if_constant_false_without_else_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q0_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("q1_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentSingleResetLhsElseMultiLhsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_single_reset_lhs_else_multi_lhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_single_reset_lhs_else_multi_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_single_reset_lhs_else_multi_lhs_supported(
  input  logic       clk_i,
  input  logic       reset_i,
  input  logic       en_i,
  input  logic       q0_d,
  input  logic [7:0] q1_d,
  input  logic [7:0] q2_d,
  output logic       q0_o,
  output logic [7:0] q1_o,
  output logic [7:0] q2_o
);
  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      q0_o <= 1'b0;
    end else begin
      q0_o <= q0_d;
      if (en_i) begin
        q1_o <= q1_d;
        q2_o <= q2_d;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_single_reset_lhs_else_multi_lhs_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(17u, dffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseBlockConditionResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_block_condition_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_else_block_condition_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_block_condition_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [3:0] a_i,
  input  logic       d0_i,
  input  logic       d1_i,
  output logic       q0_o,
  output logic       q1_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      if (bad_cond(a_i)) begin
        q0_o <= d0_i;
        q1_o <= d1_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve single-bit condition net while lowering conditional"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseBlockConditionalElseUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_block_conditional_else_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_else_block_conditional_else_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_block_conditional_else_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic hold_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      if (en_i) begin
        q0_o <= d0_i;
        q1_o <= d1_i;
      end else while (hold_i) begin
        q0_o <= d0_i;
        q1_o <= d1_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while lowering sequential block"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseBlockUnsupportedWhileStatementUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_multi_assignment_else_block_unsupported_while_statement_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_else_block_unsupported_while_statement_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_block_unsupported_while_statement_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      while (en_i) begin
        q0_o <= d0_i;
        q1_o <= d1_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while lowering sequential block"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetImmediateAssertionIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_immediate_assertion_ignored_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_immediate_assertion_ignored_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_immediate_assertion_ignored_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      assert (1'b1) else $error("disabled");
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_reset_immediate_assertion_ignored_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffrnModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffrnCount = 0;
  size_t muxCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    } else if (inst->getModel() == mux2Model) {
      ++muxCount;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(2u, dffrnCount);
  EXPECT_EQ(0u, muxCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetBranchCollectionFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_multi_assignment_reset_branch_collection_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_branch_collection_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_branch_collection_failure_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic hold_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      while (hold_i) begin
        q0_o <= 1'b0;
        q1_o <= 1'b0;
      end
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetBranchConditionalIfTrueCollectionFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "seq_multi_assignment_reset_branch_conditional_iftrue_collection_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "seq_multi_assignment_reset_branch_conditional_iftrue_collection_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_branch_conditional_iftrue_collection_failure_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic sel_i,
  input  logic hold_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      if (sel_i) begin
        while (hold_i) begin
          q0_o <= 1'b0;
          q1_o <= 1'b0;
        end
      end else begin
        q0_o <= 1'b0;
        q1_o <= 1'b0;
      end
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetBranchConditionalElseCollectionFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "seq_multi_assignment_reset_branch_conditional_else_collection_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "seq_multi_assignment_reset_branch_conditional_else_collection_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_branch_conditional_else_collection_failure_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic sel_i,
  input  logic hold_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      if (sel_i) begin
        q0_o <= 1'b0;
        q1_o <= 1'b0;
      end else begin
        while (hold_i) begin
          q0_o <= 1'b0;
          q1_o <= 1'b0;
        end
      end
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetBranchCaseItemCollectionFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_multi_assignment_reset_branch_case_item_collection_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_branch_case_item_collection_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_branch_case_item_collection_failure_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic sel_i,
  input  logic hold_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      case (sel_i)
        1'b0: begin
          while (hold_i) begin
            q0_o <= 1'b0;
            q1_o <= 1'b0;
          end
        end
        default: begin
          q0_o <= 1'b0;
          q1_o <= 1'b0;
        end
      endcase
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetBranchCaseDefaultCollectionFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "seq_multi_assignment_reset_branch_case_default_collection_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "seq_multi_assignment_reset_branch_case_default_collection_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_branch_case_default_collection_failure_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic sel_i,
  input  logic hold_i,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      case (sel_i)
        1'b0: begin
          q0_o <= 1'b0;
          q1_o <= 1'b0;
        end
        default: begin
          while (hold_i) begin
            q0_o <= 1'b0;
            q1_o <= 1'b0;
          end
        end
      endcase
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetBranchWithoutAssignmentsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_multi_assignment_reset_branch_without_assignments_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_branch_without_assignments_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_branch_without_assignments_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      ;
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"reset branch does not contain assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseBlockVariableDeclarationsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_block_variable_declarations_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_else_block_variable_declarations_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_block_variable_declarations_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       valid_i,
  input  logic       q0_d,
  input  logic [7:0] q1_d,
  output logic       q0,
  output logic [7:0] q1
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      logic reset_local;
      q0 <= 1'b0;
      q1 <= '0;
    end else begin
      logic enable_local;
      if (valid_i) begin
        q0 <= q0_d;
        q1 <= q1_d;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_else_block_variable_declarations_supported"));
  ASSERT_NE(top, nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(9u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseBlockForLoopSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_block_for_loop_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_assignment_else_block_for_loop_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_block_for_loop_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       q0_d,
  input  logic [7:0] q1_d,
  output logic       q0,
  output logic [7:0] q1
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      for (int i = 0; i < 2; i++) begin
        if (i < 1) begin
          q0 <= 1'b0;
          q1 <= '0;
        end else begin
          q0 <= 1'b0;
          q1 <= '0;
        end
      end
    end else begin
      for (int i = 0; i < 2; i++) begin
        if (i < 1) begin
          q0 <= q0_d;
          q1 <= q1_d;
        end else begin
          q0 <= q0_d;
          q1 <= q1_d;
        end
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_multi_assignment_else_block_for_loop_supported"));
  ASSERT_NE(top, nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(9u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetForLoopElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_for_loop_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_for_loop_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_for_loop_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] q0_d,
  input  logic [7:0] q1_d,
  input  logic [7:0] r0_d,
  input  logic [7:0] r1_d,
  output logic [7:0] q0,
  output logic [7:0] q1,
  output logic [7:0] r0,
  output logic [7:0] r1
);
  logic [7:0] q [0:1];
  logic [7:0] r [0:1];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      for (int i = 0; i < 2; i++) begin
        if (i < 1) begin
          q[i] <= 8'h01;
          r[i] <= 8'h10;
        end else begin
          q[i] <= 8'h02;
          r[i] <= 8'h20;
        end
      end
    end else begin
      q[0] <= q0_d;
      q[1] <= q1_d;
      r[0] <= r0_d;
      r[1] <= r1_d;
    end
  end

  assign q0 = q[0];
  assign q1 = q[1];
  assign r0 = r[0];
  assign r1 = r[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_reset_for_loop_element_select_supported"));
  ASSERT_NE(top, nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(32u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialForLoopDirectLoopVariableConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_for_loop_direct_loop_variable_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_for_loop_direct_loop_variable_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_for_loop_direct_loop_variable_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       d0_i,
  input  logic       d1_i,
  output logic       q0_o,
  output logic       q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      for (int i = 0; i < 2; i++) begin
        if (i) begin
          q0_o <= d0_i;
          q1_o <= d1_i;
        end else begin
          q0_o <= 1'b0;
          q1_o <= 1'b0;
        end
      end
      for (int j = 0; j < 2; j++) begin
        if (j >>> 0) begin
          q0_o <= d0_i;
          q1_o <= d1_i;
        end else begin
          q0_o <= 1'b0;
          q1_o <= 1'b0;
        end
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_for_loop_direct_loop_variable_condition_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q0_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("q1_o")), nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
        ++ffCount;
    }
  }
  EXPECT_EQ(2u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialForLoopDirectLoopVariableRHSBitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_for_loop_direct_loop_variable_rhs_bit_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_for_loop_direct_loop_variable_rhs_bit_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_for_loop_direct_loop_variable_rhs_bit_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      for (int i = 0; i < 2; i++) begin
        if (en_i) begin
          q0_o <= i;
          q1_o <= i;
        end else begin
          q0_o <= 1'b0;
          q1_o <= 1'b1;
        end
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_for_loop_direct_loop_variable_rhs_bit_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q0_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("q1_o")), nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(2u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetDynamicElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_dynamic_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_dynamic_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_dynamic_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       idx_i,
  input  logic [7:0] q_d,
  input  logic [7:0] r_d,
  output logic [7:0] q0,
  output logic [7:0] q1,
  output logic [7:0] r0,
  output logic [7:0] r1
);
  logic [7:0] q [0:1];
  logic [7:0] r [0:1];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q[idx_i] <= 8'h01;
      r[idx_i] <= 8'h10;
    end else begin
      q[idx_i] <= q_d;
      r[idx_i] <= r_d;
    end
  end

  assign q0 = q[0];
  assign q1 = q[1];
  assign r0 = r[0];
  assign r1 = r[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_reset_dynamic_element_select_supported"));
  ASSERT_NE(top, nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(32u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetDynamicElementSelectPartialConstantSelectorSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "seq_multi_assignment_reset_dynamic_element_select_partial_constant_selector_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "seq_multi_assignment_reset_dynamic_element_select_partial_constant_selector_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_dynamic_element_select_partial_constant_selector_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       idx_i,
  input  logic [7:0] q_d,
  input  logic [7:0] r_d,
  output logic [7:0] q0,
  output logic [7:0] q1,
  output logic [7:0] q2,
  output logic [7:0] q3,
  output logic [7:0] r0,
  output logic [7:0] r1,
  output logic [7:0] r2,
  output logic [7:0] r3
);
  logic [7:0] q [0:3];
  logic [7:0] r [0:3];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q[{1'b0, idx_i}] <= 8'h01;
      r[{1'b0, idx_i}] <= 8'h10;
    end else begin
      q[{1'b0, idx_i}] <= q_d;
      r[{1'b0, idx_i}] <= r_d;
    end
  end

  assign q0 = q[0];
  assign q1 = q[1];
  assign q2 = q[2];
  assign q3 = q[3];
  assign r0 = r[0];
  assign r1 = r[1];
  assign r2 = r[2];
  assign r3 = r[3];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName(
    "seq_multi_assignment_reset_dynamic_element_select_partial_constant_selector_supported"));
  ASSERT_NE(top, nullptr);

  size_t ffCount = 0;
  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
  }
  EXPECT_EQ(64u, ffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetElementSelectCompoundAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_element_select_compound_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_element_select_compound_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_element_select_compound_assignment_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       idx_i
);
  logic [1:0][7:0] q;
  logic [1:0][7:0] r;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q <= '0;
      r <= '0;
    end else begin
      q[idx_i] += 8'h01;
      r[idx_i] += 8'h01;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported sequential assignment action for element-select LHS"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialDirectMultiAssignmentUnsupportedRHSActionReported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_direct_multi_assignment_unsupported_rhs_action_reported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_direct_multi_assignment_unsupported_rhs_action_reported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_direct_multi_assignment_unsupported_rhs_action_reported(
  input  logic       clk_i,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  input  logic [7:0] c_i,
  output logic [7:0] q_o,
  output logic [7:0] r_o
);
  always_ff @(posedge clk_i) begin
    q_o <= a_i / b_i;
    r_o <= c_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to lower assignment action for LHS 'q_o'"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialDirectMultiAssignmentConcatLHSTargetUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_direct_multi_assignment_concat_lhs_target_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_direct_multi_assignment_concat_lhs_target_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_direct_multi_assignment_concat_lhs_target_unsupported(
  input  logic       clk_i,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  output logic [7:0] q_o,
  output logic [7:0] r_o
);
  always_ff @(posedge clk_i) begin
    {q_o, r_o} <= {a_i, b_i};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"direct multi-assignment LHS is not a supported integral target"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialDirectMultiAssignmentUnsupportedIntegralSelectionTargetUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath /
    "seq_direct_multi_assignment_unsupported_integral_selection_target_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath /
    "seq_direct_multi_assignment_unsupported_integral_selection_target_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_direct_multi_assignment_unsupported_integral_selection_target_unsupported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [7:0] d_i,
  output logic [7:0] r_o
);
  logic [7:0] q [0:1][0:1];

  always_ff @(posedge clk_i) begin
    q[sel_i ? 1'b0 : 1'bx] <= '{default: d_i};
    r_o <= d_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"direct multi-assignment LHS is not a supported integral selection"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialSingleLHSFallbackMultipleDirectAssignmentsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_single_lhs_fallback_multiple_direct_assignments_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_single_lhs_fallback_multiple_direct_assignments_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_single_lhs_fallback_multiple_direct_assignments_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       a_i,
  input  logic       b_i,
  output logic       q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q_o <= 1'b0;
    end else begin
      q_o <= a_i;
      q_o <= b_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"single-LHS fallback path contains multiple direct assignments"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialSingleLHSFallbackMismatchedResetTargetUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_single_lhs_fallback_mismatched_reset_target_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_single_lhs_fallback_mismatched_reset_target_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_single_lhs_fallback_mismatched_reset_target_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic d_i,
  output logic q_o,
  output logic r_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q_o <= 1'b0;
    end else begin
      r_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"fallback currently supports only multi-LHS reset branches"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombSignedLocalparamConstantSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_signed_localparam_constant_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_signed_localparam_constant_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_signed_localparam_constant_supported(
  output logic [7:0] y_o
);
  localparam integer NEG = -3;
  always_comb begin
    y_o = NEG;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_signed_localparam_constant_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseConcatRHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_concat_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_else_concat_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_concat_rhs_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  input  logic [7:0] c_i,
  output logic [7:0] q_o,
  output logic [7:0] r_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q_o <= '0;
      r_o <= '0;
    end else begin
      {q_o, r_o} <= {a_i / b_i, c_i};
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to lower sequential concatenation assignment for"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialPartialAssignmentRHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_partial_assignment_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_partial_assignment_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_partial_assignment_rhs_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  output logic [7:0] q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q_o <= '0;
    end else begin
      q_o[0] <= a_i / b_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to lower concrete sequential partial assignment for LHS"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetElementSelectRHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_element_select_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_element_select_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_element_select_rhs_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       idx_i,
  input  logic [3:0] a_i
);
  logic [1:0][7:0] q;
  logic [1:0][7:0] r;

  function automatic logic [7:0] bad_fn(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_fn = 8'hFF;
      default:          bad_fn = 8'h00;
    endcase
  endfunction

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q <= '0;
      r <= '0;
    end else begin
      q[idx_i] <= bad_fn(a_i);
      r[idx_i] <= bad_fn(a_i);
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve sequential element-select RHS bits"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetConstantElementSelectOutOfRangeUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_multi_assignment_reset_constant_element_select_out_of_range_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_constant_element_select_out_of_range_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_constant_element_select_out_of_range_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] q_d,
  input  logic [7:0] r_d
);
  logic [1:0][7:0] q;
  logic [1:0][7:0] r;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q <= '0;
      r <= '0;
    end else begin
      q[2] <= q_d;
      r[2] <= r_d;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"constant element-select index out of range in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetConstantElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_constant_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_constant_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_constant_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] q_d,
  input  logic [7:0] r_d,
  output logic [7:0] q_o,
  output logic [7:0] r_o
);
  logic [1:0][7:0] q;
  logic [1:0][7:0] r;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q <= '0;
      r <= '0;
    end else begin
      q[1] <= q_d;
      r[0] <= r_d;
    end
  end

  assign q_o = q[1];
  assign r_o = r[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_reset_constant_element_select_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("r_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetConstantBitElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_constant_bit_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_constant_bit_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_constant_bit_element_select_supported(
  input  logic clk_i,
  input  logic rst_ni,
  output logic q_o,
  output logic r_o
);
  logic q [0:1];
  logic r [0:1];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q[0] <= 1'b0;
      q[1] <= 1'b0;
      r[0] <= 1'b0;
      r[1] <= 1'b0;
    end else begin
      q[1] <= 1'b1;
      r[0] <= 1'b0;
    end
  end

  assign q_o = q[1];
  assign r_o = r[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_reset_constant_bit_element_select_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("r_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetDynamicElementSelectUnknownBitsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "seq_multi_assignment_reset_dynamic_element_select_unknown_bits_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_reset_dynamic_element_select_unknown_bits_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_dynamic_element_select_unknown_bits_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       sel_i,
  input  logic [7:0] q_d,
  input  logic [7:0] r_d
);
  logic [1:0][7:0] q;
  logic [1:0][7:0] r;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q <= '0;
      r <= '0;
    end else begin
      q[sel_i ? 1'b0 : 1'bx] <= q_d;
      r[sel_i ? 1'b0 : 1'bx] <= r_d;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve dynamic index bits in sequential assignment LHS"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableElseDefaultNonAssignmentSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_else_default_non_assignment" /
      "seq_enable_else_default_non_assignment.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableElseDefaultLHSMismatchSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_else_default_lhs_mismatch" /
      "seq_enable_else_default_lhs_mismatch.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableBus1Supported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_enable_bus1_supported" / "seq_enable_bus1_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_enable_bus1_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_enable_bus1_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableHoldUsesDFFESupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_hold_uses_dffe_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_hold_uses_dffe_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_hold_uses_dffe_supported(
  input logic       clk,
  input logic       en,
  input logic [7:0] d,
  output logic [7:0] q
);
  always_ff @(posedge clk) begin
    if (en) q <= d;
    else q <= q;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_hold_uses_dffe_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffeModel = NLDB0::getDFFE();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffeModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffeCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffeModel) {
      ++dffeCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(8u, dffeCount);
  EXPECT_EQ(0u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialNegedgeEnableHoldUsesDFFNSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_negedge_enable_hold_uses_dffn_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_negedge_enable_hold_uses_dffn_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_negedge_enable_hold_uses_dffn_supported(
  input logic       clk,
  input logic       en,
  input logic [7:0] d,
  output logic [7:0] q
);
  always_ff @(negedge clk) begin
    if (en) q <= d;
    else q <= q;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_negedge_enable_hold_uses_dffn_supported"));
  ASSERT_NE(top, nullptr);

  auto dffnModel = NLDB0::getDFFN();
  auto dffeModel = NLDB0::getDFFE();
  ASSERT_NE(dffnModel, nullptr);
  ASSERT_NE(dffeModel, nullptr);

  size_t dffnCount = 0;
  size_t dffeCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffnModel) {
      ++dffnCount;
    } else if (inst->getModel() == dffeModel) {
      ++dffeCount;
    } else if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(8u, dffnCount);
  EXPECT_EQ(0u, dffeCount);
  EXPECT_EQ(1u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialNegedgeResetUsesDFFNAndMuxSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_negedge_reset_uses_dffn_and_mux_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_negedge_reset_uses_dffn_and_mux_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_negedge_reset_uses_dffn_and_mux_supported(
  input logic       clk,
  input logic       rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  always_ff @(negedge clk) begin
    if (rst) q <= '0;
    else q <= d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_negedge_reset_uses_dffn_and_mux_supported"));
  ASSERT_NE(top, nullptr);

  auto dffnModel = NLDB0::getDFFN();
  ASSERT_NE(dffnModel, nullptr);

  size_t dffnCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffnModel) {
      ++dffnCount;
    } else if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(8u, dffnCount);
  EXPECT_EQ(1u, mux2Count);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialNegedgeResetMultiAssignmentUsesDFFNSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_negedge_reset_multi_assignment_uses_dffn_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_negedge_reset_multi_assignment_uses_dffn_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_negedge_reset_multi_assignment_uses_dffn_supported(
  input logic       clk,
  input logic       rst,
  input logic [7:0] a_d,
  input logic [7:0] b_d,
  output logic [7:0] a_q,
  output logic [7:0] b_q
);
  always_ff @(negedge clk) begin
    if (rst) begin
      a_q <= '0;
      b_q <= '0;
    end else begin
      a_q <= a_d;
      b_q <= b_d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_negedge_reset_multi_assignment_uses_dffn_supported"));
  ASSERT_NE(top, nullptr);

  auto dffnModel = NLDB0::getDFFN();
  ASSERT_NE(dffnModel, nullptr);

  size_t dffnCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffnModel) {
      ++dffnCount;
    } else if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(16u, dffnCount);
  EXPECT_EQ(2u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableBus2Skipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_bus2_skipped" / "seq_enable_bus2_skipped.sv",
    {"unable to resolve enable condition net"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialPreincrementSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_preincrement_supported" / "seq_preincrement_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_preincrement_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_GT(faCount, 0u);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_preincrement_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialDecrementSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_decrement_skipped" / "seq_decrement_skipped.sv",
    {"Unsupported decrement assignment in sequential block"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialModuloUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_modulo_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_modulo_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_modulo_unsupported(
  input  logic       clk,
  input  logic [7:0] d_i,
  output logic [7:0] q_o
);
  always_ff @(posedge clk) begin
    q_o <= q_o % d_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported binary operator in sequential assignment: %"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetOnlySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_reset_only_supported" / "seq_reset_only_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_reset_only_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_reset_only_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetAllZeroLiteralWideSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_reset_all_zero_literal_wide_supported" /
    "seq_reset_all_zero_literal_wide_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_reset_all_zero_literal_wide_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(128u, dffCount);
  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 128));
  EXPECT_EQ(0u, countMux2Instances(top, 1));

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_reset_all_zero_literal_wide_supported");
  const auto dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(128)"));
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSWideConstantSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_rhs_wide_const_supported" / "seq_rhs_wide_const_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_rhs_wide_const_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(128u, dffCount);
  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 128));
  EXPECT_EQ(0u, countMux2Instances(top, 1));

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_rhs_wide_const_supported");
  const auto dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(128)"));
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSUint64ConstantBranchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_rhs_uint64_constant_branch_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_rhs_uint64_constant_branch_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_rhs_uint64_constant_branch_supported(
  input  logic        clk,
  input  logic        rst,
  output logic [15:0] q
);
  // Exercises resolveConstantExpressionBits() branch that expands a uint64 literal.
  always_ff @(posedge clk) begin
    if (rst) begin
      q <= 16'hA55A;
    end else begin
      q <= 16'h1357;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_rhs_uint64_constant_branch_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(16u, dffCount);
  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 16));
  EXPECT_EQ(0u, countMux2Instances(top, 1));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSWideUnknownConstantLocalparamFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_rhs_wide_unknown_const_unsupported" /
    "seq_rhs_wide_unknown_const_unsupported.sv");

  auto top = library_->getSNLDesign(NLName("seq_rhs_wide_unknown_const_unsupported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(128u, dffCount);
  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 128));
  EXPECT_EQ(0u, countMux2Instances(top, 1));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetStructDefaultZeroSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_reset_struct_default_zero_supported" /
      "seq_reset_struct_default_zero_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_reset_struct_default_zero_supported"));
  ASSERT_NE(top, nullptr);

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(13u, dffrnCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetStructMemberSettersWithDefaultZeroSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_struct_member_setters_with_default_zero_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_reset_struct_member_setters_with_default_zero_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_struct_member_setters_with_default_zero_supported(
  input  logic clk,
  input  logic rst_n,
  input  logic [6:0] d_bits,
  output logic [6:0] q_bits
);
  typedef struct packed {
    logic [3:0] xdebugver;
    logic [1:0] prv;
    logic       step;
  } dcsr_t;

  dcsr_t q, d;
  assign d = dcsr_t'(d_bits);
  assign q_bits = q;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      q <= '{xdebugver: 4'h4, prv: 2'b11, default: '0};
    end else begin
      q <= d;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_reset_struct_member_setters_with_default_zero_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffrnModel, nullptr);
  size_t ffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel || inst->getModel() == dffrnModel) {
      ++ffCount;
    }
  }
  EXPECT_EQ(7u, ffCount);
  EXPECT_EQ(1u, countMux2Instances(top));
  EXPECT_EQ(1u, countMux2Instances(top, 7));
  EXPECT_EQ(0u, countMux2Instances(top, 1));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialConcatLHSSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_concat_lhs_skipped" / "seq_concat_lhs_skipped.sv",
    {"unable to resolve assignment LHS net"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetElseConcatenationSizeMismatchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_else_concat_size_mismatch_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_else_concat_size_mismatch_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_else_concat_size_mismatch_supported(
  input  logic clk_i,
  input  logic rst_i,
  input  logic d_i,
  output logic q0_o,
  output logic q1_o,
  output logic q2_o
);
  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      {q0_o, q1_o} <= 2'b00;
    end else begin
      {q0_o, q1_o, q2_o} <= {d_i, d_i, d_i};
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_else_concat_size_mismatch_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("q0_o")), nullptr);
  EXPECT_NE(top->getScalarNet(NLName("q1_o")), nullptr);
  EXPECT_NE(top->getScalarNet(NLName("q2_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentElseConcatLHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_else_concat_lhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_assignment_else_concat_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_else_concat_lhs_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic d0_i,
  input  logic d1_i,
  output logic q0_o,
  output logic q1_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q0_o <= 1'b0;
      q1_o <= 1'b0;
    end else begin
      {q0_o, q1_o} <= {d0_i, d1_i};
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_multi_assignment_else_concat_lhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getScalarNet(NLName("q0_o")), nullptr);
  EXPECT_NE(top->getScalarNet(NLName("q1_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentResetConcatLHSSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_reset_concat_lhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_multi_assignment_reset_concat_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_reset_concat_lhs_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic d0_i,
  input  logic d1_i,
  input  logic d2_i,
  output logic q0_o,
  output logic q1_o,
  output logic q2_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      {q0_o, q1_o} <= 2'b00;
      q2_o <= 1'b0;
    end else begin
      q0_o <= d0_i;
      q1_o <= d1_i;
      q2_o <= d2_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_reset_concat_lhs_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffrnModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffrnCount = 0;
  size_t muxCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    } else if (inst->getModel() == mux2Model) {
      ++muxCount;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(3u, dffrnCount);
  EXPECT_EQ(0u, muxCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialLHSElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_lhs_element_select_skipped" / "seq_lhs_element_select_skipped.sv");

  auto top = library_->getSNLDesign(NLName("seq_lhs_element_select_skipped"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("q")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialLHSScalarBitSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_lhs_scalar_bit_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_lhs_scalar_bit_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_lhs_scalar_bit_select_supported(
  input  logic       clk_i,
  input  logic       d_i,
  output logic [1:0] q_o
);
  always_ff @(posedge clk_i) begin
    q_o[0] <= d_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_lhs_scalar_bit_select_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetNonAssignmentSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_reset_non_assignment_skipped" /
      "seq_reset_non_assignment_skipped.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableIfTrueNonAssignmentSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_iftrue_non_assignment_skipped" /
      "seq_enable_iftrue_non_assignment_skipped.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableIfTrueExpressionStatementUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_iftrue_expression_statement_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_iftrue_expression_statement_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_iftrue_expression_statement_unsupported(
  input logic       clk,
  input logic       rst,
  input logic       en,
  input logic [7:0] d,
  output logic [7:0] q
);
  function automatic logic [7:0] passthrough(input logic [7:0] value);
    passthrough = value;
  endfunction

  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en) passthrough(d);
    else q <= q;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableLHSMismatchNoDefaultSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_lhs_mismatch_no_default" /
      "seq_enable_lhs_mismatch_no_default.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialDefaultLHSMismatchSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_default_lhs_mismatch_skipped" /
      "seq_default_lhs_mismatch_skipped.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialDefaultNonAssignmentSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_default_non_assignment_skipped" /
      "seq_default_non_assignment_skipped.sv",
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondBinarySkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_cond_binary_skipped" / "seq_enable_cond_binary_skipped.sv",
    {"unable to resolve enable condition net"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondConditionalUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_conditional_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_conditional_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_conditional_unsupported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic sel_i,
  input  logic a_i,
  input  logic b_i,
  input  logic d_i,
  output logic q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (sel_i ? a_i : b_i) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported ternary enable condition";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential block in module "
                  "'seq_enable_cond_conditional_unsupported': "
                  "unable to resolve enable condition net"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondCaseEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_case_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_case_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_case_equality_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic d_i,
  output logic q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (en_i === 1'b1) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_case_equality_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondCaseInequalitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_case_inequality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_case_inequality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_case_inequality_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic d_i,
  output logic q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (en_i !== 1'b0) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_case_inequality_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialEnableCondInequalityResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_inequality_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_enable_cond_inequality_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_inequality_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [3:0] a_i,
  input  logic       d_i,
  output logic       q_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (bad_cond(a_i) != 1'b0) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported enable condition with inequality resolution failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential block in module "
                  "'seq_enable_cond_inequality_resolve_failure_unsupported': "
                  "unable to resolve enable condition net"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondLogicalAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_logical_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_and_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic d_i,
  output logic q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (en_i && (d_i === 1'b1)) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_logical_and_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialEnableCondLogicalAndReductionOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_and_reduction_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_logical_and_reduction_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_and_reduction_or_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       hold_i,
  input  logic [2:0] csr_valid_i,
  input  logic       d_i,
  output logic       q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if ((~hold_i) && (|csr_valid_i)) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_enable_cond_logical_and_reduction_or_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondBitwiseAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_bitwise_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_bitwise_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_bitwise_and_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic d_i,
  output logic q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (en_i & (d_i === 1'b1)) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_bitwise_and_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);

  size_t andGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "and") {
      ++andGateCount;
    }
  }
  EXPECT_GE(andGateCount, 1u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondLogicalOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_logical_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_or_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic a_i,
  input  logic b_i,
  input  logic d_i,
  output logic q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (a_i || b_i) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_logical_or_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);

  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetIndexedLHSTernaryHoldSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_indexed_lhs_ternary_hold_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_indexed_lhs_ternary_hold_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_indexed_lhs_ternary_hold_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic en_i,
  input  logic d_i,
  output logic q_o
);
  logic [2:0] q;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q[2] <= 1'b0;
    end else begin
      q[2] <= en_i ? d_i : q[2];
    end
  end

  assign q_o = q[2];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_indexed_lhs_ternary_hold_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(1u, dffrnCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetIndexedLHSNestedTernarySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_indexed_lhs_nested_ternary_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_indexed_lhs_nested_ternary_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_indexed_lhs_nested_ternary_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic clear_i,
  input  logic en_i,
  input  logic d_i,
  output logic q_o
);
  logic [2:0] q;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q[2] <= 1'b0;
    end else begin
      q[2] <= clear_i ? 1'b0 : en_i ? d_i : q[2];
    end
  end

  assign q_o = q[2];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_indexed_lhs_nested_ternary_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(1u, dffrnCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondLogicalOrRelationalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_or_relational_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_logical_or_relational_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_or_relational_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       a_i,
  input  logic [2:0] count_i,
  input  logic       d_i,
  output logic       q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (a_i || (count_i > 3'd2)) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_logical_or_relational_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isGate(inst->getModel()) && NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_EQ(1u, dffCount);
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondBitwiseOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_bitwise_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_bitwise_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_bitwise_or_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic enq_i,
  input  logic deq_i,
  output logic enq_r_o,
  output logic deq_r_o
);
  logic enq_r, deq_r;
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      enq_r <= 1'b0;
      deq_r <= 1'b1;
    end else begin
      if (enq_i | deq_i) begin
        enq_r <= enq_i;
        deq_r <= deq_i;
      end
    end
  end
  assign enq_r_o = enq_r;
  assign deq_r_o = deq_r;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_bitwise_or_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("enq_r_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("deq_r_o")), nullptr);

  size_t orGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (!NLDB0::isGate(inst->getModel())) {
      continue;
    }
    if (NLDB0::getGateName(inst->getModel()) == "or") {
      ++orGateCount;
    }
  }
  EXPECT_GE(orGateCount, 1u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialEnableCondLogicalAndLHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_and_lhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_enable_cond_logical_and_lhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_and_lhs_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [3:0] a_i,
  input  logic       d_i,
  output logic       q_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (bad_cond(a_i) && d_i) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported enable condition with lhs resolution failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential block in module "
                  "'seq_enable_cond_logical_and_lhs_resolve_failure_unsupported': "
                  "unable to resolve enable condition net"));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialEnableCondLogicalAndRHSResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_and_rhs_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_enable_cond_logical_and_rhs_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_and_rhs_resolve_failure_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [3:0] a_i,
  input  logic       d_i,
  output logic       q_o
);
  function automatic logic bad_cond(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_cond = 1'b1;
      default:          bad_cond = 1'b0;
    endcase
  endfunction
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (en_i && bad_cond(a_i)) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported enable condition with rhs resolution failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential block in module "
                  "'seq_enable_cond_logical_and_rhs_resolve_failure_unsupported': "
                  "unable to resolve enable condition net"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondLogicalShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_logical_shortcuts_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_logical_shortcuts_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_logical_shortcuts_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       a_i,
  input  logic       b_i,
  input  logic       d_i,
  output logic       q0_o,
  output logic       q1_o,
  output logic       q2_o,
  output logic       q3_o,
  output logic       q4_o,
  output logic       q5_o,
  output logic       q6_o,
  output logic       q7_o,
  output logic       q8_o,
  output logic       q9_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q0_o <= 1'b0;
    else if ((a_i && 1'b0) && b_i) q0_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q1_o <= 1'b0;
    else if ((a_i || 1'b1) || b_i) q1_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q2_o <= 1'b0;
    else if (a_i && 1'b0) q2_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q3_o <= 1'b0;
    else if (1'b1 && a_i) q3_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q4_o <= 1'b0;
    else if (a_i && 1'b1) q4_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q5_o <= 1'b0;
    else if (a_i && a_i) q5_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q6_o <= 1'b0;
    else if (a_i || 1'b1) q6_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q7_o <= 1'b0;
    else if (1'b0 || a_i) q7_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q8_o <= 1'b0;
    else if (a_i || 1'b0) q8_o <= d_i;
  end
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) q9_o <= 1'b0;
    else if (a_i || a_i) q9_o <= d_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_logical_shortcuts_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q0_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("q9_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAssignStructMemberRhsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_assign_struct_member_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_assign_struct_member_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_assign_struct_member_rhs_supported(
  input  logic clk_i,
  input  logic rst_ni,
  input  logic req_valid_i,
  output logic q_o
);
  typedef struct packed {
    logic valid;
  } req_t;
  req_t req_i;
  assign req_i.valid = req_valid_i;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else begin
      q_o <= req_i.valid;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_assign_struct_member_rhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondUnaryNotSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_enable_cond_unary_not_supported" /
    "seq_enable_cond_unary_not_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_unary_not_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);

  size_t dffCount = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (NLDB0::isGate(inst->getModel()) &&
               NLDB0::getGateName(inst->getModel()) == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(16u, dffCount);
  EXPECT_EQ(4u, countMux2Instances(top));
  EXPECT_EQ(4u, countMux2Instances(top, 8));
  EXPECT_EQ(0u, countMux2Instances(top, 1));
  EXPECT_EQ(4u, notGateCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_enable_cond_unary_not_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondUnaryRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_unary_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_enable_cond_unary_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_unary_range_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] data_i,
  input  logic       d_i,
  output logic       q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (!data_i[3+:1]) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_unary_range_select_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondUnaryRangeSelectWidthMismatchUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_unary_range_select_width_mismatch_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_enable_cond_unary_range_select_width_mismatch_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_unary_range_select_width_mismatch_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] data_i,
  input  logic       d_i,
  output logic       q_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (!data_i[3+:2]) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported enable condition with non-single-bit range select";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential block in module "
                  "'seq_enable_cond_unary_range_select_width_mismatch_unsupported': "
                  "unable to resolve enable condition net"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondUnaryRangeSelectUnknownIndexUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_enable_cond_unary_range_select_unknown_index_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_enable_cond_unary_range_select_unknown_index_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_enable_cond_unary_range_select_unknown_index_unsupported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [7:0] data_i,
  input  logic [3:0] sel_i,
  input  logic       d_i,
  output logic       q_o
);
  function automatic logic [2:0] bad_idx(input logic [3:0] op_i);
    case (op_i) inside
      [4'bxxxx : 4'd7]: bad_idx = 3'd1;
      default:          bad_idx = 3'd0;
    endcase
  endfunction
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (rst_ni === 1'b0) begin
      q_o <= 1'b0;
    end else if (!data_i[bad_idx(sel_i)+:1]) begin
      q_o <= d_i;
    end
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported enable condition with unresolved range-select index";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential block in module "
                  "'seq_enable_cond_unary_range_select_unknown_index_unsupported': "
                  "unable to resolve enable condition net"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondUnaryEdgeCasesSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_enable_cond_unary_edge_cases_skipped" /
      "seq_enable_cond_unary_edge_cases_skipped.sv",
    {"unable to resolve enable condition net"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetCondBus2Skipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_reset_cond_bus2_skipped" / "seq_reset_cond_bus2_skipped.sv",
    {"unable to resolve reset condition net"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialListSingleWrapperSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_list_single_wrapper_supported" /
    "seq_list_single_wrapper_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_list_single_wrapper_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_list_single_wrapper_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialNestedBeginWrapperSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_nested_begin_wrapper_supported" / "seq_nested_begin_wrapper_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_nested_begin_wrapper_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_nested_begin_wrapper_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialBinaryAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_binary_non_add" / "seq_binary_non_add.sv");

  auto top = library_->getSNLDesign(NLName("seq_binary_non_add"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);

  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialBitwiseSetClearSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_bitwise_set_clear_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_bitwise_set_clear_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_bitwise_set_clear_supported(
  input  logic       clk_i,
  input  logic       reset_i,
  input  logic [7:0] set_i,
  input  logic [7:0] clear_i,
  output logic [7:0] data_o
);
  logic [7:0] data_r;

  always_ff @(posedge clk_i)
    if (reset_i)
      data_r <= '0;
    else
      data_r <= (data_r & ~clear_i) | set_i;

  assign data_o = data_r;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_bitwise_set_clear_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);

  size_t dffCount = 0;
  size_t otherInstCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else {
      ++otherInstCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_GT(otherInstCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialNegedgeTimingSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_negedge_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_timing_negedge_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_negedge_supported(
  input logic clk,
  input logic load,
  input logic d,
  output logic q
);
  always_ff @(negedge clk) begin
    if (load) q <= d;
    else q <= q;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_timing_negedge_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffnModel = NLDB0::getDFFN();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffnModel, nullptr);

  size_t dffCount = 0;
  size_t dffnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffnModel) {
      ++dffnCount;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(1u, dffnCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialNegedgeAssertionOnlyConditionalIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_negedge_assertion_only_conditional_ignored_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_timing_negedge_assertion_only_conditional_ignored_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_negedge_assertion_only_conditional_ignored_supported(
  input logic clk_i,
  input logic reset_i,
  input logic v_i,
  input logic d_i,
  output logic q_o
);
  always_ff @(negedge clk_i) begin
    if (~reset_i) begin
      assert (v_i) else $error("v_i must stay high");
    end
  end

  assign q_o = d_i;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_timing_negedge_assertion_only_conditional_ignored_supported"));
  ASSERT_NE(top, nullptr);

  size_t assignCount = 0;
  size_t dffCount = 0;
  size_t dffnCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isAssign(inst->getModel())) {
      ++assignCount;
    } else if (inst->getModel() == NLDB0::getDFF()) {
      ++dffCount;
    } else if (inst->getModel() == NLDB0::getDFFN()) {
      ++dffnCount;
    }
  }
  EXPECT_EQ(1u, assignCount);
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(0u, dffnCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialNegedgeSystemTaskOnlyConditionalIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_negedge_system_task_only_conditional_ignored_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_timing_negedge_system_task_only_conditional_ignored_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_negedge_system_task_only_conditional_ignored_supported(
  input logic clk_i,
  input logic reset_i,
  input logic d_i,
  input logic underflow_i,
  output logic q_o
);
  always_ff @(posedge clk_i) begin
    if (reset_i) q_o <= 1'b0;
    else q_o <= d_i;
  end

  always_ff @(negedge clk_i) begin
    if (!reset_i && underflow_i)
      $display("underflow");
    if (~reset_i & d_i)
      $error("mismatch");
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_timing_negedge_system_task_only_conditional_ignored_supported"));
  ASSERT_NE(top, nullptr);

  size_t dffCount = 0;
  size_t dffnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == NLDB0::getDFF()) {
      ++dffCount;
    } else if (inst->getModel() == NLDB0::getDFFN()) {
      ++dffnCount;
    }
  }
  EXPECT_EQ(1u, dffCount);
  EXPECT_EQ(0u, dffnCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingListUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_timing_list_unsupported" / "seq_timing_list_unsupported.sv");
    FAIL() << "Expected unsupported statement-list timing extraction exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported statement list while extracting sequential timing control"));
    EXPECT_NE(std::string::npos, reason.find("size="));
    EXPECT_NE(std::string::npos, reason.find("first kind="));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingListTimedAssertionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_timing_list_timed_assertion_supported" /
    "seq_timing_list_timed_assertion_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_timing_list_timed_assertion_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(1u, dffCount);

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "seq_timing_list_timed_assertion_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingListTwoTimedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_timing_list_two_timed_unsupported" /
      "seq_timing_list_two_timed_unsupported.sv");
    FAIL() << "Expected unsupported multi-timed statement-list extraction exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported statement list while extracting sequential timing control"));
    EXPECT_NE(std::string::npos, reason.find("unsupported kind=Timed"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingMissingUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_timing_missing_unsupported" / "seq_timing_missing_unsupported.sv");
    FAIL() << "Expected unsupported missing timing control exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported statement while extracting sequential timing control"));
    EXPECT_NE(std::string::npos, reason.find("kind=Conditional"));
    EXPECT_NE(std::string::npos, reason.find("expected timed '@(...)' statement"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialConcurrentAssertionIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_concurrent_assertion_ignored" /
    "seq_concurrent_assertion_ignored.sv");

  auto top = library_->getSNLDesign(NLName("seq_concurrent_assertion_ignored"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);

  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(1u, dffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListNegedgeResetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_timing_event_list_negedge_reset_supported" /
    "seq_timing_event_list_negedge_reset_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_timing_event_list_negedge_reset_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffrnModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);
  size_t dffCount = 0;
  size_t dffrnCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(8u, dffrnCount);
  EXPECT_EQ(0u, mux2Count);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top,
    "seq_timing_event_list_negedge_reset_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListNonSignalUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_event_list_nonsignal_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_timing_event_list_nonsignal_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_event_list_nonsignal_unsupported(
  input logic clk,
  input logic trig,
  input logic d,
  output logic q
);
  always @(posedge clk or (posedge trig or negedge trig)) begin
    q <= d;
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported non-signal event-list timing exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListPosedgeResetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_timing_event_list_unsupported" /
    "seq_timing_event_list_unsupported.sv");

  auto top = library_->getSNLDesign(NLName("seq_timing_event_list_unsupported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffreModel = NLDB0::getDFFRE();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffreModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffreCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffreModel) {
      ++dffreCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(8u, dffreCount);
  EXPECT_EQ(0u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListPosedgeSetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_event_list_posedge_set_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_timing_event_list_posedge_set_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_event_list_posedge_set_supported(
  input logic clk,
  input logic set,
  input logic [7:0] d,
  output logic [7:0] q
);
  always_ff @(posedge clk or posedge set) begin
    if (set) q <= 8'hFF;
    else q <= d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_timing_event_list_posedge_set_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffseModel = NLDB0::getDFFSE();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffseModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffseCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffseModel) {
      ++dffseCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(8u, dffseCount);
  EXPECT_EQ(0u, mux2Count);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentTimingEventListPosedgeResetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_timing_event_list_posedge_reset_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_timing_event_list_posedge_reset_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_timing_event_list_posedge_reset_supported(
  input  logic clk,
  input  logic rst,
  input  logic d0,
  input  logic d1,
  output logic q0,
  output logic q1
);
  always_ff @(posedge clk or posedge rst) begin
    if (rst) begin
      q0 <= 1'b0;
      q1 <= 1'b0;
    end else begin
      q0 <= d0;
      q1 <= d1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_timing_event_list_posedge_reset_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffreModel = NLDB0::getDFFRE();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffreModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffreCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffreModel) {
      ++dffreCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(2u, dffreCount);
  EXPECT_EQ(0u, mux2Count);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialMultiAssignmentTimingEventListPosedgeSetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_multi_assignment_timing_event_list_posedge_set_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_multi_assignment_timing_event_list_posedge_set_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_multi_assignment_timing_event_list_posedge_set_supported(
  input  logic clk,
  input  logic set,
  input  logic d0,
  input  logic d1,
  output logic q0,
  output logic q1
);
  always_ff @(posedge clk or posedge set) begin
    if (set) begin
      q0 <= 1'b1;
      q1 <= 1'b1;
    end else begin
      q0 <= d0;
      q1 <= d1;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("seq_multi_assignment_timing_event_list_posedge_set_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffseModel = NLDB0::getDFFSE();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffseModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffseCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffseModel) {
      ++dffseCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(0u, dffCount);
  EXPECT_EQ(2u, dffseCount);
  EXPECT_EQ(0u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListMissingPosedgeUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_event_list_missing_posedge_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_timing_event_list_missing_posedge_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_event_list_missing_posedge_unsupported(
  input logic rst_n,
  input logic hold_n,
  input logic d,
  output logic q
);
  always @(negedge rst_n or negedge hold_n) begin
    q <= d;
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported event-list timing with missing posedge clock";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential event list; missing posedge clock event"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysStarCombinationalSnippetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_star_combinational_snippet_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_star_combinational_snippet_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module top(
  input logic req,
  input logic mode,
  input logic [3:0] data,
  output logic [3:0] result,
  output logic valid
);
  logic [3:0] next_value;

  always @* begin
    next_value = data;

    if (req) begin
      if (mode) begin
        next_value = data ^ 4'b1010;
      end else begin
        next_value = data + 4'b0001;
      end
    end
  end

  assign result = next_value;
  assign valid = req | mode;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);

  auto nextValue = top->getBusNet(NLName("next_value"));
  ASSERT_NE(nextValue, nullptr);
  EXPECT_EQ(4, nextValue->getWidth());

  auto result = top->getBusNet(NLName("result"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(4, result->getWidth());

  auto valid = dynamic_cast<SNLBitNet*>(top->getNet(NLName("valid")));
  ASSERT_NE(valid, nullptr);

  EXPECT_EQ(2u, countMux2Instances(top, 4));
  EXPECT_EQ(4u, countFAInstances(top));

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "always_star_combinational_snippet_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysExplicitSensitivityCombinationalSnippetSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_explicit_sensitivity_combinational_snippet_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_explicit_sensitivity_combinational_snippet_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module top(
  input logic req,
  input logic mode,
  input logic [3:0] data,
  output logic [3:0] result,
  output logic valid
);
  logic [3:0] next_value;

  always @(req or mode or data) begin
    next_value = data;

    if (req) begin
      if (mode) begin
        next_value = data ^ 4'b1010;
      end else begin
        next_value = data + 4'b0001;
      end
    end
  end

  assign result = next_value;
  assign valid = req | mode;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);

  auto nextValue = top->getBusNet(NLName("next_value"));
  ASSERT_NE(nextValue, nullptr);
  EXPECT_EQ(4, nextValue->getWidth());

  auto result = top->getBusNet(NLName("result"));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(4, result->getWidth());

  auto valid = dynamic_cast<SNLBitNet*>(top->getNet(NLName("valid")));
  ASSERT_NE(valid, nullptr);

  EXPECT_EQ(2u, countMux2Instances(top, 4));
  EXPECT_EQ(4u, countFAInstances(top));

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "always_explicit_sensitivity_combinational_snippet_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysStarCombinationalSnippetMatchesAlwaysComb) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_star_combinational_matches_always_comb";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto starPath = outPath / "always_star_combinational_matches_always_comb_star.sv";
  std::ofstream starFile(starPath);
  ASSERT_TRUE(starFile.good());
  starFile
    << R"(module top_star(
  input logic req,
  input logic mode,
  input logic [3:0] data,
  output logic [3:0] result,
  output logic valid
);
  logic [3:0] next_value;

  always @* begin
    next_value = data;

    if (req) begin
      if (mode) begin
        next_value = data ^ 4'b1010;
      end else begin
        next_value = data + 4'b0001;
      end
    end
  end

  assign result = next_value;
  assign valid = req | mode;
endmodule
)";
  starFile.close();

  const auto combPath = outPath / "always_star_combinational_matches_always_comb_comb.sv";
  std::ofstream combFile(combPath);
  ASSERT_TRUE(combFile.good());
  combFile
    << R"(module top_comb(
  input logic req,
  input logic mode,
  input logic [3:0] data,
  output logic [3:0] result,
  output logic valid
);
  logic [3:0] next_value;

  always_comb begin
    next_value = data;

    if (req) begin
      if (mode) begin
        next_value = data ^ 4'b1010;
      end else begin
        next_value = data + 4'b0001;
      end
    end
  end

  assign result = next_value;
  assign valid = req | mode;
endmodule
)";
  combFile.close();

  constructor.construct(starPath);
  constructor.construct(combPath);

  auto topStar = library_->getSNLDesign(NLName("top_star"));
  auto topComb = library_->getSNLDesign(NLName("top_comb"));
  ASSERT_NE(topStar, nullptr);
  ASSERT_NE(topComb, nullptr);

  EXPECT_EQ(countMux2Instances(topStar), countMux2Instances(topComb));
  EXPECT_EQ(countMux2Instances(topStar, 4), countMux2Instances(topComb, 4));
  EXPECT_EQ(countFAInstances(topStar), countFAInstances(topComb));
  EXPECT_EQ(topStar->getInstances().size(), topComb->getInstances().size());

  auto starResult = topStar->getBusNet(NLName("result"));
  auto combResult = topComb->getBusNet(NLName("result"));
  ASSERT_NE(starResult, nullptr);
  ASSERT_NE(combResult, nullptr);
  EXPECT_EQ(starResult->getWidth(), combResult->getWidth());
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingDelayUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_timing_delay_unsupported" / "seq_timing_delay_unsupported.sv");
    FAIL() << "Expected unsupported delay timing exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported sequential timing control"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableActionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_enable_action_unsupported" / "seq_enable_action_unsupported.sv");

  auto top = library_->getSNLDesign(NLName("seq_enable_action_unsupported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t faCount = 0;
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetActionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_reset_action_unsupported" / "seq_reset_action_unsupported.sv");

  auto top = library_->getSNLDesign(NLName("seq_reset_action_unsupported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t faCount = 0;
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddConstTwoSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_add_const_two_unsupported" / "seq_add_const_two_unsupported.sv");

  auto top = library_->getSNLDesign(NLName("seq_add_const_two_unsupported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffreModel = NLDB0::getDFFRE();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffreModel, nullptr);
  size_t faCount = 0;
  size_t flopCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel || inst->getModel() == dffreModel) {
      ++flopCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, flopCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddWithXLiteralUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_add_with_x_literal_unsupported" /
      "seq_add_with_x_literal_unsupported.sv");
    FAIL() << "Expected unsupported add with x-literal exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetStructDefaultUnknownUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_struct_default_unknown_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_struct_default_unknown_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << "module seq_reset_struct_default_unknown_unsupported (\n"
    << "  input  logic        clk,\n"
    << "  input  logic        rst_ni,\n"
    << "  input  logic [11:0] d,\n"
    << "  input  logic        en,\n"
    << "  output logic [11:0] q_out\n"
    << ");\n"
    << "  struct packed {\n"
    << "    logic [11:0] csr_address;\n"
    << "  } q, n;\n"
    << "  always_comb begin\n"
    << "    n = q;\n"
    << "    if (en) begin\n"
    << "      n.csr_address = d;\n"
    << "    end\n"
    << "  end\n"
    << "  always_ff @(posedge clk or negedge rst_ni) begin\n"
    << "    if (~rst_ni) begin\n"
    << "      q <= '{default: 'x};\n"
    << "    end else begin\n"
    << "      q <= n;\n"
    << "    end\n"
    << "  end\n"
    << "  assign q_out = q.csr_address;\n"
    << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported struct default unknown reset assignment";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddWithUnbasedXLiteralUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_add_unbased_x_literal_unsupported" /
      "seq_add_unbased_x_literal_unsupported.sv");
    FAIL() << "Expected unsupported unbased unknown literal in sequential add";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialUnbasedUnknownLiteralDirectRhsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_unbased_unknown_literal_direct_rhs_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_unbased_unknown_literal_direct_rhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_unbased_unknown_literal_direct_rhs_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] d_i,
  output logic [1:0] q_o
);
  always_ff @(posedge clk_i) begin
    if (en_i)
      q_o <= d_i;
    else
      q_o <= 'X;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_unbased_unknown_literal_direct_rhs_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(2u, dffCount);
  EXPECT_EQ(1u, countMux2Instances(top));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddNonIncrementSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_add_non_increment" / "seq_add_non_increment.sv");

  auto top = library_->getSNLDesign(NLName("seq_add_non_increment"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t faCount = 0;
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_GT(faCount, 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSUnaryNotSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_rhs_unresolved" / "seq_rhs_unresolved.sv");

  auto top = library_->getSNLDesign(NLName("seq_rhs_unresolved"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSWidthMismatchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_rhs_width_mismatch" / "seq_rhs_width_mismatch.sv");

  auto top = library_->getSNLDesign(NLName("seq_rhs_width_mismatch"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSDirectMatchSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_rhs_direct_match" / "seq_rhs_direct_match.sv");

  auto top = library_->getSNLDesign(NLName("seq_rhs_direct_match"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_EQ(0u, faCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_rhs_direct_match");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSFixedUnpackedSelectionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_rhs_fixed_unpacked_selection_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_rhs_fixed_unpacked_selection_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_rhs_fixed_unpacked_selection_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [7:0] a_i,
  input  logic [7:0] b_i,
  output logic [7:0] q_o
);
  logic [7:0] lanes [0:1];

  always_comb begin
    lanes[0] = a_i;
    lanes[1] = b_i;
  end

  always_ff @(posedge clk_i) begin
    q_o <= lanes[sel_i];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_rhs_fixed_unpacked_selection_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTopLevelAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_top_level_assignment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_top_level_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_top_level_assignment_supported(
  input  logic       clk_i,
  input  logic [7:0] d_i,
  output logic [7:0] q_o
);
  always @(posedge clk_i)
    q_o <= d_i;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_top_level_assignment_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_EQ(0u, faCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTopLevelShiftLeftSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_top_level_shift_left_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_top_level_shift_left_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_top_level_shift_left_supported(
  input  logic       clk_i,
  output logic [7:0] q_o
);
  always_ff @(posedge clk_i)
    q_o <= q_o << 1'b1;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_top_level_shift_left_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTopLevelMultiAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_top_level_multi_assignment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_top_level_multi_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_top_level_multi_assignment_supported(
  input  logic       clk_i,
  input  logic [7:0] d_i,
  input  logic       s_i,
  output logic [7:0] q0_o,
  output logic [7:0] q1_o,
  output logic       s0_o,
  output logic       s1_o
);
  always_ff @(posedge clk_i) begin
    q0_o <= d_i;
    q1_o <= q0_o;
    s0_o <= s_i;
    s1_o <= s0_o;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_top_level_multi_assignment_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(18u, dffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTopLevelMultiAssignmentNegedgeSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_top_level_multi_assignment_negedge_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_top_level_multi_assignment_negedge_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_top_level_multi_assignment_negedge_supported(
  input  logic       clk_i,
  input  logic [7:0] d_i,
  input  logic       s_i,
  output logic [7:0] q0_o,
  output logic [7:0] q1_o,
  output logic       s0_o,
  output logic       s1_o
);
  always_ff @(negedge clk_i) begin
    q0_o <= d_i;
    q1_o <= q0_o;
    s0_o <= s_i;
    s1_o <= s0_o;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_top_level_multi_assignment_negedge_supported"));
  ASSERT_NE(top, nullptr);

  auto dffnModel = NLDB0::getDFFN();
  ASSERT_NE(dffnModel, nullptr);
  size_t dffnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffnModel) {
      ++dffnCount;
    }
  }
  EXPECT_EQ(18u, dffnCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialTopLevelUnpackedArrayShiftRegisterSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_top_level_unpacked_array_shift_register_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_top_level_unpacked_array_shift_register_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_top_level_unpacked_array_shift_register_supported(
  input  logic       clk_i,
  input  logic       load_i,
  input  logic [7:0] d_i
);
  logic [7:0] q [3:0];
  always_ff @(posedge clk_i) begin
    q[0]   <= load_i ? d_i : '0;
    q[3:1] <= q[2:0];
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_top_level_unpacked_array_shift_register_supported"));
  ASSERT_NE(top, nullptr);

  auto qNet = top->getBusNet(NLName("q"));
  ASSERT_NE(qNet, nullptr);
  EXPECT_EQ(32, qNet->getWidth());

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(32u, dffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetEnableElseBlockConditionalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_enable_else_block_conditional_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_enable_else_block_conditional_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_enable_else_block_conditional_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic       en_i,
  input  logic [7:0] d_i,
  output logic [7:0] q_o
);
  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      q_o <= 8'h00;
    end else begin
      if (en_i) begin
        q_o <= d_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_enable_else_block_conditional_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(8u, dffCount);
  EXPECT_EQ(2u, countMux2Instances(top));
  EXPECT_EQ(2u, countMux2Instances(top, 8));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetNestedClearElseIfIncrementSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_nested_clear_elseif_increment_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_nested_clear_elseif_increment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_nested_clear_elseif_increment_supported(
  input  logic       clk_i,
  input  logic       reset_i,
  input  logic       clear_i,
  input  logic       up_i,
  output logic [3:0] count_o
);
  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      count_o <= 4'h0;
    end else begin
      if (clear_i) begin
        count_o <= {3'b000, up_i};
      end else if (up_i) begin
        count_o <= count_o + 1'b1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_nested_clear_elseif_increment_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  size_t faCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
    if (NLDB0::isFA(inst->getModel())) {
      ++faCount;
    }
  }
  EXPECT_EQ(4u, dffCount);
  EXPECT_GE(countMux2Instances(top), 3u);
  EXPECT_GT(faCount, 0u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetBranchNestedBlockDirectAssignmentsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_branch_nested_block_direct_assignments_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "seq_reset_branch_nested_block_direct_assignments_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_branch_nested_block_direct_assignments_supported(
  input  logic clk_i,
  input  logic reset_i,
  input  logic a_i,
  input  logic b_i,
  output logic q_o,
  output logic r_o
);
  always_ff @(posedge clk_i) begin
    if (reset_i) begin
      begin
        q_o <= 1'b0;
      end
      r_o <= 1'b0;
    end else begin
      q_o <= a_i;
      r_o <= b_i;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_branch_nested_block_direct_assignments_supported"));
  ASSERT_NE(top, nullptr);
  auto dffModel = NLDB0::getDFF();
  ASSERT_NE(dffModel, nullptr);
  size_t dffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    }
  }
  EXPECT_EQ(2u, dffCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetLoopBaseCopyAndNestedFieldWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_loop_base_copy_and_nested_field_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_loop_base_copy_and_nested_field_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_loop_base_copy_and_nested_field_write_supported #(
  parameter int unsigned NR_ROWS = 4,
  parameter int unsigned INSTR_PER_FETCH = 2
) (
  input logic clk_i,
  input logic rst_ni,
  input logic flush_i
);
  typedef struct packed {
    logic       valid;
    logic [0:0] target_address;
  } pred_t;

  pred_t table_d[NR_ROWS-1:0][INSTR_PER_FETCH-1:0];
  pred_t table_q[NR_ROWS-1:0][INSTR_PER_FETCH-1:0];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < NR_ROWS; i++) table_q[i] <= '{default: 0};
    end else begin
      if (flush_i) begin
        for (int i = 0; i < NR_ROWS; i++) begin
          for (int j = 0; j < INSTR_PER_FETCH; j++) begin
            table_q[i][j].valid <= 1'b0;
          end
        end
      end else begin
        table_q <= table_d;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_loop_base_copy_and_nested_field_write_supported"));
  ASSERT_NE(top, nullptr);

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(16u, dffrnCount);
  EXPECT_GT(countMux2Instances(top), 0u);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetPackedArrayPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_packed_array_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_packed_array_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_packed_array_partial_writes_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [3:0] en_i,
  input  logic [7:0] data_i,
  output logic [31:0] out_o
);
  logic [3:0][7:0] mem;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem <= '{default: '0};
    end else begin
      for (int i = 0; i < 4; i++) begin
        if (en_i[i]) begin
          mem[i] <= data_i;
        end
      end
      mem[0] <= '0;
    end
  end

  assign out_o = mem;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_packed_array_partial_writes_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(32u, dffrnCount);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseSequentialResetDynamicIndexedRangePartialWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_reset_dynamic_indexed_range_partial_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_reset_dynamic_indexed_range_partial_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_reset_dynamic_indexed_range_partial_write_supported(
  input  logic        clk_i,
  input  logic        rst_ni,
  input  logic [1:0]  idx_i,
  input  logic [3:0]  data_i,
  output logic [15:0] out_o
);
  logic [15:0] q;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      q <= '0;
    end else begin
      q[idx_i+:4] <= data_i;
    end
  end

  assign out_o = q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("seq_reset_dynamic_indexed_range_partial_write_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(16u, dffrnCount);
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSRealConstantSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_rhs_real_constant_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_rhs_real_constant_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_rhs_real_constant_supported(
  input  logic clk_i,
  input  logic rst_ni,
  output logic [1:0] q_o
);
  localparam real THREE_R = 3.0;
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      q_o <= 2'b00;
    end else begin
      q_o <= THREE_R;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_rhs_real_constant_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffrnModel, nullptr);
  size_t ffCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel || inst->getModel() == dffrnModel) {
      ++ffCount;
    }
  }
  EXPECT_EQ(2u, ffCount);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
    if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ(0u, mux2Count);

  auto* model = memoryInst->getModel();
  ASSERT_NE(nullptr, model);
  EXPECT_TRUE(NLDB0::isMemory(model));
  EXPECT_EQ("4", model->getParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ("8", model->getParameter(NLName("WIDTH"))->getValue());

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  auto* abitsParam = memoryInst->getInstParameter(NLName("ABITS"));
  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  ASSERT_NE(nullptr, abitsParam);
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, rstEnableParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
  EXPECT_EQ("2", abitsParam->getValue());
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());
  EXPECT_EQ("0", rstEnableParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".DEPTH(4)"));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("naja_mux2"));
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceWriteOnlySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_write_only_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_write_only_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_write_only_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = en_i;
    mem_d = mem_q;
    if (en_i) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_write_only_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());

  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_raddr")));
  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_rdata")));
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceNonBitstreamElementFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_non_bitstream_element_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_non_bitstream_element_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_non_bitstream_element_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct {
    logic [7:0] data;
    string      tag;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i].data = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i].data;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00110011001000100001000100000000", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_reset_init_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ENABLE(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ASYNC(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ACTIVE_LOW(1)"));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00110011001000100001000100000000)"));

  const auto primitiveDumpPath = dumpedVerilog.parent_path() / "primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitiveDumpPath));
  std::string primitiveDump = readTextFile(primitiveDumpPath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceAsyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_async_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      mem_q <= '{8'hA0, 8'hB1, 8'hC2, 8'hD3};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_async_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b11010011110000101011000110100000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sync_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      mem_q <= '{8'h10, 8'h21, 8'h32, 8'h43};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_sync_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000011001100100010000100010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSyncLowBitwiseResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_low_bitwise_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (~rst_ni) begin
      mem_q <= '{8'h55, 8'h66, 8'h77, 8'h88};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_sync_low_bitwise_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b10001000011101110110011001010101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceAsyncResetConditionMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_reset_condition_mismatch_fallback(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (~rst_i) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after async reset mismatch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSyncLogicalNotResetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_logical_not_reset_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_logical_not_reset_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_logical_not_reset_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (!rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after sync logical-not reset";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceResetInitUnknownBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unknown_bits_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{8'hxx, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after unknown-bit reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceResetInitNonConstantFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_non_constant_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= data_i;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after non-constant reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceResetInitNarrowEntryZeroExtendSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_narrow_entry_zero_extend_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 4'h1;
        end else if (i == 1) begin
          mem_q[i] <= 4'h2;
        end else if (i == 2) begin
          mem_q[i] <= 4'h3;
        end else begin
          mem_q[i] <= 4'h4;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_narrow_entry_zero_extend_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000100000000110000001000000001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceResetInitOversizedEntryFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_oversized_entry_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 16'h1234;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after oversized reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceResetInitRealConstantsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_real_constants_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_real_constants_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_real_constants_supported(
  input  logic        clk_i,
  input  logic        rst_i,
  input  logic [1:0]  addr_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:3];
  logic [63:0] mem_d [0:3];

  localparam real ZERO_R  = 0.0;
  localparam real ONE_R   = 1.0;
  localparam real TWO_R   = 2.0;
  localparam real THREE_R = 3.0;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= ZERO_R;
        end else if (i == 1) begin
          mem_q[i] <= ONE_R;
        end else if (i == 2) begin
          mem_q[i] <= TWO_R;
        end else begin
          mem_q[i] <= THREE_R;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_real_constants_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);

  const std::array<uint64_t, 4> values{0, 1, 2, 3};
  std::string expectedInit = "256'b";
  for (auto it = values.rbegin(); it != values.rend(); ++it) {
    for (int bit = 63; bit >= 0; --bit) {
      expectedInit += ((*it >> bit) & 1ULL) ? '1' : '0';
    }
  }
  EXPECT_EQ(expectedInit, initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitPackedArrayElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_packed_array_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [0:3][7:0] INIT_PACKED = '{8'h11, 8'h22, 8'h33, 8'h44};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_PACKED[0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_PACKED[1];
        end else if (i == 2) begin
          mem_q[i] <= INIT_PACKED[2];
        end else begin
          mem_q[i] <= INIT_PACKED[3];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_packed_array_element_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitStructMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_struct_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_struct_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_struct_member_access_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [7:0] payload;
    logic       valid;
  } init_t;

  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam init_t INIT0 = '{payload: 8'h12, valid: 1'b0};
  localparam init_t INIT1 = '{payload: 8'h23, valid: 1'b1};
  localparam init_t INIT2 = '{payload: 8'h34, valid: 1'b0};
  localparam init_t INIT3 = '{payload: 8'h45, valid: 1'b1};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT0.payload;
        end else if (i == 1) begin
          mem_q[i] <= INIT1.payload;
        end else if (i == 2) begin
          mem_q[i] <= INIT2.payload;
        end else begin
          mem_q[i] <= INIT3.payload;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_struct_member_access_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_range_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [31:0] INIT_WORD = 32'h44332211;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_WORD[7:0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_WORD[15:8];
        end else if (i == 2) begin
          mem_q[i] <= INIT_WORD[23:16];
        end else begin
          mem_q[i] <= INIT_WORD[31:24];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_range_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitUnaryConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (!(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_unary_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitLiteralConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_literal_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (1'b1) begin
          mem_q[i] <= 8'hA5;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_literal_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100101101001011010010110100101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitBitwiseNotConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_bitwise_not_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_bitwise_not_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitUnaryNonConstantConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_non_constant_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~addr_i[0]) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitComparisonConstantConditionsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_comparison_constant_conditions_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i > 3) begin
          mem_q[i] <= 8'hF0;
        end else if (i >= 2) begin
          mem_q[i] <= 8'hA0;
        end else if (i == 1) begin
          mem_q[i] <= 8'hB0;
        end else if (i <= 0) begin
          mem_q[i] <= 8'hC0;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_comparison_constant_conditions_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100000101000001011000011000000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitInequalityConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_inequality_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i != 1) begin
          mem_q[i] <= 8'hD0;
        end else begin
          mem_q[i] <= 8'hE1;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_inequality_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b11010000110100001110000111010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple,
       parseQDMemoryInferenceResetInitLogicalAndConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_logical_and_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if ((i < 2) && 1'b1) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceConditionalSideLogicSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_conditional_side_logic_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_conditional_side_logic_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_conditional_side_logic_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] alt_addr_i,
  input  logic [7:0] data_i,
  input  logic [7:0] alt_data_i,
  output logic [7:0] data_o,
  output logic       side_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    side_o = sel_i;
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[alt_addr_i] = alt_data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_conditional_side_logic_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("side_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceLoopConditionalElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_conditional_else_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_conditional_else_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_conditional_else_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_d[i] = data_i;
      end else begin
        mem_d[i] = 8'h00;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_conditional_else_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceLoopBreakSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_break_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_break_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_break_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      mem_d[i] = data_i;
      break;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_break_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCasezFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_casez_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_casez_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_casez_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    casez (mode_i)
      2'b1?: mem_d[0] = data_i;
      default: mem_d[addr_i] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceIfFalseBranchFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_iffalse_branch_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[addr_i] += data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCaseItemFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[addr_i] += data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCaseDefaultFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_default_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_default_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_default_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[0] = data_i;
      default: mem_d[addr_i] += data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceConditionGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_condition_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_condition_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_condition_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if ($urandom_range(1, 0)) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceNoIndexedWritesAfterLateConstantFalseGuardFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_no_indexed_writes_after_late_constant_false_guard(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (addr_i[0] && 1'b0) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCaseItemGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      $urandom_range(1, 0): mem_d[addr_i] = data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCompoundShadowActionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_compound_shadow_action_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_compound_shadow_action_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_compound_shadow_action_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported compound assignment in always_comb without current LHS bits",
     "unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceNestedBaseCopyFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_base_copy_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_base_copy_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_base_copy_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d = mem_q;
    end
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCaseAndForSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_and_for_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_and_for_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_and_for_supported(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  input  logic [1:0] token_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = 1'b0;
    mem_d = mem_q;
    case (mode_i)
      2'b00: begin
        for (int i = 0; i < 2; i++) begin
          mem_d[i] = {7'b0, token_i[i]};
        end
      end
      2'b01: begin
        mem_d[addr_i] = data_i;
      end
      default: begin
        flag_o = 1'b1;
      end
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_case_and_for_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceConstantConditionalsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_constant_conditionals_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_constant_conditionals_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_constant_conditionals_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (1'b0) begin
      mem_d[0] = 8'hAA;
    end
    if (1'b1) begin
      mem_d[1] = data_i;
    end else begin
      mem_d[1] = 8'h55;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_constant_conditionals_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSharedSequentialResetLoopSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_loop_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 8'h11;
        end else if (i == 1) begin
          mem_q[i] <= 8'h22;
        end else begin
          mem_q[i] <= 8'h00;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_loop_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000010001000010001", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top, "qd_memory_inference_shared_sequential_reset_loop_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00000000000000000010001000010001)"));
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialIgnoresUnrelatedAlwaysFFChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      scratch_q <= 1'b0;
    else
      scratch_q <= en_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseGeneratedDirectSequentialMemoryWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "generated_direct_sequential_memory_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "generated_direct_sequential_memory_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module generated_direct_sequential_memory_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                    clk_i,
  input  logic                    w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]      w_data_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]      r_data_o
);
  if (width_p == 0 || els_p == 0) begin : z
    assign r_data_o = '0;
  end else begin : nz
    logic [width_p-1:0] mem [els_p-1:0];
    wire [addr_width_lp-1:0] r_addr_li = (els_p > 0) ? r_addr_i : '0;
    wire [addr_width_lp-1:0] w_addr_li = (els_p > 0) ? w_addr_i : '0;

    assign r_data_o = mem[r_addr_li];

    always_ff @(posedge clk_i) begin
      if (w_v_i) begin
        mem[w_addr_li] <= w_data_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("generated_direct_sequential_memory_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseDirectSequentialMemoryMaskedBitWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_masked_bit_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_masked_bit_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_masked_bit_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                     clk_i,
  input  logic                     w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]       w_data_i,
  input  logic [width_p-1:0]       w_mask_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]       r_data_o
);
  logic [width_p-1:0] mem [els_p-1:0];

  assign r_data_o = mem[r_addr_i];

  always_ff @(posedge clk_i)
    if (w_v_i)
      for (integer i = 0; i < width_p; i=i+1)
        if (w_mask_i[i])
          mem[w_addr_i][i] <= w_data_i[i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_masked_bit_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseDirectSequentialMemoryAsyncLowResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_async_low_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_async_low_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_async_low_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       w_v_i,
  input  logic [1:0] w_addr_i,
  input  logic [7:0] w_data_i,
  output logic [7:0] r_data_o
);
  logic [7:0] mem [0:3];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem <= '{default: '0};
    end else begin
      for (int i = 0; i < 4; i++) begin
        if (w_v_i && (w_addr_i == i[1:0])) begin
          mem[i] <= w_data_i;
        end
      end
    end
  end

  assign r_data_o = mem[w_addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_async_low_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialWholeArrayCopyIgnoresUnaryExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    scratch_o = 2'b00;
    scratch_o++;
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialMissingDefaultChainFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_missing_default_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_missing_default_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_missing_default_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialInvalidDefaultCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_invalid_default_commit_target(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_q;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialSimpleResetChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_simple_reset_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_simple_reset_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_simple_reset_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_simple_reset_chain"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000100001100110010001000010001", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialResetConstantFalseNoElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_constant_false_no_else(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
        if (1'b0) begin
          mem_q[i] <= 8'hAA;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_constant_false_no_else"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialResetIgnoresNonAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_ignores_non_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      case (addr_i)
        default: ;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_ignores_non_assignment"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialDynamicResetIndexFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_dynamic_reset_index_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[addr_i] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected dynamic reset index fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialResetMissingTargetAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_missing_target_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected missing reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialResetPartialInitializationFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_partial_initialization(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected partial reset initialization fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialResetCompoundAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_compound_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] += 8'h01;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected compound reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialSelfCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_self_commit_target_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_q;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected self commit target fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialMismatchedCommitSignatureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_mismatched_commit_signature(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [9:0] mem_next_bad [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    for (int i = 0; i < 4; i++) begin
      mem_next_bad[i] = {2'b00, mem_d[i]};
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next_bad;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected mismatched commit signature fallback";
  } catch (const SNLSVConstructorException& e) {
    EXPECT_FALSE(std::string(e.what()).empty());
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialIgnoresTopLevelConditionalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_top_level_conditional(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      scratch_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      if (en_i) begin
        scratch_q <= addr_i[0];
      end
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q ^ scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_top_level_conditional"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialIgnoresNonAssignmentExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_non_assignment_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [1:0] scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      scratch_q <= 2'b00;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      ++scratch_q;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_non_assignment_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDMemoryInferenceSharedSequentialMultipleTargetAssignmentsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_multiple_target_assignments_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected shared sequential duplicate target assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitElseGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_else_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_else_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_else_guard_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_else_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (i == 0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitLateConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_late_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && 1'b0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_late_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitConstantSelectorFastPathsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_selector_fast_paths_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[1] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_constant_selector_fast_paths_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[0][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitRhsSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_rhs_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[0];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextDynamicIndexedCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_dynamic_indexed_commit_target_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] commit_idx_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[commit_idx_i] = mem_d[commit_idx_i];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed commit target";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIfTrueIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iftrue_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] += mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifTrue branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIfFalseIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iffalse_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] += mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifFalse branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIgnoresEmptyAndUnrelatedCommitCandidates) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_empty_and_unrelated_commit_candidates(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    ;
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_empty_and_unrelated_commit_candidates"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIgnoresNonAssignmentCommitCandidate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_non_assignment_commit_candidate";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_non_assignment_commit_candidate.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_non_assignment_commit_candidate(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    case (addr_i)
      default: ;
    endcase
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_non_assignment_commit_candidate"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextCompoundCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_compound_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_compound_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_compound_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextSelfCopyCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_self_copy_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_self_copy_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_self_copy_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_next;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitFieldFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_field_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_field_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_field_fallback_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i].mode == 2'b11) begin
        mem_next[i].mode = mem_q[i].mode;
      end
      if (mem_d[i].access == 2'b01) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_field_fallback_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferencePartialFieldWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_field_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_field_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_field_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [5:0] payload_i,
  output logic [5:0] payload_o,
  output logic [1:0] reserved_o
);
  typedef struct packed {
    logic [5:0] payload;
    logic [1:0] reserved;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].reserved = 2'b00;
    mem_d[addr_i].payload = payload_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign payload_o = mem_q[addr_i].payload;
  assign reserved_o = mem_q[addr_i].reserved;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_field_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferencePartialSliceWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_slice_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_slice_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_slice_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] upper_i,
  input  logic [1:0] lower_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][7:4] = upper_i;
    mem_d[addr_i][1:0] = lower_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_slice_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferencePackedElementWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_packed_element_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_packed_element_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_packed_element_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_packed_element_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceDynamicPackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] bit_sel_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][bit_sel_i] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceOutOfRangePackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][8] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceIndexedUpRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_up_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_up_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_up_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][2 +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_up_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitLogicalOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_or_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if ((mem_d[i].mode == 2'b11) || (mem_d[i].access == 2'b01)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_or_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitSingleBitEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_equality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitSingleBitInequalitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_inequality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_inequality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_inequality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] != mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_inequality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitEqualityRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_equality_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_equality_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_equality_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitLogicalAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_and_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitLogicalAndLhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_lhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[0][0] && mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextLocalCommitLogicalAndRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDNextIndexedCommitUnaryLocalConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_local_condition_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (!mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_unary_local_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseQDNextIndexedCommitUnaryConstantLocalConditionShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_constant_local_condition_shortcuts(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_q[i];
      if (!(mem_d[i][0] && 1'b0)) begin
        mem_next[i] = mem_d[i];
      end
      if (!(mem_d[i][0] || 1'b1)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_unary_constant_local_condition_shortcuts"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceIndexedDownRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_down_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_down_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_down_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][5 -: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_down_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceDynamicIndexedRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_indexed_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] start_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][start_i +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCurrentEntryBitsSelectorFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_current_entry_bits_selector_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_current_entry_bits_selector_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_current_entry_bits_selector_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[$urandom_range(3, 0)][3:0] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported current-entry selector lowering";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceOutOfRangeSimpleRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_simple_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][9:8] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range simple range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceWholeEntryWriteDataBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_whole_entry_write_data_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = $urandom_range(8'hFF, 8'h00);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferencePartialWriteAssignBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_write_assign_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3:0] = $urandom_range(4'hF, 4'h0);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseNestedInferredMemoryModulesKeepPerDesignStateSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "nested_inferred_memory_modules_keep_per_design_state_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "nested_inferred_memory_modules_keep_per_design_state_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module leaf_mem(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule

module mid_wrap(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  leaf_mem u_leaf (
    .clk_i (clk_i),
    .addr_i(addr_i),
    .data_i(data_i),
    .data_o(data_o)
  );
endmodule

module top_wrap(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  mid_wrap u_mid (
    .clk_i (clk_i),
    .addr_i(addr_i),
    .data_i(data_i),
    .data_o(data_o)
  );
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(NLName("top_wrap"));
  auto* mid = library_->getSNLDesign(NLName("mid_wrap"));
  auto* leaf = library_->getSNLDesign(NLName("leaf_mem"));
  ASSERT_NE(nullptr, top);
  ASSERT_NE(nullptr, mid);
  ASSERT_NE(nullptr, leaf);

  size_t topMemoryCount = 0;
  for (auto* inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++topMemoryCount;
    }
  }
  EXPECT_EQ(0, topMemoryCount);

  size_t midMemoryCount = 0;
  for (auto* inst : mid->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++midMemoryCount;
    }
  }
  EXPECT_EQ(0, midMemoryCount);

  size_t leafMemoryCount = 0;
  for (auto* inst : leaf->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++leafMemoryCount;
    }
  }
  EXPECT_EQ(1, leafMemoryCount);
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
  constructor.construct(benchmarksPath / "simple" / "simple.sv", options);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);
  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "simple_ast_json_dump");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));

  ASSERT_TRUE(std::filesystem::exists(jsonPath));
  std::ifstream jsonFile(jsonPath);
  ASSERT_TRUE(jsonFile.good());
  std::string json{
    std::istreambuf_iterator<char>(jsonFile),
    std::istreambuf_iterator<char>()};
  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("\"top\""), std::string::npos);
}

TEST_F(SNLSVConstructorTestSimple, parseSimpleModuleDumpDiagnosticsReportNoDiagnostics) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "simple_diagnostics_report";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  auto reportPath = outPath / "reports" / "diagnostics.txt";
  SNLSVConstructor::ConstructOptions options;
  options.diagnosticsReportPath = reportPath;
  constructor.construct(benchmarksPath / "simple" / "simple.sv", options);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(top, nullptr);

  ASSERT_TRUE(std::filesystem::exists(reportPath));
  std::ifstream reportFile(reportPath);
  ASSERT_TRUE(reportFile.good());
  std::string report{
    std::istreambuf_iterator<char>(reportFile),
    std::istreambuf_iterator<char>()};
  EXPECT_NE(report.find("No SystemVerilog diagnostics."), std::string::npos);
}

TEST_F(SNLSVConstructorTestSimple, parseSyntaxErrorDumpDiagnosticsReportWithDetails) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "syntax_error_diagnostics_report";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "syntax_error_diagnostics_report.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile << "module syntax_error_diagnostics_report(input logic a, output logic y)\n"
         << "  assign y = a;\n"
         << "endmodule\n";
  svFile.close();

  auto reportPath = outPath / "reports" / "diagnostics.txt";
  SNLSVConstructor::ConstructOptions options;
  options.diagnosticsReportPath = reportPath;

  try {
    constructor.construct(svPath, options);
    FAIL() << "Expected SystemVerilog compilation failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("SystemVerilog compilation failed"));
  }

  ASSERT_TRUE(std::filesystem::exists(reportPath));
  std::ifstream reportFile(reportPath);
  ASSERT_TRUE(reportFile.good());
  std::string report{
    std::istreambuf_iterator<char>(reportFile),
    std::istreambuf_iterator<char>()};
  EXPECT_NE(report.find("error"), std::string::npos);
}

TEST_F(SNLSVConstructorTestSimple, parseEmptyDiagnosticsReportPathThrows) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  SNLSVConstructor::ConstructOptions options;
  options.diagnosticsReportPath = std::filesystem::path();

  try {
    constructor.construct(benchmarksPath / "simple" / "simple.sv", options);
    FAIL() << "Expected empty diagnostics report path exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Empty path for diagnostics report dump"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseEmptyElaboratedASTJsonPathThrows) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  SNLSVConstructor::ConstructOptions options;
  options.elaboratedASTJsonPath = std::filesystem::path();

  try {
    constructor.construct(benchmarksPath / "simple" / "simple.sv", options);
    FAIL() << "Expected empty elaborated AST JSON path exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Empty path for elaborated AST JSON dump"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListImplicitEdgeUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_event_list_implicit_edge_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_timing_event_list_implicit_edge_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_event_list_implicit_edge_unsupported(
  input logic clk,
  input logic rst_n,
  input logic d,
  output logic q
);
  always @(clk or negedge rst_n) begin
    q <= d;
  end
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported event-list timing edge exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported sequential timing edge in event list; only posedge/negedge are supported"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListNegedgeResetNonZeroUsesMuxDFF) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "seq_timing_event_list_negedge_reset_nonzero_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "seq_timing_event_list_negedge_reset_nonzero_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module seq_timing_event_list_negedge_reset_nonzero_supported(
  input logic clk,
  input logic rst_n,
  input logic d,
  output logic q
);
  always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
      q <= 1'b1;
    else
      q <= d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("seq_timing_event_list_negedge_reset_nonzero_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto dffrnModel = NLDB0::getDFFRN();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(dffrnModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t dffrnCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }

  EXPECT_EQ(1u, dffCount);
  EXPECT_EQ(0u, dffrnCount);
  EXPECT_EQ(1u, mux2Count);
}

TEST_F(SNLSVConstructorTestSimple, parseTranslateOffInitialIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "translate_off_initial_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "translate_off_initial_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module translate_off_initial_ignored(
  input logic clk,
  input logic rst_n,
  input logic d,
  output logic q
);
  // pragma translate_off
  initial begin
    assert (1) else $error("disabled");
  end
  // pragma translate_on

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)
      q <= 1'b0;
    else
      q <= d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("translate_off_initial_ignored"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchProceduralBlockSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_procedural_block_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_procedural_block_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_procedural_block_supported(
  input logic [7:0] a,
  input logic en,
  output logic [7:0] y
);
  logic [7:0] q_r;
  always_latch begin
    if (en)
      q_r <= a;
  end
  assign y = q_r;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_latch_procedural_block_supported"));
  ASSERT_NE(top, nullptr);

  auto dlatchModel = NLDB0::getDLatch();
  ASSERT_NE(dlatchModel, nullptr);

  size_t dlatchCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dlatchModel) {
      ++dlatchCount;
    }
  }
  EXPECT_EQ(8u, dlatchCount);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchIncrementerSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_incrementer_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_incrementer_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_incrementer_supported(
  input  logic en,
  output logic [3:0] y
);
  logic [3:0] q_r;
  always_latch begin
    if (en)
      q_r <= q_r + 4'd1;
    else
      q_r <= q_r;
  end
  assign y = q_r;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_latch_incrementer_supported"));
  ASSERT_NE(top, nullptr);

  auto dlatchModel = NLDB0::getDLatch();
  ASSERT_NE(dlatchModel, nullptr);

  size_t dlatchCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dlatchModel) {
      ++dlatchCount;
    }
  }
  EXPECT_EQ(4u, dlatchCount);
  EXPECT_GT(countFAInstances(top), 0u);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchElseAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_else_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_else_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_else_assignment_unsupported(
  input logic a,
  input logic b,
  input logic en,
  output logic y
);
  always_latch begin
    if (en)
      y = a;
    else
      y = b;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_else_assignment_unsupported'",
     "always_latch currently supports only implicit or explicit hold default branches"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchEnableConditionResolveFailureUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_enable_condition_resolve_failure_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "always_latch_enable_condition_resolve_failure_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_enable_condition_resolve_failure_unsupported(
  input logic a_i,
  input logic b_i,
  input logic d_i,
  output logic q_o
);
  always_latch begin
    if (a_i ^ b_i)
      q_o <= d_i;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_enable_condition_resolve_failure_unsupported'",
     "unable to resolve latch enable condition net"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchDirectAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_direct_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_direct_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_direct_assignment_unsupported(
  input logic a,
  output logic y
);
  always_latch begin
    y <= a;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_direct_assignment_unsupported'",
     "unsupported statement pattern for always_latch lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchPatternConditionUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_pattern_condition_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_pattern_condition_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_pattern_condition_unsupported(
  input logic a,
  output logic y
);
  always_latch begin
    if (a matches (1'b1))
      y <= a;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_pattern_condition_unsupported'",
     "unsupported statement pattern for always_latch lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchIfTrueNonAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_if_true_non_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_if_true_non_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_if_true_non_assignment_unsupported(
  input logic en,
  output logic y
);
  always_latch begin
    if (en)
      begin
        logic tmp;
      end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_if_true_non_assignment_unsupported'",
     "unsupported statement pattern for always_latch lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchIfFalseNonAssignmentUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_if_false_non_assignment_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_if_false_non_assignment_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_if_false_non_assignment_unsupported(
  input logic a,
  input logic en,
  output logic y
);
  always_latch begin
    if (en)
      y <= a;
    else
      $display("ignored");
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_if_false_non_assignment_unsupported'",
     "unsupported statement pattern for always_latch lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchElseDifferentLHSUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_else_different_lhs_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_else_different_lhs_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_else_different_lhs_unsupported(
  input logic a,
  input logic b,
  input logic en,
  output logic y,
  output logic z
);
  always_latch begin
    if (en)
      y <= a;
    else
      z <= b;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_else_different_lhs_unsupported'",
     "unsupported statement pattern for always_latch lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchConcatLHSErrorUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_concat_lhs_error_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_concat_lhs_error_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_concat_lhs_error_unsupported(
  input logic a,
  input logic b,
  input logic en,
  output logic y,
  output logic z
);
  always_latch begin
    if (en)
      {y, z} <= {a, b};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported latch block in module 'always_latch_concat_lhs_error_unsupported'",
     "unable to resolve latch assignment LHS net"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchDisplayOnlyIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_display_only_ignored_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_display_only_ignored_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_display_only_ignored_supported(
  input  logic clk_i,
  input  logic d_i,
  output logic q_o
);
  always_latch begin
    if (d_i)
      $display("ignored");
  end

  always_ff @(posedge clk_i) begin
    q_o <= d_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_latch_display_only_ignored_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q_o")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchDataMultiplyUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_data_multiply_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_data_multiply_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_data_multiply_unsupported(
  input  logic       en_i,
  output logic [3:0] q_o
);
  always_latch begin
    if (en_i)
      q_o <= q_o * 4'd2;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported binary operator in sequential assignment: *"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysLatchDefaultMultiplyUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_latch_default_multiply_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_latch_default_multiply_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_latch_default_multiply_unsupported(
  input  logic [3:0] d_i,
  input  logic       en_i,
  output logic [3:0] q_o
);
  always_latch begin
    if (en_i)
      q_o <= d_i;
    else
      q_o <= q_o * 4'd2;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported binary operator in sequential assignment: *"});
}

TEST_F(SNLSVConstructorTestSimple, parseInitialSystemTaskConditionalIgnoredSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "initial_system_task_conditional_ignored_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "initial_system_task_conditional_ignored_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module initial_system_task_conditional_ignored_supported #(
  parameter int width_p = 8,
  parameter int els_p = 64
) (
  input  logic a,
  output logic y
);
  initial begin
    if (width_p * els_p > 256)
      $display("checker only");
  end

  assign y = a;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("initial_system_task_conditional_ignored_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseFinalProceduralBlockUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "final_procedural_block_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "final_procedural_block_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module final_procedural_block_unsupported(
  input  logic a,
  output logic y
);
  assign y = a;
  final begin
    $display("final");
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported procedural block in module 'final_procedural_block_unsupported'",
     "unsupported procedure kind Final"});
}

TEST_F(SNLSVConstructorTestSimple, parseAlwaysCombTranslateOffStatementIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "always_comb_translate_off_statement_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "always_comb_translate_off_statement_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_translate_off_statement_ignored(
  input logic a,
  output logic y
);
  always_comb begin
    // pragma translate_off
    assert (a) else $error("disabled");
    // pragma translate_on
    y = a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_translate_off_statement_ignored"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("y")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseSynthesisTranslateOffSequentialUnsupportedIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "synthesis_translate_off_sequential_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "synthesis_translate_off_sequential_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module synthesis_translate_off_sequential_ignored(
  input logic clk,
  input logic rst_n,
  input logic d,
  output logic q
);
  logic q_main;
  logic dbg_q;

  // synthesis translate_off
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      dbg_q <= 1'b0;
    end else begin
      if (d) begin
        dbg_q <= 1'b1;
        for (int i = 0; i < 2; i++) begin
          dbg_q <= dbg_q & 1'b1;
        end
      end
    end
  end
  // synthesis translate_on

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n)
      q_main <= 1'b0;
    else
      q_main <= d;
  end

  assign q = q_main;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("synthesis_translate_off_sequential_ignored"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("q")), nullptr);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceDuplicateDynamicSelectorPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i][3:0] = lo_i;
      mem_d[addr_i][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceDisjointConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_disjoint_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_lo_i,
  input  logic       sel_hi_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_lo_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (sel_hi_i) begin
      mem_d[1][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_disjoint_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSameConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[0][3:0] = lo_i;
    mem_d[0][7:4] = hi_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSymbolicConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic       en_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (en_i) begin
      mem_d[0][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceNestedDuplicateGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_duplicate_guard_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      if (sel_i) begin
        mem_d[addr_i] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_nested_duplicate_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceCaseGuardConstantTrueShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_guard_constant_true_shortcut_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      case (1'b0)
        1'b0: mem_d[addr_i] = data_i;
        default: mem_d[addr_i] = 8'h00;
      endcase
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_case_guard_constant_true_shortcut_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceUntimedSequentialCandidateIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_untimed_sequential_candidate_ignored(
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported statement while extracting sequential timing control",
     "unable to resolve a single-bit clock net"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSequentialNedgeClockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_nedge_clock_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(negedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestSimple, parseQDMemoryInferenceSequentialClockNetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_clock_net_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_clock_net_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_clock_net_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(posedge (clk_i + 2'd1)) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve a single-bit clock net"});
}
