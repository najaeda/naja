// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <Python.h>

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLPyLoader.h"
using namespace naja::NL;

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

TEST_F(SNLPyDBLoaderTest0, testWithoutImportedNajaModule) {
  auto db = NLDB::create(NLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH) /
      "scripts" / "db_without_naja_module.py";
  EXPECT_NO_THROW(SNLPyLoader::loadDB(db, scriptPath));
  ASSERT_EQ(0, PyRun_SimpleString(
      "import sys, builtins\n"
      "sys.modules['naja'] = builtins._saved_naja_module\n"
      "del builtins._saved_naja_module\n"));
}

TEST_F(SNLPyDBLoaderTest0, testWithoutLoggingInstaller) {
  auto db = NLDB::create(NLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH) /
      "scripts" / "db_without_logging_installer.py";
  EXPECT_NO_THROW(SNLPyLoader::loadDB(db, scriptPath));
}

TEST_F(SNLPyDBLoaderTest0, testFailingLoggingInstaller) {
  auto db = NLDB::create(NLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH) /
      "scripts" / "db_failing_logging_installer.py";
  try {
    SNLPyLoader::loadDB(db, scriptPath);
    FAIL() << "Expected logging handler installation to fail";
  } catch (const NLException& exception) {
    EXPECT_NE(std::string(exception.what()).find(
        "Cannot install Naja Python logging handler: RuntimeError: logging failure"),
        std::string::npos);
  }
}
