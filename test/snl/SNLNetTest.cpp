#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLBusNet.h"

using namespace SNL;

class SNLNetTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto library = SNLLibrary::create(db, "LIB");
      design_ = SNLDesign::create(library, "Design");
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  design_;
};

TEST_F(SNLNetTest, testCreation) {
  ASSERT_TRUE(design_);
  EXPECT_EQ(SNLName("Design"), design_->getName());
  EXPECT_EQ(0, design_->getID());
  EXPECT_FALSE(design_->isAnonymous());
  EXPECT_EQ(design_, design_->getLibrary()->getDesign(0));
  EXPECT_EQ(design_, design_->getLibrary()->getDesign(SNLName("Design")));

  SNLBusNet* net0 = SNLBusNet::create(design_, SNLName("net0"));
  ASSERT_TRUE(net0);
}
