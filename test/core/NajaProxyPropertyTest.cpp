// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NajaObject.h"
#include "SNLProxyProperty.h"
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

class TestPrivateProperty: public SNLProxyProperty {
  public:
    using super = SNLProxyProperty;
    static const inline std::string Name = "TestPrivateProperty";
    static void preCreate(NajaObject* object) {
      super::preCreate(object, Name);
    }

    void postCreate(NajaObject* owner) {
      super::postCreate(owner);
    }

    /*static TestPrivateProperty* create(NajaObject* owner) {
      preCreate(owner);
      TestPrivateProperty* property = new TestPrivateProperty();
      property->postCreate(owner);
      return property;
    }*/

    std::string getName() const override {
      return Name;
    }

    std::string getString() const override {
      return getName();
    }
};

}

class SNLProxyPropertyTest: public ::testing::Test {
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

TEST_F(SNLProxyPropertyTest, test) {
  ASSERT_NE(nullptr, testObject_);
  naja::SNLProxyProperty*  proxy = naja::SNLProxyProperty::create ( (void*)proxy );     
  ASSERT_NE(nullptr, proxy);
  testObject_->put ( proxy ); 
  naja::SNLProxyProperty*  proxyNew = naja::SNLProxyProperty::create ( (void*)proxy );     
  ASSERT_NE(nullptr, proxyNew);
  testObject_->put ( proxyNew ); 
  EXPECT_THROW(testObject_->put ( nullptr ), NajaException);
  EXPECT_THROW(testObject_->remove( nullptr ), NajaException);
}