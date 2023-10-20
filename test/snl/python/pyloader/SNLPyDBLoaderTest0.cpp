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
}