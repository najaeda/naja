#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLPath.h"

class SNLPathTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNL::SNLUniverse::create();
      auto db = SNL::SNLDB::create(universe);
      auto primitivesLib = SNL::SNLLibrary::create(db, SNL::SNLLibrary::Type::Primitives);
      auto designsLib = SNL::SNLLibrary::create(db);
      auto prim = SNL::SNLDesign::create(primitivesLib, SNL::SNLDesign::Type::Primitive);
      auto top = SNL::SNLDesign::create(designsLib);
      auto h0 = SNL::SNLDesign::create(designsLib);
      auto h1 = SNL::SNLDesign::create(designsLib);
      auto h2 = SNL::SNLDesign::create(designsLib);
      primInstance_ = SNL::SNLInstance::create(h2, prim);
      h2Instance_ = SNL::SNLInstance::create(h1, h2);
      h1Instance_ = SNL::SNLInstance::create(h0, h1);
      h0Instance_ = SNL::SNLInstance::create(top, h0);
    }
    void TearDown() override {
      SNL::SNLUniverse::get()->destroy();
    }

    SNL::SNLInstance* primInstance_ {nullptr};
    SNL::SNLInstance* h2Instance_   {nullptr};
    SNL::SNLInstance* h1Instance_   {nullptr};
    SNL::SNLInstance* h0Instance_   {nullptr};
};

TEST_F(SNLPathTest, test) {
  ASSERT_NE(SNL::SNLUniverse::get(), nullptr);
  auto emptyPath = SNL::SNLPath();
  EXPECT_TRUE(emptyPath.empty());

  ASSERT_NE(h0Instance_, nullptr);
  auto topInstancePath = SNL::SNLPath(h0Instance_);
  EXPECT_FALSE(topInstancePath.empty());
  EXPECT_EQ(h0Instance_, topInstancePath.getHeadInstance());
  EXPECT_EQ(h0Instance_, topInstancePath.getTailInstance());
  EXPECT_EQ(h0Instance_->getModel(), topInstancePath.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), topInstancePath.getDesign());
  EXPECT_EQ(SNL::SNLPath(), topInstancePath.getTailPath());
  EXPECT_EQ(SNL::SNLPath(), topInstancePath.getHeadPath());

  ASSERT_NE(h1Instance_, nullptr);
  EXPECT_EQ(h0Instance_->getModel(), h1Instance_->getDesign());
  auto h0LevelPath = SNL::SNLPath(topInstancePath, h1Instance_);
  EXPECT_FALSE(h0LevelPath.empty());
  EXPECT_EQ(topInstancePath, h0LevelPath.getHeadInstance());
  EXPECT_EQ(h1Instance_, h0LevelPath.getTailInstance());
  EXPECT_EQ(h1Instance_->getModel(), h0LevelPath.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), topInstancePath.getDesign());
  EXPECT_EQ(SNL::SNLPath(), topInstancePath.getTailPath());
  EXPECT_EQ(topInstancePath, topInstancePath.getHeadPath());
}
