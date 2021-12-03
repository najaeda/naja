#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLInstance.h"
using namespace std;
using namespace SNL;

class SNLInstanceTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDB*  db_;
};

TEST_F(SNLInstanceTest, testCreation) {
  SNLLibrary* library = SNLLibrary::create(db_, "MYLIB");
  SNLDesign* design = SNLDesign::create(library, "design");
  SNLDesign* model = SNLDesign::create(library, "model");
  EXPECT_EQ("design", design->getName());
  EXPECT_EQ("model", model->getName());
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i0"));
  SNLBusTerm::create(model, SNLTerm::Direction::Input, 0, 3);
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i1"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i2"));
  EXPECT_FALSE(model->getTerms().empty());
  EXPECT_EQ(4, model->getTerms().size());

  SNLInstance* instance1 = SNLInstance::create(design, model, "instance1");
  ASSERT_NE(instance1, nullptr);
  EXPECT_EQ("instance1", instance1->getName());
  EXPECT_EQ(design, instance1->getDesign());
  EXPECT_EQ(model, instance1->getModel());
  EXPECT_EQ(7, instance1->getInstTerms().size());

  SNLInstance* instance2 = SNLInstance::create(design, model, "instance2");
  ASSERT_NE(instance2, nullptr);
  EXPECT_EQ("instance2", instance2->getName());
  EXPECT_EQ(design, instance2->getDesign());
  EXPECT_EQ(model, instance2->getModel());

  SNLInstance* instance1Test = design->getInstance(SNLName("instance1"));
  EXPECT_EQ(instance1Test, instance1);
  SNLInstance* instance2Test = design->getInstance(SNLName("instance2"));
  EXPECT_EQ(instance2Test, instance2);

  for (auto instance: design->getInstances()) {
    cerr << instance->getDescription() << endl;
  }
  
  instance1Test->destroy();
  EXPECT_EQ(design->getInstance(SNLName("instance1")), nullptr);
  instance2Test->destroy();
  EXPECT_EQ(design->getInstance(SNLName("instance2")), nullptr);
}
