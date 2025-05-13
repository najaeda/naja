// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using namespace std;

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLException.h"
#include "PNLDesignObject.h"

using namespace naja::NL;

class PNLInstanceTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      db_ = NLDB::create(universe);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB* db_;
};

TEST_F(PNLInstanceTest0, testCreation) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  EXPECT_EQ(NLName("design"), design->getName());
  EXPECT_EQ(NLName("model"), model->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_EQ(1, model->getID());

  auto instance = PNLInstance::create(design, model, NLName("instance"));
  EXPECT_EQ(NLName("instance"), instance->getName());
  EXPECT_EQ(0, instance->getID());
  EXPECT_EQ(design, instance->getDesign());
  EXPECT_EQ(model, instance->getModel());
  EXPECT_EQ(instance, design->getInstance(NLName("instance")));
  EXPECT_EQ(nullptr, design->getInstance(NLName("instance3")));
  EXPECT_EQ(instance, design->getInstance(0));
  EXPECT_EQ(nullptr, design->getInstance(1));
  auto instance2 = PNLInstance::create(design, model, NLName("instance2"));
  instance->destroy();
  instance2->destroy();
  EXPECT_THROW(PNLInstance::create(nullptr, model), NLException);
  EXPECT_THROW(PNLInstance::create(design, nullptr), NLException);
  auto ins = PNLInstance::create(design, model, NLName("name"));
  EXPECT_THROW(PNLInstance::create(design, model, NLName("name")), NLException);
  ins->destroy();
}

TEST_F(PNLInstanceTest0, testInstTermRenameError) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  auto a = PNLScalarTerm::create(model, PNLTerm::Direction::Input, NLName("a"));
  auto b = PNLScalarTerm::create(model, PNLTerm::Direction::Input, NLName("b"));
  auto c = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("c"));
  auto ins = PNLInstance::create(design, model, NLName("instance"));
  design->rename(ins, NLName("instance"));
  auto instTerm = ins->getInstTerm(a);
  EXPECT_EQ(instTerm->getNet(), nullptr);
  EXPECT_EQ(ins->getInstTerm(b), ins->getInstTerm(1));
  a->destroy();
  //model->removeTerm(a);
  EXPECT_THROW(ins->getInstTerm(c), NLException);
  EXPECT_THROW(ins->getInstTerm(b)->destroy(), NLException);  
  // Test getDesign for inst term
  EXPECT_EQ(design, ins->getInstTerm(b)->getDesign());
  // Test getDirection for inst term
  EXPECT_EQ(PNLTerm::Direction::Input, ins->getInstTerm(b)->getDirection());
  // Test isAnno for inst term
  EXPECT_FALSE(ins->getInstTerm(b)->isAnonymous());
  EXPECT_EQ(b->getBit(), 0);
  EXPECT_EQ(b->getWidth(), 1);
  EXPECT_EQ(NLID::DesignObjectReference(1, 0, 1, 1), b->getReference());
  auto d = PNLScalarTerm::create(model, PNLTerm::Direction::Input, NLName("d"));
  // Test getNLID for inst term
  EXPECT_EQ(naja::NL::NLID(NLID::Type::InstTerm, ins->getInstTerm(b)->getDB()->getID(), ins->getInstTerm(b)->getLibrary()->getID(),
               ins->getInstTerm(b)->getDesign()->getID(), ins->getInstTerm(b)->getBitTerm()->getID(), 
               ins->getID(), ins->getInstTerm(b)->getBitTerm()->getBit()), ins->getInstTerm(b)->getNLID());
}

TEST_F(PNLInstanceTest0, testInstTermNullTerm) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  auto ins = PNLInstance::create(design, model, NLName("instance"));
  auto anonym = PNLInstance::create(design, model);
  EXPECT_EQ(true, anonym->isAnonymous());
  EXPECT_THROW(ins->getInstTerm(nullptr), NLException);
}

TEST_F(PNLInstanceTest0, testModelDestroy) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model0 = PNLDesign::create(library, NLName("model0"));
  PNLDesign* model1 = PNLDesign::create(library, NLName("model1"));

  EXPECT_FALSE(library->getPNLDesigns().empty());
  EXPECT_EQ(3, library->getPNLDesigns().size());

  EXPECT_TRUE(design->getInstances().empty());
  EXPECT_TRUE(design->getPrimitiveInstances().empty());
  EXPECT_TRUE(design->getNonPrimitiveInstances().empty());
  EXPECT_TRUE(model0->getSlaveInstances().empty());
  EXPECT_TRUE(model1->getSlaveInstances().empty());

  for (int i=0; i<4; ++i) {
    auto inst0 = PNLInstance::create(design, model0);
    auto inst1 = PNLInstance::create(design, model1);
  }

  EXPECT_FALSE(design->getInstances().empty());
  EXPECT_FALSE(model0->getSlaveInstances().empty());
  EXPECT_FALSE(model1->getSlaveInstances().empty());
  EXPECT_EQ(8, design->getInstances().size());
  EXPECT_EQ(4, model0->getSlaveInstances().size());
  EXPECT_EQ(4, model1->getSlaveInstances().size());

  model0->destroy();
  EXPECT_EQ(2, library->getPNLDesigns().size());
  EXPECT_EQ(4, design->getInstances().size());
  EXPECT_EQ(4, model1->getSlaveInstances().size());

  model1->destroy();
  EXPECT_EQ(1, library->getPNLDesigns().size());
  EXPECT_EQ(0, design->getInstances().size());
  EXPECT_TRUE(design->getInstances().empty());
}

TEST_F(PNLInstanceTest0, testTransform) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  auto ins = PNLInstance::create(design, model, NLName("instance"));
  EXPECT_EQ(ins->getTransform().getOffset().getX(), 0);
  EXPECT_EQ(ins->getTransform().getOffset().getY(), 0);
  EXPECT_EQ(ins->getTransform().getOrientation().getType(), PNLOrientation::Type(PNLOrientation::Type::R0));
  auto transform = PNLTransform(PNLPoint(1, 2), PNLOrientation::Type(PNLOrientation::Type::R90));
  ins->setTransform(transform);
  EXPECT_EQ(ins->getTransform().getOffset().getX(), 1);
  EXPECT_EQ(ins->getTransform().getOffset().getY(), 2);
  EXPECT_EQ(ins->getTransform().getOrientation().getType(), PNLOrientation::Type(PNLOrientation::Type::R90));
}

TEST_F(PNLInstanceTest0, testgetInstTerm) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  auto a = PNLScalarTerm::create(model, PNLTerm::Direction::Input, NLName("a"));
  auto b = PNLScalarTerm::create(model, PNLTerm::Direction::Input, NLName("b"));
  auto c = PNLScalarTerm::create(design, PNLTerm::Direction::Input, NLName("c"));
  auto ins = PNLInstance::create(design, model, NLName("instance"));
  EXPECT_EQ(ins->getInstTerm(NLName("a")), ins->getInstTerm(a));
  EXPECT_EQ(ins->getInstTerm(NLName("b")), ins->getInstTerm(b));
  EXPECT_THROW(ins->getInstTerm(NLName("c")), NLException);
}