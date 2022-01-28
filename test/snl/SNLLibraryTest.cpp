#include "gtest/gtest.h"

#include "SNLUniverse.h"
using namespace SNL;

class SNLLibraryTest: public ::testing::Test {
  protected:
    void SetUp() override {
      universe_ = SNLUniverse::create();
    }
    void TearDown() override {
      universe_->destroy();
    }
    SNLUniverse*  universe_;
};

TEST_F(SNLLibraryTest, test) {
  SNLDB* db = SNLDB::create(universe_);
  EXPECT_EQ(0, db->getLibraries().size());
  EXPECT_TRUE(db->getLibraries().empty());

  SNLLibrary* lib1 = db->getLibrary(SNLName("LIB1"));
  EXPECT_FALSE(lib1);
  lib1 = SNLLibrary::create(db, SNLName("LIB1"));
  ASSERT_TRUE(lib1);
  SNLLibrary* testLib1 = db->getLibrary(SNLName("LIB1"));
  ASSERT_TRUE(testLib1);
  EXPECT_EQ(testLib1, lib1);
  EXPECT_EQ("LIB1", lib1->getName().getString());

  EXPECT_EQ(1, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getDesigns().size());
  EXPECT_TRUE(lib1->getDesigns().empty());

  SNLLibrary* lib2 = SNLLibrary::create(db, SNLName("LIB2"));
  ASSERT_TRUE(lib2);
  SNLLibrary* testLib2 = db->getLibrary(SNLName("LIB2"));
  ASSERT_TRUE(testLib2);
  EXPECT_EQ(testLib2, lib2);
  EXPECT_EQ("LIB2", lib2->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getDesigns().size());
  EXPECT_TRUE(lib1->getDesigns().empty());
  EXPECT_EQ(0, lib2->getLibraries().size());
  EXPECT_TRUE(lib2->getLibraries().empty());
  EXPECT_EQ(0, lib2->getDesigns().size());
  EXPECT_TRUE(lib2->getDesigns().empty());

  SNLLibrary* lib3 = SNLLibrary::create(lib2, SNLName("LIB1"));
  ASSERT_TRUE(lib3);
  SNLLibrary* testLib3 = lib2->getLibrary(SNLName("LIB1"));
  ASSERT_TRUE(testLib3);
  EXPECT_EQ(testLib3, lib3);
  EXPECT_EQ("LIB1", lib3->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(1, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());
  EXPECT_EQ(0, lib3->getDesigns().size());
  EXPECT_TRUE(lib3->getDesigns().empty());

  SNLLibrary* lib4 = SNLLibrary::create(lib2, SNLName("LIB2"));
  ASSERT_TRUE(lib4);
  SNLLibrary* testLib4 = lib2->getLibrary(SNLName("LIB2"));
  ASSERT_TRUE(testLib4);
  EXPECT_EQ(testLib4, lib4);
  EXPECT_EQ("LIB2", lib4->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(2, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());
  EXPECT_EQ(0, lib4->getLibraries().size());
  EXPECT_TRUE(lib4->getLibraries().empty());

  lib4->destroy();
  testLib4 = lib2->getLibrary(SNLName("LIB2"));
  EXPECT_FALSE(testLib4);

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(1, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());

  lib2->destroy();
  testLib2 = db->getLibrary(SNLName("LIB2"));
  EXPECT_FALSE(testLib2);

  EXPECT_EQ(1, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getDesigns().size());
  EXPECT_TRUE(lib1->getDesigns().empty());
}
