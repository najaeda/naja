#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLLibraryTest: public ::testing::Test {
  protected:
    void SetUp() override {
      universe_ = SNLUniverse::create();
    }
    void TearDown() override {
      universe_->destroy();
    }
    SNLUniverse*  universe_;
};

TEST_F(SNLLibraryTest, test0) {
  SNLDB* db = SNLDB::create(universe_);
  EXPECT_EQ(0, db->getLibraries().size());
  EXPECT_TRUE(db->getLibraries().empty());

  SNLLibrary* lib1 = db->getLibrary(SNLName("LIB1"));
  EXPECT_FALSE(lib1);
  lib1 = SNLLibrary::create(db, SNLName("LIB1"));
  ASSERT_TRUE(lib1);
  EXPECT_EQ(0, lib1->getID());
  EXPECT_EQ(SNLID(1, 0), lib1->getSNLID());
  SNLLibrary* testLib1 = db->getLibrary(SNLName("LIB1"));
  ASSERT_TRUE(testLib1);
  EXPECT_EQ(db->getLibrary(0), lib1);
  EXPECT_EQ(lib1, SNLUniverse::get()->getLibrary(1, 0));
  EXPECT_EQ(lib1, SNLUniverse::get()->getObject(SNLID(1, 0)));
  EXPECT_EQ(nullptr, db->getLibrary(1));
  EXPECT_EQ(nullptr, SNLUniverse::get()->getLibrary(1, 1));
  EXPECT_EQ(nullptr, SNLUniverse::get()->getObject(SNLID(1, 1)));
  EXPECT_EQ(nullptr, db->getLibrary(2));

  EXPECT_EQ(testLib1, lib1);
  EXPECT_EQ("LIB1", lib1->getName().getString());

  EXPECT_EQ(1, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_THAT(std::vector(db->getLibraries().begin(), db->getLibraries().end()), ElementsAre(lib1));
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getDesigns().size());
  EXPECT_TRUE(lib1->getDesigns().empty());
  EXPECT_EQ(nullptr, lib1->getLibrary(2));
  EXPECT_EQ(nullptr, lib1->getLibrary(SNLName("UNKNOWN")));
  EXPECT_EQ(nullptr, lib1->getLibrary(3));

  SNLLibrary* lib2 = SNLLibrary::create(db, SNLName("LIB2"));
  EXPECT_EQ(SNLID(1, 1), lib2->getSNLID());
  EXPECT_LT(lib1->getSNLID(), lib2->getSNLID());
  ASSERT_TRUE(lib2);
  EXPECT_EQ(1, lib2->getID());
  SNLLibrary* testLib2 = db->getLibrary(SNLName("LIB2"));
  ASSERT_TRUE(testLib2);
  EXPECT_EQ(testLib2, lib2);
  EXPECT_EQ("LIB2", lib2->getName().getString());
  EXPECT_EQ(lib2, SNLUniverse::get()->getLibrary(1, 1));
  EXPECT_EQ(lib2, SNLUniverse::get()->getObject(SNLID(1, 1)));

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_THAT(std::vector(db->getLibraries().begin(), db->getLibraries().end()),
    ElementsAre(lib1, lib2));
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getDesigns().size());
  EXPECT_TRUE(lib1->getDesigns().empty());
  EXPECT_EQ(0, lib2->getLibraries().size());
  EXPECT_TRUE(lib2->getLibraries().empty());
  EXPECT_EQ(0, lib2->getDesigns().size());
  EXPECT_TRUE(lib2->getDesigns().empty());

  SNLLibrary* lib3 = SNLLibrary::create(lib2, SNLName("LIB1"));
  ASSERT_TRUE(lib3);
  EXPECT_EQ(2, lib3->getID());
  EXPECT_LT(lib2->getSNLID(), lib3->getSNLID());
  SNLLibrary* testLib3 = lib2->getLibrary(SNLName("LIB1"));
  ASSERT_TRUE(testLib3);
  EXPECT_EQ(testLib3, lib3);
  EXPECT_EQ("LIB1", lib3->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(3, db->getGlobalLibraries().size());
  EXPECT_FALSE(db->getGlobalLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(1, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_THAT(std::vector(lib2->getLibraries().begin(), lib2->getLibraries().end()), ElementsAre(lib3));
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());
  EXPECT_EQ(0, lib3->getDesigns().size());
  EXPECT_TRUE(lib3->getDesigns().empty());

  SNLLibrary* lib4 = SNLLibrary::create(lib2, SNLName("LIB2"));
  ASSERT_TRUE(lib4);
  SNLLibrary* testLib4 = lib2->getLibrary(SNLName("LIB2"));
  ASSERT_TRUE(testLib4);
  EXPECT_EQ(testLib4, lib4);
  EXPECT_EQ("LIB2", lib4->getName().getString());

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(2, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_THAT(std::vector(lib2->getLibraries().begin(), lib2->getLibraries().end()), ElementsAre(lib3, lib4));
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());
  EXPECT_EQ(0, lib4->getLibraries().size());
  EXPECT_TRUE(lib4->getLibraries().empty());

  lib4->destroy();
  testLib4 = lib2->getLibrary(SNLName("LIB2"));
  EXPECT_FALSE(testLib4);

  EXPECT_EQ(2, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(1, lib2->getLibraries().size());
  EXPECT_FALSE(lib2->getLibraries().empty());
  EXPECT_THAT(std::vector(lib2->getLibraries().begin(), lib2->getLibraries().end()), ElementsAre(lib3));
  EXPECT_EQ(0, lib3->getLibraries().size());
  EXPECT_TRUE(lib3->getLibraries().empty());

  lib2->destroy();
  testLib2 = db->getLibrary(SNLName("LIB2"));
  EXPECT_FALSE(testLib2);
  EXPECT_THAT(std::vector(db->getLibraries().begin(), db->getLibraries().end()), ElementsAre(lib1));
  EXPECT_EQ(1, db->getLibraries().size());
  EXPECT_FALSE(db->getLibraries().empty());
  EXPECT_EQ(0, lib1->getLibraries().size());
  EXPECT_TRUE(lib1->getLibraries().empty());
  EXPECT_EQ(0, lib1->getDesigns().size());
  EXPECT_TRUE(lib1->getDesigns().empty());
}

TEST_F(SNLLibraryTest, test1) {
  SNLDB* db = SNLDB::create(universe_);
  ASSERT_TRUE(db);
  SNLLibrary* root = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  ASSERT_TRUE(root);
  EXPECT_TRUE(root->isAnonymous());
  EXPECT_EQ(0, root->getID());
  EXPECT_EQ(nullptr, root->getParentLibrary());

  SNLLibrary* primitives0 = SNLLibrary::create(root, SNLLibrary::Type::Primitives, SNLName("Primitives0"));
  EXPECT_FALSE(primitives0->isAnonymous());
  EXPECT_EQ(SNLName("Primitives0"), primitives0->getName());
  EXPECT_EQ(1, primitives0->getID());

  SNLLibrary* primitives1 = SNLLibrary::create(root, SNLLibrary::Type::Primitives);
  EXPECT_TRUE(primitives1->isAnonymous());
  EXPECT_EQ(2, primitives1->getID());

  EXPECT_EQ(primitives0, root->getLibrary(SNLName("Primitives0")));
  EXPECT_EQ(primitives0, root->getLibrary(1));
  EXPECT_EQ(primitives1, root->getLibrary(2));

  EXPECT_FALSE(root->getLibrary(3));
  EXPECT_FALSE(root->getLibrary(SNLName("UNKNOWN")));

  EXPECT_EQ(primitives0, universe_->getLibrary(1, 1));
  EXPECT_EQ(primitives1, universe_->getLibrary(1, 2));
  EXPECT_EQ(nullptr, universe_->getLibrary(1, 5));
}

TEST_F(SNLLibraryTest, testDesignSearch) {
  SNLDB* db = SNLDB::create(universe_);
  EXPECT_NE(nullptr, db);
  SNLLibrary* root = SNLLibrary::create(db);
  EXPECT_NE(nullptr, root);
  SNLDesign* design = SNLDesign::create(root, SNLName("DESIGN"));
  EXPECT_NE(nullptr, design);
  EXPECT_EQ(design, root->getDesign(SNLName("DESIGN")));
  EXPECT_EQ(design, root->getDesign(0));
  EXPECT_EQ(nullptr, root->getDesign(SNLName("UNKNOWN")));
  EXPECT_EQ(nullptr, root->getDesign(1));
}

TEST_F(SNLLibraryTest, testErrors) {
  SNLDB* db = SNLDB::create(universe_);
  ASSERT_TRUE(db);
  SNLLibrary* root = SNLLibrary::create(db);
  ASSERT_TRUE(root);
  EXPECT_TRUE(root->isAnonymous());
  EXPECT_EQ(SNLID::LibraryID(0), root->getID());

  SNLDB* nullDB = nullptr;
  EXPECT_THROW(SNLLibrary::create(nullDB), SNLException);
  SNLLibrary* nullLibrary = nullptr;
  EXPECT_THROW(SNLLibrary::create(nullLibrary), SNLException);

  SNLLibrary* prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  //library and parent library must share same type
  EXPECT_THROW(SNLLibrary::create(prims, SNLName("SUB_PRIMS")), SNLException);

  //name collision error
  EXPECT_THROW(SNLLibrary::create(db, SNLName("PRIMS")), SNLException);

  auto subPrims = SNLLibrary::create(prims, SNLLibrary::Type::Primitives, SNLName("SUB_PRIMS"));
  ASSERT_NE(nullptr, subPrims);
  //name collision
  EXPECT_THROW(SNLLibrary::create(prims, SNLLibrary::Type::Primitives, SNLName("SUB_PRIMS")), SNLException);
  EXPECT_EQ(prims, subPrims->getParentLibrary());

  EXPECT_EQ(SNLID::LibraryID(0), root->getID());
  //ID collision
  EXPECT_THROW(SNLLibrary::create(db, SNLID::LibraryID(0), SNLLibrary::Type::Standard), SNLException);

  EXPECT_EQ(SNLID::LibraryID(2), subPrims->getID());
  EXPECT_EQ(prims, subPrims->getParentLibrary());
  EXPECT_EQ(subPrims, prims->getLibrary(SNLID::LibraryID(2)));
  EXPECT_EQ(nullptr, prims->getLibrary(SNLID::LibraryID(3)));
  //ID collision
  EXPECT_THROW(SNLLibrary::create(prims, SNLID::LibraryID(2), SNLLibrary::Type::Primitives), SNLException);
}