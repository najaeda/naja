#include "gtest/gtest.h"

#include "SNLUniverse.h"
using namespace SNL;

class SNLDBTest: public ::testing::Test {
  protected:
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLDBTest, test) {
  ASSERT_FALSE(SNLUniverse::get());
  SNLUniverse::create();
  ASSERT_TRUE(SNLUniverse::get());
  auto universe = SNLUniverse::get();
  ASSERT_TRUE(universe);
  SNLDB* db1 = SNLDB::create(universe);
  ASSERT_TRUE(db1);
  EXPECT_EQ(1, db1->getID());
  EXPECT_EQ(universe->getDB(1), db1);
  SNLDB* db2 = SNLDB::create(universe);
  ASSERT_TRUE(db2);
  EXPECT_EQ(2, db2->getID());
  EXPECT_EQ(universe->getDB(2), db2);

  EXPECT_TRUE(db1->getLibraries().empty());
  EXPECT_EQ(0, db1->getLibraries().size());
  EXPECT_TRUE(db2->getLibraries().empty());
  EXPECT_EQ(0, db2->getLibraries().size());
    
  db1->destroy();
  EXPECT_FALSE(universe->getDB(1));
}
