// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <fstream>
#include <system_error>

#include "NLUniverse.h"

#include "SNLScalarTerm.h"

#include "SNLLibertyConstructor.h"

#include "YosysLibertyException.h"
#include "SNLLibertyConstructorException.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*  library_;
};

TEST_F(SNLLibertyConstructorTest0, test0) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("asap7_excerpt")
      / std::filesystem::path("test0.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("asap7sc7p5t_AO_LVT_FF_ccs_201020"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto design = library_->getSNLDesign(NLName("A2O1A1Ixp33_ASAP7_75t_L"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  auto a1 = design->getScalarTerm(NLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(NLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b = design->getScalarTerm(NLName("B"));
  ASSERT_NE(nullptr, b);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto c = design->getScalarTerm(NLName("C"));
  ASSERT_NE(nullptr, c);
  EXPECT_EQ(SNLTerm::Direction::Input, c->getDirection());
  auto y = design->getScalarTerm(NLName("Y"));
  ASSERT_NE(nullptr, y);
  EXPECT_EQ(SNLTerm::Direction::Output, y->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testInOut) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("inout_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("iocell"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  auto z = design->getScalarTerm(NLName("Z"));
  EXPECT_EQ(SNLTerm::Direction::InOut, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testInternalPin) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("internal_pin_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("internal_pin_test"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  auto i = design->getScalarTerm(NLName("I"));
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testNonExistingFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("missing.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongFileType) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("not_a_file.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongSyntaxFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("syntax_error.lib"));
  try {
    constructor.construct(testPath);
    FAIL() << "Expected YosysLibertyException";
  } catch (const naja::liberty::YosysLibertyException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(testPath.string()));
    EXPECT_NE(std::string::npos, reason.find("line 1"));
  }
}

TEST_F(SNLLibertyConstructorTest0, testDetailedParserErrorMessage) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_parser_detail_error_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << ":";
  }
  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected YosysLibertyException";
  } catch (const naja::liberty::YosysLibertyException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(tempPath.string()));
    EXPECT_NE(std::string::npos, reason.find("line 1"));
    EXPECT_NE(std::string::npos, reason.find("Unexpected ':'"));
  }
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testFunctionErrorHasLocationContext) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_bad_function_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output
      << "library (CTX) {\n"
      << "  cell (BAD) {\n"
      << "    pin (A) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (B) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (CI) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (Y) {\n"
      << "      direction : output;\n"
      << "      function : \"(((A ^ B) CI) | (A B\";\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  }
  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(tempPath.string()));
    EXPECT_NE(std::string::npos, reason.find("cell `BAD`"));
    EXPECT_NE(std::string::npos, reason.find("pin `Y`"));
  }
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testMultiOutputFunctionErrorHasLocationContext) {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_bad_multi_output_function_" + std::to_string(stamp) + ".lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output
      << "library (CTX_MULTI) {\n"
      << "  cell (BAD_MULTI) {\n"
      << "    pin (A) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (B) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (CI) {\n"
      << "      direction : input;\n"
      << "    }\n"
      << "    pin (Y0) {\n"
      << "      direction : output;\n"
      << "      function : \"A & B\";\n"
      << "    }\n"
      << "    pin (Y1) {\n"
      << "      direction : output;\n"
      << "      function : \"(((A ^ B) CI) | (A B\";\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  }
  SNLLibertyConstructor constructor(library_);
  try {
    constructor.construct(tempPath);
    FAIL() << "Expected SNLLibertyConstructorException";
  } catch (const SNLLibertyConstructorException& e) {
    auto reason = e.getReason();
    EXPECT_NE(std::string::npos, reason.find(tempPath.string()));
    EXPECT_NE(std::string::npos, reason.find("cell `BAD_MULTI`"));
    EXPECT_NE(std::string::npos, reason.find("pin `Y1`"));
  }
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testZipNotSupported) {
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_test.lib.zip");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << "PK";
  }
  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testUnreadableFile) {
  auto tempPath = std::filesystem::temp_directory_path()
    / std::filesystem::path("naja_liberty_unreadable.lib");
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << "library (x) { }";
  }
  std::error_code ec;
  std::filesystem::permissions(tempPath, std::filesystem::perms::none, ec);
  if (ec) {
    std::filesystem::remove(tempPath, ec);
    GTEST_SKIP() << "Permissions not supported on this platform";
  }

  {
    std::ifstream probe(tempPath, std::ios::binary);
    if (probe.is_open()) {
      std::filesystem::permissions(tempPath, std::filesystem::perms::owner_all, ec);
      std::filesystem::remove(tempPath, ec);
      GTEST_SKIP() << "Unreadable file still readable (likely running as root)";
    }
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(tempPath), SNLLibertyConstructorException);
  std::filesystem::permissions(tempPath, std::filesystem::perms::owner_all, ec);
  std::filesystem::remove(tempPath, ec);
}

TEST_F(SNLLibertyConstructorTest0, testWrongDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("direction_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testMissingDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("missing_direction_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testMissingBusType) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("missing_bus_type_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

// Test error for truth table on bus with truth_table_on_bus_error.lib
TEST_F(SNLLibertyConstructorTest0, testTruthTableOnBusError) {
  NLLibrary*  library = NLLibrary::create(NLDB::create(NLUniverse::get()),
      NLLibrary::Type::Primitives, NLName("MYLIB"));
  SNLLibertyConstructor constructor(library);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("truth_table_on_bus_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}
