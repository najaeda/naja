#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include <filesystem>
#include <fstream>

#include "SNLNetlist0.h"

#include "SNLUniverse.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLFlattener.h"
#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerInstanceTreeNode.h"
#include "SNLFlattenerNetForest.h"
#include "SNLFlattenerNetTreeNode.h"

using namespace naja::SNL;

#ifndef FLATTENER_DUMP_PATHS
#define FLATTENER_DUMP_PATHS "Undefined"
#endif

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

  SNLFlattenerInstanceTree* instanceTree = flattener.getInstanceTree();
  ASSERT_NE(nullptr, instanceTree);
  std::filesystem::path dumpsPath(FLATTENER_DUMP_PATHS);
  std::filesystem::path instanceTreePath = dumpsPath/"SNLFlattenerTest0Test0InstanceTree.debug";
  std::ofstream instanceTreeFile;
  instanceTreeFile.open(instanceTreePath, std::ios::out);
  instanceTree->print(instanceTreeFile);

  SNLFlattenerNetForest* forest = flattener.getNetForest();
  ASSERT_NE(nullptr, forest);
  std::filesystem::path netForestPath = dumpsPath/"SNLFlattenerTest0Test0NetForest.debug";
  std::ofstream netForestFile;
  netForestFile.open(netForestPath, std::ios::out);
  forest->print(netForestFile);

  auto root = instanceTree->getRoot();
  ASSERT_NE(nullptr, root);
  EXPECT_TRUE(root->isRoot());
  EXPECT_FALSE(root->isLeaf());
  EXPECT_EQ(nullptr, root->getParent());
  EXPECT_EQ(nullptr, root->getInstance());
  auto top = root->getDesign();
  ASSERT_NE(nullptr, top);
  EXPECT_EQ(top_, top);

  EXPECT_EQ(instanceTree, root->getTree());
  auto ins0Node = root->getChildNode(SNLNetlist0::getTopIns0());
  ASSERT_NE(nullptr, ins0Node);
  auto ins1Node = root->getChildNode(SNLNetlist0::getTopIns1());
  ASSERT_NE(nullptr, ins1Node);
  EXPECT_EQ(nullptr, root->getChildNode(nullptr));
  EXPECT_EQ(root, ins0Node->getParent());
  EXPECT_EQ(root, ins1Node->getParent());
  EXPECT_EQ(instanceTree, ins0Node->getTree());
  EXPECT_EQ(instanceTree, ins1Node->getTree());
  EXPECT_FALSE(ins0Node->isRoot());
  EXPECT_FALSE(ins1Node->isRoot());
  EXPECT_FALSE(ins0Node->isLeaf());
  EXPECT_FALSE(ins1Node->isLeaf());
  EXPECT_EQ(nullptr, ins0Node->getDesign());
  EXPECT_EQ(nullptr, ins1Node->getDesign());

  EXPECT_FALSE(root->getChildren().empty());
  EXPECT_EQ(2, root->getChildren().size());
  EXPECT_THAT(std::vector(root->getChildren().begin(), root->getChildren().end()),
    ElementsAre(ins0Node, ins1Node));

  EXPECT_FALSE(root->getLeaves().empty());
  EXPECT_EQ(4, root->getLeaves().size());

  EXPECT_FALSE(forest->getTrees().empty());
  EXPECT_EQ(4, forest->getTrees().size());
  using NetTrees = std::vector<SNLFlattenerNetTree*>;
  NetTrees netTrees(forest->getTrees().begin(), forest->getTrees().end());
  EXPECT_EQ(4, netTrees.size());

  auto netTree0 = netTrees[0];
  EXPECT_EQ(0, netTree0->getID());
  auto netTree0Root = netTree0->getRoot();
  ASSERT_TRUE(netTree0Root);
  EXPECT_EQ(netTree0, netTree0Root->getTree());
  EXPECT_EQ(forest, netTree0Root->getForest());
  //netTree0 is in top
  EXPECT_EQ(root, netTree0Root->getInstanceTreeNode());
  EXPECT_EQ(nullptr, netTree0Root->getInstTerm());
  EXPECT_EQ(nullptr, netTree0Root->getTerm());
  const SNLBitNet* netTree0RootNet = netTree0Root->getNet();
  EXPECT_NE(nullptr, netTree0RootNet);
  EXPECT_EQ(SNLID::DesignObjectID(0), netTree0RootNet->getID());
  auto netTree0RootBusNetBit = dynamic_cast<const SNLBusNetBit*>(netTree0RootNet);
  EXPECT_TRUE(netTree0RootBusNetBit);
  EXPECT_EQ(netTree0Root, root->getNetNode(netTree0RootBusNetBit));

  EXPECT_FALSE(netTree0Root->getChildren().empty());
  //one child is term node, other is inst term
  EXPECT_EQ(2, netTree0Root->getChildren().size());
  using NetTreeNodes = std::vector<SNLFlattenerNetTreeNode*>;
  NetTreeNodes netTreeNodes(netTree0Root->getChildren().begin(), netTree0Root->getChildren().end());
  EXPECT_EQ(2, netTreeNodes.size());
  auto termNode = netTreeNodes[0];
  EXPECT_TRUE(termNode->isTerm());
  EXPECT_FALSE(termNode->isInstTerm());
  EXPECT_FALSE(termNode->isRoot());
  EXPECT_TRUE(termNode->isLeaf());
  EXPECT_EQ(netTree0Root, termNode->getParent());
  EXPECT_EQ(netTree0, termNode->getTree());
  EXPECT_EQ(nullptr, termNode->getInstTerm());
  EXPECT_NE(nullptr, termNode->getTerm());

  auto instTermNode = netTreeNodes[1];
  EXPECT_FALSE(instTermNode->isTerm());
  EXPECT_TRUE(instTermNode->isInstTerm());
  EXPECT_FALSE(instTermNode->isRoot());
  EXPECT_FALSE(instTermNode->isLeaf());
  EXPECT_EQ(netTree0Root, instTermNode->getParent());
  EXPECT_EQ(netTree0, instTermNode->getTree());
  EXPECT_EQ(nullptr, instTermNode->getTerm());
  EXPECT_NE(nullptr, instTermNode->getInstTerm());
  EXPECT_EQ(ins0Node, instTermNode->getInstanceTreeNode());
  EXPECT_EQ(instTermNode, ins0Node->getInstTermNode(instTermNode->getInstTerm()->getTerm()));
  EXPECT_EQ(nullptr, root->getInstTermNode(instTermNode->getInstTerm()->getTerm()));
  EXPECT_EQ(nullptr, ins0Node->getInstTermNode(nullptr));
  EXPECT_EQ(nullptr, ins0Node->getNetNode(netTree0RootBusNetBit));

  EXPECT_EQ(2, netTree0Root->getLeaves().size());
}