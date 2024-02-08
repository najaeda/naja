// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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
  EXPECT_EQ(SNLID::DesignObjectReference(1, 0, 0, 0), term0->getReference());
  EXPECT_EQ(term0, SNLUniverse::get()->getTerm(SNLID::DesignObjectReference(1, 0, 0, 0)));
  EXPECT_EQ(term0, SNLUniverse::get()->getObject(term0->getSNLID()));
  EXPECT_EQ(SNLTerm::Direction::InOut, term0->getDirection());
  EXPECT_EQ(design, term0->getDesign()); 
  EXPECT_EQ(-1, term0->getMSB());
  EXPECT_EQ(-4, term0->getLSB());
  EXPECT_EQ(4, term0->getSize());
  EXPECT_FALSE(term0->getBits().empty());
  EXPECT_EQ(4, term0->getBits().size());
  for (auto bit: term0->getBusBits()) {
    EXPECT_EQ(SNLTerm::Direction::InOut, bit->getDirection());
    EXPECT_EQ(term0, bit->getBus());
    EXPECT_FALSE(bit->isAnonymous());
    EXPECT_EQ(bit->getName(), bit->getBus()->getName());
  }
  EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
    ElementsAre(term0->getBit(-1), term0->getBit(-2), term0->getBit(-3), term0->getBit(-4)));
  EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
    ElementsAre(
      term0->getBitAtPosition(0), term0->getBitAtPosition(1),
      term0->getBitAtPosition(2), term0->getBitAtPosition(3)));
  EXPECT_EQ(nullptr, term0->getBitAtPosition(4));
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -1), term0->getBit(-1)->getSNLID());
  EXPECT_EQ(term0->getBit(-1), SNLUniverse::get()->getBusTermBit(term0->getBit(-1)->getSNLID()));
  EXPECT_EQ(term0->getBit(-1), SNLUniverse::get()->getObject(term0->getBit(-1)->getSNLID()));
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -2), term0->getBit(-2)->getSNLID());
  EXPECT_EQ(term0->getBit(-2), SNLUniverse::get()->getBusTermBit(term0->getBit(-2)->getSNLID()));
  EXPECT_EQ(term0->getBit(-2), SNLUniverse::get()->getObject(term0->getBit(-2)->getSNLID()));
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -3), term0->getBit(-3)->getSNLID());
  EXPECT_EQ(term0->getBit(-3), SNLUniverse::get()->getBusTermBit(term0->getBit(-3)->getSNLID()));
  EXPECT_EQ(term0->getBit(-3), SNLUniverse::get()->getObject(term0->getBit(-3)->getSNLID()));
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -4), term0->getBit(-4)->getSNLID());
  EXPECT_EQ(term0->getBit(-4), SNLUniverse::get()->getBusTermBit(term0->getBit(-4)->getSNLID()));
  EXPECT_EQ(term0->getBit(-4), SNLUniverse::get()->getObject(term0->getBit(-4)->getSNLID()));
  EXPECT_EQ(0, term0->getFlatID());
  EXPECT_EQ(0, term0->getBit(-1)->getFlatID());
  EXPECT_EQ(1, term0->getBit(-2)->getFlatID());
  EXPECT_EQ(2, term0->getBit(-3)->getFlatID());
  EXPECT_EQ(3, term0->getBit(-4)->getFlatID());
  EXPECT_EQ(0, term0->getBit(-1)->getPositionInBus());
  EXPECT_EQ(1, term0->getBit(-2)->getPositionInBus());
  EXPECT_EQ(2, term0->getBit(-3)->getPositionInBus());
  EXPECT_EQ(3, term0->getBit(-4)->getPositionInBus());

  ASSERT_EQ(1, term0->getBit(-1)->getBits().size());
  EXPECT_EQ(*term0->getBit(-1)->getBits().begin(), term0->getBit(-1));
  ASSERT_EQ(1, term0->getBit(-2)->getBits().size());
  EXPECT_EQ(*term0->getBit(-2)->getBits().begin(), term0->getBit(-2));
  ASSERT_EQ(1, term0->getBit(-3)->getBits().size());
  EXPECT_EQ(*term0->getBit(-3)->getBits().begin(), term0->getBit(-3));

  EXPECT_EQ(nullptr, term0->getBit(-5));
  EXPECT_EQ(nullptr, term0->getBit(0));
  EXPECT_EQ(nullptr, SNLUniverse::get()->getBusTermBit(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -5)));
  EXPECT_EQ(nullptr, SNLUniverse::get()->getObject(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -5)));

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
  EXPECT_EQ(nullptr, term0->getNet());
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
  EXPECT_THROW(term0->setNet(net), SNLException);
  net->destroy();
 
  //different size SNLBusNet case
  net = SNLBusNet::create(design, 0, 2, SNLName("n0"));
  EXPECT_THROW(term0->setNet(net), SNLException);
  net->destroy();

  //other design
  auto other = SNLDesign::create(library, SNLName("other"));
  SNLNet* otherNet = SNLScalarNet::create(other, SNLName("n0"));
  EXPECT_THROW(term0->setNet(otherNet), SNLException);
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
  EXPECT_THROW(SNLBusTerm::create(design, SNLID::DesignObjectID(0), SNLTerm::Direction::Input, 31, 0), SNLException);
  EXPECT_THROW(SNLScalarTerm::create(design, SNLID::DesignObjectID(1), SNLTerm::Direction::Input), SNLException);
  EXPECT_THROW(term1->getBit(1)->setName(SNLName("bit1")), SNLException);
}

TEST_F(SNLTermTest, testRename) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  ASSERT_NE(design, nullptr);

  auto term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("term0"));
  auto term1 = SNLBusTerm::create(design, SNLTerm::Direction::Output, 31, 0, SNLName("term1"));
  auto term2 = SNLScalarTerm::create(design, SNLTerm::Direction::Input);
  EXPECT_EQ(term0, design->getTerm(SNLName("term0")));
  EXPECT_EQ(term1, design->getTerm(SNLName("term1")));
  EXPECT_FALSE(term0->isAnonymous());
  term0->setName(SNLName());
  EXPECT_TRUE(term0->isAnonymous());
  EXPECT_EQ(nullptr, design->getTerm(SNLName("term0")));
  term0->setName(SNLName("term0"));
  EXPECT_FALSE(term0->isAnonymous());
  EXPECT_EQ(term0, design->getTerm(SNLName("term0")));
  EXPECT_FALSE(term1->isAnonymous());
  term1->setName(SNLName("term1")); //nothing should happen...
  EXPECT_EQ(term1, design->getTerm(SNLName("term1")));
  term1->setName(SNLName("t1"));
  EXPECT_FALSE(term1->isAnonymous());
  EXPECT_EQ(nullptr, design->getTerm(SNLName("term1")));
  EXPECT_EQ(term1, design->getTerm(SNLName("t1")));
  EXPECT_TRUE(term2->isAnonymous());
  term2->setName(SNLName("term2"));
  EXPECT_FALSE(term2->isAnonymous());
  EXPECT_EQ(term2, design->getTerm(SNLName("term2")));
  //Collision error
  EXPECT_THROW(term2->setName(SNLName("term0")), SNLException);
}