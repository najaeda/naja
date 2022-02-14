#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLNetlists.h"

#include "SNLUniverse.h"

#include "SNLFlattener.h"
#include "SNLFlattenerInstanceTree.h"

using namespace naja::SNL;

class SNLFlattenerTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      top_ = SNLNetlists::createNetlist0(db_);
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
  SNLFlattener flattener;
  flattener.process(top_);
  SNLFlattenerInstanceTree* tree = flattener.getTree();
  tree->print(std::cerr);
  ASSERT_NE(nullptr, tree);
}