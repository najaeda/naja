#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

#include "SNLPrimitivesLoader.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestErrors: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestErrors, test0) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"error0.v");
  constructor.setFirstPass(false);
  EXPECT_THROW(constructor.parse(benchmarksPath/"error0.v"), SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test1) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"error1.v"),
    SNLVRLConstructorException);
}