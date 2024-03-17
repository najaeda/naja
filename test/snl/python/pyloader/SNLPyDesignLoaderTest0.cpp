// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLPyLoader.h"
#include "SNLException.h"
using namespace naja::SNL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPyDesignLoaderTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      primitives_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("primitives"));
      auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, SNLName("primitive"));
      auto pi = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, SNLName("i"));
      auto po = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, SNLName("o"));
      auto designsLibrary = SNLLibrary::create(db, SNLName("designs"));
      design_ = SNLDesign::create(designsLibrary, SNLName("design"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* primitives_;
    SNLDesign*  design_;
};

TEST_F(SNLPyDesignLoaderTest0, test) {
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  scriptPath /= "scripts";
  scriptPath /= "design_loader.py";
  SNLPyLoader::loadDesign(design_, scriptPath);

  //ASSERT_EQ(2, db->getLibraries().size());
}

TEST_F(SNLPyDesignLoaderTest0, testDesignLoadingError) {
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  scriptPath /= "scripts";
  scriptPath /= "design_faulty.py";
  EXPECT_THROW(SNLPyLoader::loadDesign(design_, scriptPath), SNLException);
}

TEST_F(SNLPyDesignLoaderTest0, testPrimitiveLoadingError) {
  auto design = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, SNLName("top"));
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  scriptPath /= "scripts";
  scriptPath /= "design_loader.py";
  EXPECT_THROW(SNLPyLoader::loadDesign(design, scriptPath), SNLException);
}