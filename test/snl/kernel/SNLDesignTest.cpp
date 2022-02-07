#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
using namespace SNL;

class SNLDesignTest: public ::testing::Test {
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

TEST_F(SNLDesignTest, testCreation0) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getDesigns().size());
  EXPECT_TRUE(library->getDesigns().empty());
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  ASSERT_NE(design, nullptr);
  EXPECT_EQ(SNLName("design"), design->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_FALSE(design->isAnonymous());
  EXPECT_EQ(design, library->getDesign(0));
  EXPECT_EQ(design, library->getDesign(SNLName("design")));
  EXPECT_EQ(library, design->getLibrary());
  EXPECT_EQ(1, library->getDesigns().size());
  EXPECT_FALSE(library->getDesigns().empty());
  EXPECT_EQ(db_, design->getDB());
  EXPECT_EQ(SNLID(1, 0, 0), design->getSNLID());
  EXPECT_TRUE(design->isStandard());
  EXPECT_TRUE(design->getTerms().empty());
  EXPECT_TRUE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_TRUE(design->getBitTerms().empty());

  SNLScalarTerm* term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("term0"));
  ASSERT_NE(term0, nullptr);
  EXPECT_EQ(SNLName("term0"), term0->getName());
  ASSERT_FALSE(term0->isAnonymous());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 0, 0, 0, 0), term0->getSNLID());
  EXPECT_EQ(design, term0->getDesign());
  EXPECT_EQ(SNLTerm::Direction::Input, term0->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBitTerms().size());

  SNLScalarTerm* term1 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, SNLName("term1"));
  ASSERT_NE(term1, nullptr);
  EXPECT_EQ(SNLName("term1"), term1->getName());
  ASSERT_FALSE(term1->isAnonymous());
  EXPECT_EQ(1, term1->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 0, 1, 0, 0), term1->getSNLID());
  EXPECT_EQ(design, term1->getDesign());
  EXPECT_EQ(SNLTerm::Direction::Output, term1->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_EQ(2, design->getBitTerms().size());

  //anonymous scalar term
  SNLScalarTerm* term2 = SNLScalarTerm::create(design, SNLTerm::Direction::InOut);
  ASSERT_NE(term2, nullptr);
  EXPECT_TRUE(term2->getName().empty());
  ASSERT_TRUE(term2->isAnonymous());
  EXPECT_EQ(2, term2->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 0, 2, 0, 0), term2->getSNLID());
  EXPECT_EQ(SNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_EQ(design, term2->getDesign());
  EXPECT_EQ(SNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_TRUE(design->getBusTerms().empty());
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBitTerms().size());

  SNLBusTerm* term3 = SNLBusTerm::create(design, SNLTerm::Direction::Input, 4, 0, SNLName("term3"));
  ASSERT_NE(term3, nullptr);
  EXPECT_EQ(SNLName("term3"), term3->getName());
  ASSERT_FALSE(term3->isAnonymous());
  EXPECT_EQ(3, term3->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 0, 3, 0, 0), term3->getSNLID());
  EXPECT_EQ(SNLTerm::Direction::Input, term3->getDirection());
  EXPECT_EQ(design, term3->getDesign()); 
  EXPECT_EQ(4, term3->getMSB());
  EXPECT_EQ(0, term3->getLSB());
  EXPECT_EQ(5, term3->getSize());
  EXPECT_FALSE(design->getTerms().empty());
  EXPECT_FALSE(design->getScalarTerms().empty());
  EXPECT_FALSE(design->getBusTerms().empty());
  EXPECT_FALSE(design->getBitTerms().empty());
  EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_EQ(1, design->getBusTerms().size());
  EXPECT_EQ(1+1+1+5, design->getBitTerms().size());

  EXPECT_THAT(std::vector(design->getTerms().begin(), design->getTerms().end()),
    ElementsAre(term0, term1, term2, term3));
  auto scalarTermsBegin = design->getScalarTerms().begin();
  auto scalarTermsEnd = design->getScalarTerms().end();
  EXPECT_THAT(std::vector(scalarTermsBegin, scalarTermsEnd), ElementsAre(term0, term1, term2));
  EXPECT_THAT(std::vector(design->getScalarTerms().begin(), design->getScalarTerms().end()),
    ElementsAre(term0, term1, term2));
  auto busTermsBegin = design->getBusTerms().begin();
  auto busTermsEnd = design->getBusTerms().end();
  EXPECT_THAT(std::vector(busTermsBegin, busTermsEnd), ElementsAre(term3));
  EXPECT_THAT(std::vector(design->getBitTerms().begin(), design->getBitTerms().end()),
    ElementsAre(term0, term1, term2,
      term3->getBit(4), term3->getBit(3), term3->getBit(2), term3->getBit(1), term3->getBit(0)));
  auto bitTermsBegin = design->getBitTerms().begin();
  auto bitTermsEnd = design->getBitTerms().end();
  EXPECT_THAT(std::vector(bitTermsBegin, bitTermsEnd),
    ElementsAre(term0, term1, term2,
      term3->getBit(4), term3->getBit(3), term3->getBit(2), term3->getBit(1), term3->getBit(0)));

  SNLDesign* model = SNLDesign::create(library, SNLName("model"));
  ASSERT_NE(model, nullptr);
  EXPECT_EQ("model", model->getName().getString());
  EXPECT_EQ(1, model->getID());
  EXPECT_FALSE(model->isAnonymous());
  EXPECT_EQ(model, library->getDesign(1));
  EXPECT_EQ(model, library->getDesign(SNLName("model")));
  EXPECT_EQ(2, library->getDesigns().size());
  EXPECT_FALSE(library->getDesigns().empty());
  EXPECT_THAT(std::vector(library->getDesigns().begin(), library->getDesigns().end()),
    ElementsAre(design, model));

  //anonymous design
  SNLDesign* anon = SNLDesign::create(library);
  ASSERT_NE(anon, nullptr);
  EXPECT_EQ(2, anon->getID());
  EXPECT_TRUE(anon->isAnonymous());
  EXPECT_EQ(anon, library->getDesign(2));
  EXPECT_EQ(library, anon->getLibrary());
  EXPECT_EQ(db_, anon->getDB());
  EXPECT_EQ(SNLID(1, 0, 2), anon->getSNLID());
  EXPECT_TRUE(anon->isStandard());
  EXPECT_EQ(3, library->getDesigns().size());
  EXPECT_FALSE(library->getDesigns().empty());
  EXPECT_THAT(std::vector(library->getDesigns().begin(), library->getDesigns().end()),
    ElementsAre(design, model, anon));
}

TEST_F(SNLDesignTest, testCreation1) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_NE(library, nullptr);
  EXPECT_EQ(0, library->getDesigns().size());
  EXPECT_TRUE(library->getDesigns().empty());

  //anonymous design
  SNLDesign* anon = SNLDesign::create(library);
  ASSERT_NE(anon, nullptr);
  EXPECT_EQ(0, anon->getID());
  EXPECT_TRUE(anon->isAnonymous());
  EXPECT_EQ(anon, library->getDesign(0));
  EXPECT_EQ(library, anon->getLibrary());
  EXPECT_EQ(db_, anon->getDB());
  EXPECT_EQ(SNLID(1, 0, 0), anon->getSNLID());
  EXPECT_TRUE(anon->isStandard());
  EXPECT_EQ(1, library->getDesigns().size());
  EXPECT_FALSE(library->getDesigns().empty());
  EXPECT_THAT(std::vector(library->getDesigns().begin(), library->getDesigns().end()),
    ElementsAre(anon));
}
