#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
using namespace SNL;

class SNLDesignTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, "MYLIB");
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLDesignTest, testCreation) {
  SNLLibrary* library = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_TRUE(library);
  SNLDesign* design = SNLDesign::create(library, "design");
  ASSERT_TRUE(design);
  EXPECT_EQ("design", design->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_FALSE(design->isAnonymous());
  SNLScalarTerm* term0 = SNLScalarTerm::create(design, SNLName("term0"), SNLTerm::Direction::Input);
  ASSERT_TRUE(term0);
  EXPECT_EQ(SNLName("term0"), term0->getName());
  ASSERT_FALSE(term0->isAnonymous());
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(design, term0->getDesign());
  EXPECT_EQ(SNLTerm::Direction::Input, term0->getDirection());
  SNLScalarTerm* term1 = SNLScalarTerm::create(design, SNLName("term1"), SNLTerm::Direction::Output);
  ASSERT_TRUE(term1);
  EXPECT_EQ(SNLName("term1"), term1->getName());
  ASSERT_FALSE(term1->isAnonymous());
  EXPECT_EQ(1, term1->getID());
  EXPECT_EQ(design, term1->getDesign());
  EXPECT_EQ(SNLTerm::Direction::Output, term1->getDirection());
  //anonymous scalar term
  SNLScalarTerm* term2 = SNLScalarTerm::create(design, SNLTerm::Direction::InOut);
  ASSERT_TRUE(term2);
  EXPECT_TRUE(term2->getName().empty());
  ASSERT_TRUE(term2->isAnonymous());
  EXPECT_EQ(2, term2->getID());
  EXPECT_EQ(SNLTerm::Direction::InOut, term2->getDirection());
  EXPECT_EQ(design, term2->getDesign());

  SNLBusTerm* term3 = SNLBusTerm::create(design, SNLName("term3"), SNLTerm::Direction::Input);
  ASSERT_TRUE(term3);
  EXPECT_EQ(SNLName("term3"), term3->getName());
  ASSERT_FALSE(term3->isAnonymous());
  EXPECT_EQ(3, term3->getID());
  EXPECT_EQ(SNLTerm::Direction::Input, term3->getDirection());
  EXPECT_EQ(design, term3->getDesign()); 

  SNLDesign* model = SNLDesign::create(library, "model");
  ASSERT_TRUE(model);
  EXPECT_EQ("model", model->getName());
  EXPECT_EQ(1, model->getID());
  EXPECT_FALSE(model->isAnonymous());

  //anonymous design
  SNLDesign* anon = SNLDesign::create(library);
  ASSERT_TRUE(anon);
  EXPECT_EQ(2, anon->getID());
  EXPECT_TRUE(anon->isAnonymous());
}
