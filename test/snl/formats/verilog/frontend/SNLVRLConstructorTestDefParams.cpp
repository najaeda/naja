// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLUtils.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLException.h"

#include "SNLPyLoader.h"
#include "SNLVRLConstructor.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestDefParams: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestDefParams, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitivesPath = std::filesystem::path(SNL_VRL_BENCHMARKS_PATH);
  primitivesPath /= "primitives1.py";
  SNLPyLoader::loadPrimitives(prims, primitivesPath);
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_defparams.v");

  ASSERT_EQ(1, library_->getDesigns().size());
  auto ins_decode = library_->getDesign(SNLName("ins_decode"));
  ASSERT_NE(ins_decode, nullptr);
  ASSERT_EQ(5, ins_decode->getInstances().size());
  using Instances = std::vector<SNLInstance*>;
  Instances instances(ins_decode->getInstances().begin(), ins_decode->getInstances().end());
  EXPECT_THAT(instances,
    ElementsAre(
      ins_decode->getInstance(SNLName("decodes_in_0_0_1_0[7]")),
      ins_decode->getInstance(SNLName("decodes_in_0_a2_i_o3[8]")),
      ins_decode->getInstance(SNLName("decodes_RNO[6]")),
      ins_decode->getInstance(SNLName("GND_Z")),
      ins_decode->getInstance(SNLName("VCC_Z"))
    )
  );
  auto ins0 = ins_decode->getInstance(SNLName("decodes_in_0_0_1_0[7]"));
  ASSERT_NE(ins0, nullptr);
  EXPECT_EQ(1, ins0->getInstParameters().size());
  auto param = *(ins0->getInstParameters().begin());
  EXPECT_EQ("INIT", param->getName().getString());
  EXPECT_EQ("16'h5054", param->getValue());

  auto ins1 = ins_decode->getInstance(SNLName("decodes_in_0_a2_i_o3[8]"));
  ASSERT_NE(ins1, nullptr);
  EXPECT_TRUE(ins1->getInstParameters().empty());

  auto ins2 = ins_decode->getInstance(SNLName("decodes_RNO[6]"));
  ASSERT_NE(ins2, nullptr);
  EXPECT_EQ(1, ins2->getInstParameters().size());
  param = *(ins2->getInstParameters().begin());
  EXPECT_EQ("INIT", param->getName().getString());
  EXPECT_EQ("16'h0001", param->getValue());
}

TEST_F(SNLVRLConstructorTestDefParams, testErrors0) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(constructor.construct(benchmarksPath/"errors"/"error_defparams_0.v"), SNLException);
}

TEST_F(SNLVRLConstructorTestDefParams, testErrors1) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(constructor.construct(benchmarksPath/"errors"/"error_defparams_1.v"), SNLException);
}

TEST_F(SNLVRLConstructorTestDefParams, testErrors2) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(constructor.construct(benchmarksPath/"errors"/"error_defparams_2.v"), SNLException);
}