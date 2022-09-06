#include "gtest/gtest.h"

#include "NajaObject.h"
#include "NajaPrivateProperty.h"
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
    NajaPrivateProperty* create(const std::string& name) {
      return nullptr;
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

}