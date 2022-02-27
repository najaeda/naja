#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

using namespace naja::SNL;

class SNLNetTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMITIVES"));
      auto library = SNLLibrary::create(db, SNLName("LIB"));
      design_ = SNLDesign::create(library, SNLName("Design"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  design_;
};

TEST_F(SNLNetTest, testCreation) {
  //Create Model
  auto db = design_->getDB();
  ASSERT_TRUE(db);
  auto primitives = db->getLibrary(SNLName("PRIMITIVES"));
  EXPECT_EQ(0, primitives->getID());
  ASSERT_TRUE(primitives);
  EXPECT_TRUE(primitives->isPrimitives());
  auto primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive);
  ASSERT_TRUE(primitive);
  EXPECT_TRUE(primitive->isPrimitive());
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, SNLName("i0"));
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, SNLName("i1"));
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, SNLName("o"));

  ASSERT_TRUE(design_);
  auto lib = design_->getLibrary();
  ASSERT_TRUE(lib);
  EXPECT_EQ(1, lib->getID());
  EXPECT_EQ(SNLName("Design"), design_->getName());
  EXPECT_EQ(0, design_->getID());
  EXPECT_FALSE(design_->isAnonymous());
  EXPECT_EQ(design_, design_->getLibrary()->getDesign(0));
  EXPECT_EQ(design_, design_->getLibrary()->getDesign(SNLName("Design")));
  EXPECT_TRUE(design_->getNets().empty());
  EXPECT_TRUE(design_->getScalarNets().empty());
  EXPECT_TRUE(design_->getBusNets().empty());
  EXPECT_TRUE(design_->getBitNets().empty());

  auto i0Term = SNLScalarTerm::create(design_, SNLTerm::Direction::Input, SNLName("I0"));
  auto i1Term = SNLScalarTerm::create(design_, SNLTerm::Direction::Input, SNLName("I1"));
  auto oTerm = SNLBusTerm::create(design_, SNLTerm::Direction::Input, 31, 0, SNLName("O"));

  auto i0Net = SNLScalarNet::create(design_);
  EXPECT_EQ(0, i0Net->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Net, 1, 1, 0, 0, 0, 0), i0Net->getSNLID());
  EXPECT_FALSE(design_->getNets().empty());
  EXPECT_FALSE(design_->getScalarNets().empty());
  EXPECT_FALSE(design_->getBitNets().empty());
  EXPECT_TRUE(design_->getBusNets().empty());
  EXPECT_EQ(1, design_->getNets().size());
  EXPECT_EQ(1, design_->getScalarNets().size());
  EXPECT_EQ(1, design_->getBitNets().size());

  EXPECT_TRUE(i0Net->getComponents().empty());
  EXPECT_TRUE(i0Net->getInstTerms().empty());
  EXPECT_TRUE(i0Net->getBitTerms().empty());

  EXPECT_FALSE(i0Term->getNet());
  i0Term->setNet(i0Net);
  EXPECT_EQ(i0Net, i0Term->getNet());

  EXPECT_FALSE(i0Net->getComponents().empty());
  EXPECT_EQ(1, i0Net->getComponents().size());
  EXPECT_TRUE(i0Net->getInstTerms().empty());
  EXPECT_FALSE(i0Net->getBitTerms().empty());
  EXPECT_EQ(1, i0Net->getBitTerms().size());

  auto i1Net = SNLScalarNet::create(design_);
  EXPECT_EQ(1, i1Net->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Net, 1, 1, 0, 1, 0, 0), i1Net->getSNLID());
  EXPECT_FALSE(design_->getNets().empty());
  EXPECT_FALSE(design_->getScalarNets().empty());
  EXPECT_FALSE(design_->getBitNets().empty());
  EXPECT_TRUE(design_->getBusNets().empty());
  EXPECT_EQ(2, design_->getNets().size());
  EXPECT_EQ(2, design_->getScalarNets().size());
  EXPECT_EQ(1+1, design_->getBitNets().size());

  EXPECT_FALSE(i1Term->getNet());
  i1Term->setNet(i1Net);
  EXPECT_EQ(i1Net, i1Term->getNet());

  EXPECT_EQ(SNLNet::Type::Standard ,i0Net->getType());
  i0Net->setType(SNLBitNet::Type::Assign0);
  EXPECT_EQ(SNLNet::Type::Assign0 ,i0Net->getType());
  i0Net->setType(SNLBitNet::Type::Supply1);
  EXPECT_EQ(SNLNet::Type::Supply1 ,i0Net->getType());

  auto instance0 = SNLInstance::create(design_, primitive, SNLName("instance0"));
  auto instance1 = SNLInstance::create(design_, primitive, SNLName("instance1"));
  auto instance2 = SNLInstance::create(design_, primitive, SNLName("instance2"));

  SNLBusNet* net0 = SNLBusNet::create(design_, 31, 0, SNLName("net0"));
  ASSERT_TRUE(net0);
  EXPECT_EQ(SNLName("net0"), net0->getName());
  EXPECT_EQ(SNLID(SNLID::Type::Net, 1, 1, 0, 2, 0, 0), net0->getSNLID());
  EXPECT_EQ(2, net0->getID());
  EXPECT_EQ(31, net0->getMSB());
  EXPECT_EQ(0, net0->getLSB());
  EXPECT_EQ(32, net0->getSize());
  EXPECT_EQ(design_, net0->getDesign());
  EXPECT_FALSE(net0->isAnonymous());
  EXPECT_EQ(net0, design_->getNet(2));
  EXPECT_EQ(net0, design_->getNet(SNLName("net0")));
  EXPECT_EQ(SNLID(SNLID::Type::Net, 1, 1, 0, 2, 0, 0), net0->getSNLID());
  EXPECT_FALSE(design_->getNets().empty());
  EXPECT_FALSE(design_->getScalarNets().empty());
  EXPECT_FALSE(design_->getBitNets().empty());
  EXPECT_FALSE(design_->getBusNets().empty());
  EXPECT_EQ(3, design_->getNets().size());
  EXPECT_EQ(2, design_->getScalarNets().size());
  EXPECT_EQ(1, design_->getBusNets().size());
  EXPECT_EQ(1+1+32, design_->getBitNets().size());
  EXPECT_THAT(std::vector(design_->getNets().begin(), design_->getNets().end()),
    ElementsAre(i0Net, i1Net, net0));
  EXPECT_THAT(std::vector(design_->getScalarNets().begin(), design_->getScalarNets().end()),
    ElementsAre(i0Net, i1Net));
  EXPECT_THAT(std::vector(design_->getBusNets().begin(), design_->getBusNets().end()),
    ElementsAre(net0));
  EXPECT_THAT(std::vector(design_->getBitNets().begin(), design_->getBitNets().end()),
    ElementsAre(i0Net, i1Net,
      net0->getBit(31), net0->getBit(30), net0->getBit(29), net0->getBit(28),
      net0->getBit(27), net0->getBit(26), net0->getBit(25), net0->getBit(24),
      net0->getBit(23), net0->getBit(22), net0->getBit(21), net0->getBit(20),
      net0->getBit(19), net0->getBit(18), net0->getBit(17), net0->getBit(16),
      net0->getBit(15), net0->getBit(14), net0->getBit(13), net0->getBit(12),
      net0->getBit(11), net0->getBit(10), net0->getBit(9),  net0->getBit(8),
      net0->getBit(7),  net0->getBit(6),  net0->getBit(5),  net0->getBit(4),
      net0->getBit(3) , net0->getBit(2),  net0->getBit(1),  net0->getBit(0)));
  

  EXPECT_EQ(nullptr, net0->getBit(32));
  EXPECT_EQ(nullptr, net0->getBit(-1));

  SNLID::Bit bitNumber = 31;
  for (auto bit: net0->getBits()) {
    EXPECT_EQ(SNLNet::Type::Standard, bit->getType());
    EXPECT_EQ(net0, bit->getBus());
    EXPECT_EQ(net0->getID(), bit->getID());
    EXPECT_EQ(design_, bit->getDesign());
    EXPECT_FALSE(bit->getType().isDriving());
    EXPECT_FALSE(bit->isAnonymous());
    EXPECT_EQ(SNLID(SNLID::Type::NetBit, 1, 1, 0, 2, 0, bitNumber--), bit->getSNLID());
  }
  net0->setType(SNLBitNet::Type::Supply1);
  for (auto bit: net0->getBits()) {
    EXPECT_EQ(SNLNet::Type::Supply1, bit->getType());
    EXPECT_TRUE(bit->getType().isSupply());
    EXPECT_TRUE(bit->getType().isDriving());
  }
  net0->setType(SNLBitNet::Type::Assign0);
  for (auto bit: net0->getBits()) {
    EXPECT_EQ(SNLNet::Type::Assign0, bit->getType());
    EXPECT_TRUE(bit->getType().isDriving());
  }

  net0->destroy();
  EXPECT_EQ(nullptr, design_->getNet(2));
  EXPECT_EQ(nullptr, design_->getNet(SNLName("net0")));

  i1Net->destroy();
  EXPECT_EQ(nullptr, design_->getNet(1));
}

TEST_F(SNLNetTest, testNetType) {
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Assign0).isAssign());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Assign1).isAssign());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Supply0).isSupply());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Supply1).isSupply());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Assign0).isDriving());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Assign1).isDriving());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Supply0).isDriving());
  EXPECT_TRUE(SNLNet::Type(SNLNet::Type::Supply1).isDriving());
  EXPECT_FALSE(SNLNet::Type(SNLNet::Type::Standard).isDriving());
}