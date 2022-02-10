#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLParameter.h"

using namespace naja::SNL;

class SNLParameterTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto designsLib = SNLLibrary::create(db);
      design_ = SNLDesign::create(designsLib);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      design_ = nullptr;
    }

   SNLDesign*   design_ {nullptr};
};

TEST_F(SNLParameterTest, test) {
  ASSERT_NE(SNLUniverse::get(), nullptr);
  ASSERT_NE(design_, nullptr);
  EXPECT_TRUE(design_->getParameters().empty());

  auto param1 = SNLParameter::create(design_, SNLName("PARAM1"), "45");
  ASSERT_NE(nullptr, param1);
  EXPECT_EQ(SNLName("PARAM1"), param1->getName());
  EXPECT_EQ("45", param1->getValue());

  EXPECT_FALSE(design_->getParameters().empty());
  EXPECT_EQ(1, design_->getParameters().size());

  auto designParam1 = design_->getParameter(SNLName("PARAM1"));
  ASSERT_NE(designParam1, nullptr);

  auto param2 = SNLParameter::create(design_, SNLName("PARAM2"), "56");
  ASSERT_NE(nullptr, param2);
  EXPECT_EQ(SNLName("PARAM2"), param2->getName());
  EXPECT_EQ("56", param2->getValue());

  EXPECT_FALSE(design_->getParameters().empty());
  EXPECT_EQ(2, design_->getParameters().size());

  auto designParam2 = design_->getParameter(SNLName("PARAM2"));
  ASSERT_NE(designParam2, nullptr);

  EXPECT_EQ(param1, designParam1);
  EXPECT_EQ(param2, designParam2);

  using ParamsVector = std::vector<SNLParameter*>;
  ParamsVector paramsVector(design_->getParameters().begin(), design_->getParameters().end());
  ASSERT_EQ(2, paramsVector.size());
  EXPECT_EQ(param1, paramsVector[0]);
  EXPECT_EQ(param2, paramsVector[1]);
  EXPECT_THAT(std::vector(design_->getParameters().begin(), design_->getParameters().end()),
    ElementsAre(param1, param2));
}