#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLPyLoader.h"
#include "SNLException.h"
using namespace naja::SNL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPyDBLoaderTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse::create();
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPyDBLoaderTest0, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto dbScriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  dbScriptPath /= "scripts";
  dbScriptPath /= "db_loader.py";
  SNLPyLoader::loadDB(db, dbScriptPath);

  ASSERT_EQ(2, db->getLibraries().size());

#if 0
  ASSERT_EQ(1, library->getDesigns().size());
  auto lut4 = library->getDesign(SNLName("LUT4")); 
  EXPECT_TRUE(lut4->isPrimitive());
  ASSERT_NE(nullptr, lut4);
  ASSERT_EQ(5, lut4->getScalarTerms().size());
  using Terms = std::vector<SNLScalarTerm*>;
  Terms terms(lut4->getScalarTerms().begin(), lut4->getScalarTerms().end()); 
  ASSERT_EQ(5, terms.size());
  EXPECT_EQ("I0", terms[0]->getName().getString());
  EXPECT_EQ("I1", terms[1]->getName().getString());
  EXPECT_EQ("I2", terms[2]->getName().getString());
  EXPECT_EQ("I3", terms[3]->getName().getString());
  EXPECT_EQ("O", terms[4]->getName().getString());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[0]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[1]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[2]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[3]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output,  terms[4]->getDirection());
#endif
}