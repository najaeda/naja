#include "gtest/gtest.h"
#include "gmock/gmock.h"
using namespace std;
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
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
      outBus1_ = SNLBusTerm::create(model, SNLTerm::Direction::Output, 5, 5, SNLName("outBus1"));
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
    SNLBusTerm*     outBus1_;
    SNLScalarTerm*  inScalar_;
    SNLScalarTerm*  outScalar_;
    SNLInstance*    leftInstance_;
    SNLInstance*    rightInstance_;
};

TEST_F(SNLInstanceTest1, setTermNetTest) {
  ASSERT_NE(nullptr, inBus0_);
  ASSERT_NE(nullptr, outBus0_);
  ASSERT_NE(nullptr, inBus1_);
  ASSERT_NE(nullptr, outBus1_);
  ASSERT_NE(nullptr, inScalar_);
  ASSERT_NE(nullptr, outScalar_);
  ASSERT_NE(nullptr, leftInstance_);
  ASSERT_NE(nullptr, rightInstance_);

  //create 2 bus nets of inBus0, outBus0 size
  auto busNet0 = SNLBusNet::create(leftInstance_->getDesign(), inBus0_->getMSB(), inBus0_->getLSB());
  auto busNet1 = SNLBusNet::create(leftInstance_->getDesign(), inBus0_->getLSB(), inBus0_->getMSB());
  leftInstance_->setTermNet(outBus0_, busNet0);
  rightInstance_->setTermNet(inBus0_, busNet0);
  rightInstance_->setTermNet(outBus0_, busNet1);
  leftInstance_->setTermNet(inBus0_, busNet1);

  for (auto inBus0Bit: inBus0_->getBits()) {
    ASSERT_NE(nullptr, inBus0Bit);
    auto leftInstTerm = leftInstance_->getInstTerm(inBus0Bit);
    ASSERT_NE(nullptr, leftInstTerm);
    EXPECT_NE(nullptr, leftInstTerm->getNet());
    auto rightInstTerm = rightInstance_->getInstTerm(inBus0Bit);
    ASSERT_NE(nullptr, rightInstTerm);
    EXPECT_NE(nullptr, rightInstTerm->getNet());
  }
}