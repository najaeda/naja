#include "gtest/gtest.h"

#include "SNLDB.h"

using namespace SNL;

class SNLNetTest: public ::testing::Test {
  protected:
    void SetUp() override {
      db_ = SNLDB::create();
      library_ = SNLLibrary::create(db_, "LIB");
      SNLDesign* design = SNLDesign::create(library_, "Design");
    }
    void TearDown() override {
      db_->destroy();
    }
  protected:
    SNLDB*      db_;
    SNLLibrary* library_;
};

TEST_F(SNLNetTest, testCreation) {
}
