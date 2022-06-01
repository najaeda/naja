#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

class SNLVRLDumperTestParameters: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      SNLDB* db = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
      top_ = SNLDesign::create(library, SNLName("top"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  top_;
};

TEST_F(SNLVRLDumperTestParameters, test0) {
  ASSERT_TRUE(top_);
  SNLParameter::create(top_, SNLName("PARAM0"), "8");
  SNLParameter::create(top_, SNLName("PARAM1"), "14");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParameters0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString());
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testParameters0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = "diff " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}