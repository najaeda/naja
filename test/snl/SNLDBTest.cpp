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
  EXPECT_EQ(1, db1->getID());
  SNLDB* db2 = SNLDB::create(universe);
  EXPECT_EQ(2, db2->getID());
  db1->destroy();
}
