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

class SNLPyDesignLoaderTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      primitives_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
      auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("primitive"));
      auto pi = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("i"));
      auto po = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("o"));
      auto designsLibrary = NLLibrary::create(db, NLName("designs"));
      design_ = SNLDesign::create(designsLibrary, NLName("design"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    NLLibrary* primitives_;
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
  EXPECT_THROW(SNLPyLoader::loadDesign(design_, scriptPath), NLException);
}

TEST_F(SNLPyDesignLoaderTest0, testPrimitiveLoadingError) {
  auto design = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("top"));
  auto scriptPath = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  scriptPath /= "scripts";
  scriptPath /= "design_loader.py";
  EXPECT_THROW(SNLPyLoader::loadDesign(design, scriptPath), NLException);
}
