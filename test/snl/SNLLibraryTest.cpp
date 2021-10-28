#include "gtest/gtest.h"

#include "SNLDB.h"
using namespace SNL;

class SNLLibraryTest: public ::testing::Test {
  protected:
    void SetUp() override {
      db_ = SNLDB::create();
    }
    void TearDown() override {
      db_->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLLibraryTest, test) {
  SNLLibrary* lib1 = db_->getLibrary(SNLName("LIB1"));
  EXPECT_FALSE(lib1);
  lib1 = SNLLibrary::create(db_, SNLName("LIB1"));
  ASSERT_TRUE(lib1);
  SNLLibrary* testLib1 = db_->getLibrary(SNLName("LIB1"));
  ASSERT_TRUE(testLib1);
  EXPECT_EQ(testLib1, lib1);
  EXPECT_EQ("LIB1", lib1->getName());

  SNLLibrary* lib2 = SNLLibrary::create(db_, SNLName("LIB2"));
  ASSERT_TRUE(lib2);
  SNLLibrary* testLib2 = db_->getLibrary(SNLName("LIB2"));
  ASSERT_TRUE(testLib2);
  EXPECT_EQ(testLib2, lib2);
  EXPECT_EQ("LIB2", lib2->getName());

  SNLLibrary* lib3 = SNLLibrary::create(lib2, SNLName("LIB1"));
  ASSERT_TRUE(lib3);
  SNLLibrary* testLib3 = lib2->getLibrary(SNLName("LIB1"));
  ASSERT_TRUE(testLib3);
  EXPECT_EQ(testLib3, lib3);
  EXPECT_EQ("LIB1", lib3->getName());

  SNLLibrary* lib4 = SNLLibrary::create(lib2, SNLName("LIB2"));
  ASSERT_TRUE(lib4);
  SNLLibrary* testLib4 = lib2->getLibrary(SNLName("LIB2"));
  ASSERT_TRUE(testLib4);
  EXPECT_EQ(testLib4, lib4);
  EXPECT_EQ("LIB2", lib4->getName());

  lib4->destroy();
  testLib4 = lib2->getLibrary(SNLName("LIB2"));
  EXPECT_FALSE(testLib4);

  lib2->destroy();
  testLib2 = db_->getLibrary(SNLName("LIB2"));
  EXPECT_FALSE(testLib2);
}
