// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLException.h"
#include "SNLPyEdit.h"
using namespace naja::SNL;

#ifndef SNL_PYEDIT_TEST_PATH
#define SNL_PYEDIT_TEST_PATH "Undefined"
#endif

class SNLPyDBEditTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      auto primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("primitives"));
      auto prim0 = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive, SNLName("prim0"));
      auto designsLibrary = SNLLibrary::create(db, SNLName("designs"));
      auto top = SNLDesign::create(designsLibrary, SNLName("top"));
      auto inst0 = SNLInstance::create(top, prim0, SNLName("instance0"));
      auto bbox = SNLDesign::create(designsLibrary, SNLName("bbox"));
      bbox->setType(SNLDesign::Type::Blackbox);
      auto inst1 = SNLInstance::create(top, bbox, SNLName("instance1"));
      auto inst2 = SNLInstance::create(top, SNLDB0::getAssign(), SNLName("instance2"));
      universe->setTopDesign(top);
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPyDBEditTest0, test) {
  auto universe = SNLUniverse::get();
  ASSERT_TRUE(universe);
  auto top = universe->getTopDesign();
  ASSERT_TRUE(top);
  auto instance0 = top->getInstance(SNLName("instance0"));
  ASSERT_TRUE(instance0);
  EXPECT_EQ(SNLName("instance0"), instance0->getName());
  auto instance1 = top->getInstance(SNLName("instance1"));
  ASSERT_TRUE(instance1);
  EXPECT_EQ(SNLName("instance1"), instance1->getName());
  auto instance2 = top->getInstance(SNLName("instance2"));
  ASSERT_TRUE(instance2);
  EXPECT_EQ(SNLName("instance2"), instance2->getName());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "naja_edit";
  scriptPath /= "naja_edit_test0.py";
  SNLPyEdit::najaEdit(scriptPath);
  EXPECT_EQ(SNLName("instance00"), instance0->getName());
  EXPECT_EQ(nullptr, top->getInstance(SNLName("instance0")));
  EXPECT_EQ(instance0, top->getInstance(SNLName("instance00")));

  EXPECT_EQ(SNLName("bbox_instance"), instance1->getName());
  EXPECT_EQ(nullptr, top->getInstance(SNLName("instance1")));
  EXPECT_EQ(instance1, top->getInstance(SNLName("bbox_instance")));

  EXPECT_EQ(SNLName("assign_instance"), instance2->getName());
  EXPECT_EQ(nullptr, top->getInstance(SNLName("instance2")));
  EXPECT_EQ(instance2, top->getInstance(SNLName("assign_instance")));
}

TEST_F(SNLPyDBEditTest0, testEditDBError) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "naja_edit";
  scriptPath /= "naja_edit_faulty.py";
  EXPECT_THROW(SNLPyEdit::najaEdit(scriptPath), SNLException);
}