// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLParameter.h"
using namespace naja::NL;

class SNLParameterTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      designsLib_ = NLLibrary::create(db);
      design_ = SNLDesign::create(designsLib_);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      design_ = nullptr;
    }
    NLLibrary*   designsLib_ {nullptr};
    SNLDesign*    design_     {nullptr};
};

TEST_F(SNLParameterTest, test) {
  ASSERT_NE(NLUniverse::get(), nullptr);
  ASSERT_NE(design_, nullptr);
  EXPECT_TRUE(design_->getParameters().empty());

  auto param1 = SNLParameter::create(design_, NLName("PARAM1"), SNLParameter::Type::Decimal, "45");
  ASSERT_NE(nullptr, param1);
  EXPECT_EQ(NLName("PARAM1"), param1->getName());
  EXPECT_EQ(SNLParameter::Type::Decimal, param1->getType());
  EXPECT_EQ("45", param1->getValue());
  EXPECT_EQ(design_, param1->getDesign());

  EXPECT_FALSE(design_->getParameters().empty());
  EXPECT_EQ(1, design_->getParameters().size());

  auto designParam1 = design_->getParameter(NLName("PARAM1"));
  ASSERT_NE(designParam1, nullptr);

  auto param2 = SNLParameter::create(design_, NLName("PARAM2"), SNLParameter::Type::Binary, "56");
  ASSERT_NE(nullptr, param2);
  EXPECT_EQ(NLName("PARAM2"), param2->getName());
  EXPECT_EQ(SNLParameter::Type::Binary, param2->getType());
  EXPECT_EQ("56", param2->getValue());
  EXPECT_EQ(design_, param2->getDesign());

  EXPECT_FALSE(design_->getParameters().empty());
  EXPECT_EQ(2, design_->getParameters().size());

  auto designParam2 = design_->getParameter(NLName("PARAM2"));
  ASSERT_NE(designParam2, nullptr);

  EXPECT_EQ(param1, designParam1);
  EXPECT_EQ(param2, designParam2);

  //Instance Parameters
  auto top = SNLDesign::create(design_->getLibrary());
  auto instance = SNLInstance::create(top, design_);
  EXPECT_TRUE(instance->getInstParameters().empty());
  auto instParam1 = SNLInstParameter::create(instance, param1, "73");
  auto instParam2 = SNLInstParameter::create(instance, param2, "87");
  EXPECT_EQ(2, instance->getInstParameters().size());
  EXPECT_EQ("PARAM1", instParam1->getName().getString());
  EXPECT_EQ("PARAM2", instParam2->getName().getString());
  EXPECT_EQ("73", instParam1->getValue());
  EXPECT_EQ("87", instParam2->getValue());
  EXPECT_EQ(instParam1, instance->getInstParameter(NLName("PARAM1")));
  EXPECT_EQ(instParam2, instance->getInstParameter(NLName("PARAM2")));

  //Change value
  instParam1->setValue("99");
  EXPECT_EQ("99", instParam1->getValue());

  //InstParam destruction
  instParam1->destroy();
  EXPECT_EQ(1, instance->getInstParameters().size());
  EXPECT_EQ(nullptr, instance->getInstParameter(NLName("PARAM1")));

  using ParamsVector = std::vector<SNLParameter*>;
  ParamsVector paramsVector(design_->getParameters().begin(), design_->getParameters().end());
  ASSERT_EQ(2, paramsVector.size());
  EXPECT_EQ(param1, paramsVector[0]);
  EXPECT_EQ(param2, paramsVector[1]);
  EXPECT_THAT(std::vector(design_->getParameters().begin(), design_->getParameters().end()),
    ElementsAre(param1, param2));

  EXPECT_THROW(SNLParameter::create(design_, NLName("PARAM1"), SNLParameter::Type::Decimal, "56"), NLException);

  param1->destroy();
  EXPECT_EQ(nullptr, design_->getParameter(NLName("PARAM1")));
  EXPECT_NE(nullptr, design_->getParameter(NLName("PARAM2")));
  EXPECT_EQ(1, design_->getParameters().size());
  EXPECT_THAT(std::vector(design_->getParameters().begin(), design_->getParameters().end()),
    ElementsAre(param2));
}

TEST_F(SNLParameterTest, testInstanceParameterCreationError) {
  auto model1 = SNLDesign::create(designsLib_);
  auto param = SNLParameter::create(model1, NLName("PARAM"), SNLParameter::Type::String, "TEST");
  auto model2 = SNLDesign::create(designsLib_);
  auto top = SNLDesign::create(designsLib_);
  auto instance = SNLInstance::create(top, model2);
  EXPECT_THROW(
    SNLInstParameter::create(instance, param, "ERROR"),
    NLException);
}
