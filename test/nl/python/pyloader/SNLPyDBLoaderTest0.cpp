// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLPyLoader.h"
using namespace naja::SNL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPyDBLoaderTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse::create();
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPyDBLoaderTest0, test) {
  auto db = NLDB::create(NLUniverse::get());
  auto dbScriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  dbScriptPath /= "scripts";
  dbScriptPath /= "db_loader.py";
  SNLPyLoader::loadDB(db, dbScriptPath);

  ASSERT_EQ(2, db->getLibraries().size());
}

TEST_F(SNLPyDBLoaderTest0, testDBLoadingError) {
  auto db = NLDB::create(NLUniverse::get());
  auto dbScriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  dbScriptPath /= "scripts";
  dbScriptPath /= "db_faulty.py";
  EXPECT_THROW(SNLPyLoader::loadDB(db, dbScriptPath), NLException);
}
