#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLUtils.h"

#include "SNLVRLConstructor.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTest1, test) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test0.v");
  ASSERT_EQ(3, library_->getDesigns().size());
  auto mod0 = library_->getDesign(SNLName("mod0"));
  auto mod1 = library_->getDesign(SNLName("mod1"));
  auto test = library_->getDesign(SNLName("test")); 
  ASSERT_TRUE(mod0);
  ASSERT_TRUE(test);
  EXPECT_TRUE(mod0->getNets().empty());
  EXPECT_TRUE(test->getNets().empty());
  EXPECT_TRUE(mod0->getInstances().empty());
  EXPECT_TRUE(test->getInstances().empty());

  EXPECT_EQ(2, mod0->getTerms().size());
  auto mod0i0 = mod0->getScalarTerm(SNLName("i0"));
  ASSERT_NE(mod0i0, nullptr);
  EXPECT_EQ(mod0i0->getDirection(), SNLTerm::Direction::Input);
  auto mod0o0 = mod0->getScalarTerm(SNLName("o0"));
  EXPECT_NE(mod0o0, nullptr);
  EXPECT_EQ(mod0o0->getDirection(), SNLTerm::Direction::Output);
  
  EXPECT_EQ(2, mod1->getTerms().size());
  auto mod1i = mod1->getBusTerm(SNLName("i"));
  ASSERT_NE(mod1i, nullptr);
  EXPECT_EQ(mod1i->getDirection(), SNLTerm::Direction::Input);
  EXPECT_EQ(5, mod1i->getSize());
  EXPECT_EQ(4, mod1i->getMSB());
  EXPECT_EQ(0, mod1i->getLSB());

  {
    EXPECT_EQ(3, test->getTerms().size());
    auto i = test->getTerm(SNLName("i"));
    EXPECT_NE(i, nullptr);
    EXPECT_EQ(i->getDirection(), SNLTerm::Direction::Input);
    auto o = test->getTerm(SNLName("o"));
    EXPECT_NE(o, nullptr);
    EXPECT_EQ(o->getDirection(), SNLTerm::Direction::Output);
    auto io = test->getTerm(SNLName("io"));
    EXPECT_NE(io, nullptr);
    EXPECT_EQ(io->getDirection(), SNLTerm::Direction::InOut);
  }
  
  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test0.v");
  auto top = SNLUtils::findTop(library_);
  EXPECT_EQ(top, test);
  //2 assign nets + 10 (3 terms) standard nets
  EXPECT_EQ(12, test->getNets().size());
  using Nets = std::vector<SNLNet*>;
  Nets nets(test->getNets().begin(), test->getNets().end());
  ASSERT_EQ(12, nets.size());
  EXPECT_TRUE(nets[0]->isAnonymous());
  EXPECT_TRUE(nets[1]->isAnonymous());
  for (size_t i=2; i<12; ++i) {
    EXPECT_FALSE(nets[i]->isAnonymous());
  }

  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[0]));
  EXPECT_EQ(SNLNet::Type::Assign0, dynamic_cast<SNLScalarNet*>(nets[0])->getType());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[1]));
  EXPECT_EQ(SNLNet::Type::Assign1, dynamic_cast<SNLScalarNet*>(nets[1])->getType());

  EXPECT_EQ("i", nets[2]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[2]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[2])->getType());

  EXPECT_EQ("o", nets[3]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[3]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[3])->getType());

  EXPECT_EQ("io", nets[4]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[4]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[4])->getType());

  EXPECT_EQ("net0", nets[5]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[5]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[5])->getType());

  EXPECT_EQ("net1", nets[6]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[6]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[6])->getType());

  EXPECT_EQ("net2", nets[7]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[7]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[7])->getType());

  EXPECT_EQ("net3", nets[8]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[8]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[8])->getType());

  EXPECT_EQ("net4", nets[9]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLBusNet*>(nets[9]));
  EXPECT_EQ(3, dynamic_cast<SNLBusNet*>(nets[9])->getMSB());
  EXPECT_EQ(-1, dynamic_cast<SNLBusNet*>(nets[9])->getLSB());
  for (auto bit: dynamic_cast<SNLBusNet*>(nets[9])->getBits()) {
    EXPECT_EQ(SNLNet::Type::Standard, bit->getType());
  }

  EXPECT_EQ("constant0", nets[10]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[10]));
  EXPECT_EQ(SNLNet::Type::Supply0, dynamic_cast<SNLScalarNet*>(nets[10])->getType());

  EXPECT_EQ("constant1", nets[11]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[11]));
  EXPECT_EQ(SNLNet::Type::Supply1, dynamic_cast<SNLScalarNet*>(nets[11])->getType());

  ASSERT_EQ(4, test->getInstances().size());

  using Instances = std::vector<SNLInstance*>;
  Instances instances(test->getInstances().begin(), test->getInstances().end());
  ASSERT_EQ(4, instances.size());
  EXPECT_EQ("inst0", instances[0]->getName().getString());
  EXPECT_EQ("inst1", instances[1]->getName().getString());
  EXPECT_EQ("inst2", instances[2]->getName().getString());
  EXPECT_EQ("inst3", instances[3]->getName().getString());
  EXPECT_EQ(mod0, instances[0]->getModel());
  EXPECT_EQ(mod0, instances[1]->getModel());
  EXPECT_EQ(mod1, instances[2]->getModel());
  EXPECT_EQ(mod1, instances[3]->getModel());

  auto inst1i0 = instances[1]->getInstTerm(mod0i0);
  ASSERT_NE(nullptr, inst1i0);
  EXPECT_EQ(nets[0], inst1i0->getNet());

  //5'h1A == 5'b11010 connected to inst2.i[4:0] 
  EXPECT_EQ(nets[0], instances[2]->getInstTerm(mod1i->getBit(0))->getNet());
  EXPECT_EQ(nets[1], instances[2]->getInstTerm(mod1i->getBit(1))->getNet());
  EXPECT_EQ(nets[0], instances[2]->getInstTerm(mod1i->getBit(2))->getNet());
  EXPECT_EQ(nets[1], instances[2]->getInstTerm(mod1i->getBit(3))->getNet());
  EXPECT_EQ(nets[1], instances[2]->getInstTerm(mod1i->getBit(4))->getNet());
}