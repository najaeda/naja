#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

using namespace SNL;

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
  ASSERT_TRUE(primitives);
  EXPECT_TRUE(primitives->isPrimitives());
  auto primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive);
  ASSERT_TRUE(primitive);
  EXPECT_TRUE(primitive->isPrimitive());
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, SNLName("i0"));
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, SNLName("i1"));
  SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, SNLName("o"));

  ASSERT_TRUE(design_);
  EXPECT_EQ(SNLName("Design"), design_->getName());
  EXPECT_EQ(0, design_->getID());
  EXPECT_FALSE(design_->isAnonymous());
  EXPECT_EQ(design_, design_->getLibrary()->getDesign(0));
  EXPECT_EQ(design_, design_->getLibrary()->getDesign(SNLName("Design")));

  auto i0Term = SNLScalarTerm::create(design_, SNLTerm::Direction::Input, SNLName("I0"));
  auto i1Term = SNLScalarTerm::create(design_, SNLTerm::Direction::Input, SNLName("I1"));
  auto oTerm = SNLBusTerm::create(design_, SNLTerm::Direction::Input, 31, 0, SNLName("O"));

  auto i0Net = SNLScalarNet::create(design_);
  EXPECT_EQ(0, i0Net->getID());
  EXPECT_FALSE(i0Term->getNet());
  i0Term->setNet(i0Net);
  EXPECT_EQ(i0Net, i0Term->getNet());
  auto i1Net = SNLScalarNet::create(design_);
  EXPECT_EQ(1, i1Net->getID());
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
  EXPECT_EQ(2, net0->getID());
  EXPECT_EQ(31, net0->getMSB());
  EXPECT_EQ(0, net0->getLSB());
  //EXPECT_EQ(32, net0->getSize());
  EXPECT_EQ(design_, net0->getDesign());
  EXPECT_FALSE(net0->isAnonymous());
  EXPECT_EQ(net0, design_->getNet(2));
  EXPECT_EQ(net0, design_->getNet(SNLName("net0")));

  for (auto bit: net0->getBits()) {
    EXPECT_EQ(SNLNet::Type::Standard, bit->getType());
  }
  net0->setType(SNLBitNet::Type::Supply1);
  for (auto bit: net0->getBits()) {
    EXPECT_EQ(SNLNet::Type::Supply1, bit->getType());
  }
  net0->setType(SNLBitNet::Type::Assign0);
  for (auto bit: net0->getBits()) {
    EXPECT_EQ(SNLNet::Type::Assign0, bit->getType());
  }
}
