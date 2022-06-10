#include "gtest/gtest.h"
#include "gmock/gmock.h"
using namespace std;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLInstanceTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);

      auto library = SNLLibrary::create(db, SNLName("MYLIB"));
      auto design = SNLDesign::create(library, SNLName("design"));
      auto model = SNLDesign::create(library, SNLName("model"));

      inBus0_ = SNLBusTerm::create(model, SNLTerm::Direction::Input, -2, 2, SNLName("inBus0"));
      outBus0_ = SNLBusTerm::create(model, SNLTerm::Direction::Output, 3, -1, SNLName("outBus0"));
      inBus1_ = SNLBusTerm::create(model, SNLTerm::Direction::Input, -1, -1, SNLName("inBus1"));
      inScalar_ = SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("in"));
      outScalar_ = SNLScalarTerm::create(model, SNLTerm::Direction::Output, SNLName("out"));

      leftInstance_ = SNLInstance::create(design, model, SNLName("left"));
      rightInstance_ = SNLInstance::create(design, model, SNLName("right"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLBusTerm*     inBus0_;
    SNLBusTerm*     outBus0_;
    SNLBusTerm*     inBus1_;
    SNLScalarTerm*  inScalar_;
    SNLScalarTerm*  outScalar_;
    SNLInstance*    leftInstance_;
    SNLInstance*    rightInstance_;
};

TEST_F(SNLInstanceTest1, setTermNetTest0) {
  ASSERT_NE(nullptr, inBus0_);
  ASSERT_NE(nullptr, outBus0_);
  ASSERT_EQ(inBus0_->getSize(), outBus0_->getSize());
  ASSERT_NE(nullptr, inBus1_);
  EXPECT_EQ(1, inBus1_->getSize());
  ASSERT_NE(nullptr, inScalar_);
  ASSERT_NE(nullptr, outScalar_);
  ASSERT_NE(nullptr, leftInstance_);
  ASSERT_NE(nullptr, rightInstance_);
  EXPECT_FALSE(leftInstance_->getInstTerms().empty());
  EXPECT_FALSE(rightInstance_->getInstTerms().empty());
  EXPECT_EQ(13, leftInstance_->getInstTerms().size());
  EXPECT_EQ(13, rightInstance_->getInstTerms().size());
  EXPECT_TRUE(leftInstance_->getConnectedInstTerms().empty());
  EXPECT_TRUE(rightInstance_->getConnectedInstTerms().empty());

  //create 2 bus nets of inBus0, outBus0 size
  auto busNet0 = SNLBusNet::create(leftInstance_->getDesign(), inBus0_->getMSB(), inBus0_->getLSB());
  auto busNet1 = SNLBusNet::create(leftInstance_->getDesign(), inBus0_->getLSB(), inBus0_->getMSB());
  leftInstance_->setTermNet(outBus0_, busNet0);
  EXPECT_EQ(5, leftInstance_->getConnectedInstTerms().size());
  rightInstance_->setTermNet(inBus0_, busNet0);
  EXPECT_EQ(5, rightInstance_->getConnectedInstTerms().size());
  rightInstance_->setTermNet(outBus0_, busNet1);
  EXPECT_EQ(10, rightInstance_->getConnectedInstTerms().size());
  leftInstance_->setTermNet(inBus0_, busNet1);
  EXPECT_EQ(10, leftInstance_->getConnectedInstTerms().size());

  //connect left out scalar with right inBus1 (size 1) with scalar net
  auto scalarNet0 = SNLScalarNet::create(leftInstance_->getDesign());
  leftInstance_->setTermNet(outScalar_, scalarNet0);
  rightInstance_->setTermNet(inBus1_, scalarNet0);
  EXPECT_EQ(11, rightInstance_->getConnectedInstTerms().size());
  EXPECT_EQ(11, leftInstance_->getConnectedInstTerms().size());

  //connect right out scalar with left inBus1 (size 1) with bus net size 1
  auto busNet3 = SNLBusNet::create(leftInstance_->getDesign(), inBus1_->getMSB(), inBus1_->getLSB());
  rightInstance_->setTermNet(outScalar_, busNet3);
  leftInstance_->setTermNet(inBus1_, busNet3);
  EXPECT_EQ(12, rightInstance_->getConnectedInstTerms().size());
  EXPECT_EQ(12, leftInstance_->getConnectedInstTerms().size());

  for (auto outBus0Bit: outBus0_->getBits()) {
    ASSERT_NE(nullptr, outBus0Bit);
    auto leftInstTerm = leftInstance_->getInstTerm(outBus0Bit);
    ASSERT_NE(nullptr, leftInstTerm);
    EXPECT_NE(nullptr, leftInstTerm->getNet());
    auto bitValue = outBus0Bit->getBit();
    auto bitPosition = outBus0Bit->getPositionInBus();
    auto rightBitTerm = inBus0_->getBitAtPosition(bitPosition);
    ASSERT_NE(nullptr, rightBitTerm); 
    auto rightInstTerm = rightInstance_->getInstTerm(rightBitTerm);
    ASSERT_NE(nullptr, rightInstTerm);
    EXPECT_NE(nullptr, rightInstTerm->getNet());
    auto bitNet = busNet0->getBitAtPosition(bitPosition);
    EXPECT_NE(nullptr, bitNet);
    EXPECT_EQ(leftInstTerm->getNet(), bitNet);
    EXPECT_EQ(rightInstTerm->getNet(), bitNet);
  }

  for (auto outBus0Bit: outBus0_->getBits()) {
    ASSERT_NE(nullptr, outBus0Bit);
    auto rightInstTerm = rightInstance_->getInstTerm(outBus0Bit);
    ASSERT_NE(nullptr, rightInstTerm);
    EXPECT_NE(nullptr, rightInstTerm->getNet());
    auto bitValue = outBus0Bit->getBit();
    auto bitPosition = outBus0Bit->getPositionInBus();
    auto leftBitTerm = inBus0_->getBitAtPosition(bitPosition);
    ASSERT_NE(nullptr, leftBitTerm); 
    auto leftInstTerm = leftInstance_->getInstTerm(leftBitTerm);
    ASSERT_NE(nullptr, leftInstTerm);
    EXPECT_NE(nullptr, leftInstTerm->getNet());
    auto bitNet = busNet1->getBitAtPosition(bitPosition);
    EXPECT_NE(nullptr, bitNet);
    EXPECT_EQ(leftInstTerm->getNet(), bitNet);
    EXPECT_EQ(rightInstTerm->getNet(), bitNet);
  }

  {
    auto leftInstTerm = leftInstance_->getInstTerm(outScalar_);
    auto rightTerm = inBus1_->getBitAtPosition(0);
    ASSERT_NE(nullptr, rightTerm);
    auto rightInstTerm = rightInstance_->getInstTerm(rightTerm);
    ASSERT_NE(nullptr, rightInstTerm);
    EXPECT_EQ(scalarNet0, leftInstTerm->getNet());
    EXPECT_EQ(scalarNet0, rightInstTerm->getNet());
  }

  {
    auto rightInstTerm = rightInstance_->getInstTerm(outScalar_);
    auto leftTerm = inBus1_->getBitAtPosition(0);
    ASSERT_NE(nullptr, leftTerm);
    auto leftInstTerm = leftInstance_->getInstTerm(leftTerm);
    ASSERT_NE(nullptr, leftInstTerm);
    EXPECT_EQ(busNet3->getBitAtPosition(0), rightInstTerm->getNet());
    EXPECT_EQ(busNet3->getBitAtPosition(0), leftInstTerm->getNet());
  }
}

TEST_F(SNLInstanceTest1, setTermNetTest1) {
  //connect left out scalar with with scalar net through array method
  SNLInstance::Terms terms({outScalar_});
  SNLInstance::Nets nets({SNLScalarNet::create(leftInstance_->getDesign())});
  leftInstance_->setTermsNets(terms, nets);
  EXPECT_EQ(1, leftInstance_->getConnectedInstTerms().size());
}

TEST_F(SNLInstanceTest1, setTermNetTestErrors) {
  auto busNet0 = SNLBusNet::create(leftInstance_->getDesign(), 2, 0);
  EXPECT_EQ(inBus0_->getSize(), outBus0_->getSize());
  EXPECT_NE(busNet0->getSize(), inBus0_->getSize());
  EXPECT_THROW(leftInstance_->setTermNet(inBus0_, busNet0), SNLException);
  EXPECT_THROW(leftInstance_->setTermNet(outBus0_, busNet0), SNLException);
  
  // testing different terms/nets size
  SNLInstance::Terms terms {nullptr, nullptr};
  SNLInstance::Nets nets {nullptr};
  EXPECT_THROW(leftInstance_->setTermsNets(terms, nets), SNLException);
}