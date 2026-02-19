// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusNet.h"
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
#include "SNLSVConstructorException.h"

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

std::filesystem::path dumpTopAndGetVerilogPath(const SNLDesign* top,
                                               const std::string& outDirName) {
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
  dumper.dumpDesign(top, outPath);
  return outPath / fileName;
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
  constructor.construct(benchmarksPath/"simple"/"simple.sv");

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

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "simple_module");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
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
    EXPECT_NE(std::string::npos, reason.find("Failed to load SystemVerilog file: "));
    EXPECT_NE(std::string::npos, reason.find(missingPath.string()));
  }
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

TEST_F(SNLSVConstructorTestSimple, parseInterfacePortReportedUnsupportedAtEnd) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(benchmarksPath / "interface_port_skip" / "interface_port_skip.sv");
    FAIL() << "Expected unsupported interface port exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog interface port declaration"));
    EXPECT_NE(std::string::npos, reason.find("for port: bus"));
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
    << "module unsupported_elements(ref logic r0, ref logic r1, input logic a, input logic b, output logic y);\n"
    << "  assign y = a + b;\n"
    << "endmodule\n";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected aggregated unsupported language elements exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"));
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog port direction"));
    EXPECT_NE(std::string::npos, reason.find("for port: r0"));
    EXPECT_NE(std::string::npos, reason.find("for port: r1"));
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary operator in continuous assign: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseBinaryOperatorsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath / "binary_ops_supported" / "binary_ops_supported.sv");

  auto top = library_->getSNLDesign(NLName("binary_ops_supported_top"));
  ASSERT_NE(top, nullptr);
  const std::array<const char*, 4> supportedBinaryOpOutputs{
    "y_and",
    "y_or",
    "y_xor",
    "y_xnor"};
  for (const auto* output : supportedBinaryOpOutputs) {
    auto net = top->getNet(NLName(output));
    ASSERT_NE(net, nullptr);
    auto bitNet = dynamic_cast<SNLBitNet*>(net);
    ASSERT_NE(bitNet, nullptr);
    EXPECT_FALSE(bitNet->getInstTerms().empty());
  }

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

  EXPECT_EQ(1u, andGateCount);
  EXPECT_EQ(1u, orGateCount);
  EXPECT_EQ(1u, xorGateCount);
  EXPECT_EQ(1u, xnorGateCount);
  EXPECT_EQ(0u, otherGateCount);
  EXPECT_EQ(supportedBinaryOpOutputs.size(), assignCount);
  EXPECT_EQ(8u, top->getInstances().size());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "binary_ops_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
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

TEST_F(SNLSVConstructorTestSimple, parseSequentialBinaryNonAddUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_binary_non_add" / "seq_binary_non_add.sv");
    FAIL() << "Expected unsupported sequential binary operator exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary operator in sequential assignment: &"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddNonIncrementUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_add_non_increment" / "seq_add_non_increment.sv");
    FAIL() << "Expected unsupported sequential add expression exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in sequential assignment: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSUnresolvedUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_rhs_unresolved" / "seq_rhs_unresolved.sv");
    FAIL() << "Expected unsupported sequential RHS exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialRHSWidthMismatchUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_rhs_width_mismatch" / "seq_rhs_width_mismatch.sv");
    FAIL() << "Expected sequential width mismatch exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported width mismatch in sequential assignment"));
  }
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
