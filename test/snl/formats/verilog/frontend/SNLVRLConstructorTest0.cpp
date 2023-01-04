#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLConstructor.h"

#include "SNLUniverse.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTest0: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTest0, test) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test0.v");
  ASSERT_EQ(2, library_->getDesigns().size());
  auto mod0 = library_->getDesign(SNLName("mod0"));
  auto test = library_->getDesign(SNLName("test")); 
  ASSERT_TRUE(mod0);
  ASSERT_TRUE(test);
  EXPECT_TRUE(mod0->getNets().empty());
  EXPECT_TRUE(test->getNets().empty());
  EXPECT_TRUE(mod0->getInstances().empty());
  EXPECT_TRUE(test->getInstances().empty());
  EXPECT_EQ(2, mod0->getTerms().size());
}