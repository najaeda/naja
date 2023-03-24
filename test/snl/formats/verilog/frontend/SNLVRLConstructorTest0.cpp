#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLUtils.h"
#include "SNLVRLConstructor.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTest0: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTest0, test) {
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
  {
    auto i0 = mod0->getTerm(SNLName("i0"));
    ASSERT_NE(i0, nullptr);
    EXPECT_EQ(i0->getDirection(), SNLTerm::Direction::Input);
    auto o0 = mod0->getTerm(SNLName("o0"));
    EXPECT_NE(o0, nullptr);
    EXPECT_EQ(o0->getDirection(), SNLTerm::Direction::Output);
  }
  
  EXPECT_EQ(3, test->getTerms().size());
    {
    auto i = test->getTerm(SNLName("i"));
    ASSERT_NE(i, nullptr);
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
  //10 (3 terms) standard nets + 2 constant nets
  EXPECT_EQ(12, test->getNets().size());
  using Nets = std::vector<SNLNet*>;
  Nets nets(test->getNets().begin(), test->getNets().end());
  ASSERT_EQ(12, nets.size());
  for (size_t i=0; i<10; ++i) {
    EXPECT_FALSE(nets[i]->isAnonymous());
  }
  EXPECT_TRUE(nets[10]->isAnonymous());
  EXPECT_TRUE(nets[11]->isAnonymous());

  EXPECT_EQ("i", nets[0]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[0]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[0])->getType());

  EXPECT_EQ("o", nets[1]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[1]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[1])->getType());

  EXPECT_EQ("io", nets[2]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[2]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[2])->getType());

  EXPECT_EQ("net0", nets[3]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[3]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[3])->getType());

  EXPECT_EQ("net1", nets[4]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[4]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[4])->getType());

  EXPECT_EQ("net2", nets[5]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[5]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[5])->getType());

  EXPECT_EQ("net3", nets[6]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[6]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[6])->getType());

  EXPECT_EQ("net4", nets[7]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLBusNet*>(nets[7]));
  EXPECT_EQ(3, dynamic_cast<SNLBusNet*>(nets[7])->getMSB());
  EXPECT_EQ(-1, dynamic_cast<SNLBusNet*>(nets[7])->getLSB());
  for (auto bit: dynamic_cast<SNLBusNet*>(nets[7])->getBits()) {
    EXPECT_EQ(SNLNet::Type::Standard, bit->getType());
  }

  EXPECT_EQ("constant0", nets[8]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[8]));
  EXPECT_EQ(SNLNet::Type::Supply0, dynamic_cast<SNLScalarNet*>(nets[8])->getType());

  EXPECT_EQ("constant1", nets[9]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[9]));
  EXPECT_EQ(SNLNet::Type::Supply1, dynamic_cast<SNLScalarNet*>(nets[9])->getType());

  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[10]));
  EXPECT_EQ(SNLNet::Type::Assign1, dynamic_cast<SNLScalarNet*>(nets[10])->getType());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[11]));
  EXPECT_EQ(SNLNet::Type::Assign0, dynamic_cast<SNLScalarNet*>(nets[11])->getType());

  ASSERT_EQ(3, test->getInstances().size());
}