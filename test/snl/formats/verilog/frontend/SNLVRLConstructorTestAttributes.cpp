// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"

#include "SNLVRLConstructor.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestAttributes: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestAttributes, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_attributes.v");

#if 0
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
#endif
}