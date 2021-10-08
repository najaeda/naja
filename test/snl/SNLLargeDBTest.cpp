#include "gtest/gtest.h"

#include "SNLDB.h"
using namespace SNL;

class SNLLargeDBTest: public ::testing::Test {
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

TEST_F(SNLLargeDBTest, test) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_TRUE(library);
  for (size_t i=0; i<10000000; i++) {
    SNLName designName = "DESIGN_" + std::to_string(i);
    SNLDesign::create(library, designName);
  }
}
