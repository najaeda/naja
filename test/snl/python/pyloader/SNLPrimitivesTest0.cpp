// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLDesignModeling.h"
#include "SNLPyLoader.h"
#include "SNLException.h"
using namespace naja::SNL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPrimitivesTest0: public ::testing::Test {
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

TEST_F(SNLPrimitivesTest0, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "primitives0.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  EXPECT_EQ("PRIMITIVES0", library->getName().getString());
  ASSERT_EQ(1, library->getDesigns().size());
  auto lut4 = library->getDesign(SNLName("LUT4")); 
  ASSERT_NE(nullptr, lut4);
  EXPECT_TRUE(lut4->isPrimitive());
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
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(terms[4]).empty());
  EXPECT_EQ(4, SNLDesignModeling::getCombinatorialInputs(terms[4]).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(terms[4]).begin(),
      SNLDesignModeling::getCombinatorialInputs(terms[4]).end()),
    ElementsAre(terms[0], terms[1], terms[2], terms[3]));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(terms[0]).empty());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(terms[0]).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(terms[0]).begin(),
      SNLDesignModeling::getCombinatorialOutputs(terms[0]).end()),
    ElementsAre(terms[4]));
}

TEST_F(SNLPrimitivesTest0, testError0) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "wrong_path.py";
  //Wrong path
  EXPECT_THROW(SNLPyLoader::loadPrimitives(library, primitives0Path), SNLException);
  
  library->destroy();
  //non Primitives library
  library = SNLLibrary::create(db, SNLName("PRIMS"));
  EXPECT_THROW(SNLPyLoader::loadPrimitives(library, primitives0Path), SNLException);
}

TEST_F(SNLPrimitivesTest0, testNonPrimitivesError) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  //non python script
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "primitives0.py";
  EXPECT_THROW(SNLPyLoader::loadLibrary(library, primitives0Path, false), SNLException);
}

TEST_F(SNLPrimitivesTest0, testWrongSyntax) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  //non python script
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "wrong_syntax.py";
  EXPECT_THROW(SNLPyLoader::loadPrimitives(library, primitives0Path), SNLException);
}

TEST_F(SNLPrimitivesTest0, testFaultyPythonScript) {
  //faulty python script
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "faulty_script.py";
  EXPECT_THROW(SNLPyLoader::loadPrimitives(library, primitives0Path), SNLException);
}