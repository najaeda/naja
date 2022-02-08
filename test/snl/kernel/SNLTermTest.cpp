#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLException.h"
using namespace SNL;

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
  }
  EXPECT_THAT(std::vector(term0->getBits().begin(), term0->getBits().end()),
    ElementsAre(term0->getBit(-1), term0->getBit(-2), term0->getBit(-3), term0->getBit(-4)));
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -1), term0->getBit(-1)->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -2), term0->getBit(-2)->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -3), term0->getBit(-3)->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::TermBit, 1, 0, 0, 0, 0, -4), term0->getBit(-4)->getSNLID());

  EXPECT_FALSE(term0->getBit(-5));
  EXPECT_FALSE(term0->getBit(0));

  EXPECT_THROW(term0->getBit(-4)->destroy(), SNLException);
}