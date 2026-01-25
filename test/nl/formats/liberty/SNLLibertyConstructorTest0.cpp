// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

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
  EXPECT_THROW(constructor.construct(testPath), naja::liberty::YosysLibertyException);
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

// -----------------------------------------------------------------------------
// Additional negative tests for SNLLibertyConstructor error coverage
// Add these to the same test file that contains the other SNLLibertyConstructor tests.
// Ensure the files referenced below exist under SNL_LIBERTY_BENCHMARKS/benchmarks/errors/
// -----------------------------------------------------------------------------

// Suggested file contents (create these files under benchmarks/errors/):
//
// missing_library_name.lib
//   /* top node with no args - minimal invalid liberty */
//   library ( ) { }
// 
// cell_missing_args.lib
//   library (badlib) {
//     cell ( ) { }  // cell with empty args
//   }
// 
// pin_missing_args.lib
//   library (badlib) {
//     cell (badcell) {
//       pin () { direction : input; } // pin with empty args
//     }
//   }
// 
// corrupted_gzip.lib.gz
//   // create by truncating a valid .lib.gz or writing random bytes to file
// 
// not_gzip_but_gz_ext.lib.gz
//   // plain text (not gzipped), e.g. same content as a valid .lib but saved without gzip
//   library (badlib) { cell (c) { pin (A) { direction : input; } } }
// 
// duplicate_pin_names.lib
//   library (badlib) {
//     cell (dupcell) {
//       pin (A) { direction : input; }
//       pin (A) { direction : input; } // duplicate
//       pin (Y) { direction : output; }
//     }
//   }
// 
// missing_clock_ref.lib
//   library (badlib) {
//     cell (ffcell) {
//       pin (D) { direction : input; timing ( timing_type : rising_edge; related_pin : CLK ); }
//       pin (Y) { direction : output; }
//       // CLK pin missing on purpose
//       ff ( ) { } // mark as sequential
//     }
//   }
// 
// case_insensitive_ext.LIB.GZ
//   // valid gzipped liberty content but with uppercase extension
// 
// permission_denied.lib
//   // create a file and chmod 000 to simulate permission denied
// 
// empty.lib
//   // zero bytes file
//

TEST_F(SNLLibertyConstructorTest0, testMissingLibraryName) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "missing_library_name.lib");
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException due to missing library name";
  } catch (const SNLLibertyConstructorException& e) {
    std::string msg = e.what();
    EXPECT_TRUE(msg.find("library") != std::string::npos || msg.find("missing") != std::string::npos);
  } catch (...) {
    FAIL() << "Expected SNLLibertyConstructorException";
  }
}

TEST_F(SNLLibertyConstructorTest0, testCellMissingArgs) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "cell_missing_args.lib");
  EXPECT_THROW({
    constructor.construct(testPath);
  }, SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testPinMissingArgs) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "pin_missing_args.lib");
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException due to pin with missing args";
  } catch (const SNLLibertyConstructorException& e) {
    std::string msg = e.what();
    EXPECT_NE(msg.find("pin"), std::string::npos);
  } catch (...) {
    FAIL() << "Expected SNLLibertyConstructorException";
  }
}

TEST_F(SNLLibertyConstructorTest0, testCorruptedGzipFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "corrupted_gzip.lib.gz");
  // corrupted gzip should produce SNLLibertyConstructorException (gzip error)
  EXPECT_THROW({
    constructor.construct(testPath);
  }, SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testNotGzipButGzExtension) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "not_gzip_but_gz_ext.lib.gz");
  EXPECT_THROW({
    constructor.construct(testPath);
  }, SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testDuplicatePinNames) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "duplicate_pin_names.lib");
  try {
    constructor.construct(testPath);
    FAIL() << "Expected SNLLibertyConstructorException due to duplicate pin names";
  } catch (const SNLLibertyConstructorException& e) {
    std::string msg = e.what();
    EXPECT_TRUE(msg.find("duplicate") != std::string::npos || msg.find("already") != std::string::npos);
  } catch (...) {
    FAIL() << "Expected SNLLibertyConstructorException";
  }
}

TEST_F(SNLLibertyConstructorTest0, testMissingClockReference) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "missing_clock_ref.lib");
  // Expect constructor to detect missing related_pin (clock) and throw
  EXPECT_THROW({
    constructor.construct(testPath);
  }, SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testCaseInsensitiveExtensions) {
  SNLLibertyConstructor constructor(library_);
  // file name with uppercase extension; ensure constructor handles it (either parse or throw meaningful error)
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "case_insensitive_ext.LIB.GZ");
  // We accept either successful parse or a specific exception; assert no crash and that an exception is thrown for invalid content.
  try {
    constructor.construct(testPath);
    // If parsing succeeds, at least library name should be set
    EXPECT_FALSE(library_->getName().getString().empty());
  } catch (const SNLLibertyConstructorException& e) {
    std::string msg = e.what();
    EXPECT_TRUE(msg.find("gzip") != std::string::npos || msg.find("parse") != std::string::npos || msg.find("Liberty") != std::string::npos);
  }
}

TEST_F(SNLLibertyConstructorTest0, testPermissionDeniedFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "permission_denied.lib");
  // The file should exist but be unreadable (mode 000). Expect an exception when opening.
  EXPECT_THROW({
    constructor.construct(testPath);
  }, SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testEmptyFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / "benchmarks" / "errors" / "empty.lib");
  // Empty file should cause parser to fail; expect YosysLibertyException or SNLLibertyConstructorException
  try {
    constructor.construct(testPath);
    FAIL() << "Expected parse error for empty file";
  } catch (const naja::liberty::YosysLibertyException& e) {
    SUCCEED();
  } catch (const SNLLibertyConstructorException& e) {
    SUCCEED();
  } catch (...) {
    FAIL() << "Expected YosysLibertyException or SNLLibertyConstructorException";
  }
}

// Optional: test parser throwing unexpected std::exception (requires test harness to simulate parser behavior).
// If you can inject a mock or stub for Yosys::LibertyParser to throw std::runtime_error, add a test similar to:
//
// TEST_F(SNLLibertyConstructorTest0, testParserThrowsStdException) {
//   // This test requires ability to substitute/mocK Yosys::LibertyParser to throw std::runtime_error.
//   // If mocking is available, ensure SNLLibertyConstructor wraps and rethrows as SNLLibertyConstructorException.
// }

