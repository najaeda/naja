#include "gtest/gtest.h"

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
using namespace SNL;

class SNLDesignTest: public ::testing::Test {
  protected:
    void SetUp() override {
      db_ = SNLDB::create();
      SNLLibrary* library = SNLLibrary::create(db_, "MYLIB");
    }
    void TearDown() override {
      db_->destroy();
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
  EXPECT_EQ(0, term0->getID());
  EXPECT_EQ(design, term0->getDesign());
  EXPECT_EQ(SNLTerm::Direction::Input, term0->getDirection());
  SNLScalarTerm* term1 = SNLScalarTerm::create(design, SNLName("term1"), SNLTerm::Direction::Output);
  ASSERT_TRUE(term1);
  EXPECT_EQ(SNLName("term1"), term1->getName());
  EXPECT_EQ(1, term1->getID());
  EXPECT_EQ(design, term1->getDesign());
  EXPECT_EQ(SNLTerm::Direction::Output, term0->getDirection());
  //anonymous scalar term
  SNLScalarTerm* term2 = SNLScalarTerm::create(design, SNLTerm::Direction::InOut);
  ASSERT_TRUE(term2);
  EXPECT_TRUE(term2->getName().empty());
  ASSERT_TRUE(term2->isAnonymous());
  EXPECT_EQ(2, term2->getID());
  EXPECT_EQ(SNLTerm::Direction::InOut, term0->getDirection());
  EXPECT_EQ(design, term2->getDesign());

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
