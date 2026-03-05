// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
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
#include "SNLScalarNet.h"
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

void expectUnsupportedConstruct(
  SNLSVConstructor& constructor,
  const std::filesystem::path& svPath,
  std::initializer_list<const char*> expectedSubstrings) {
  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"));
    for (const auto* expected : expectedSubstrings) {
      EXPECT_NE(std::string::npos, reason.find(expected));
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
    EXPECT_NE(std::string::npos, reason.find("Failed to load SystemVerilog file: "));
    EXPECT_NE(std::string::npos, reason.find(missingPath.string()));
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

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported RHS in continuous assign"});
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic element-select under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported const-variable-index add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-int32 literal index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-int32 const-longint index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported too-wide const index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported negative out-of-int32 index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported unknown const index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported huge literal index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported negative literal out-of-int32 index under add failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseElementSelectFunctionCallBaseUnderAddUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "element_select_function_call_base_under_add_unsupported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "element_select_function_call_base_under_add_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module element_select_function_call_base_under_add_unsupported_top(
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

  EXPECT_THROW(constructor.construct(svPath), SNLSVConstructorException);
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
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "unsupported_generic_type" / "unsupported_generic_type.sv",
    {"Unsupported RHS in continuous assign"});
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

  EXPECT_TRUE(hasAttribute(aNet, "sv_src_file"));
  EXPECT_TRUE(hasAttribute(bNet, "sv_src_file"));
  EXPECT_TRUE(hasAttribute(yNet, "sv_src_file"));
  EXPECT_TRUE(hasAttribute(zNet, "sv_src_file"));

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

TEST_F(SNLSVConstructorTestSimple, parseInstanceConnectionEdgeCases) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "instance_connection_edge_cases" / "instance_connection_edge_cases.sv",
    {"Unsupported instance connection"});
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
  }
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

  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(mux2Model, nullptr);

  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(9u, mux2Count);
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

TEST_F(SNLSVConstructorTestSimple, parseDirectAssignMismatchSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "direct_assign_mismatch" / "direct_assign_mismatch.sv",
    {"Unsupported net compatibility in continuous assign"});
}

TEST_F(SNLSVConstructorTestSimple, parseContinuousAssignConcatLHSSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "continuous_assign_concat_lhs_skip" / "continuous_assign_concat_lhs_skip.sv",
    {"Unsupported LHS in continuous assign"});
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

TEST_F(SNLSVConstructorTestSimple, parseContinuousBinaryExpressionFallbacksUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath /
      "continuous_binary_expression_fallbacks_unsupported" /
      "continuous_binary_expression_fallbacks_unsupported.sv");
    FAIL() << "Expected unsupported continuous binary expression fallbacks";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: <<<"));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: *"));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: -"));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: =="));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: !="));
  }
}

TEST_F(
  SNLSVConstructorTestSimple,
  parseContinuousResolveExpressionBitsFailurePathsUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath /
      "continuous_resolve_expression_bits_failure_paths_unsupported" /
      "continuous_resolve_expression_bits_failure_paths_unsupported.sv");
    FAIL() << "Expected unsupported resolveExpressionBits continuous-assign paths";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: +"));
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in continuous assign: =="));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported multiply right-operand resolution failure";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: *"));
  }
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

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported integral-cast-of-real under add";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported binary expression in continuous assign: +"));
  }
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
  size_t muxCount = 0;
  for (auto inst : top->getInstances()) {
    if ((dffModel && inst->getModel() == dffModel) ||
        (dffrnModel && inst->getModel() == dffrnModel)) {
      ++ffCount;
    }
    if (inst->getModel() == NLDB0::getMux2()) {
      ++muxCount;
    }
  }
  EXPECT_EQ(17u, ffCount);
  EXPECT_GE(muxCount, 18u);

  auto dumpedVerilog =
    dumpTopAndGetVerilogPath(top, "seq_multi_assignment_else_block_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
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
    {"unsupported statement pattern for sequential lowering"});
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
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);
  size_t dffCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(128u, dffCount);
  EXPECT_EQ(128u, mux2Count);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_reset_all_zero_literal_wide_supported");
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
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);
  size_t dffCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(128u, dffCount);
  EXPECT_EQ(128u, mux2Count);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_rhs_wide_const_supported");
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
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);
  size_t dffCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(16u, dffCount);
  EXPECT_EQ(16u, mux2Count);
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
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);
  size_t dffCount = 0;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    }
  }
  EXPECT_EQ(128u, dffCount);
  EXPECT_EQ(128u, mux2Count);
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

TEST_F(SNLSVConstructorTestSimple, parseSequentialConcatLHSSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_concat_lhs_skipped" / "seq_concat_lhs_skipped.sv",
    {"unable to resolve assignment LHS net"});
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialLHSElementSelectSkipped) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_lhs_element_select_skipped" / "seq_lhs_element_select_skipped.sv",
    {"unsupported statement pattern for sequential lowering"});
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

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableCondUnaryNotSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  constructor.construct(
    benchmarksPath / "seq_enable_cond_unary_not_supported" /
    "seq_enable_cond_unary_not_supported.sv");

  auto top = library_->getSNLDesign(NLName("seq_enable_cond_unary_not_supported"));
  ASSERT_NE(top, nullptr);

  auto dffModel = NLDB0::getDFF();
  auto mux2Model = NLDB0::getMux2();
  ASSERT_NE(dffModel, nullptr);
  ASSERT_NE(mux2Model, nullptr);

  size_t dffCount = 0;
  size_t mux2Count = 0;
  size_t notGateCount = 0;
  for (auto inst : top->getInstances()) {
    if (inst->getModel() == dffModel) {
      ++dffCount;
    } else if (inst->getModel() == mux2Model) {
      ++mux2Count;
    } else if (NLDB0::isGate(inst->getModel()) &&
               NLDB0::getGateName(inst->getModel()) == "not") {
      ++notGateCount;
    }
  }
  EXPECT_EQ(16u, dffCount);
  EXPECT_EQ(32u, mux2Count);
  EXPECT_EQ(4u, notGateCount);

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "seq_enable_cond_unary_not_supported");
  EXPECT_TRUE(std::filesystem::exists(dumpedVerilog));
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

TEST_F(SNLSVConstructorTestSimple, parseSequentialNegedgeTimingUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_timing_negedge_unsupported" / "seq_timing_negedge_unsupported.sv");
    FAIL() << "Expected unsupported timing edge exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported sequential timing edge"));
    EXPECT_NE(std::string::npos, reason.find("only posedge is supported"));
  }
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

TEST_F(SNLSVConstructorTestSimple, parseSequentialConcurrentAssertionIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  expectUnsupportedConstruct(
    constructor,
    benchmarksPath / "seq_concurrent_assertion_ignored" /
      "seq_concurrent_assertion_ignored.sv",
    {"unsupported statement pattern for sequential lowering"});
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
    EXPECT_NE(std::string::npos, reason.find("Unsupported sequential event list"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialTimingEventListUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_timing_event_list_unsupported" /
      "seq_timing_event_list_unsupported.sv");
    FAIL() << "Expected unsupported event-list timing exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported sequential event list"));
  }
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

TEST_F(SNLSVConstructorTestSimple, parseSequentialEnableActionUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_enable_action_unsupported" / "seq_enable_action_unsupported.sv");
    FAIL() << "Expected unsupported enable action exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in sequential assignment: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialResetActionUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_reset_action_unsupported" / "seq_reset_action_unsupported.sv");
    FAIL() << "Expected unsupported reset action exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in sequential assignment: +"));
  }
}

TEST_F(SNLSVConstructorTestSimple, parseSequentialAddConstTwoUnsupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_SV_BENCHMARKS_PATH);
  try {
    constructor.construct(
      benchmarksPath / "seq_add_const_two_unsupported" / "seq_add_const_two_unsupported.sv");
    FAIL() << "Expected unsupported add-constant action exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in sequential assignment: +"));
  }
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
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in sequential assignment: +"));
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
    EXPECT_NE(
      std::string::npos,
      reason.find("Unsupported binary expression in sequential assignment: +"));
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
