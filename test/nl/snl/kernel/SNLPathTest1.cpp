// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLPath.h"
using namespace naja::NL;

class SNLPathTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      // top
      //  |-> h0
      //       |-> h1
      //       |    |-> h2
      //       |         |-> prim
      //       |-> h1'
      //            |-> h2
      //                 |-> prim
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto primitivesLib = NLLibrary::create(db, NLLibrary::Type::Primitives);
      auto designsLib = NLLibrary::create(db);
      auto prim = SNLDesign::create(primitivesLib, SNLDesign::Type::Primitive, NLName("PRIM"));
      auto top = SNLDesign::create(designsLib, NLName("TOP"));
      auto h0 = SNLDesign::create(designsLib, NLName("H0"));
      auto h1 = SNLDesign::create(designsLib, NLName("H1"));
      auto h2 = SNLDesign::create(designsLib, NLName("H2"));
      primInstance_ = SNLInstance::create(h2, prim, NLName("prim"));
      h2Instance_ = SNLInstance::create(h1, h2, NLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, NLName("h1"));
      h1pInstance_ = SNLInstance::create(h0, h1, NLName("h1p"));
      h0Instance_ = SNLInstance::create(top, h0, NLName("h0"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    SNLInstance* primInstance_  {nullptr};
    SNLInstance* h2Instance_    {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h1pInstance_   {nullptr};
    SNLInstance* h0Instance_    {nullptr};
};

TEST_F(SNLPathTest1, testCompare) {
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h1p", "h2", "prim"};

  auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
  auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);

  EXPECT_FALSE(path0.empty());
  EXPECT_FALSE(path1.empty());
  EXPECT_EQ(path0.getDesign(), path1.getDesign());
  EXPECT_EQ(path0.getModel(), path1.getModel());
  EXPECT_NE(path0, path1);
  EXPECT_LT(path0, path1);
}

TEST_F(SNLPathTest1, testDestroy0) {
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1", "h2", "prim"};
    SNLPath::PathStringDescriptor pathDescriptor1 = {"h0", "h1p", "h2", "prim"};

    auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
    auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(4, path0.size());
    EXPECT_EQ(4, path1.size());
  }

  //delete h2
  h2Instance_->destroy();

  //
  SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1"};
  auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
  EXPECT_EQ(2, path0.size());
}
 
TEST_F(SNLPathTest1, testDestroy1) {
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1", "h2", "prim"};
    SNLPath::PathStringDescriptor pathDescriptor1 = {"h0", "h1p", "h2", "prim"};

    auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
    auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(4, path0.size());
    EXPECT_EQ(4, path1.size());
  }

  //delete prim
  primInstance_->destroy();

  {
    SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1", "h2"};
    SNLPath::PathStringDescriptor pathDescriptor1 = {"h0", "h1p", "h2"};

    auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
    auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(3, path0.size());
    EXPECT_EQ(3, path1.size());
  }
}
