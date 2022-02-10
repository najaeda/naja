#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLNetlists.h"

#include "SNLUniverse.h"

#include "SNLFlattener.h"

using namespace naja::SNL;

class SNLFlattenerTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* primitiveslibrary = SNLLibrary::create(db_, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
      SNLLibrary* designslibrary = SNLLibrary::create(db_, SNLName("DESIGNS"));
      top_ = SNLNetlists::createNetlist0(primitiveslibrary, designslibrary);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDB*      db_;
    SNLDesign*  top_;
};

TEST_F(SNLFlattenerTest0, test0) {
  ASSERT_NE(nullptr, top_);
  SNLFlattener flattener(top_);
}