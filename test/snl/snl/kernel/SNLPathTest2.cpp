// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLPath.h"
#include "SNLException.h"

#include "Utils.h"

using namespace naja::SNL;

class SNLPathTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      // top
      //  |-> h0
      //       |-> h1
      //       |    |-> h2
      //       |         |-> prim
      //       |-> h3
      //            |-> h2
      //                 |-> prim
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto primitivesLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
      auto designsLib = SNLLibrary::create(db);
      auto prim = SNLDesign::create(primitivesLib, SNLDesign::Type::Primitive, SNLName("PRIM"));
      auto top = SNLDesign::create(designsLib, SNLName("TOP"));
      auto h0 = SNLDesign::create(designsLib, SNLName("H0"));
      auto h1 = SNLDesign::create(designsLib, SNLName("H1"));
      auto h2 = SNLDesign::create(designsLib, SNLName("H2"));
      auto h3 = SNLDesign::create(designsLib, SNLName("H3"));
      primInstance_ = SNLInstance::create(h2, prim, SNLName("prim"));
      h1h2Instance_ = SNLInstance::create(h1, h2, SNLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, SNLName("h1"));
      h3Instance_ = SNLInstance::create(h0, h3, SNLName("h3"));
      h3h2Instance_ = SNLInstance::create(h3, h2, SNLName("h2")); 
      h0Instance_ = SNLInstance::create(top, h0, SNLName("h0"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }

    SNLInstance* primInstance_  {nullptr};
    SNLInstance* h1h2Instance_  {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h3Instance_    {nullptr};
    SNLInstance* h3h2Instance_  {nullptr};
    SNLInstance* h0Instance_    {nullptr};
};

TEST_F(SNLPathTest2, testCompare) {
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h3", "h2", "prim"};

  auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
  auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);

  EXPECT_FALSE(path0.empty());
  EXPECT_FALSE(path1.empty());
  EXPECT_EQ(path0.getDesign(), path1.getDesign());
  EXPECT_EQ(path0.getModel(), path1.getModel());
  EXPECT_NE(path0, path1);
  EXPECT_LT(path0, path1);

  naja::BNE::SNLUniquifier uniquifier0(path0);
  naja::BNE::SNLUniquifier uniquifier1(path1);

  //Test Uniquifier comparators
  EXPECT_EQ(uniquifier0 == uniquifier0, true);
  EXPECT_EQ(uniquifier0 != uniquifier0, false);
  EXPECT_EQ(uniquifier0 < uniquifier0, false);
  EXPECT_EQ(uniquifier0 > uniquifier0, false);
  EXPECT_EQ(uniquifier0 <= uniquifier0, true);
  EXPECT_EQ(uniquifier0 >= uniquifier0, true);
  EXPECT_EQ(uniquifier0 == uniquifier1, false);
  EXPECT_EQ(uniquifier0 != uniquifier1, true);
  EXPECT_EQ(uniquifier0 < uniquifier1, true);
  EXPECT_EQ(uniquifier0 > uniquifier1, false);
  EXPECT_EQ(uniquifier0 <= uniquifier1, true);
  EXPECT_EQ(uniquifier0 >= uniquifier1, false);

  EXPECT_NE(uniquifier0.getString(), uniquifier1.getString());
}

TEST_F(SNLPathTest2, testDestroy0) {
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1", "h2", "prim"};
    SNLPath::PathStringDescriptor pathDescriptor1 = {"h0", "h3", "h2", "prim"};

    auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
    auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(4, path0.size());
    EXPECT_EQ(4, path1.size());
  }

  //delete h1/h2
  h1h2Instance_->destroy();

  //
  SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1"};
  auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
  EXPECT_EQ(2, path0.size());
}
 
TEST_F(SNLPathTest2, testDestroy1) {
  {
    SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1", "h2", "prim"};
    SNLPath::PathStringDescriptor pathDescriptor1 = {"h0", "h3", "h2", "prim"};

    auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
    auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(4, path0.size());
    EXPECT_EQ(4, path1.size());
  }

  //delete prim
  primInstance_->destroy();

  {
    SNLPath::PathStringDescriptor pathDescriptor0 = {"h0", "h1", "h2"};
    SNLPath::PathStringDescriptor pathDescriptor1 = {"h0", "h3", "h2"};

    auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
    auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);
    EXPECT_EQ(3, path0.size());
    EXPECT_EQ(3, path1.size());
  }
}