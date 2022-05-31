#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLTermTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLTermTest, testCreation) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getDesigns().size());
  EXPECT_TRUE(library->getDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -1, -4, SNLName("term0"));
  ASSERT_NE(term0, nullptr);
  EXPECT_EQ(SNLName("term0"), term0->getName());
  ASSERT_FALSE(term0->isAnonymous());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 0, 0, 0, 0), term0->getSNLID());
  EXPECT_EQ(SNLTerm::Direction::InOut, term0->getDirection());
  EXPECT_EQ(design, term0->getDesign()); 
  EXPECT_EQ(-1, term0->getMSB());
  EXPECT_EQ(-4, term0->getLSB());
  EXPECT_EQ(4, term0->getSize());
  EXPECT_FALSE(term0->getBits().empty());
  EXPECT_EQ(4, term0->getBits().size());
  for (auto bit: term0->getBits()) {
    EXPECT_EQ(SNLTerm::Direction::InOut, bit->getDirection());
    EXPECT_EQ(term0, bit->getBus());
    EXPECT_FALSE(bit->isAnonymous());
    EXPECT_EQ(bit->getName(), bit->getBus()->getName());
  }
  EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
    ElementsAre(term0->getBit(-1), term0->getBit(-2), term0->getBit(-3), term0->getBit(-4)));
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -1), term0->getBit(-1)->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -2), term0->getBit(-2)->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -3), term0->getBit(-3)->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -4), term0->getBit(-4)->getSNLID());
  EXPECT_EQ(0, term0->getPositionInDesign());
  EXPECT_EQ(0, term0->getBit(-1)->getPositionInDesign());
  EXPECT_EQ(1, term0->getBit(-2)->getPositionInDesign());
  EXPECT_EQ(2, term0->getBit(-3)->getPositionInDesign());
  EXPECT_EQ(3, term0->getBit(-4)->getPositionInDesign());
  EXPECT_EQ(0, term0->getBit(-1)->getPositionInBus());
  EXPECT_EQ(1, term0->getBit(-2)->getPositionInBus());
  EXPECT_EQ(2, term0->getBit(-3)->getPositionInBus());
  EXPECT_EQ(3, term0->getBit(-4)->getPositionInBus());

  EXPECT_FALSE(term0->getBit(-5));
  EXPECT_FALSE(term0->getBit(0));

  EXPECT_THROW(term0->getBit(-4)->destroy(), SNLException);
}

TEST_F(SNLTermTest, testSetNet0) {
  //SetNet for TermBus size > 1
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getDesigns().size());
  EXPECT_TRUE(library->getDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -1, -4, SNLName("term0"));

  //same MSB, LSB
  SNLBusNet* net = SNLBusNet::create(design, -1, -4, SNLName("n0"));
  term0->setNet(net);
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(i));
  }

  //destroy net and assert that term bits are NULL
  net->destroy();
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
  }
  
  //Same size: inversed MSB, LSB
  net = SNLBusNet::create(design, -4, -1, SNLName("n0"));
  term0->setNet(net);
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(-5-i));
  }
  net->destroy();
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
  }

  //Same size but not same MSB LSB
  net = SNLBusNet::create(design, 1, 4, SNLName("n0"));
  term0->setNet(net);
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(-i));
  }
  net->destroy();
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
  } 

  //Same as before but inversed
  net = SNLBusNet::create(design, 4, 1, SNLName("n0"));
  term0->setNet(net);
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(5+i));
  }
  net->destroy();
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
  }
}

TEST_F(SNLTermTest, testSetNet1) {
  //SetNet for TermBus size == 1
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getDesigns().size());
  EXPECT_TRUE(library->getDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -1, -1, SNLName("term0"));

  //SNLScalarNet case
  SNLNet* net = SNLScalarNet::create(design, SNLName("n0"));
  term0->setNet(net);
  ASSERT_EQ(term0->getBit(-1)->getNet(), net);
  net->destroy();
  ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);

  //1 bit bus SNLBusNet case
  net = SNLBusNet::create(design, -1, -1, SNLName("n0"));
  term0->setNet(net);
  ASSERT_EQ(term0->getBit(-1)->getNet(), static_cast<SNLBusNet*>(net)->getBit(-1));
  net->destroy();
  ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);

  //1 bit bus SNLBusNet case
  net = SNLBusNet::create(design, 5, 5, SNLName("n0"));
  term0->setNet(net);
  ASSERT_EQ(term0->getBit(-1)->getNet(), static_cast<SNLBusNet*>(net)->getBit(5));
  net->destroy();
  ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);
}

TEST_F(SNLTermTest, testSetNetErrors) {
   //SetNet for TermBus size == 1
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getDesigns().size());
  EXPECT_TRUE(library->getDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -2, 2, SNLName("term0"));

  //With scalar net
  SNLNet* net = SNLScalarNet::create(design, SNLName("n0"));
  ASSERT_THROW(term0->setNet(net), SNLException);
  net->destroy();
 
  //different size SNLBusNet case
  net = SNLBusNet::create(design, 0, 2, SNLName("n0"));
  ASSERT_THROW(term0->setNet(net), SNLException);
  net->destroy();
}

TEST_F(SNLTermTest, testErrors) {
  EXPECT_THROW(SNLScalarTerm::create(nullptr, SNLTerm::Direction::Input), SNLException);
  EXPECT_THROW(SNLBusTerm::create(nullptr, SNLTerm::Direction::Input, 31, 0), SNLException);

  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  ASSERT_NE(design, nullptr);

  SNLScalarTerm* term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("term0"));
  ASSERT_NE(nullptr, term0);
  SNLBusTerm* term1 = SNLBusTerm::create(design, SNLTerm::Direction::Input, 31, 0, SNLName("term1"));
  ASSERT_NE(nullptr, term1);
  EXPECT_THROW(SNLBusTerm::create(design, SNLTerm::Direction::Input, 31, 0, SNLName("term0")), SNLException);
  EXPECT_THROW(SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("term1")), SNLException);
}