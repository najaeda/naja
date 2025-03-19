// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NajaObject.h"
#include "NajaPythonProperty.h"
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

}

class NajaPythonPropertyTest: public ::testing::Test {
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

TEST_F(NajaPythonPropertyTest, test) {
  ASSERT_NE(nullptr, testObject_);
  TestObject copyOfTestObject = *testObject_;
  naja::NajaPythonProperty*  proxy = naja::NajaPythonProperty::create ( (void*)testObject_ );     
  ASSERT_NE(nullptr, proxy);
  testObject_->put ( proxy ); 
  naja::NajaPythonProperty*  proxyNew = naja::NajaPythonProperty::create ( (void*)testObject_ );     
  ASSERT_NE(nullptr, proxyNew);
  testObject_->put ( proxyNew ); 
  EXPECT_THROW(copyOfTestObject.put ( proxyNew ), NajaException);
  EXPECT_THROW(testObject_->put ( nullptr ), NajaException);
  EXPECT_THROW(testObject_->remove( nullptr ), NajaException);
  EXPECT_THROW(naja::NajaPythonProperty::create ( nullptr ), NajaException);
  proxyNew->getString();
}