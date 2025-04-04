// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

using namespace naja::NL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

//Test assigns
class SNLVRLDumperTestLibraryDump: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      auto prims = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("PRIMS"));

      auto andp = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("AND"));
      SNLScalarTerm::create(andp, SNLTerm::Direction::Input, NLName("I0"));
      SNLScalarTerm::create(andp, SNLTerm::Direction::Input, NLName("I1"));
      SNLScalarTerm::create(andp, SNLTerm::Direction::Output, NLName("Q"));

      auto ram = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("RAM"));
      SNLScalarTerm::create(ram, SNLTerm::Direction::Input, NLName("CLK"));
      SNLBusTerm::create(ram, SNLTerm::Direction::Input, 31, 0, NLName("A0"));
      SNLBusTerm::create(ram, SNLTerm::Direction::Input, 31, 0, NLName("A1"));
      SNLBusTerm::create(ram, SNLTerm::Direction::Output, 127, 0, NLName("Q"));
      SNLParameter::create(ram, NLName("INVERTED_CLK"), SNLParameter::Type::Boolean, "0");
      SNLParameter::create(ram, NLName("WIDTH"), SNLParameter::Type::Decimal, "56");
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB*      db_;
};

TEST_F(SNLVRLDumperTestLibraryDump, test0) {
  auto lib = db_->getLibrary(NLName("PRIMS"));  
  ASSERT_TRUE(lib);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testLibrary";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setLibraryFileName("primitives.v");
  dumper.dumpLibrary(lib, outPath);

  outPath = outPath / "primitives.v";

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testLibrary" / "primitives.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestLibraryDump, testErrors) {
  auto lib = db_->getLibrary(NLName("PRIMS"));  
  ASSERT_TRUE(lib);
  std::filesystem::path outPath("ERROR");
  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setLibraryFileName("primitives.v");
  EXPECT_THROW(dumper.dumpLibrary(lib, outPath), SNLVRLDumperException);
}
