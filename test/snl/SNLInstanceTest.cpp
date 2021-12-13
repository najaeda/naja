#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
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
  EXPECT_EQ(0, design->getID());
  EXPECT_EQ(1, model->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Design, 1, 0, 0, 0, 0, 0), design->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Design, 1, 0, 1, 0, 0, 0), model->getSNLID());
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i0"));
  SNLBusTerm::create(model, SNLTerm::Direction::Input, 0, 3);
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i1"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i2"));
  EXPECT_FALSE(model->getTerms().empty());
  EXPECT_EQ(4, model->getTerms().size());

  using TermsVector = std::vector<SNLTerm*>;
  TermsVector termsVector(model->getTerms().begin(), model->getTerms().end());
  EXPECT_EQ(4, termsVector.size());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 0, 0, 0), termsVector[0]->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 1, 0, 0), termsVector[1]->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 2, 0, 0), termsVector[2]->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 3, 0, 0), termsVector[3]->getSNLID());
  EXPECT_TRUE(dynamic_cast<SNLScalarTerm*>(termsVector[0]));
  EXPECT_TRUE(dynamic_cast<SNLBusTerm*>(termsVector[1]));
  EXPECT_TRUE(dynamic_cast<SNLScalarTerm*>(termsVector[2]));
  EXPECT_TRUE(dynamic_cast<SNLScalarTerm*>(termsVector[3]));

  SNLInstance* instance1 = SNLInstance::create(design, model, "instance1");
  ASSERT_NE(instance1, nullptr);
  EXPECT_EQ("instance1", instance1->getName());
  EXPECT_EQ(design, instance1->getDesign());
  EXPECT_EQ(model, instance1->getModel());
  EXPECT_EQ(7, instance1->getInstTerms().size());
  EXPECT_EQ(0, instance1->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Instance, 1, 0, 0, 0, instance1->getID(), 0), instance1->getSNLID());

  using InstTermsVector = std::vector<SNLInstTerm*>;
  InstTermsVector instTermsVector(instance1->getInstTerms().begin(), instance1->getInstTerms().end());
  EXPECT_EQ(7, instTermsVector.size());

  //for (auto i=0; i<instTermsVector.size(); ++i) {
  //  auto instTerm = instTermsVector[i];
  //  std::cerr << i << ":" << instTerm->getSNLID().getString() << std::endl;
  //  std::cerr << i << ":" << instTerm->getTerm()->getSNLID().getString() << std::endl;
  //}

  EXPECT_EQ(termsVector[0], instTermsVector[0]->getTerm());
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0), instTermsVector[1]->getTerm());
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1), instTermsVector[2]->getTerm());
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2), instTermsVector[3]->getTerm());
  EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3), instTermsVector[4]->getTerm());
  EXPECT_EQ(termsVector[2], instTermsVector[5]->getTerm());
  EXPECT_EQ(termsVector[3], instTermsVector[6]->getTerm());

  //i0
  EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 0, 0, 0), instTermsVector[0]->getSNLID());
  //anon bus[0,3]
  EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 0), instTermsVector[1]->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 1), instTermsVector[2]->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 2), instTermsVector[3]->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 3), instTermsVector[4]->getSNLID());

  SNLInstance* instance2 = SNLInstance::create(design, model, "instance2");
  ASSERT_NE(instance2, nullptr);
  EXPECT_EQ("instance2", instance2->getName());
  EXPECT_EQ(design, instance2->getDesign());
  EXPECT_EQ(model, instance2->getModel());

  SNLInstance* instance1Test = design->getInstance(SNLName("instance1"));
  EXPECT_EQ(instance1Test, instance1);
  SNLInstance* instance2Test = design->getInstance(SNLName("instance2"));
  EXPECT_EQ(instance2Test, instance2);

  //for (auto instance: design->getInstances()) {
  //  cerr << instance->getDescription() << endl;
  //}
  
  instance1Test->destroy();
  EXPECT_EQ(design->getInstance(SNLName("instance1")), nullptr);
  instance2Test->destroy();
  EXPECT_EQ(design->getInstance(SNLName("instance2")), nullptr);
}
