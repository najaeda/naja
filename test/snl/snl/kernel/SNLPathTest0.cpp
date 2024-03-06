// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLPath.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLPathTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      // top
      //  |-> h0
      //       |-> h1
      //            |-> h2
      //                 |-> prim0
      //                 |-> prim1
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
      prim0Instance_ = SNLInstance::create(h2, prim, SNLName("prim0"));
      prim1Instance_ = SNLInstance::create(h2, prim, SNLName("prim1"));
      h2Instance_ = SNLInstance::create(h1, h2, SNLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, SNLName("h1"));
      h0Instance_ = SNLInstance::create(top, h0, SNLName("h0"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }

    SNLInstance* prim0Instance_ {nullptr};
    SNLInstance* prim1Instance_ {nullptr};
    SNLInstance* h2Instance_    {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h0Instance_    {nullptr};
};

TEST_F(SNLPathTest0, testEmptyPath) {
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

TEST_F(SNLPathTest0, testTopDown0) {
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

TEST_F(SNLPathTest0, testTopDown1) {
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

  ASSERT_NE(prim0Instance_, nullptr);
  EXPECT_EQ(h2Instance_->getModel(), prim0Instance_->getDesign());
  auto prim0Path = SNLPath(h2Path, prim0Instance_);
  EXPECT_FALSE(prim0Path.empty());
  EXPECT_EQ(4, prim0Path.size());
  EXPECT_LT(SNLPath(), prim0Path);
  EXPECT_LT(h0Path, prim0Path);
  EXPECT_LT(h1Path, prim0Path);
  EXPECT_LT(h2Path, prim0Path);
  EXPECT_EQ(h0Instance_, prim0Path.getHeadInstance());
  EXPECT_EQ(prim0Instance_, prim0Path.getTailInstance());
  EXPECT_EQ(prim0Instance_->getModel(), prim0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), prim0Path.getDesign());
  EXPECT_EQ(h2Path, prim0Path.getHeadPath());
  EXPECT_EQ(SNLPath(SNLPath(SNLPath(h1Instance_), h2Instance_), prim0Instance_), prim0Path.getTailPath());
}

TEST_F(SNLPathTest0, testBottomUp0) {
  ASSERT_NE(prim0Instance_, nullptr);
  auto prim0Path = SNLPath(SNLPath(), prim0Instance_);
  EXPECT_FALSE(prim0Path.empty());
  EXPECT_EQ(prim0Instance_, prim0Path.getHeadInstance());
  EXPECT_EQ(prim0Instance_, prim0Path.getTailInstance());
  EXPECT_EQ(prim0Instance_->getModel(), prim0Path.getModel());
  EXPECT_EQ(prim0Instance_->getDesign(), prim0Path.getDesign());
  EXPECT_EQ(SNLPath(), prim0Path.getHeadPath());
  EXPECT_EQ(SNLPath(), prim0Path.getTailPath());

  EXPECT_EQ(SNLPath(prim0Instance_), prim0Path);
  EXPECT_EQ(SNLPath(prim0Instance_, SNLPath()), prim0Path);
}

TEST_F(SNLPathTest0, testBottomUp1) {
  ASSERT_NE(prim0Instance_, nullptr);
  auto prim0Path = SNLPath(prim0Instance_);
  EXPECT_FALSE(prim0Path.empty());
  EXPECT_EQ(1, prim0Path.size());
  EXPECT_LT(SNLPath(), prim0Path);
  EXPECT_EQ(prim0Instance_, prim0Path.getHeadInstance());
  EXPECT_EQ(prim0Instance_, prim0Path.getTailInstance());
  EXPECT_EQ(prim0Instance_->getModel(), prim0Path.getModel());
  EXPECT_EQ(prim0Instance_->getDesign(), prim0Path.getDesign());
  EXPECT_EQ(SNLPath(), prim0Path.getHeadPath());
  EXPECT_EQ(SNLPath(), prim0Path.getTailPath());

  ASSERT_NE(h2Instance_, nullptr);
  auto h2Path = SNLPath(h2Instance_, prim0Path);
  EXPECT_FALSE(h2Path.empty());
  EXPECT_EQ(2, h2Path.size());
  EXPECT_LT(SNLPath(), h2Path);
  EXPECT_LT(prim0Path, h2Path);
  EXPECT_EQ(h2Instance_, h2Path.getHeadInstance());
  EXPECT_EQ(prim0Instance_, h2Path.getTailInstance());
  EXPECT_EQ(prim0Instance_->getModel(), h2Path.getModel());
  EXPECT_EQ(h2Instance_->getDesign(), h2Path.getDesign());
  EXPECT_EQ(SNLPath(h2Instance_), h2Path.getHeadPath());
  EXPECT_EQ(prim0Path, h2Path.getTailPath());

  ASSERT_NE(h1Instance_, nullptr);
  auto h1Path = SNLPath(h1Instance_, h2Path);
  EXPECT_FALSE(h1Path.empty());
  EXPECT_EQ(3, h1Path.size());
  EXPECT_LT(SNLPath(), h1Path);
  EXPECT_LT(prim0Path, h1Path);
  EXPECT_LT(h2Path, h1Path);
  EXPECT_EQ(h1Instance_, h1Path.getHeadInstance());
  EXPECT_EQ(prim0Instance_, h1Path.getTailInstance());
  EXPECT_EQ(prim0Instance_->getModel(), h1Path.getModel());
  EXPECT_EQ(h1Instance_->getDesign(), h1Path.getDesign());
  EXPECT_EQ(SNLPath(h1Instance_, SNLPath(h2Instance_)), h1Path.getHeadPath());
  EXPECT_EQ(h2Path, h1Path.getTailPath());

  ASSERT_NE(h0Instance_, nullptr);
  auto h0Path = SNLPath(h0Instance_, h1Path);
  EXPECT_FALSE(h0Path.empty());
  EXPECT_EQ(4, h0Path.size());
  EXPECT_LT(SNLPath(), h0Path);
  EXPECT_LT(prim0Path, h0Path);
  EXPECT_LT(h2Path, h0Path);
  EXPECT_LT(h1Path, h0Path);
  EXPECT_FALSE(prim0Path < prim0Path);
  EXPECT_EQ(h0Instance_, h0Path.getHeadInstance());
  EXPECT_EQ(prim0Instance_, h0Path.getTailInstance());
  EXPECT_EQ(prim0Instance_->getModel(), h0Path.getModel());
  EXPECT_EQ(h0Instance_->getDesign(), h0Path.getDesign());
  EXPECT_EQ(SNLPath(h0Instance_, SNLPath(h1Instance_, SNLPath(h2Instance_))), h0Path.getHeadPath());
  EXPECT_EQ(h1Path, h0Path.getTailPath());
  EXPECT_NE(h1Path, h0Path);
  SNLPath copyH1Path(h1Path);
  EXPECT_EQ(h1Path, copyH1Path);
  EXPECT_NE(h0Path, copyH1Path);
}

TEST_F(SNLPathTest0, comparePaths) {
  EXPECT_EQ(SNLPath(SNLPath(), prim0Instance_), SNLPath(prim0Instance_, SNLPath()));
  EXPECT_EQ(
    SNLPath(SNLPath(h2Instance_), prim0Instance_),
    SNLPath(h2Instance_, SNLPath(prim0Instance_)));
  EXPECT_EQ(
    SNLPath(SNLPath(SNLPath(h1Instance_), h2Instance_), prim0Instance_),
    SNLPath(h1Instance_, SNLPath(h2Instance_, SNLPath(prim0Instance_))));
  EXPECT_EQ(
    SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), prim0Instance_),
    SNLPath(h0Instance_, SNLPath(h1Instance_, SNLPath(h2Instance_, SNLPath(prim0Instance_)))));
  EXPECT_LT(SNLPath(), SNLPath(prim0Instance_));
  EXPECT_FALSE(SNLPath(prim0Instance_) < SNLPath());
}

TEST_F(SNLPathTest0, testErrors) {
  SNLInstance* instance = nullptr;
  SNLPath emptyPath;
  EXPECT_THROW(SNLPath(instance, emptyPath), SNLException);
  EXPECT_THROW(SNLPath(emptyPath, instance), SNLException);
  //incompatible paths
  ASSERT_NE(h0Instance_, nullptr);
  ASSERT_NE(prim0Instance_, nullptr);
  EXPECT_THROW(SNLPath(SNLPath(h0Instance_), prim0Instance_), SNLException);
  EXPECT_THROW(SNLPath(h0Instance_, SNLPath(prim0Instance_)), SNLException);

  //Path Descriptor errors
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "", "prim"};
  EXPECT_THROW(SNLPath(h0Instance_->getDesign(), pathDescriptor0), SNLException);

  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h1", "h3", "prim"};
  EXPECT_THROW(SNLPath(h0Instance_->getDesign(), pathDescriptor1), SNLException);
}

TEST_F(SNLPathTest0, testInstanceDestroy0) {
  {
    auto path = SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), prim0Instance_);
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

TEST_F(SNLPathTest0, testInstanceDestroy1) {
  auto top = h0Instance_->getDesign();
  {
    auto path = SNLPath(SNLPath(SNLPath(SNLPath(h0Instance_), h1Instance_), h2Instance_), prim0Instance_);
    EXPECT_FALSE(path.empty());
  }
  h0Instance_->destroy();
  h0Instance_ = nullptr;
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim0"};
    EXPECT_THROW(SNLPath(top, pathDescriptor0), SNLException);

    SNLPath::PathStringDescriptor pathDescriptor1 = { "h1", "h2", "prim0"};
    auto path = SNLPath(h1Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(3, path.size());
  }
}

TEST_F(SNLPathTest0, testInstanceDestroy2) {
  auto top = h0Instance_->getDesign();
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim0"};
    auto path0 = SNLPath(top, pathDescriptor0);
    SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h1", "h2", "prim1"};
    auto path1 = SNLPath(top, pathDescriptor1);
    EXPECT_FALSE(path0.empty());
    EXPECT_FALSE(path1.empty());
    EXPECT_EQ(4, path0.size());
    EXPECT_EQ(4, path1.size());
  }
  h2Instance_->destroy();
  h2Instance_ = nullptr;
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim0" };
    EXPECT_THROW(SNLPath(top, pathDescriptor0), SNLException);

    SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h1" };
    auto path = SNLPath(top, pathDescriptor1);
    EXPECT_EQ(2, path.size());
  }
}
