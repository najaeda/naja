#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDesignModeling.h"
#include "SNLScalarTerm.h"
using namespace naja::SNL;

class SNLDesignModelingTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignModelingTest0, test0) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto designs = SNLLibrary::create(db);
  auto top = SNLDesign::create(designs, SNLName("top"));
  auto lut = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("LUT"));
  auto luti0 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I0"));
  auto luti1 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I1"));
  auto luti2 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I2"));
  auto luti3 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I3"));
  auto luto = SNLScalarTerm::create(lut, SNLTerm::Direction::Output, SNLName("O"));
  SNLDesignModeling::addCombinatorialArcs({luti0, luti1, luti2, luti3}, {luto});
  auto lutIns0 = SNLInstance::create(top, lut, SNLName("ins0"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(luto).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(luto).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(luto).begin(),
      SNLDesignModeling::getCombinatorialInputs(luto).end()),
    ElementsAre(luti0, luti1, luti2, luti3));
  //lutIns0/luto
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luto)).empty());
  //ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).size());
  
  //luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti0).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti0).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti0).end()),
    ElementsAre(luto));
  //lutIns0/luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).size());
  //EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luti0)).empty());
  //EXPECT_THAT(
  //  std::vector(
  //    SNLDesignModeling::getCombinatorialOutputs(luti0).begin(),
  //    SNLDesignModeling::getCombinatorialOutputs(luti0).end()),
  //  ElementsAre(luto));
  //


  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti1).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti1).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti1).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti2).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti2).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti2).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti3).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti3).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti3).end()),
    ElementsAre(luto));

  //auto reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("REG"));
}