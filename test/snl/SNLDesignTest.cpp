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
  SNLDesign* model = SNLDesign::create(library, "model");
  ASSERT_TRUE(model);
  ASSERT_EQ("design", design->getName());
  ASSERT_EQ("model", model->getName());
}
