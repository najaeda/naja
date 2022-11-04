#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLDBTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDBTest, test) {
  ASSERT_EQ(SNLUniverse::get(), nullptr);
  SNLUniverse::create();
  ASSERT_NE(SNLUniverse::get(), nullptr);
  auto universe = SNLUniverse::get();
  ASSERT_NE(universe, nullptr);
  SNLDB* db1 = SNLDB::create(universe);
  ASSERT_NE(db1, nullptr);
  EXPECT_EQ(1, db1->getID());
  EXPECT_EQ(universe->getDB(1), db1);
  EXPECT_EQ(SNLID(1), db1->getSNLID());
  EXPECT_EQ(db1, universe->getObject(SNLID(1)));
  SNLDB* db2 = SNLDB::create(universe);
  ASSERT_NE(db2, nullptr);
  EXPECT_EQ(2, db2->getID());
  EXPECT_EQ(universe->getDB(2), db2);
  EXPECT_EQ(SNLID(2), db2->getSNLID());
  EXPECT_EQ(db2, universe->getObject(SNLID(2)));

  EXPECT_FALSE(universe->getDBs().empty());
  EXPECT_EQ(3, universe->getDBs().size());
  EXPECT_FALSE(universe->getUserDBs().empty());
  EXPECT_EQ(2, universe->getUserDBs().size());

  EXPECT_TRUE(db1->getLibraries().empty());
  EXPECT_EQ(0, db1->getLibraries().size());
  EXPECT_TRUE(db2->getLibraries().empty());
  EXPECT_EQ(0, db2->getLibraries().size());
    
  db1->destroy();
  EXPECT_EQ(nullptr, universe->getDB(1));
  EXPECT_EQ(nullptr, universe->getObject(SNLID(1)));
}

TEST_F(SNLDBTest, testErrors) {
  EXPECT_THROW(SNLDB::create(nullptr), SNLException);
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  EXPECT_EQ(1, db->getID());
  EXPECT_THROW(SNLDB::create(SNLUniverse::get(), SNLID::DBID(1)), SNLException);
}
