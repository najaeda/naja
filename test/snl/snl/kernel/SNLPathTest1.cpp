#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLPath.h"
#include "SNLException.h"
using namespace naja::SNL;

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
      h1pInstance_ = SNLInstance::create(h0, h1, SNLName("h1p"));
      h0Instance_ = SNLInstance::create(top, h0, SNLName("h0"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }

    SNLInstance* primInstance_  {nullptr};
    SNLInstance* h2Instance_    {nullptr};
    SNLInstance* h1Instance_    {nullptr};
    SNLInstance* h1pInstance_    {nullptr};
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