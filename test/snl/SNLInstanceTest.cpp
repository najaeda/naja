#include "gtest/gtest.h"

#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
using namespace std;
using namespace SNL;

class SNLInstanceTest: public ::testing::Test {
  protected:
    void SetUp() override {
      db_ = SNLDB::create();
    }
    void TearDown() override {
      db_->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLInstanceTest, testCreation) {
    SNLLibrary* library = SNLLibrary::create(db_, "MYLIB");
    SNLDesign* design = SNLDesign::create(library, "design");
    SNLDesign* model = SNLDesign::create(library, "model");
    SNLInstance* instance1 = SNLInstance::create(design, model, "instance1");
    SNLInstance* instance2 = SNLInstance::create(design, model, "instance2");
    ASSERT_TRUE(instance1);
    ASSERT_TRUE(instance2);
    ASSERT_EQ(design, instance1->getDesign());
    ASSERT_EQ(model, instance1->getModel());
    ASSERT_EQ("design", design->getName());
    ASSERT_EQ("model", model->getName());
    ASSERT_EQ("instance1", instance1->getName());
    SNLInstance* instance1Test = design->getInstance(SNLName("instance1"));
    ASSERT_EQ(instance1Test, instance1);
    SNLInstance* instance2Test = design->getInstance(SNLName("instance2"));
    ASSERT_EQ(instance2Test, instance2);

    SNLCollection<SNLInstance> instances = design->getInstances();
    SNLIterator<SNLInstance> instanceIt = instances.getIterator();
    while (instanceIt.isValid()) {
      cerr << instanceIt.getElement()->getDescription() << endl;
      ++instanceIt;
    }
    
    instance1Test->destroy();
    EXPECT_FALSE(design->getInstance(SNLName("instance1")));
    instance2Test->destroy();
    EXPECT_FALSE(design->getInstance(SNLName("instance2")));
}
