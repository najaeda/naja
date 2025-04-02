// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "PNLScalarTerm.h"
// #include "PNLBusTerm.h"
// #include "PNLBusTermBit.h"
#include "PNLScalarNet.h"
// #include "PNLBusNet.h"
// #include "PNLBusNetBit.h"

using namespace naja::NL;

class PNLTermTest: public ::testing::Test {
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

TEST_F(PNLTermTest, testCreation) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getPNLDesigns().size());
  EXPECT_TRUE(library->getPNLDesigns().empty());
  PNLDesign* design = PNLDesign::create(library, NLName("design"));

//   //PNLBusTerm* term0 = PNLBusTerm::create(design, PNLTerm::Direction::InOut, -1, -4, NLName("term0"));
//  // ASSERT_NE(term0, nullptr);
//   EXPECT_EQ(NLName("term0"), term0->getName());
//   ASSERT_FALSE(term0->isAnonymous());
//   EXPECT_EQ(0, term0->getID());
//   EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 0, 0, 0), term0->getNLID());
//   EXPECT_EQ(NLID::DesignObjectReference(1, 0, 0, 0), term0->getReference());
//   EXPECT_EQ(term0, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 0)));
//   EXPECT_EQ(term0, NLUniverse::get()->getObject(term0->getNLID()));
//   EXPECT_EQ(PNLTerm::Direction::InOut, term0->getDirection());
//   EXPECT_EQ(design, term0->getDesign()); 
//   EXPECT_EQ(-1, term0->getMSB());
//   EXPECT_EQ(-4, term0->getLSB());
//   EXPECT_EQ(4, term0->getWidth());
//   EXPECT_FALSE(term0->getBits().empty());
//   EXPECT_EQ(4, term0->getBits().size());
  // for (auto bit: term0->getBusBits()) {
  //   EXPECT_EQ(PNLTerm::Direction::InOut, bit->getDirection());
  //   EXPECT_EQ(term0, bit->getBus());
  //   EXPECT_FALSE(bit->isAnonymous());
  //   EXPECT_EQ(bit->getName(), bit->getBus()->getName());
  // }
  // EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
  //   ElementsAre(term0->getBit(-1), term0->getBit(-2), term0->getBit(-3), term0->getBit(-4)));
  // EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
  //   ElementsAre(
  //     term0->getBitAtPosition(0), term0->getBitAtPosition(1),
  //     term0->getBitAtPosition(2), term0->getBitAtPosition(3)));
  // EXPECT_EQ(nullptr, term0->getBitAtPosition(4));
  // EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -1), term0->getBit(-1)->getNLID());
  // //EXPECT_EQ(term0->getBit(-1), NLUniverse::get()->getBusTermBit(term0->getBit(-1)->getNLID()));
  // //EXPECT_EQ(term0->getBit(-1), NLUniverse::get()->getObject(term0->getBit(-1)->getNLID()));
  // EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -2), term0->getBit(-2)->getNLID());
  // //EXPECT_EQ(term0->getBit(-2), NLUniverse::get()->getBusTermBit(term0->getBit(-2)->getNLID()));
  // EXPECT_EQ(term0->getBit(-2), NLUniverse::get()->getObject(term0->getBit(-2)->getNLID()));
  // EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -3), term0->getBit(-3)->getNLID());
  // //EXPECT_EQ(term0->getBit(-3), NLUniverse::get()->getBusTermBit(term0->getBit(-3)->getNLID()));
  // EXPECT_EQ(term0->getBit(-3), NLUniverse::get()->getObject(term0->getBit(-3)->getNLID()));
  // EXPECT_EQ(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -4), term0->getBit(-4)->getNLID());
  // //EXPECT_EQ(term0->getBit(-4), NLUniverse::get()->getBusTermBit(term0->getBit(-4)->getNLID()));
  // EXPECT_EQ(term0->getBit(-4), NLUniverse::get()->getObject(term0->getBit(-4)->getNLID()));
  // EXPECT_EQ(0, term0->getFlatID());
  // EXPECT_EQ(0, term0->getBit(-1)->getFlatID());
  // EXPECT_EQ(1, term0->getBit(-2)->getFlatID());
  // EXPECT_EQ(2, term0->getBit(-3)->getFlatID());
  // EXPECT_EQ(3, term0->getBit(-4)->getFlatID());
  // EXPECT_EQ(0, term0->getBit(-1)->getPositionInBus());
  // EXPECT_EQ(1, term0->getBit(-2)->getPositionInBus());
  // EXPECT_EQ(2, term0->getBit(-3)->getPositionInBus());
  // EXPECT_EQ(3, term0->getBit(-4)->getPositionInBus());

  //EXPECT_EQ(term0, design->getBusTerm(NLName("term0")));
  //EXPECT_EQ(term0, design->getBusTerm(0));
  // EXPECT_EQ(term0, design->getTerm(0));
  // EXPECT_EQ(term0->getBit(-1), design->getBitTerm(0, -1));
  // EXPECT_EQ(term0->getBit(-4), design->getBitTerm(0, -4));

  // ASSERT_EQ(1, term0->getBit(-1)->getBits().size());
  // EXPECT_EQ(*term0->getBit(-1)->getBits().begin(), term0->getBit(-1));
  // ASSERT_EQ(1, term0->getBit(-2)->getBits().size());
  // EXPECT_EQ(*term0->getBit(-2)->getBits().begin(), term0->getBit(-2));
  // ASSERT_EQ(1, term0->getBit(-3)->getBits().size());
  // EXPECT_EQ(*term0->getBit(-3)->getBits().begin(), term0->getBit(-3));

  // EXPECT_EQ(nullptr, term0->getBit(-5));
  // EXPECT_EQ(nullptr, term0->getBit(0));
  // //EXPECT_EQ(nullptr, NLUniverse::get()->getBusTermBit(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -5)));
  // EXPECT_EQ(nullptr, NLUniverse::get()->getObject(NLID(NLID::Type::TermBit, 1, 0, 0, 0, 0, -5)));

  // EXPECT_THROW(term0->getBit(-4)->destroy(), NLException);
}

// TEST_F(PNLTermTest, testSetNet0) {
//   //SetNet for TermBus size > 1
//   NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
//   ASSERT_NE(library, nullptr);
//   EXPECT_EQ(0, library->getPNLDesigns().size());
//   EXPECT_TRUE(library->getPNLDesigns().empty());
//   PNLDesign* design = PNLDesign::create(library, NLName("design"));

//   //PNLBusTerm* term0 = PNLBusTerm::create(design, PNLTerm::Direction::InOut, -1, -4, NLName("term0"));

//   //same MSB, LSB
//   //PNLBusNet* net = PNLBusNet::create(design, -1, -4, NLName("n0"));
//   term0->setNet(net);
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(i));
//   }

//   //destroy net and assert that term bits are NULL
//   net->destroy();
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
//   }
  
//   //Same size: inversed MSB, LSB
//   //net = PNLBusNet::create(design, -4, -1, NLName("n0"));
//   term0->setNet(net);
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(-5-i));
//   }
//   net->destroy();
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
//   }

//   //Same size but not same MSB LSB
//   //net = PNLBusNet::create(design, 1, 4, NLName("n0"));
//   term0->setNet(net);
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(-i));
//   }
//   net->destroy();
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
//   } 

//   //Same as before but inversed
//   //net = PNLBusNet::create(design, 4, 1, NLName("n0"));
//   term0->setNet(net);
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), net->getBit(5+i));
//   }
//   net->destroy();
//   for (auto i=-1; i>=-4; i--) {
//     ASSERT_EQ(term0->getBit(i)->getNet(), nullptr);
//   }
// }

// TEST_F(PNLTermTest, testSetNet1) {
//   //SetNet for TermBus size == 1
//   NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
//   ASSERT_NE(library, nullptr);
//   EXPECT_EQ(0, library->getPNLDesigns().size());
//   EXPECT_TRUE(library->getPNLDesigns().empty());
//   PNLDesign* design = PNLDesign::create(library, NLName("design"));

//   //PNLBusTerm* term0 = PNLBusTerm::create(design, PNLTerm::Direction::InOut, -1, -1, NLName("term0"));

  //PNLScalarNet case
  //PNLNet* net = PNLScalarNet::create(design, NLName("n0"));
  // term0->setNet(net);
  // ASSERT_EQ(term0->getBit(-1)->getNet(), net);
  //net->destroy();
  // ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);

//   // //1 bit bus PNLBusNet case
//   // //net = PNLBusNet::create(design, -1, -1, NLName("n0"));
//   // term0->setNet(net);
//   // ASSERT_EQ(term0->getBit(-1)->getNet(), static_cast<PNLBusNet*>(net)->getBit(-1));
//   // net->destroy();
//   // ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);

//   // //1 bit bus PNLBusNet case
//   // //net = PNLBusNet::create(design, 5, 5, NLName("n0"));
//   // term0->setNet(net);
//   // EXPECT_EQ(nullptr, term0->getNet());
//   // //ASSERT_EQ(term0->getBit(-1)->getNet(), static_cast<PNLBusNet*>(net)->getBit(5));
//   // net->destroy();
//   // ASSERT_EQ(term0->getBit(-1)->getNet(), nullptr);
// }

TEST_F(PNLTermTest, testSetNetErrors) {
   //SetNet for TermBus size == 1
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getPNLDesigns().size());
  EXPECT_TRUE(library->getPNLDesigns().empty());
  PNLDesign* design = PNLDesign::create(library, NLName("design"));

//   //PNLBusTerm* term0 = PNLBusTerm::create(design, PNLTerm::Direction::InOut, -2, 2, NLName("term0"));

  //With scalar net
  PNLNet* net = PNLScalarNet::create(design, NLName("n0"));
  //EXPECT_THROW(term0->setNet(net), NLException);
  net->destroy();
 
//   //different size PNLBusNet case
//   //net = PNLBusNet::create(design, 0, 2, NLName("n0"));
//   EXPECT_THROW(term0->setNet(net), NLException);
//   net->destroy();

  //other design
  auto other = PNLDesign::create(library, NLName("other"));
  PNLNet* otherNet = PNLScalarNet::create(other, NLName("n0"));
  //EXPECT_THROW(term0->setNet(otherNet), NLException);
}

TEST_F(PNLTermTest, testErrors) {
  EXPECT_THROW(PNLScalarTerm::create(nullptr, PNLTerm::Direction::Input), NLException);
  //EXPECT_THROW(PNLBusTerm::create(nullptr, PNLTerm::Direction::Input, 31, 0), NLException);

  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  PNLScalarTerm* term0 = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("term0"));
  ASSERT_NE(nullptr, term0);
  //PNLBusTerm* term1 = PNLBusTerm::create(design, PNLTerm::Direction::Input, 31, 0, NLName("term1"));
  //ASSERT_NE(nullptr, term1);
  //EXPECT_THROW(PNLBusTerm::create(design, PNLTerm::Direction::Input, 31, 0, NLName("term0")), NLException);
  PNLScalarTerm* term1 = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("term1"));
  EXPECT_THROW(PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("term1")), NLException);
  //EXPECT_THROW(PNLBusTerm::create(design, NLID::DesignObjectID(0), PNLTerm::Direction::Input, 31, 0), NLException);
  EXPECT_THROW(PNLScalarTerm::create(design, NLID::DesignObjectID(1), PNLTerm::Direction::Input), NLException);
  //EXPECT_THROW(term1->getBit(1)->setName(NLName("bit1")), NLException);
  PNLScalarTerm::create(design, NLID::DesignObjectID(2), PNLTerm::Direction::Input, NLName("term2"));
}

TEST_F(PNLTermTest, testRename) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  auto term0 = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("term0"));
  //auto term1 = PNLBusTerm::create(design, PNLTerm::Direction::Output, 31, 0, NLName("term1"));
  auto term2 = PNLScalarTerm::create(design, PNLTerm::Direction::Input);
  EXPECT_EQ(term0, design->getTerm(NLName("term0")));
  EXPECT_EQ(term0, design->getBitTerm(0, 0));
  EXPECT_EQ(term0, design->getBitTerm(0, -12));
  EXPECT_EQ(term0, design->getBitTerm(0, 100));
  //EXPECT_EQ(term1, design->getTerm(NLName("term1")));
  EXPECT_EQ(term2, design->getBitTerm(1, 0));
  EXPECT_EQ(term2, design->getBitTerm(1, -12));
  EXPECT_EQ(term2, design->getBitTerm(1, 100));
  EXPECT_FALSE(term0->isAnonymous());
  term0->setName(NLName());
  EXPECT_TRUE(term0->isAnonymous());
  EXPECT_EQ(nullptr, design->getTerm(NLName("term0")));
  term0->setName(NLName("term0"));
  EXPECT_FALSE(term0->isAnonymous());
  EXPECT_EQ(term0, design->getTerm(NLName("term0")));
  // EXPECT_FALSE(term1->isAnonymous());
  // term1->setName(NLName("term1")); //nothing should happen...
  // EXPECT_EQ(term1, design->getTerm(NLName("term1")));
  // term1->setName(NLName("t1"));
  // EXPECT_FALSE(term1->isAnonymous());
  // EXPECT_EQ(nullptr, design->getTerm(NLName("term1")));
  // EXPECT_EQ(term1, design->getTerm(NLName("t1")));
  EXPECT_TRUE(term2->isAnonymous());
  term2->setName(NLName("term2"));
  EXPECT_FALSE(term2->isAnonymous());
  EXPECT_EQ(term2, design->getTerm(NLName("term2")));
  //Collision error
  EXPECT_THROW(term2->setName(NLName("term0")), NLException);
}

TEST_F(PNLTermTest, testDestroy) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);

  auto term0 = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("term0"));
  //auto term1 = PNLBusTerm::create(design, PNLTerm::Direction::Output, 31, 0, NLName("term1"));
  auto term2 = PNLScalarTerm::create(design, PNLTerm::Direction::Input);

  EXPECT_EQ(term0->getNet(), nullptr);
  using BitTerms = std::vector<PNLBitTerm*>;
  // BitTerms term1Bits(term1->getBits().begin(), term1->getBits().end());
  // EXPECT_THAT(term1Bits,
  //   ::testing::Each(::testing::Property(
  //       &PNLBusTermBit::getNet, ::testing::IsNull())));
  EXPECT_EQ(term2->getNet(), nullptr);

  auto net0 = PNLScalarNet::create(design, NLName("net0"));
  // auto net1 = PNLBusNet::create(design, 31, 0, NLName("net1"));
  // auto net2 = PNLBusNet::create(design, 0, 0, NLName("net2"));
  term0->setNet(net0);
  //term1->setNet(net1);
  //term2->setNet(net2);
  EXPECT_EQ(term0->getNet(), net0);
  //EXPECT_EQ(term2->getNet(), net2->getBit(0));

  //start destroying
  term0->getNet()->destroy();
  ASSERT_EQ(term0->getNet(), nullptr);

  //net2->destroy();
  ASSERT_EQ(term2->getNet(), nullptr);

  //destroy one bit of net1
  //ASSERT_NE(term1->getBit(10)->getNet(), nullptr);
  //net1->getBit(10)->destroy();
  //ASSERT_EQ(term1->getBit(10)->getNet(), nullptr);
  //net1->getBit(6)->destroy();
  //ASSERT_EQ(term1->getBit(6)->getNet(), nullptr);

  //net1->destroy();
  // EXPECT_THAT(term1Bits,
  //   ::testing::Each(::testing::Property(
  //       &PNLBusTermBit::getNet, ::testing::IsNull())));
}
