#include "gtest/gtest.h"

#include "NajaObject.h"
using namespace naja;

namespace {

class TestObject: public NajaObject {
  public:
    using super = NajaObject;
    TestObject* create() {
      preCreate();
      TestObject* object = new TestObject();
      object->postCreate();
      return object;
    }

    std::string getTypeName() const override {
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


class NajaPrivatePropertyTest: public ::testing::Test {
  protected:
    void SetUp() override {
      TestObject* testObject = TestObject::create();
    }
    void TearDown() override {
      testObject_->destroy();
    }
    TestObject* testObject_;
};

TEST_F(NajaPrivatePropertyTest, test) {
}