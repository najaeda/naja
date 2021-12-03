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
      auto primInstance = SNL::SNLInstance::create(h2, prim);
      auto h2Instance = SNL::SNLInstance::create(h1, h2);
      auto h1Instance = SNL::SNLInstance::create(h0, h1);
      auto h0Instance = SNL::SNLInstance::create(top, h0);
    }
    void TearDown() override {
      SNL::SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLPathTest, test) {
  ASSERT_NE(SNL::SNLUniverse::get(), nullptr);
  auto emptyPath = SNL::SNLPath();
  EXPECT_TRUE(emptyPath.empty());
}
