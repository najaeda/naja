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
    EXPECT_EQ("design", design->getName());
    EXPECT_EQ("model", model->getName());
    SNLInstance* instance1 = SNLInstance::create(design, model, "instance1");
    ASSERT_TRUE(instance1);
    EXPECT_EQ("instance1", instance1->getName());
    EXPECT_EQ(design, instance1->getDesign());
    EXPECT_EQ(model, instance1->getModel());

    SNLInstance* instance2 = SNLInstance::create(design, model, "instance2");
    ASSERT_TRUE(instance2);
    EXPECT_EQ("instance2", instance2->getName());
    EXPECT_EQ(design, instance2->getDesign());
    EXPECT_EQ(model, instance2->getModel());

    SNLInstance* instance1Test = design->getInstance(SNLName("instance1"));
    EXPECT_EQ(instance1Test, instance1);
    SNLInstance* instance2Test = design->getInstance(SNLName("instance2"));
    EXPECT_EQ(instance2Test, instance2);

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
