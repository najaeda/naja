#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLPath.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLPathTest: public ::testing::Test {
  protected:
    void SetUp() override {
      // top
      //  |-> h0
      //       |-> h1
      //            |-> h2
      //                 |-> prim
      // 
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto primitivesLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
      auto designsLib = SNLLibrary::create(db);
      auto prim = SNLDesign::create(primitivesLib, SNLDesign::Type::Primitive, SNLName("PRIM"));
      auto top = SNLDesign::create(designsLib, SNLName("TOP"));
      auto h0 = SNLDesign::create(designsLib, SNLName("H0"));
      auto h1 = SNLDesign::create(designsLib, SNLName("H1"));
      auto h2 = SNLDesign::create(designsLib, SNLName("H2"));
      primInstance_ = SNLInstance::create(h2, prim, SNLName("prim"));
      h2Instance_ = SNLInstance::create(h1, h2, SNLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, SNLName("h1"));
      h0Instance_ = SNLInstance::create(top, h0, SNLName("h0"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }

    SNLInstance* primInstance_  {nullptr};
    SNLInstance* h2Instance_    {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h0Instance_    {nullptr};
};

TEST_F(SNLPathTest, testEmptyPath) {
  ASSERT_NE(SNLUniverse::get(), nullptr);
  auto emptyPath = SNLPath();
  EXPECT_TRUE(emptyPath.empty());
  EXPECT_EQ(0, emptyPath.size());
  EXPECT_EQ(nullptr, emptyPath.getHeadInstance());
  EXPECT_EQ(SNLPath(), emptyPath.getTailPath());
  EXPECT_EQ(SNLPath(), emptyPath.getHeadPath());
  EXPECT_EQ(nullptr, emptyPath.getTailInstance());
  EXPECT_EQ(nullptr, emptyPath.getDesign());
  EXPECT_EQ(nullptr, emptyPath.getModel());
  EXPECT_FALSE(emptyPath < emptyPath);
}

TEST_F(SNLPathTest, testTopDown0) {
  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_, SNLPath());
  EXPECT_FALSE(h0Path.empty());
  EXPECT_EQ(1, h0Path.size());
  EXPECT_EQ(h0Instance_, h0Path.getHeadInstance());
  EXPECT_EQ(h0Instance_, h0Path.getTailInstance());
  EXPECT_EQ(h0Instance_->getModel(), h0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(SNLPath(), h0Path.getHeadPath());
  EXPECT_EQ(SNLPath(), h0Path.getTailPath());
  EXPECT_LT(SNLPath(), h0Path);

  EXPECT_EQ(SNLPath(h0Instance_), h0Path);
  EXPECT_EQ(SNLPath(SNLPath(), h0Instance_), h0Path);
}

TEST_F(SNLPathTest, testTopDown1) {
  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_);
  EXPECT_FALSE(h0Path.empty());
  EXPECT_EQ(h0Instance_, h0Path.getHeadInstance());
  EXPECT_EQ(h0Instance_, h0Path.getTailInstance());
  EXPECT_EQ(h0Instance_->getModel(), h0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(SNLPath(), h0Path.getHeadPath());
  EXPECT_EQ(SNLPath(), h0Path.getTailPath());
  EXPECT_EQ(1, h0Path.size());
  EXPECT_LT(SNLPath(), h0Path);

  ASSERT_NE(h1Instance_, nullptr);
  EXPECT_EQ(h0Instance_->getModel(), h1Instance_->getDesign());
  auto h1Path = SNLPath(h0Path, h1Instance_);
  EXPECT_FALSE(h1Path.empty());
  EXPECT_EQ(2, h1Path.size());
  EXPECT_LT(SNLPath(), h1Path);
  EXPECT_LT(h0Path, h1Path);
  EXPECT_EQ(h0Instance_, h1Path.getHeadInstance());
  EXPECT_EQ(h1Instance_, h1Path.getTailInstance());
  EXPECT_EQ(h1Instance_->getModel(), h1Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(h0Path, h1Path.getHeadPath());
  EXPECT_EQ(SNLPath(h1Instance_), h1Path.getTailPath());

  ASSERT_NE(h2Instance_, nullptr);
  EXPECT_EQ(h1Instance_->getModel(), h2Instance_->getDesign());
  auto h2Path = SNLPath(h1Path, h2Instance_);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(3, h2Path.size());
  EXPECT_LT(SNLPath(), h2Path);
  EXPECT_LT(h0Path, h2Path);
  EXPECT_LT(h1Path, h2Path);
  EXPECT_EQ(h0Instance_, h2Path.getHeadInstance());
  EXPECT_EQ(h2Instance_, h2Path.getTailInstance());
  EXPECT_EQ(h2Instance_->getModel(), h2Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h2Path.getDesign());
  EXPECT_EQ(h1Path, h2Path.getHeadPath());
  EXPECT_EQ(SNLPath(SNLPath(h1Instance_), h2Instance_), h2Path.getTailPath());

  ASSERT_NE(primInstance_, nullptr);
  EXPECT_EQ(h2Instance_->getModel(), primInstance_->getDesign());
  auto primPath = SNLPath(h2Path, primInstance_);
  EXPECT_FALSE(primPath.empty());
  EXPECT_EQ(4, primPath.size());
  EXPECT_LT(SNLPath(), primPath);
  EXPECT_LT(h0Path, primPath);
  EXPECT_LT(h1Path, primPath);
  EXPECT_LT(h2Path, primPath);
  EXPECT_EQ(h0Instance_, primPath.getHeadInstance());
  EXPECT_EQ(primInstance_, primPath.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), primPath.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), primPath.getDesign());
  EXPECT_EQ(h2Path, primPath.getHeadPath());
  EXPECT_EQ(SNLPath(SNLPath(SNLPath(h1Instance_), h2Instance_), primInstance_), primPath.getTailPath());
}

TEST_F(SNLPathTest, testBottomUp0) {
  ASSERT_NE(primInstance_, nullptr);
  auto primPath = SNLPath(SNLPath(), primInstance_);
  EXPECT_FALSE(primPath.empty());
  EXPECT_EQ(primInstance_, primPath.getHeadInstance());
  EXPECT_EQ(primInstance_, primPath.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), primPath.getModel());
  EXPECT_EQ(primInstance_->getDesign(), primPath.getDesign());
  EXPECT_EQ(SNLPath(), primPath.getHeadPath());
  EXPECT_EQ(SNLPath(), primPath.getTailPath());

  EXPECT_EQ(SNLPath(primInstance_), primPath);
  EXPECT_EQ(SNLPath(primInstance_, SNLPath()), primPath);
}

TEST_F(SNLPathTest, testBottomUp1) {
  ASSERT_NE(primInstance_, nullptr);
  auto primPath = SNLPath(primInstance_);
  EXPECT_FALSE(primPath.empty());
  EXPECT_EQ(1, primPath.size());
  EXPECT_LT(SNLPath(), primPath);
  EXPECT_EQ(primInstance_, primPath.getHeadInstance());
  EXPECT_EQ(primInstance_, primPath.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), primPath.getModel());
  EXPECT_EQ(primInstance_->getDesign(), primPath.getDesign());
  EXPECT_EQ(SNLPath(), primPath.getHeadPath());
  EXPECT_EQ(SNLPath(), primPath.getTailPath());

  ASSERT_NE(h2Instance_, nullptr);
  auto h2Path = SNLPath(h2Instance_, primPath);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(2, h2Path.size());
  EXPECT_LT(SNLPath(), h2Path);
  EXPECT_LT(primPath, h2Path);
  EXPECT_EQ(h2Instance_, h2Path.getHeadInstance());
  EXPECT_EQ(primInstance_, h2Path.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), h2Path.getModel());
  EXPECT_EQ(h2Instance_->getDesign(), h2Path.getDesign());
  EXPECT_EQ(SNLPath(h2Instance_), h2Path.getHeadPath());
  EXPECT_EQ(primPath, h2Path.getTailPath());

  ASSERT_NE(h1Instance_, nullptr);
  auto h1Path = SNLPath(h1Instance_, h2Path);
  EXPECT_FALSE(h1Path.empty());
  EXPECT_EQ(3, h1Path.size());
  EXPECT_LT(SNLPath(), h1Path);
  EXPECT_LT(primPath, h1Path);
  EXPECT_LT(h2Path, h1Path);
  EXPECT_EQ(h1Instance_, h1Path.getHeadInstance());
  EXPECT_EQ(primInstance_, h1Path.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), h1Path.getModel());
  EXPECT_EQ(h1Instance_->getDesign(), h1Path.getDesign());
  EXPECT_EQ(SNLPath(h1Instance_, SNLPath(h2Instance_)), h1Path.getHeadPath());
  EXPECT_EQ(h2Path, h1Path.getTailPath());

  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_, h1Path);
  EXPECT_FALSE(h0Path.empty());
  EXPECT_EQ(4, h0Path.size());
  EXPECT_LT(SNLPath(), h0Path);
  EXPECT_LT(primPath, h0Path);
  EXPECT_LT(h2Path, h0Path);
  EXPECT_LT(h1Path, h0Path);
  EXPECT_EQ(h0Instance_, h0Path.getHeadInstance());
  EXPECT_EQ(primInstance_, h0Path.getTailInstance());
  EXPECT_EQ(primInstance_->getModel(), h0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(SNLPath(h0Instance_, SNLPath(h1Instance_, SNLPath(h2Instance_))), h0Path.getHeadPath());
  EXPECT_EQ(h1Path, h0Path.getTailPath());
  EXPECT_NE(h1Path, h0Path);
  SNLPath copyH1Path(h1Path);
  EXPECT_EQ(h1Path, copyH1Path);
  EXPECT_NE(h0Path, copyH1Path);
}

TEST_F(SNLPathTest, comparePaths) {
  EXPECT_EQ(SNLPath(SNLPath(), primInstance_), SNLPath(primInstance_, SNLPath()));
  EXPECT_EQ(
    SNLPath(SNLPath(h2Instance_), primInstance_),
    SNLPath(h2Instance_, SNLPath(primInstance_)));
  EXPECT_EQ(
    SNLPath(SNLPath(SNLPath(h1Instance_), h2Instance_), primInstance_),
    SNLPath(h1Instance_, SNLPath(h2Instance_, SNLPath(primInstance_))));
  EXPECT_EQ(
    SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), primInstance_),
    SNLPath(h0Instance_, SNLPath(h1Instance_, SNLPath(h2Instance_, SNLPath(primInstance_)))));
}

TEST_F(SNLPathTest, testErrors) {
  SNLInstance* instance = nullptr;
  SNLPath emptyPath;
  EXPECT_THROW(SNLPath(instance, emptyPath), SNLException);
  EXPECT_THROW(SNLPath(emptyPath, instance), SNLException);
  //incompatible paths
  ASSERT_NE(h0Instance_, nullptr);
  ASSERT_NE(primInstance_, nullptr);
  EXPECT_THROW(SNLPath(SNLPath(h0Instance_), primInstance_), SNLException);
  EXPECT_THROW(SNLPath(h0Instance_, SNLPath(primInstance_)), SNLException);
}

TEST_F(SNLPathTest, testInstanceDestroy) {
  {
    auto path = SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), primInstance_);
    EXPECT_FALSE(path.empty());
  }
  EXPECT_EQ(1, h1Instance_->getModel()->getInstances().size());
  h2Instance_->destroy();
  h2Instance_ = nullptr;
  EXPECT_TRUE(h1Instance_->getModel()->getInstances().empty());
  {
    auto path = SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_)));
    EXPECT_FALSE(path.empty());
  }
}