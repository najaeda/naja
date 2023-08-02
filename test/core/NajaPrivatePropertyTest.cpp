// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

#include "gtest/gtest.h"

#include "NajaObject.h"
#include "NajaPrivateProperty.h"
#include "NajaException.h"
using namespace naja;

namespace {

class TestObject: public NajaObject {
  public:
    using super = NajaObject;
    static TestObject* create() {
      preCreate();
      TestObject* object = new TestObject();
      object->postCreate();
      return object;
    }

    const char* getTypeName() const override {
      return "TestObject";
    }

    void postCreate() {
      super::postCreate();
    }

    void preDestroy() override {
      super::preDestroy();
    }

    std::string getString() const override {
      return getTypeName();
    }

    std::string getDescription() const override {
      return getTypeName();
    }
};

class TestPrivateProperty: public NajaPrivateProperty {
  public:
    using super = NajaPrivateProperty;
    static const inline std::string Name = "TestPrivateProperty";
    static void preCreate(NajaObject* object) {
      super::preCreate(object, Name);
    }

    void postCreate(NajaObject* owner) {
      super::postCreate(owner);
    }

    static TestPrivateProperty* create(NajaObject* owner) {
      preCreate(owner);
      TestPrivateProperty* property = new TestPrivateProperty();
      property->postCreate(owner);
      return property;
    }

    std::string getName() const override {
      return Name;
    }

    std::string getString() const override {
      return getName();
    }
};


}


class NajaPrivatePropertyTest: public ::testing::Test {
  protected:
    void SetUp() override {
      testObject_ = TestObject::create();
    }
    void TearDown() override {
      if (testObject_) {
        testObject_->destroy();
        testObject_ = nullptr;
      }

    }
    TestObject* testObject_;
};

TEST_F(NajaPrivatePropertyTest, test) {
  ASSERT_NE(nullptr, testObject_);
  TestPrivateProperty* property = TestPrivateProperty::create(testObject_);
  ASSERT_NE(nullptr, property);
  EXPECT_TRUE(testObject_->hasProperty(TestPrivateProperty::Name));
  EXPECT_EQ(property, testObject_->getProperty(TestPrivateProperty::Name));
  EXPECT_EQ(1, testObject_->getProperties().size());
  EXPECT_FALSE(property->isDumpable());
  EXPECT_THROW(TestPrivateProperty::create(testObject_), NajaException);
}
