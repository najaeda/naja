// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLException.h"

#include "SNLPyEdit.h"
using namespace naja::SNL;

#ifndef SNL_PYEDIT_TEST_PATH
#define SNL_PYEDIT_TEST_PATH "Undefined"
#endif

class SNLPyDBEditTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto primitivesLibrary = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
      auto prim0 = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive, NLName("prim0"));
      auto designsLibrary = NLLibrary::create(db, NLName("designs"));
      auto top = SNLDesign::create(designsLibrary, NLName("top"));
      auto inst0 = SNLInstance::create(top, prim0, NLName("instance0"));
      auto bbox = SNLDesign::create(designsLibrary, NLName("bbox"));
      bbox->setType(SNLDesign::Type::Blackbox);
      auto inst1 = SNLInstance::create(top, bbox, NLName("instance1"));
      auto inst2 = SNLInstance::create(top, NLDB0::getAssign(), NLName("instance2"));
      universe->setTopDesign(top);
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPyDBEditTest0, test) {
  auto universe = NLUniverse::get();
  ASSERT_TRUE(universe);
  auto top = universe->getTopDesign();
  ASSERT_TRUE(top);
  auto instance0 = top->getInstance(NLName("instance0"));
  ASSERT_TRUE(instance0);
  EXPECT_EQ(NLName("instance0"), instance0->getName());
  auto instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  EXPECT_EQ(NLName("instance1"), instance1->getName());
  auto instance2 = top->getInstance(NLName("instance2"));
  ASSERT_TRUE(instance2);
  EXPECT_EQ(NLName("instance2"), instance2->getName());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "edit";
  scriptPath /= "edit_test0.py";
  SNLPyEdit::edit(scriptPath);
  EXPECT_EQ(NLName("instance00"), instance0->getName());
  EXPECT_EQ(nullptr, top->getInstance(NLName("instance0")));
  EXPECT_EQ(instance0, top->getInstance(NLName("instance00")));

  EXPECT_EQ(NLName("bbox_instance"), instance1->getName());
  EXPECT_EQ(nullptr, top->getInstance(NLName("instance1")));
  EXPECT_EQ(instance1, top->getInstance(NLName("bbox_instance")));

  EXPECT_EQ(NLName("assign_instance"), instance2->getName());
  EXPECT_EQ(nullptr, top->getInstance(NLName("instance2")));
  EXPECT_EQ(instance2, top->getInstance(NLName("assign_instance")));
}

TEST_F(SNLPyDBEditTest0, testEditDBError) {
  auto db = NLDB::create(NLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "edit";
  scriptPath /= "edit_faulty.py";
  EXPECT_THROW(SNLPyEdit::edit(scriptPath), NLException);
}
