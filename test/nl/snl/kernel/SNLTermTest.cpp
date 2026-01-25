// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
using namespace naja::NL;

class SNLTermTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB*  db_;
};

TEST_F(SNLTermTest, testCreation) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, NLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -1, -4, NLName("term0"));
  ASSERT_NE(term0, nullptr);
  EXPECT_EQ(NLName("term0"), term0->getName());
  ASSERT_FALSE(term0->isUnnamed());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 0, 0, 0), term0->getNLID());
  EXPECT_EQ(NLID::DesignObjectReference(1, 0, 0, 0), term0->getReference());
  EXPECT_EQ(term0, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 0)));
  EXPECT_EQ(term0, NLUniverse::get()->getObject(term0->getNLID()));
  EXPECT_EQ(SNLTerm::Direction::InOut, term0->getDirection());
  EXPECT_EQ(design, term0->getDesign()); 
  EXPECT_EQ(-1, term0->getMSB());
  EXPECT_EQ(-4, term0->getLSB());
  EXPECT_EQ(4, term0->getWidth());
  EXPECT_FALSE(term0->getBits().empty());
  EXPECT_EQ(4, term0->getBits().size());
  for (auto bit: term0->getBusBits()) {
    EXPECT_EQ(SNLTerm::Direction::InOut, bit->getDirection());
    EXPECT_EQ(term0, bit->getBus());
    EXPECT_FALSE(bit->isUnnamed());
    EXPECT_EQ(bit->getName(), bit->getBus()->getName());
  }
  EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
    ElementsAre(term0->getBit(-1), term0->getBit(-2), term0->getBit(-3), term0->getBit(-4)));
  EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
    ElementsAre(
      term0->getBitAtPosition(0), term0->getBitAtPosition(1),
      term0->getBitAtPosition(2), term0->getBitAtPosition(3)));
  EXPECT_EQ(nullptr, term0->getBitAtPosition(4));
  EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -1), term0->getBit(-1)->getNLID());
  EXPECT_EQ(term0->getBit(-1), NLUniverse::get()->getBusTermBit(term0->getBit(-1)->getNLID()));
  EXPECT_EQ(term0->getBit(-1), NLUniverse::get()->getObject(term0->getBit(-1)->getNLID()));
  EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -2), term0->getBit(-2)->getNLID());
  EXPECT_EQ(term0->getBit(-2), NLUniverse::get()->getBusTermBit(term0->getBit(-2)->getNLID()));
  EXPECT_EQ(term0->getBit(-2), NLUniverse::get()->getObject(term0->getBit(-2)->getNLID()));
  EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -3), term0->getBit(-3)->getNLID());
  EXPECT_EQ(term0->getBit(-3), NLUniverse::get()->getBusTermBit(term0->getBit(-3)->getNLID()));
  EXPECT_EQ(term0->getBit(-3), NLUniverse::get()->getObject(term0->getBit(-3)->getNLID()));
  EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -4), term0->getBit(-4)->getNLID());
  EXPECT_EQ(term0->getBit(-4), NLUniverse::get()->getBusTermBit(term0->getBit(-4)->getNLID()));
  EXPECT_EQ(term0->getBit(-4), NLUniverse::get()->getObject(term0->getBit(-4)->getNLID()));
  EXPECT_EQ(0, term0->getFlatID());
  EXPECT_EQ(0, term0->getBit(-1)->getFlatID());
  EXPECT_EQ(1, term0->getBit(-2)->getFlatID());
  EXPECT_EQ(2, term0->getBit(-3)->getFlatID());
  EXPECT_EQ(3, term0->getBit(-4)->getFlatID());
  EXPECT_EQ(0, term0->getBit(-1)->getPositionInBus());
  EXPECT_EQ(1, term0->getBit(-2)->getPositionInBus());
  EXPECT_EQ(2, term0->getBit(-3)->getPositionInBus());
  EXPECT_EQ(3, term0->getBit(-4)->getPositionInBus());

  EXPECT_EQ(term0, design->getBusTerm(NLName("term0")));
  EXPECT_EQ(term0, design->getBusTerm(0));
  EXPECT_EQ(term0, design->getTerm(0));
  EXPECT_EQ(term0->getBit(-1), design->getBitTerm(0, -1));
  EXPECT_EQ(term0->getBit(-4), design->getBitTerm(0, -4));

  ASSERT_EQ(1, term0->getBit(-1)->getBits().size());
  EXPECT_EQ(*term0->getBit(-1)->getBits().begin(), term0->getBit(-1));
  ASSERT_EQ(1, term0->getBit(-2)->getBits().size());
  EXPECT_EQ(*term0->getBit(-2)->getBits().begin(), term0->getBit(-2));
  ASSERT_EQ(1, term0->getBit(-3)->getBits().size());
  EXPECT_EQ(*term0->getBit(-3)->getBits().begin(), term0->getBit(-3));

  EXPECT_EQ(nullptr, term0->getBit(-5));
  EXPECT_EQ(nullptr, term0->getBit(0));
  EXPECT_EQ(nullptr, NLUniverse::get()->getBusTermBit(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -5)));
  EXPECT_EQ(nullptr, NLUniverse::get()->getObject(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -5)));

  EXPECT_THROW(term0->getBit(-4)->destroy(), NLException);
}

TEST_F(SNLTermTest, testSetNet0) {
  //SetNet for TermBus size > 1
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, NLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -1, -4, NLName("term0"));

  //same MSB, LSB
  SNLBusNet* net = SNLBusNet::create(design, -1, -4, NLName("n0"));
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
  net = SNLBusNet::create(design, -4, -1, NLName("n0"));
  term0->setNet(net);
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(-5-i));
  }
  net->destroy();
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
  }

  //Same size but not same MSB LSB
  net = SNLBusNet::create(design, 1, 4, NLName("n0"));
  term0->setNet(net);
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(-i));
  }
  net->destroy();
  for (auto i=-1; i>=-4; i--) {
    ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
  } 

  //Same as before but inversed
  net = SNLBusNet::create(design, 4, 1, NLName("n0"));
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
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, NLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -1, -1, NLName("term0"));

  //SNLScalarNet case
  SNLNet* net = SNLScalarNet::create(design, NLName("n0"));
  term0->setNet(net);
  ASSERT_EQ(term0->getBit(-1)->getNet(), net);
  net->destroy();
  ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);

  //1 bit bus SNLBusNet case
  net = SNLBusNet::create(design, -1, -1, NLName("n0"));
  term0->setNet(net);
  ASSERT_EQ(term0->getBit(-1)->getNet(), static_cast<SNLBusNet*>(net)->getBit(-1));
  net->destroy();
  ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);

  //1 bit bus SNLBusNet case
  net = SNLBusNet::create(design, 5, 5, NLName("n0"));
  term0->setNet(net);
  EXPECT_EQ(nullptr, term0->getNet());
  ASSERT_EQ(term0->getBit(-1)->getNet(), static_cast<SNLBusNet*>(net)->getBit(5));
  net->destroy();
  ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);
}

TEST_F(SNLTermTest, testSetNetErrors) {
   //SetNet for TermBus size == 1
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getSNLDesigns().size());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, NLName("design"));

  SNLBusTerm* term0 = SNLBusTerm::create(design, SNLTerm::Direction::InOut, -2, 2, NLName("term0"));

  //With scalar net
  SNLNet* net = SNLScalarNet::create(design, NLName("n0"));
  EXPECT_THROW(term0->setNet(net), NLException);
  net->destroy();
 
  //different size SNLBusNet case
  net = SNLBusNet::create(design, 0, 2, NLName("n0"));
  EXPECT_THROW(term0->setNet(net), NLException);
  net->destroy();

  //other design
  auto other = SNLDesign::create(library, NLName("other"));
  SNLNet* otherNet = SNLScalarNet::create(other, NLName("n0"));
  EXPECT_THROW(term0->setNet(otherNet), NLException);
}

TEST_F(SNLTermTest, testErrors) {
  EXPECT_THROW(SNLScalarTerm::create(nullptr, SNLTerm::Direction::Input), NLException);
  EXPECT_THROW(SNLBusTerm::create(nullptr, SNLTerm::Direction::Input, 31, 0), NLException);

  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design = SNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  SNLScalarTerm* term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term0"));
  ASSERT_NE(nullptr, term0);
  SNLBusTerm* term1 = SNLBusTerm::create(design, SNLTerm::Direction::Input, 31, 0, NLName("term1"));
  ASSERT_NE(nullptr, term1);
  EXPECT_THROW(SNLBusTerm::create(design, SNLTerm::Direction::Input, 31, 0, NLName("term0")), NLException);
  EXPECT_THROW(SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term1")), NLException);
  EXPECT_THROW(SNLBusTerm::create(design, NLID::DesignObjectID(0), SNLTerm::Direction::Input, 31, 0), NLException);
  EXPECT_THROW(SNLScalarTerm::create(design, NLID::DesignObjectID(1), SNLTerm::Direction::Input), NLException);
  EXPECT_THROW(term1->getBit(1)->setName(NLName("bit1")), NLException);
}

TEST_F(SNLTermTest, testRename) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design = SNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  auto term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term0"));
  auto term1 = SNLBusTerm::create(design, SNLTerm::Direction::Output, 31, 0, NLName("term1"));
  auto term2 = SNLScalarTerm::create(design, SNLTerm::Direction::Input);
  EXPECT_EQ(term0, design->getTerm(NLName("term0")));
  EXPECT_EQ(term0, design->getBitTerm(0, 0));
  EXPECT_EQ(term0, design->getBitTerm(0, -12));
  EXPECT_EQ(term0, design->getBitTerm(0, 100));
  EXPECT_EQ(term1, design->getTerm(NLName("term1")));
  EXPECT_EQ(term2, design->getBitTerm(2, 0));
  EXPECT_EQ(term2, design->getBitTerm(2, -12));
  EXPECT_EQ(term2, design->getBitTerm(2, 100));
  EXPECT_FALSE(term0->isUnnamed());
  term0->setName(NLName());
  EXPECT_TRUE(term0->isUnnamed());
  EXPECT_EQ(nullptr, design->getTerm(NLName("term0")));
  term0->setName(NLName("term0"));
  EXPECT_FALSE(term0->isUnnamed());
  EXPECT_EQ(term0, design->getTerm(NLName("term0")));
  EXPECT_FALSE(term1->isUnnamed());
  term1->setName(NLName("term1")); //nothing should happen...
  EXPECT_EQ(term1, design->getTerm(NLName("term1")));
  term1->setName(NLName("t1"));
  EXPECT_FALSE(term1->isUnnamed());
  EXPECT_EQ(nullptr, design->getTerm(NLName("term1")));
  EXPECT_EQ(term1, design->getTerm(NLName("t1")));
  EXPECT_TRUE(term2->isUnnamed());
  term2->setName(NLName("term2"));
  EXPECT_FALSE(term2->isUnnamed());
  EXPECT_EQ(term2, design->getTerm(NLName("term2")));
  //Collision error
  EXPECT_THROW(term2->setName(NLName("term0")), NLException);
}

TEST_F(SNLTermTest, testDestroy) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  SNLDesign* design = SNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  auto term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term0"));
  auto term1 = SNLBusTerm::create(design, SNLTerm::Direction::Output, 31, 0, NLName("term1"));
  auto term2 = SNLScalarTerm::create(design, SNLTerm::Direction::Input);

  EXPECT_EQ(term0->getNet(), nullptr);
  using BitTerms = std::vector<SNLBitTerm*>;
  BitTerms term1Bits(term1->getBits().begin(), term1->getBits().end());
  EXPECT_THAT(term1Bits,
    ::testing::Each(::testing::Property(
        &SNLBusTermBit::getNet, ::testing::IsNull())));
  EXPECT_EQ(term2->getNet(), nullptr);

  auto net0 = SNLScalarNet::create(design, NLName("net0"));
  auto net1 = SNLBusNet::create(design, 31, 0, NLName("net1"));
  auto net2 = SNLBusNet::create(design, 0, 0, NLName("net2"));
  term0->setNet(net0);
  term1->setNet(net1);
  term2->setNet(net2);
  EXPECT_EQ(term0->getNet(), net0);
  EXPECT_EQ(term2->getNet(), net2->getBit(0));

  //start destroying
  term0->getNet()->destroy();
  ASSERT_EQ(term0->getNet(), nullptr);

  net2->destroy();
  ASSERT_EQ(term2->getNet(), nullptr);

  //destroy one bit of net1
  ASSERT_NE(term1->getBit(10)->getNet(), nullptr);
  net1->getBit(10)->destroy();
  ASSERT_EQ(term1->getBit(10)->getNet(), nullptr);
  net1->getBit(6)->destroy();
  ASSERT_EQ(term1->getBit(6)->getNet(), nullptr);

  net1->destroy();
  EXPECT_THAT(term1Bits,
    ::testing::Each(::testing::Property(
        &SNLBusTermBit::getNet, ::testing::IsNull())));
}

TEST_F(SNLTermTest, testResizeMSBSuccessWithSlaveInstance) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto model = SNLDesign::create(library, NLName("model"));
  auto term = SNLBusTerm::create(model, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  auto top = SNLDesign::create(library, NLName("top"));
  auto inst = SNLInstance::create(top, model, NLName("u0"));

  term->setMSB(1);
  EXPECT_EQ(1, term->getMSB());
  EXPECT_EQ(0, term->getLSB());
  EXPECT_EQ(2, term->getWidth());
  EXPECT_NE(nullptr, term->getBit(1));
  EXPECT_NE(nullptr, term->getBit(0));
  EXPECT_EQ(nullptr, term->getBit(2));

  auto instTerm1 = inst->getInstTerm(term->getBit(1));
  auto instTerm0 = inst->getInstTerm(term->getBit(0));
  EXPECT_NE(nullptr, instTerm1);
  EXPECT_NE(nullptr, instTerm0);
  EXPECT_EQ(nullptr, instTerm1->getNet());
  EXPECT_EQ(nullptr, instTerm0->getNet());
}

TEST_F(SNLTermTest, testResizeMSBNoOp) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto design = SNLDesign::create(library, NLName("design"));
  auto term = SNLBusTerm::create(design, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  term->setMSB(3);
  EXPECT_EQ(3, term->getMSB());
  EXPECT_EQ(0, term->getLSB());
  EXPECT_EQ(4, term->getWidth());
}

TEST_F(SNLTermTest, testResizeMSBInvalid) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto design = SNLDesign::create(library, NLName("design"));
  auto term = SNLBusTerm::create(design, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  EXPECT_THROW(term->setMSB(-1), NLException);
}

TEST_F(SNLTermTest, testResizeLSBSuccessWithSlaveInstance) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto model = SNLDesign::create(library, NLName("model"));
  auto term = SNLBusTerm::create(model, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  auto top = SNLDesign::create(library, NLName("top"));
  auto inst = SNLInstance::create(top, model, NLName("u0"));

  term->setLSB(2);
  EXPECT_EQ(3, term->getMSB());
  EXPECT_EQ(2, term->getLSB());
  EXPECT_EQ(2, term->getWidth());
  EXPECT_NE(nullptr, term->getBit(3));
  EXPECT_NE(nullptr, term->getBit(2));
  EXPECT_EQ(nullptr, term->getBit(1));

  auto instTerm3 = inst->getInstTerm(term->getBit(3));
  auto instTerm2 = inst->getInstTerm(term->getBit(2));
  EXPECT_NE(nullptr, instTerm3);
  EXPECT_NE(nullptr, instTerm2);
}

TEST_F(SNLTermTest, testResizeLSBNoOp) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto design = SNLDesign::create(library, NLName("design"));
  auto term = SNLBusTerm::create(design, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  term->setLSB(0);
  EXPECT_EQ(3, term->getMSB());
  EXPECT_EQ(0, term->getLSB());
  EXPECT_EQ(4, term->getWidth());
}

TEST_F(SNLTermTest, testResizeLSBInvalid) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto design = SNLDesign::create(library, NLName("design"));
  auto term = SNLBusTerm::create(design, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  EXPECT_THROW(term->setLSB(4), NLException);
}

TEST_F(SNLTermTest, testResizeFailsOnNonConstantNet) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto design = SNLDesign::create(library, NLName("design"));
  auto term = SNLBusTerm::create(design, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));
  auto net = SNLBusNet::create(design, 3, 0, NLName("n0"));
  term->setNet(net);
  EXPECT_THROW(term->setMSB(1), NLException);
}

TEST_F(SNLTermTest, testResizeLSBFailsOnNonConstantNet) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto design = SNLDesign::create(library, NLName("design"));
  auto term = SNLBusTerm::create(design, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));
  auto net = SNLBusNet::create(design, 3, 0, NLName("n0"));
  term->setNet(net);
  EXPECT_THROW(term->setLSB(2), NLException);
}

TEST_F(SNLTermTest, testResizeFailsOnInternalInstanceConnection) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto model = SNLDesign::create(library, NLName("model"));
  auto term = SNLBusTerm::create(model, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));
  auto net = SNLBusNet::create(model, 3, 0, NLName("n0"));
  net->setType(SNLNet::Type::Assign0);
  term->setNet(net);

  auto leaf = SNLDesign::create(library, NLName("leaf"));
  auto leafTerm = SNLScalarTerm::create(leaf, SNLTerm::Direction::InOut, NLName("t"));
  auto inst = SNLInstance::create(model, leaf, NLName("u0"));
  inst->setTermNet(leafTerm, net->getBit(3));

  EXPECT_THROW(term->setMSB(2), NLException);
}

TEST_F(SNLTermTest, testResizeLSBFailsOnInternalInstanceConnection) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto model = SNLDesign::create(library, NLName("model"));
  auto term = SNLBusTerm::create(model, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));
  auto net = SNLBusNet::create(model, 3, 0, NLName("n0"));
  net->setType(SNLNet::Type::Assign0);
  term->setNet(net);

  auto leaf = SNLDesign::create(library, NLName("leaf"));
  auto leafTerm = SNLScalarTerm::create(leaf, SNLTerm::Direction::InOut, NLName("t"));
  auto inst = SNLInstance::create(model, leaf, NLName("u0"));
  inst->setTermNet(leafTerm, net->getBit(0));

  EXPECT_THROW(term->setLSB(1), NLException);
}

TEST_F(SNLTermTest, testResizeFailsOnSlaveInstanceConnected) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto model = SNLDesign::create(library, NLName("model"));
  auto term = SNLBusTerm::create(model, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  auto top = SNLDesign::create(library, NLName("top"));
  auto inst = SNLInstance::create(top, model, NLName("u0"));
  auto topNet = SNLScalarNet::create(top, NLName("n0"));
  auto instTerm = inst->getInstTerm(term->getBit(3));
  instTerm->setNet(topNet);

  EXPECT_THROW(term->setMSB(2), NLException);
}

TEST_F(SNLTermTest, testResizeLSBFailsOnSlaveInstanceConnected) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  auto model = SNLDesign::create(library, NLName("model"));
  auto term = SNLBusTerm::create(model, SNLTerm::Direction::InOut, 3, 0, NLName("bus"));

  auto top = SNLDesign::create(library, NLName("top"));
  auto inst = SNLInstance::create(top, model, NLName("u0"));
  auto topNet = SNLScalarNet::create(top, NLName("n0"));
  auto instTerm = inst->getInstTerm(term->getBit(0));
  instTerm->setNet(topNet);

  EXPECT_THROW(term->setLSB(1), NLException);
}
