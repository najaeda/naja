#include "gtest/gtest.h"

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
using namespace SNL;

class SNLDesignTest: public ::testing::Test {
  protected:
    void SetUp() override {
      db_ = SNLDB::create();
      SNLLibrary* library = SNLLibrary::create(db_, "MYLIB");
    }
    void TearDown() override {
      db_->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLDesignTest, testCreation) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_TRUE(library);
  SNLDesign* design = SNLDesign::create(library, "design");
  ASSERT_TRUE(design);
  EXPECT_EQ("design", design->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_FALSE(design->isAnonymous());
  SNLDesign* model = SNLDesign::create(library, "model");
  ASSERT_TRUE(model);
  EXPECT_EQ("model", model->getName());
  EXPECT_EQ(1, model->getID());
  EXPECT_FALSE(model->isAnonymous());

  //anonymous design
  SNLDesign* anon = SNLDesign::create(library);
  ASSERT_TRUE(anon);
  EXPECT_EQ(2, anon->getID());
  EXPECT_TRUE(anon->isAnonymous());
}
