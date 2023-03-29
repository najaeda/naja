#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLUtils.h"

#include "SNLPrimitivesLoader.h"
#include "SNLVRLConstructor.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTest2: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTest2, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitivesPath = std::filesystem::path(SNL_VRL_BENCHMARKS_PATH);
  primitivesPath /= "primitives.py";
  SNLPrimitivesLoader::load(prims, primitivesPath);
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test1.v");

  ASSERT_EQ(2, library_->getDesigns().size());
  auto model = library_->getDesign(SNLName("model"));
  ASSERT_NE(model, nullptr);
  ASSERT_EQ(3, model->getTerms().size());
  auto test = library_->getDesign(SNLName("test"));
  ASSERT_NE(test, nullptr);

  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test1.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_EQ(top, test);
  ASSERT_EQ(1, model->getInstances().size());
  auto lut = model->getInstance(SNLName("lut"));
  ASSERT_NE(lut, nullptr);
  EXPECT_EQ("lut", lut->getName().getString());
  auto lutModel = lut->getModel();
  ASSERT_NE(lutModel, nullptr);
  EXPECT_EQ("LUT4", lutModel->getName().getString());
  ASSERT_EQ(1, lut->getInstParameters().size());
  auto initParam = *(lut->getInstParameters().begin());
  EXPECT_EQ("INIT", initParam->getName().getString());
}