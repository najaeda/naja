// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLDesignModeling.h"

#include "SNLCapnP.h"

using namespace naja::NL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapNpTest1: public ::testing::Test {
  //Test libraries
  protected:
    void SetUp() override {
      //
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* root1 = NLLibrary::create(db_, NLName("ROOT"));
      NLLibrary* lib1 = NLLibrary::create(root1, NLName("lib1")); 
      NLLibrary* lib2 = NLLibrary::create(root1);
      NLLibrary* lib3 = NLLibrary::create(lib2, NLName("lib3"));
      NLLibrary* lib4 = NLLibrary::create(lib2);
      SNLDesign* design0 = SNLDesign::create(lib2, NLName("design0"));
      SNLDesign* design1 = SNLDesign::create(lib2, SNLDesign::Type::UserBlackBox);
      SNLDesign* design2 = SNLDesign::create(lib2, SNLDesign::Type::AutoBlackBox);
      SNLScalarTerm::create(design2, SNLTerm::Direction::Undefined, NLName("I0"));
      SNLScalarTerm::create(design2, SNLTerm::Direction::Undefined, NLName("I1"));
      SNLScalarTerm::create(design2, SNLTerm::Direction::Undefined, NLName("O"));
      NLLibrary* primitives = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("PRIMITIVES"));
      NLLibrary* prims1 = NLLibrary::create(primitives, NLLibrary::Type::Primitives);
      NLLibrary* prims2 = NLLibrary::create(primitives, NLLibrary::Type::Primitives, NLName("prims2"));
      NLLibrary* prims3 = NLLibrary::create(prims1, NLLibrary::Type::Primitives, NLName("prims3"));
      SNLDesign* prim = SNLDesign::create(prims2, SNLDesign::Type::Primitive, NLName("prim"));
      SNLScalarTerm::create(prim, SNLTerm::Direction::Input, NLName("A"));
      SNLScalarTerm::create(prim, SNLTerm::Direction::Input, NLName("B"));
      SNLScalarTerm::create(prim, SNLTerm::Direction::Output, NLName("Y"));
      SNLDesignModeling::setTruthTable(prim, SNLTruthTable(2, 0x8));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
  protected:
    NLDB* db_;
};

TEST_F(SNLCapNpTest1, test0) {
  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapNpTest1_test0.snl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);
  NLUniverse::get()->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(nullptr, db_->getTopDesign());
  EXPECT_EQ(NLID::DBID(1), db_->getID());
  EXPECT_EQ(2, db_->getLibraries().size());
  using Libraries = std::vector<NLLibrary*>;
  Libraries libraries(db_->getLibraries().begin(), db_->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto library = libraries[0];
  ASSERT_TRUE(library);
  EXPECT_EQ(NLID::LibraryID(0), library->getID());
  EXPECT_EQ(NLName("ROOT"), library->getName());
  EXPECT_TRUE(library->isRoot());
  EXPECT_EQ(NLLibrary::Type::Standard, library->getType());
  EXPECT_TRUE(library->isStandard());
  EXPECT_TRUE(library->getSNLDesigns().empty());
  EXPECT_EQ(2, library->getLibraries().size());
  libraries = Libraries(library->getLibraries().begin(), library->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto lib1 = libraries[0];
  EXPECT_NE(nullptr, lib1);
  EXPECT_EQ(NLName("lib1"), lib1->getName());
  EXPECT_TRUE(library->isRoot());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(library, lib1->getParentLibrary());
  auto lib2 = libraries[1];
  EXPECT_NE(nullptr, lib2);
  EXPECT_TRUE(lib2->isUnnamed());
  EXPECT_EQ(library, lib2->getParentLibrary());
  EXPECT_EQ(2, lib2->getLibraries().size());
  auto design0 = lib2->getSNLDesign(NLName("design0"));
  using Designs = std::vector<SNLDesign*>;
  Designs designs(lib2->getSNLDesigns().begin(), lib2->getSNLDesigns().end());
  ASSERT_EQ(3, designs.size());
  auto design0Test = designs[0];
  EXPECT_EQ(design0, design0Test);
  auto design1 = designs[1];
  EXPECT_TRUE(design1->isUserBlackBox());
  EXPECT_TRUE(design1->isBlackBox());
  EXPECT_TRUE(design1->isUnnamed());
  auto design2 = designs[2];
  EXPECT_TRUE(design2->isAutoBlackBox());
  EXPECT_TRUE(design2->isBlackBox());
  EXPECT_TRUE(design2->isUnnamed());
  EXPECT_EQ(3, design2->getTerms().size());
  EXPECT_EQ(3, design2->getScalarTerms().size());
  EXPECT_TRUE(std::all_of(
    design2->getTerms().begin(),
    design2->getTerms().end(),
    [](const auto& term) {
        return term->getDirection() == SNLTerm::Direction::Undefined;
    }
  ));

  libraries = Libraries(lib2->getLibraries().begin(), lib2->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto lib3 = libraries[0];
  EXPECT_NE(nullptr, lib3);
  EXPECT_EQ(NLName("lib3"), lib3->getName());
  EXPECT_TRUE(lib3->getLibraries().empty());
  auto lib4 = libraries[1];
  EXPECT_NE(nullptr, lib4);
  EXPECT_TRUE(lib4->isUnnamed());
  EXPECT_TRUE(lib4->getLibraries().empty());

  libraries = Libraries(db_->getLibraries().begin(), db_->getLibraries().end());
  auto primitives = libraries[1];
  EXPECT_EQ(NLName("PRIMITIVES"), primitives->getName());
  EXPECT_TRUE(primitives->isRoot());
  EXPECT_TRUE(primitives->isPrimitives());
  EXPECT_FALSE(primitives->isStandard());
  EXPECT_FALSE(primitives->isInDB0());
  libraries = Libraries(primitives->getLibraries().begin(), primitives->getLibraries().end());
  EXPECT_EQ(2, libraries.size());
  auto prims1 = libraries[0];
  EXPECT_NE(nullptr, prims1);
  EXPECT_TRUE(prims1->isUnnamed());
  EXPECT_TRUE(prims1->isPrimitives());
  EXPECT_TRUE(prims1->getSNLDesigns().empty());
  EXPECT_EQ(1, prims1->getLibraries().size());
  auto prims2 = libraries[1];
  EXPECT_NE(nullptr, prims2);
  EXPECT_FALSE(prims2->isUnnamed());
  EXPECT_TRUE(prims2->isPrimitives());
  EXPECT_EQ(1, prims2->getSNLDesigns().size());
  auto prim = *(prims2->getSNLDesigns().begin());
  EXPECT_NE(nullptr, prim);
  EXPECT_TRUE(prim->isPrimitive());
  EXPECT_FALSE(prim->isUnnamed());
  EXPECT_EQ(NLName("prim"), prim->getName());
  auto primTT = SNLDesignModeling::getTruthTable(prim);
  EXPECT_TRUE(primTT.isInitialized());
  EXPECT_EQ(SNLTruthTable(2, 0x8), SNLDesignModeling::getTruthTable(prim));

  libraries = Libraries(prims1->getLibraries().begin(), prims1->getLibraries().end());
  EXPECT_EQ(1, libraries.size());
  auto prims3 = libraries[0];
  EXPECT_NE(nullptr, prims3);
  EXPECT_FALSE(prims3->isUnnamed());
  EXPECT_EQ(NLName("prims3"), prims3->getName());
}
