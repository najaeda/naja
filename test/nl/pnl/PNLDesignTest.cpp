// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "PNLDesign.h"
#include "PNLScalarTerm.h"
#include "NLException.h"
using namespace naja::NL;

class PNLDesignTest: public ::testing::Test {
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

TEST_F(PNLDesignTest, testCreation0) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getPNLDesigns().size());
  EXPECT_TRUE(library->getPNLDesigns().empty());
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(NLName("design"), design->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_FALSE(design->isAnonymous());
  EXPECT_EQ(design, library->getPNLDesign(0));
  EXPECT_EQ(design, library->getPNLDesign(NLName("design")));
  EXPECT_EQ(library, design->getLibrary());
  EXPECT_EQ(1, library->getPNLDesigns().size());
  EXPECT_FALSE(library->getPNLDesigns().empty());
  EXPECT_EQ(db_, design->getDB());
  EXPECT_EQ(NLID(1, 0, 0), design->getNLID());
  //EXPECT_EQ(design, NLUniverse::get()->getPNLDesign(NLID::DesignReference(1, 0, 0)));
  //EXPECT_EQ(design, NLUniverse::get()->getObject(NLID(1, 0, 0)));
  EXPECT_TRUE(design->isStandard());
  EXPECT_TRUE(design->getTerms().empty());
  EXPECT_TRUE(design->getScalarTerms().empty());
  auto designReference = NLID::DesignReference(1, 0, 0);
  EXPECT_EQ(designReference, design->getReference());
  // //EXPECT_EQ(design, NLUniverse::get()->getPNLDesign(designReference));
  // designReference = NLID::DesignReference(1, 1, 0);
  // EXPECT_EQ(nullptr, NLUniverse::get()->getPNLDesign(designReference)); 
  // EXPECT_NE(designReference, design->getReference());
  // designReference = NLID::DesignReference(1, 0, 1);
  // EXPECT_EQ(nullptr, NLUniverse::get()->getPNLDesign(designReference)); 
  // EXPECT_NE(designReference, design->getReference());

  PNLScalarTerm* term0 = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("term0"));
  ASSERT_NE(term0, nullptr);
  EXPECT_EQ(NLName("term0"), term0->getName());
  ASSERT_FALSE(term0->isAnonymous());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 0, 0, 0), term0->getNLID());
  EXPECT_EQ(design, term0->getDesign());
  //EXPECT_EQ(term0, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 0)));
  //EXPECT_EQ(term0, NLUniverse::get()->getObject(term0->getNLID()));
  EXPECT_EQ(PNLTerm::Direction::Input, term0->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBitTerms().size());

  PNLScalarTerm* term1 = PNLScalarTerm::create(design, PNLTerm::Direction::Output, NLName("term1"));
  ASSERT_NE(term1, nullptr);
  EXPECT_EQ(NLName("term1"), term1->getName());
  ASSERT_FALSE(term1->isAnonymous());
  EXPECT_EQ(1, term1->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 1, 0, 0), term1->getNLID());
  EXPECT_LT(term0->getNLID(), term1->getNLID());
  EXPECT_EQ(design, term1->getDesign());
  //EXPECT_EQ(term1, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 1)));
  //EXPECT_EQ(term1, NLUniverse::get()->getObject(term1->getNLID()));
  EXPECT_EQ(PNLTerm::Direction::Output, term1->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_EQ(2, design->getBitTerms().size());
  EXPECT_EQ(term1, design->getScalarTerm(NLName("term1")));
  EXPECT_EQ(term1, design->getScalarTerm(1));
  EXPECT_EQ(term1, design->getBitTerm(1, 0));
  EXPECT_EQ(term1, design->getBitTerm(1, 11)); //bit irrelevant
  EXPECT_EQ(nullptr, design->getScalarTerm(NLName("term2")));
  EXPECT_EQ(nullptr, design->getBitTerm(2, -100)); //bit irrelevant

  //anonymous scalar term
  PNLScalarTerm* term2 = PNLScalarTerm::create(design, PNLTerm::Direction::InOut);
  ASSERT_NE(term2, nullptr);
  EXPECT_TRUE(term2->getName().empty());
  ASSERT_TRUE(term2->isAnonymous());
  EXPECT_EQ(2, term2->getID());
  EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 2, 0, 0), term2->getNLID());
  EXPECT_LT(term1->getNLID(), term2->getNLID());
  //EXPECT_EQ(term2, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 2)));
  //EXPECT_EQ(term2, NLUniverse::get()->getObject(term2->getNLID()));
  EXPECT_EQ(PNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_EQ(design, term2->getDesign());
  EXPECT_EQ(PNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBitTerms().size());
  EXPECT_EQ(term2, design->getScalarTerm(2));

  // PNLBusTerm* term3 = PNLBusTerm::create(design, PNLTerm::Direction::Input, 4, 0, NLName("term3"));
  // ASSERT_NE(term3, nullptr);
  // EXPECT_EQ(NLName("term3"), term3->getName());
  // ASSERT_FALSE(term3->isAnonymous());
  // EXPECT_EQ(3, term3->getID());
  // EXPECT_EQ(NLID(NLID::Type::Term, 1, 0, 0, 3, 0, 0), term3->getNLID());
  // EXPECT_EQ(term3, NLUniverse::get()->getTerm(NLID::DesignObjectReference(1, 0, 0, 3)));
  // EXPECT_EQ(term3, NLUniverse::get()->getObject(term3->getNLID()));
  // EXPECT_EQ(PNLTerm::Direction::Input, term3->getDirection());
  // EXPECT_EQ(design, term3->getDesign()); 
  // EXPECT_EQ(4, term3->getMSB());
  // EXPECT_EQ(0, term3->getLSB());
  // EXPECT_EQ(5, term3->getWidth());
  // EXPECT_FALSE(term3->getBits().empty());
  // EXPECT_EQ(5, term3->getBits().size());
  // for (auto bit: term3->getBusBits()) {
  //   EXPECT_EQ(PNLTerm::Direction::Input, bit->getDirection());
  //   EXPECT_EQ(term3, bit->getBus());
  // }
  // EXPECT_EQ(term3, design->getBusTerm(NLName("term3")));
  // EXPECT_EQ(term3, design->getBusTerm(3));
  // EXPECT_EQ(nullptr, design->getBusTerm(NLName("term4")));
  // EXPECT_EQ(nullptr, design->getBusTerm(4));

  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  // EXPECT_FALSE(design->getBusTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  // EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  // EXPECT_EQ(1, design->getBusTerms().size());
  //EXPECT_EQ(1+1+1+5, design->getBitTerms().size());

  // EXPECT_THAT(std::vector(design->getTerms().begin(), design->getTerms().end()),
  //   ElementsAre(term0, term1, term2, term3));
  auto scalarTermsBegin = design->getScalarTerms().begin();
  auto scalarTermsEnd = design->getScalarTerms().end();
  EXPECT_THAT(std::vector(scalarTermsBegin, scalarTermsEnd), ElementsAre(term0, term1, term2));
  EXPECT_THAT(std::vector(design->getScalarTerms().begin(), design->getScalarTerms().end()),
    ElementsAre(term0, term1, term2));
  // auto busTermsBegin = design->getBusTerms().begin();
  // auto busTermsEnd = design->getBusTerms().end();
  // EXPECT_THAT(std::vector(busTermsBegin, busTermsEnd), ElementsAre(term3));
  // EXPECT_THAT(std::vector(design->getBitTerms().begin(), design->getBitTerms().end()),
  //   ElementsAre(term0, term1, term2,
  //     term3->getBit(4), term3->getBit(3), term3->getBit(2), term3->getBit(1), term3->getBit(0)));
  // auto bitTermsBegin = design->getBitTerms().begin();
  // auto bitTermsEnd = design->getBitTerms().end();
  // EXPECT_THAT(std::vector(bitTermsBegin, bitTermsEnd),
  //   ElementsAre(term0, term1, term2,
  //     term3->getBit(4), term3->getBit(3), term3->getBit(2), term3->getBit(1), term3->getBit(0)));

  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  ASSERT_NE(model, nullptr);
  EXPECT_EQ("model", model->getName().getString());
  EXPECT_EQ(1, model->getID());
  EXPECT_EQ(NLID(1, 0, 1), model->getNLID());
  EXPECT_LT(design->getNLID(), model->getNLID());
  //EXPECT_EQ(model, NLUniverse::get()->getPNLDesign(NLID::DesignReference(1, 0, 1)));
  //EXPECT_EQ(model, NLUniverse::get()->getObject(NLID(1, 0, 1)));
  EXPECT_FALSE(model->isAnonymous());
  EXPECT_EQ(model, library->getPNLDesign(1));
  EXPECT_EQ(model, library->getPNLDesign(NLName("model")));
  EXPECT_EQ(2, library->getPNLDesigns().size());
  EXPECT_FALSE(library->getPNLDesigns().empty());
  EXPECT_THAT(std::vector(library->getPNLDesigns().begin(), library->getPNLDesigns().end()),
    ElementsAre(design, model));

  //anonymous design
  PNLDesign* anon = PNLDesign::create(library);
  ASSERT_NE(anon, nullptr);
  EXPECT_EQ(2, anon->getID());
  EXPECT_EQ(NLID(1, 0, 2), anon->getNLID());
  //EXPECT_EQ(anon, NLUniverse::get()->getPNLDesign(NLID::DesignReference(1, 0, 2)));
  //EXPECT_EQ(anon, NLUniverse::get()->getObject(NLID(1, 0, 2)));
  EXPECT_TRUE(anon->isAnonymous());
  EXPECT_EQ(anon, library->getPNLDesign(2));
  EXPECT_EQ(library, anon->getLibrary());
  EXPECT_EQ(db_, anon->getDB());
  EXPECT_EQ(NLID(1, 0, 2), anon->getNLID());
  EXPECT_TRUE(anon->isStandard());
  EXPECT_EQ(3, library->getPNLDesigns().size());
  EXPECT_FALSE(library->getPNLDesigns().empty());
  EXPECT_THAT(std::vector(library->getPNLDesigns().begin(), library->getPNLDesigns().end()),
    ElementsAre(design, model, anon));
}

TEST_F(PNLDesignTest, testCreation1) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getPNLDesigns().size());
  EXPECT_TRUE(library->getPNLDesigns().empty());

  //anonymous design
  PNLDesign* anon = PNLDesign::create(library);
  ASSERT_NE(anon, nullptr);
  EXPECT_EQ(0, anon->getID());
  EXPECT_TRUE(anon->isAnonymous());
  EXPECT_EQ(anon, library->getPNLDesign(0));
  EXPECT_EQ(library, anon->getLibrary());
  EXPECT_EQ(db_, anon->getDB());
  EXPECT_EQ(NLID(1, 0, 0), anon->getNLID());
  EXPECT_TRUE(anon->isStandard());
  EXPECT_EQ(1, library->getPNLDesigns().size());
  EXPECT_FALSE(library->getPNLDesigns().empty());
  EXPECT_THAT(std::vector(library->getPNLDesigns().begin(), library->getPNLDesigns().end()),
    ElementsAre(anon));
}

TEST_F(PNLDesignTest, testPrimitives) {
  EXPECT_THROW(
    PNLDesign::create(nullptr, NLName("ERROR")),
    NLException);

  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);

  EXPECT_THROW(
    PNLDesign::create(library, PNLDesign::Type::Primitive, NLName("ERROR")),
    NLException);

  auto design = PNLDesign::create(library);
  EXPECT_EQ(NLID::DesignID(0), design->getID());
  EXPECT_THROW(
    PNLDesign::create(library, NLID::DesignID(0), PNLDesign::Type::Standard),
    NLException);
  EXPECT_THROW(design->setType(PNLDesign::Type::Primitive), NLException);

  auto prims = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("Primitives"));
  EXPECT_TRUE(prims->isPrimitives());
  auto prim1 = PNLDesign::create(prims, PNLDesign::Type::Primitive, NLName("Primitive"));
  EXPECT_NE(nullptr, prim1);
  EXPECT_FALSE(prim1->isStandard());
  EXPECT_FALSE(prim1->isBlackBox());
  EXPECT_TRUE(prim1->isPrimitive());
  EXPECT_TRUE(prim1->isLeaf());
  //EXPECT_EQ(nullptr, PNLUtils::findTop(prims));

  EXPECT_THROW(
    PNLDesign::create(library, PNLDesign::Type::Primitive),
    NLException);
  EXPECT_THROW(
    PNLDesign::create(prims),
    NLException);
  EXPECT_THROW(
    PNLDesign::create(prims, NLName("NonPrimitive")),
    NLException);
}

TEST_F(PNLDesignTest, testSetTop) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design0 = PNLDesign::create(library, NLName("design0"));
  ASSERT_NE(design0, nullptr);
  //EXPECT_EQ(design0, PNLUtils::findTop(library));
  PNLDesign* design1 = PNLDesign::create(library, NLName("design1"));
  design1->setType(PNLDesign::Type::Standard);
  ASSERT_NE(design1, nullptr);
  //EXPECT_EQ(nullptr, PNLUtils::findTop(library));

  // EXPECT_EQ(nullptr, NLUniverse::get()->getTopDesign());
  EXPECT_EQ(nullptr, NLUniverse::get()->getTopDB());
  // EXPECT_FALSE(design0->isTopDesign());
  // EXPECT_FALSE(design1->isTopDesign());
  // design0->getDB()->setTopDesign(design0);
  // EXPECT_TRUE(design0->isTopDesign());
  // EXPECT_FALSE(design1->isTopDesign());
  // EXPECT_EQ(design0, design0->getDB()->getTopDesign());
  // EXPECT_FALSE(NLUniverse::get()->getTopDesign());
  EXPECT_FALSE(NLUniverse::get()->getTopDB());
  NLUniverse::get()->setTopDB(design0->getDB());
  EXPECT_TRUE(design0->getDB()->isTopDB());
  EXPECT_EQ(NLUniverse::get()->getTopDB(), design0->getDB());
  // EXPECT_EQ(NLUniverse::get()->getTopDesign(), design0);
  // NLUniverse::get()->setTopDesign(design1);
  // EXPECT_FALSE(design0->isTopDesign());
  // EXPECT_TRUE(design1->isTopDesign());
  EXPECT_TRUE(design0->getDB()->isTopDB());

  auto altDB = NLDB::create(NLUniverse::get());
  auto altLibrary = NLLibrary::create(altDB);
  auto altDesign = PNLDesign::create(altLibrary);
  // EXPECT_THROW(design0->getDB()->setTopDesign(altDesign), NLException);
}

// TEST_F(PNLDesignTest, testSetName) {
//   NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
//   ASSERT_NE(library, nullptr);
//   PNLDesign* design0 = PNLDesign::create(library, NLName("design0"));
//   EXPECT_EQ(NLName("design0"), design0->getName());
//   design0->setName(NLName("design0"));
//   EXPECT_EQ(NLName("design0"), design0->getName());
//   EXPECT_EQ(design0, library->getPNLDesign(NLName("design0")));

//   design0->setName(NLName("design1"));
//   EXPECT_EQ(NLName("design1"), design0->getName());
//   EXPECT_EQ(design0, library->getPNLDesign(NLName("design1")));
//   EXPECT_EQ(nullptr, library->getPNLDesign(NLName("design0")));
//   design0->setName(NLName("design0"));
//   EXPECT_EQ(NLName("design0"), design0->getName());
//   EXPECT_EQ(design0, library->getPNLDesign(NLName("design0")));
//   EXPECT_EQ(nullptr, library->getPNLDesign(NLName("design1")));

//   auto design1 = PNLDesign::create(library, NLName("design1"));
//   EXPECT_EQ(NLName("design1"), design1->getName());
//   EXPECT_EQ(design1, library->getPNLDesign(NLName("design1")));
//   EXPECT_THROW(design1->setName(NLName("design0")), NLException);
// }

TEST_F(PNLDesignTest, testAbutmentBox) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design0 = PNLDesign::create(library, NLName("design0"));
  PNLBox box0(0, 0, 10, 10);
  design0->setAbutmentBox(box0);
  EXPECT_EQ(box0, design0->getAbutmentBox());
}

TEST_F(PNLDesignTest, testAddNet) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design0 = PNLDesign::create(library, NLName("design0"));
  PNLNet* net0 = design0->addNet(NLName("net0"));
  EXPECT_EQ(NLName("net0"), net0->getName());
  EXPECT_EQ(0, net0->getID());
  EXPECT_EQ(design0, net0->getDesign());
  EXPECT_EQ(net0, design0->getNet(NLName("net0")));
}


TEST_F(PNLDesignTest, addTerm) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design0 = PNLDesign::create(library, NLName("design0"));
  PNLTerm* term0 = design0->addTerm(NLName("term0"));
  EXPECT_EQ(NLName("term0"), term0->getName());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(design0, term0->getDesign());
  EXPECT_EQ(term0, design0->getTerm(NLName("term0")));
}


TEST_F(PNLDesignTest, testSetNameCollision) {
  NLLibrary* library = db_->getLibrary(NLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  PNLDesign* design0 = PNLDesign::create(library, NLName("design0"));
  PNLDesign* design1 = PNLDesign::create(library, NLName("design1"));
  EXPECT_THROW(design0->setName(NLName("design1")), NLException);
  EXPECT_THROW(design1->setName(NLName("design0")), NLException);
  design0->setName(NLName("design2"));
  EXPECT_EQ(NLName("design2"), design0->getName());
  design0->setName(NLName("design2"));
  EXPECT_EQ(NLName("design2"), design0->getName());
}