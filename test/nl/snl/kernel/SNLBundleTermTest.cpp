// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::ElementsAre;

#include "NLException.h"
#include "NLUniverse.h"

#include "SNLBundleTerm.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

using namespace naja::NL;

class SNLBundleTermTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("LIB"));
      primitives_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    NLLibrary* library_    {nullptr};
    NLLibrary* primitives_ {nullptr};
};

TEST_F(SNLBundleTermTest, testPrimitiveBundleInterface) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));

  auto ck = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto ctrl = SNLBusTerm::create(primitive, SNLTerm::Direction::Input, 1, 0, NLName("CTRL"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d0 = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));
  auto d1 = SNLBusTerm::create(bundle, SNLTerm::Direction::Input, 1, 0, NLName("D1"));
  auto qn = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("QN"));

  EXPECT_EQ(nullptr, ck->getBundleOwner());
  EXPECT_EQ(bundle, d0->getBundleOwner());
  EXPECT_EQ(bundle, d1->getBundleOwner());

  EXPECT_THAT(
    std::vector<SNLTerm*>(primitive->getTerms().begin(), primitive->getTerms().end()),
    ElementsAre(ck, ctrl, bundle, qn));
  EXPECT_THAT(
    std::vector<SNLScalarTerm*>(primitive->getScalarTerms().begin(), primitive->getScalarTerms().end()),
    ElementsAre(ck, qn));
  EXPECT_THAT(
    std::vector<SNLBusTerm*>(primitive->getBusTerms().begin(), primitive->getBusTerms().end()),
    ElementsAre(ctrl));
  EXPECT_THAT(
    std::vector<SNLBundleTerm*>(primitive->getBundleTerms().begin(), primitive->getBundleTerms().end()),
    ElementsAre(bundle));

  EXPECT_EQ(bundle, primitive->getBundleTerm(NLName("D")));
  EXPECT_EQ(bundle, primitive->getBundleTerm(bundle->getID()));
  EXPECT_EQ(d0, primitive->getTerm(NLName("D0")));
  EXPECT_EQ(d1, primitive->getTerm(NLName("D1")));
  EXPECT_EQ(d0, primitive->getScalarTerm(NLName("D0")));
  EXPECT_EQ(d1, primitive->getBusTerm(NLName("D1")));
  EXPECT_EQ(d0, primitive->getTerm(d0->getID()));
  EXPECT_EQ(d1, primitive->getTerm(d1->getID()));

  EXPECT_EQ(3, bundle->getWidth());
  EXPECT_EQ(2, bundle->getNumMembers());
  EXPECT_EQ(d0, bundle->getMember(0));
  EXPECT_EQ(d1, bundle->getMember(1));
  EXPECT_EQ(bundle->getFlatID(), d0->getFlatID());
  EXPECT_EQ(0u, ck->getFlatID());
  EXPECT_EQ(1u, ctrl->getFlatID());
  EXPECT_EQ(3u, bundle->getFlatID());
  EXPECT_EQ(4u, d1->getFlatID());
  EXPECT_EQ(6u, qn->getFlatID());

  EXPECT_THAT(
    std::vector<SNLBitTerm*>(bundle->getBits().begin(), bundle->getBits().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(d0),
      static_cast<SNLBitTerm*>(d1->getBit(1)),
      static_cast<SNLBitTerm*>(d1->getBit(0))));
  EXPECT_THAT(
    std::vector<SNLBitTerm*>(primitive->getBitTerms().begin(), primitive->getBitTerms().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(ck),
      static_cast<SNLBitTerm*>(ctrl->getBit(1)),
      static_cast<SNLBitTerm*>(ctrl->getBit(0)),
      static_cast<SNLBitTerm*>(d0),
      static_cast<SNLBitTerm*>(d1->getBit(1)),
      static_cast<SNLBitTerm*>(d1->getBit(0)),
      static_cast<SNLBitTerm*>(qn)));

  auto top = SNLDesign::create(library_, NLName("top"));
  auto instance = SNLInstance::create(top, primitive, NLName("u0"));

  EXPECT_EQ(7, instance->getInstTerms().size());
  EXPECT_THAT(
    std::vector<SNLInstTerm*>(instance->getInstTerms().begin(), instance->getInstTerms().end()),
    ElementsAre(
      instance->getInstTerm(ck),
      instance->getInstTerm(ctrl->getBit(1)),
      instance->getInstTerm(ctrl->getBit(0)),
      instance->getInstTerm(d0),
      instance->getInstTerm(d1->getBit(1)),
      instance->getInstTerm(d1->getBit(0)),
      instance->getInstTerm(qn)));

  auto ckNet = SNLScalarNet::create(top, NLName("n_ck"));
  auto d0Net = SNLScalarNet::create(top, NLName("n_d0"));
  auto d1Net = SNLBusNet::create(top, 1, 0, NLName("n_d1"));

  instance->setTermNet(ck, ckNet);
  instance->setTermNet(d0, d0Net);
  instance->setTermNet(d1, d1Net);

  EXPECT_THROW(
    primitive->cloneInterfaceToLibrary(primitives_, NLName("prim_clone")),
    NLException);
  EXPECT_THROW(
    primitive->cloneInterface(NLName("prim_clone_interface")),
    NLException);
  EXPECT_THROW(
    primitive->clone(NLName("prim_clone_full")),
    NLException);

  auto equivalent = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim_equiv"));
  auto equivalentCk = SNLScalarTerm::create(equivalent, SNLTerm::Direction::Input, NLName("CK"));
  auto equivalentCtrl = SNLBusTerm::create(equivalent, SNLTerm::Direction::Input, 1, 0, NLName("CTRL"));
  auto equivalentBundle = SNLBundleTerm::create(equivalent, SNLTerm::Direction::Input, NLName("D"));
  auto equivalentD0 = SNLScalarTerm::create(equivalentBundle, SNLTerm::Direction::Input, NLName("D0"));
  auto equivalentD1 = SNLBusTerm::create(equivalentBundle, SNLTerm::Direction::Input, 1, 0, NLName("D1"));
  auto equivalentQn = SNLScalarTerm::create(equivalent, SNLTerm::Direction::Output, NLName("QN"));
  ASSERT_NE(nullptr, equivalentCtrl);
  ASSERT_NE(nullptr, equivalentQn);

  std::string reason;
  EXPECT_TRUE(primitive->deepCompare(equivalent, reason, NLDesign::CompareType::IgnoreIDAndName)) << reason;

  instance->setModel(equivalent);
  EXPECT_EQ(equivalent, instance->getModel());
  EXPECT_EQ(static_cast<SNLBitNet*>(ckNet), instance->getInstTerm(equivalentCk)->getNet());
  EXPECT_EQ(static_cast<SNLBitNet*>(d0Net), instance->getInstTerm(equivalentD0)->getNet());
  EXPECT_EQ(static_cast<SNLBitNet*>(d1Net->getBit(1)), instance->getInstTerm(equivalentD1->getBit(1))->getNet());
  EXPECT_EQ(static_cast<SNLBitNet*>(d1Net->getBit(0)), instance->getInstTerm(equivalentD1->getBit(0))->getNet());
  EXPECT_EQ(7, instance->getInstTerms().size());
}

TEST_F(SNLBundleTermTest, testBundleRestrictions) {
  auto standard = SNLDesign::create(library_, NLName("std"));
  EXPECT_THROW(
    SNLBundleTerm::create(standard, SNLTerm::Direction::Input, NLName("D")),
    NLException);

  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto member = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));
  auto invalidNet = SNLScalarNet::create(standard, NLName("n"));

  EXPECT_THROW(
    SNLScalarTerm::create(bundle, SNLTerm::Direction::Output, NLName("BAD")),
    NLException);
  EXPECT_THROW(bundle->setNet(invalidNet), NLException);
  EXPECT_THROW(member->destroy(), NLException);
}

TEST_F(SNLBundleTermTest, testBundleCreateWithID) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto ck = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto bundle = SNLBundleTerm::create(
    primitive,
    NLID::DesignObjectID(4),
    SNLTerm::Direction::Input,
    NLName("D"));
  auto d0 = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));
  auto qn = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("QN"));

  ASSERT_NE(nullptr, ck);
  ASSERT_NE(nullptr, bundle);
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, qn);

  EXPECT_EQ(NLID::DesignObjectID(4), bundle->getID());
  EXPECT_EQ(bundle, primitive->getBundleTerm(NLID::DesignObjectID(4)));
  EXPECT_EQ(bundle, primitive->getBundleTerm(NLName("D")));
  EXPECT_EQ(bundle, primitive->getTerm(NLID::DesignObjectID(4)));
  EXPECT_EQ(bundle, primitive->getTerm(NLName("D")));
  EXPECT_EQ(bundle, d0->getBundleOwner());
  EXPECT_EQ(NLID::DesignObjectID(5), d0->getID());
  EXPECT_EQ(NLID::DesignObjectID(6), qn->getID());

  EXPECT_THAT(
    std::vector<SNLTerm*>(primitive->getTerms().begin(), primitive->getTerms().end()),
    ElementsAre(ck, bundle, qn));

  EXPECT_THROW(
    SNLBundleTerm::create(
      primitive,
      NLID::DesignObjectID(4),
      SNLTerm::Direction::Input,
      NLName("D_DUP_ID")),
    NLException);
  EXPECT_THROW(
    SNLBundleTerm::create(
      primitive,
      NLID::DesignObjectID(0),
      SNLTerm::Direction::Input,
      NLName("D_DUP_SCALAR_ID")),
    NLException);
  EXPECT_THROW(
    SNLBundleTerm::create(
      primitive,
      NLID::DesignObjectID(7),
      SNLTerm::Direction::Input,
      NLName("CK")),
    NLException);
}

TEST_F(SNLBundleTermTest, testBundledScalarCreateWithID) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto ck = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d0 = SNLScalarTerm::create(
    bundle,
    NLID::DesignObjectID(4),
    SNLTerm::Direction::Input,
    NLName("D0"));
  auto qn = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("QN"));

  ASSERT_NE(nullptr, ck);
  ASSERT_NE(nullptr, bundle);
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, qn);

  EXPECT_EQ(bundle, d0->getBundleOwner());
  EXPECT_EQ(NLID::DesignObjectID(4), d0->getID());
  EXPECT_EQ(d0, primitive->getScalarTerm(NLName("D0")));
  EXPECT_EQ(d0, primitive->getTerm(NLID::DesignObjectID(4)));
  EXPECT_EQ(d0, bundle->getMember(0));
  EXPECT_EQ(NLID::DesignObjectID(5), qn->getID());

  EXPECT_THROW(
    SNLScalarTerm::create(
      bundle,
      NLID::DesignObjectID(4),
      SNLTerm::Direction::Input,
      NLName("D1")),
    NLException);
  EXPECT_THROW(
    SNLScalarTerm::create(
      bundle,
      NLID::DesignObjectID(0),
      SNLTerm::Direction::Input,
      NLName("D1")),
    NLException);
  EXPECT_THROW(
    SNLScalarTerm::create(
      bundle,
      NLID::DesignObjectID(6),
      SNLTerm::Direction::Input,
      NLName("CK")),
    NLException);
  EXPECT_THROW(
    SNLScalarTerm::create(
      bundle,
      NLID::DesignObjectID(6),
      SNLTerm::Direction::Input,
      NLName("D1")),
    NLException);
}

TEST_F(SNLBundleTermTest, testBundledBusCreateWithID) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto ck = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d1 = SNLBusTerm::create(
    bundle,
    NLID::DesignObjectID(4),
    SNLTerm::Direction::Input,
    1,
    0,
    NLName("D1"));
  auto qn = SNLScalarTerm::create(primitive, SNLTerm::Direction::Output, NLName("QN"));

  ASSERT_NE(nullptr, ck);
  ASSERT_NE(nullptr, bundle);
  ASSERT_NE(nullptr, d1);
  ASSERT_NE(nullptr, qn);

  EXPECT_EQ(bundle, d1->getBundleOwner());
  EXPECT_EQ(NLID::DesignObjectID(4), d1->getID());
  EXPECT_EQ(2, d1->getWidth());
  EXPECT_EQ(1, d1->getMSB());
  EXPECT_EQ(0, d1->getLSB());
  EXPECT_EQ(d1, primitive->getBusTerm(NLName("D1")));
  EXPECT_EQ(d1, primitive->getTerm(NLID::DesignObjectID(4)));
  EXPECT_EQ(d1, bundle->getMember(0));
  EXPECT_EQ(NLID::DesignObjectID(5), qn->getID());
  EXPECT_THAT(
    std::vector<SNLBitTerm*>(bundle->getBits().begin(), bundle->getBits().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(d1->getBit(1)),
      static_cast<SNLBitTerm*>(d1->getBit(0))));

  EXPECT_THROW(
    SNLBusTerm::create(
      bundle,
      NLID::DesignObjectID(4),
      SNLTerm::Direction::Input,
      1,
      0,
      NLName("D2")),
    NLException);
  EXPECT_THROW(
    SNLBusTerm::create(
      bundle,
      NLID::DesignObjectID(0),
      SNLTerm::Direction::Input,
      1,
      0,
      NLName("D2")),
    NLException);
  EXPECT_THROW(
    SNLBusTerm::create(
      bundle,
      NLID::DesignObjectID(6),
      SNLTerm::Direction::Input,
      1,
      0,
      NLName("CK")),
    NLException);
  EXPECT_THROW(
    SNLBusTerm::create(
      bundle,
      NLID::DesignObjectID(6),
      SNLTerm::Direction::Input,
      1,
      0,
      NLName("D2")),
    NLException);
}

TEST_F(SNLBundleTermTest, testBundledBusBitDestroyRebuildsBundleBits) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d0 = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));
  auto d1 = SNLBusTerm::create(bundle, SNLTerm::Direction::Input, 3, 0, NLName("D1"));

  ASSERT_NE(nullptr, bundle);
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, d1);
  ASSERT_NE(nullptr, d1->getBit(2));

  EXPECT_THAT(
    std::vector<SNLBitTerm*>(bundle->getBits().begin(), bundle->getBits().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(d0),
      static_cast<SNLBitTerm*>(d1->getBit(3)),
      static_cast<SNLBitTerm*>(d1->getBit(2)),
      static_cast<SNLBitTerm*>(d1->getBit(1)),
      static_cast<SNLBitTerm*>(d1->getBit(0))));

  d1->getBit(2)->destroy();

  EXPECT_EQ(nullptr, d1->getBit(2));
  EXPECT_THAT(
    std::vector<SNLBitTerm*>(bundle->getBits().begin(), bundle->getBits().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(d0),
      static_cast<SNLBitTerm*>(d1->getBit(3)),
      static_cast<SNLBitTerm*>(d1->getBit(1)),
      static_cast<SNLBitTerm*>(d1->getBit(0))));
}

TEST_F(SNLBundleTermTest, testBundledBusResizeRebuildsBundleBits) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d0 = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));
  auto d1 = SNLBusTerm::create(bundle, SNLTerm::Direction::Input, 3, 0, NLName("D1"));

  ASSERT_NE(nullptr, bundle);
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, d1);

  d1->setMSB(2);
  EXPECT_EQ(2, d1->getMSB());
  EXPECT_EQ(0, d1->getLSB());
  EXPECT_EQ(4, bundle->getWidth());
  EXPECT_THAT(
    std::vector<SNLBitTerm*>(bundle->getBits().begin(), bundle->getBits().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(d0),
      static_cast<SNLBitTerm*>(d1->getBit(2)),
      static_cast<SNLBitTerm*>(d1->getBit(1)),
      static_cast<SNLBitTerm*>(d1->getBit(0))));

  d1->setLSB(1);
  EXPECT_EQ(2, d1->getMSB());
  EXPECT_EQ(1, d1->getLSB());
  EXPECT_EQ(3, bundle->getWidth());
  EXPECT_THAT(
    std::vector<SNLBitTerm*>(bundle->getBits().begin(), bundle->getBits().end()),
    ElementsAre(
      static_cast<SNLBitTerm*>(d0),
      static_cast<SNLBitTerm*>(d1->getBit(2)),
      static_cast<SNLBitTerm*>(d1->getBit(1))));
}

TEST_F(SNLBundleTermTest, testBundleRename) {
  auto primitive = SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName("prim"));
  auto ck = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d0 = SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));
  auto qn = SNLBundleTerm::create(primitive, SNLTerm::Direction::Output, NLName("QN"));

  ASSERT_NE(nullptr, ck);
  ASSERT_NE(nullptr, bundle);
  ASSERT_NE(nullptr, d0);
  ASSERT_NE(nullptr, qn);

  EXPECT_FALSE(bundle->isUnnamed());
  EXPECT_EQ(bundle, primitive->getBundleTerm(NLName("D")));
  EXPECT_EQ(bundle, primitive->getTerm(NLName("D")));

  bundle->setName(NLName());
  EXPECT_TRUE(bundle->isUnnamed());
  EXPECT_EQ(nullptr, primitive->getBundleTerm(NLName("D")));
  EXPECT_EQ(nullptr, primitive->getTerm(NLName("D")));

  bundle->setName(NLName("D"));
  EXPECT_FALSE(bundle->isUnnamed());
  EXPECT_EQ(bundle, primitive->getBundleTerm(NLName("D")));
  EXPECT_EQ(bundle, primitive->getTerm(NLName("D")));

  bundle->setName(NLName("D")); // nothing should happen
  EXPECT_EQ(bundle, primitive->getBundleTerm(NLName("D")));

  bundle->setName(NLName("DATA"));
  EXPECT_EQ(nullptr, primitive->getBundleTerm(NLName("D")));
  EXPECT_EQ(nullptr, primitive->getTerm(NLName("D")));
  EXPECT_EQ(bundle, primitive->getBundleTerm(NLName("DATA")));
  EXPECT_EQ(bundle, primitive->getTerm(NLName("DATA")));

  EXPECT_THROW(bundle->setName(NLName("CK")), NLException);
  EXPECT_THROW(bundle->setName(NLName("D0")), NLException);
  EXPECT_THROW(bundle->setName(NLName("QN")), NLException);
}
