#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"

using namespace naja::SNL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapNpTest1: public ::testing::Test {
  //Test libraries
  protected:
    void SetUp() override {
      //
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* root1 = SNLLibrary::create(db_, SNLName("ROOT"));
      SNLLibrary* lib1 = SNLLibrary::create(root1, SNLName("lib1")); 
      SNLLibrary* lib2 = SNLLibrary::create(root1);
      SNLLibrary* lib3 = SNLLibrary::create(lib2, SNLName("lib3"));
      SNLLibrary* lib4 = SNLLibrary::create(lib2);
      SNLLibrary* primitives = SNLLibrary::create(db_, SNLLibrary::Type::Primitives, SNLName("PRIMITIVES"));
      SNLLibrary* prims1 = SNLLibrary::create(primitives, SNLLibrary::Type::Primitives);
      SNLLibrary* prims2 = SNLLibrary::create(primitives, SNLLibrary::Type::Primitives, SNLName("prims2"));
      SNLLibrary* prims3 = SNLLibrary::create(prims1, SNLLibrary::Type::Primitives, SNLName("prims3"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
  protected:
    SNLDB*      db_;
};

TEST_F(SNLCapNpTest1, test0) {
  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapNpTest1_test0.snl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);
  db_->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(SNLID::DBID(1), db_->getID());
  EXPECT_EQ(2, db_->getLibraries().size());
  using Libraries = std::vector<SNLLibrary*>;
  Libraries libraries(db_->getLibraries().begin(), db_->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto library = libraries[0];
  ASSERT_TRUE(library);
  EXPECT_EQ(SNLID::LibraryID(0), library->getID());
  EXPECT_EQ(SNLName("ROOT"), library->getName());
  EXPECT_TRUE(library->isRoot());
  EXPECT_EQ(SNLLibrary::Type::Standard, library->getType());
  EXPECT_TRUE(library->isStandard());
  EXPECT_TRUE(library->getDesigns().empty());
  EXPECT_EQ(2, library->getLibraries().size());
  libraries = Libraries(library->getLibraries().begin(), library->getLibraries().end());
  ASSERT_EQ(2, libraries.size());
  auto lib1 = libraries[0];
  EXPECT_NE(nullptr, lib1);
  EXPECT_EQ(SNLName("lib1"), lib1->getName());
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
  EXPECT_EQ(SNLName("lib3"), lib3->getName());
  EXPECT_TRUE(lib3->getLibraries().empty());
  auto lib4 = libraries[1];
  EXPECT_NE(nullptr, lib4);
  EXPECT_TRUE(lib4->isAnonymous());
  EXPECT_TRUE(lib4->getLibraries().empty());

  libraries = Libraries(db_->getLibraries().begin(), db_->getLibraries().end());
  auto primitives = libraries[1];
  EXPECT_EQ(SNLName("PRIMITIVES"), primitives->getName());
  EXPECT_TRUE(primitives->isRoot());
  EXPECT_TRUE(primitives->isPrimitives());
  EXPECT_FALSE(primitives->isStandard());
  EXPECT_FALSE(primitives->isInDB0());
}