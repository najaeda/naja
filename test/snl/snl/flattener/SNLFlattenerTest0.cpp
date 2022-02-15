#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLNetlist0.h"

#include "SNLUniverse.h"

#include "SNLFlattener.h"
#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerInstanceTreeNode.h"

using namespace naja::SNL;

class SNLFlattenerTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      top_ = SNLNetlist0::create(db_);
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
  auto root = tree->getRoot();
  ASSERT_NE(nullptr, root);
  auto ins0Node = root->getChildNode(SNLNetlist0::getTopIns0());
  ASSERT_NE(nullptr, ins0Node);
  auto ins1Node = root->getChildNode(SNLNetlist0::getTopIns1());
  ASSERT_NE(nullptr, ins1Node);
  EXPECT_EQ(root, ins0Node->getParent());
  EXPECT_EQ(root, ins1Node->getParent());
  EXPECT_EQ(tree, ins0Node->getTree());
  EXPECT_EQ(tree, ins1Node->getTree());
}