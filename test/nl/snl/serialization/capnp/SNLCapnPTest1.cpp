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
#include "SNLDesignTruthTable.h"

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
      SNLDesign* design1 = SNLDesign::create(lib2, SNLDesign::Type::Blackbox);
      NLLibrary* primitives = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("PRIMITIVES"));
      NLLibrary* prims1 = NLLibrary::create(primitives, NLLibrary::Type::Primitives);
      NLLibrary* prims2 = NLLibrary::create(primitives, NLLibrary::Type::Primitives, NLName("prims2"));
      NLLibrary* prims3 = NLLibrary::create(prims1, NLLibrary::Type::Primitives, NLName("prims3"));
      SNLDesign* prim = SNLDesign::create(prims2, SNLDesign::Type::Primitive, NLName("prim"));
      SNLScalarTerm::create(prim, SNLTerm::Direction::Input, NLName("A"));
      SNLScalarTerm::create(prim, SNLTerm::Direction::Input, NLName("B"));
      SNLScalarTerm::create(prim, SNLTerm::Direction::Output, NLName("Y"));
      SNLDesignTruthTable::setTruthTable(prim, SNLTruthTable(2, 0x8));
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
  EXPECT_TRUE(library->getDesigns().empty());
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
  EXPECT_TRUE(lib2->isAnonymous());
  EXPECT_EQ(library, lib2->getParentLibrary());
  EXPECT_EQ(2, lib2->getLibraries().size());
  libraries = Libraries(lib2->getLibraries().begin(), lib2->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto lib3 = libraries[0];
  EXPECT_NE(nullptr, lib3);
  EXPECT_EQ(NLName("lib3"), lib3->getName());
  EXPECT_TRUE(lib3->getLibraries().empty());
  auto lib4 = libraries[1];
  EXPECT_NE(nullptr, lib4);
  EXPECT_TRUE(lib4->isAnonymous());
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
  EXPECT_TRUE(prims1->isAnonymous());
  EXPECT_TRUE(prims1->isPrimitives());
  EXPECT_TRUE(prims1->getDesigns().empty());
  EXPECT_EQ(1, prims1->getLibraries().size());
  auto prims2 = libraries[1];
  EXPECT_NE(nullptr, prims2);
  EXPECT_FALSE(prims2->isAnonymous());
  EXPECT_TRUE(prims2->isPrimitives());
  EXPECT_EQ(1, prims2->getDesigns().size());
  auto prim = *(prims2->getDesigns().begin());
  EXPECT_NE(nullptr, prim);
  EXPECT_TRUE(prim->isPrimitive());
  EXPECT_FALSE(prim->isAnonymous());
  EXPECT_EQ(NLName("prim"), prim->getName());
  auto primTT = SNLDesignTruthTable::getTruthTable(prim);
  EXPECT_TRUE(primTT.isInitialized());
  EXPECT_EQ(SNLTruthTable(2, 0x8), SNLDesignTruthTable::getTruthTable(prim));

  libraries = Libraries(prims1->getLibraries().begin(), prims1->getLibraries().end());
  EXPECT_EQ(1, libraries.size());
  auto prims3 = libraries[0];
  EXPECT_NE(nullptr, prims3);
  EXPECT_FALSE(prims3->isAnonymous());
  EXPECT_EQ(NLName("prims3"), prims3->getName());
}
