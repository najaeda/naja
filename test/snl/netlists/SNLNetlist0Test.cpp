#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLNetlist0.h"

#include "SNLUniverse.h"

using namespace naja::SNL;

class SNLNetlistTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      SNLNetlist0::create(db);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLNetlistTest0, test) {
  auto universe = SNLUniverse::get();
  ASSERT_NE(nullptr, universe);
  ASSERT_NE(nullptr, SNLNetlist0::getDB());
  ASSERT_NE(nullptr, SNLNetlist0::getDesignsLib());
  ASSERT_NE(nullptr, SNLNetlist0::getTop());
  ASSERT_NE(nullptr, SNLNetlist0::getTopIns0());
  ASSERT_NE(nullptr, SNLNetlist0::getTopIns1());
}