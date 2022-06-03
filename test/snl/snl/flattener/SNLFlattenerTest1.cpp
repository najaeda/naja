#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLNetlist0.h"

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"

#include "SNLFlattener.h"
#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerInstanceTreeNode.h"
#include "SNLFlattenerNetForest.h"

using namespace naja::SNL;

#ifndef FLATTENER_DUMP_PATHS
#define FLATTENER_DUMP_PATHS "Undefined"
#endif

class SNLFlattenerTest1: public ::testing::Test {
  //Test constant nets
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      auto modelLib = SNLLibrary::create(db_);
      auto model = SNLDesign::create(modelLib, SNLName("model0"));
      auto and3 = SNLDB0::getAND(3);

      auto ins0 = SNLInstance::create(model, and3, SNLName("ins0"));
      auto ins1 = SNLInstance::create(model, and3, SNLName("ins1"));
      auto ins2 = SNLInstance::create(model, and3, SNLName("ins2"));

      auto o0 = SNLScalarTerm::create(model, SNLTerm::Direction::Output, SNLName("o0"));
      auto o0Net = SNLScalarNet::create(model, SNLName("o0"));
      o0->setNet(o0Net);
      //ins2->setTermNet(SNLDB0::getANDOutput(and3), o0Net);

      //auto o1 = SNLScalarTerm::create(model, SNLTerm::Direction::Output, SNLName("o1"));
      //auto o1Net = SNLScalarNet::create(model, SNLName("o1"));
      //o1->setNet(o1Net);
      //o1Net->setType(SNLNet::Type::Assign0);

      //auto i = SNLBusTerm::create(model, SNLTerm::Direction::Input, 1, 0, SNLName("i"));
      //auto iNet = SNLBusNet::create(model, 1, 0, SNLName("i"));
      //i->setNet(iNet);

      //ins0->getInstTerm(SNLDB0::getANDInputs(and3)->getBit(0));
      //ins0->setTermNet(SNLDB0::getANDInputs(and3), 2, 1, iNet, 1, 0);

      //top_ = SNLNetlist0::create(db_);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDB*      db_;
    SNLDesign*  top_;
};

TEST_F(SNLFlattenerTest1, test0) {
#if 0
  ASSERT_NE(nullptr, top_);
  SNLFlattener flattener;
  flattener.process(top_);

  SNLFlattenerInstanceTree* tree = flattener.getInstanceTree();
  ASSERT_NE(nullptr, tree);
  std::filesystem::path dumpsPath(FLATTENER_DUMP_PATHS);
  std::filesystem::path instanceTreePath = dumpsPath/"SNLFlattenerTest0Test0InstanceTree.debug";
  std::ofstream instanceTreeFile;
  instanceTreeFile.open(instanceTreePath, std::ios::out);
  tree->print(instanceTreeFile);

  SNLFlattenerNetForest* forest = flattener.getNetForest();
  ASSERT_NE(nullptr, forest);
  std::filesystem::path netForestPath = dumpsPath/"SNLFlattenerTest0Test0NetForest.debug";
  std::ofstream netForestFile;
  netForestFile.open(netForestPath, std::ios::out);
  forest->print(netForestFile);

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
#endif
}