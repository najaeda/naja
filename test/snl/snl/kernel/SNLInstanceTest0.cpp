#include "gtest/gtest.h"
#include "gmock/gmock.h"
using namespace std;
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLInstanceTest0: public ::testing::Test {
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

TEST_F(SNLInstanceTest0, testCreation) {
  SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  SNLDesign* model = SNLDesign::create(library, SNLName("model"));
  EXPECT_EQ(SNLName("design"), design->getName());
  EXPECT_EQ(SNLName("model"), model->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_EQ(1, model->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Design, 1, 0, 0, 0, 0, 0), design->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Design, 1, 0, 1, 0, 0, 0), model->getSNLID());
  EXPECT_EQ(SNLID::DesignReference(1, 0, 0), design->getReference());
  EXPECT_EQ(design, SNLUniverse::get()->getDesign(SNLID::DesignReference(1, 0, 0)));
  EXPECT_EQ(SNLID::DesignReference(1, 0, 1), model->getReference());
  EXPECT_EQ(model, SNLUniverse::get()->getDesign(SNLID::DesignReference(1, 0, 1)));
  auto term0 = SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i0"));
  auto term1 = SNLBusTerm::create(model, SNLTerm::Direction::Output, 0, 3);
  auto term2 = SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i1"));
  auto term3 = SNLScalarTerm::create(model, SNLTerm::Direction::InOut, SNLName("i2"));
  EXPECT_FALSE(model->getTerms().empty());
  EXPECT_EQ(4, model->getTerms().size());
  EXPECT_THAT(std::vector(model->getTerms().begin(), model->getTerms().end()),
    ElementsAre(term0, term1, term2, term3));

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
  EXPECT_FALSE(termsVector[0]->isAnonymous());
  EXPECT_TRUE(termsVector[1]->isAnonymous());
  EXPECT_FALSE(termsVector[2]->isAnonymous());
  EXPECT_FALSE(termsVector[3]->isAnonymous());
  EXPECT_EQ(SNLName("i0"), termsVector[0]->getName());
  EXPECT_EQ(SNLName("i1"), termsVector[2]->getName());
  EXPECT_EQ(SNLName("i2"), termsVector[3]->getName());

  SNLInstance* instance1 = SNLInstance::create(design, model, SNLName("instance1"));
  ASSERT_NE(instance1, nullptr);
  EXPECT_EQ(SNLName("instance1"), instance1->getName());
  EXPECT_EQ(design, instance1->getDesign());
  EXPECT_EQ(model, instance1->getModel());
  EXPECT_FALSE(instance1->getInstTerms().empty());
  EXPECT_TRUE(instance1->getConnectedInstTerms().empty());
  EXPECT_FALSE(instance1->getInstScalarTerms().empty());
  EXPECT_EQ(7, instance1->getInstTerms().size());
  EXPECT_EQ(0, instance1->getConnectedInstTerms().size());
  EXPECT_EQ(3, instance1->getInstScalarTerms().size());
  EXPECT_EQ(4, instance1->getInstBusTermBits().size());
  EXPECT_EQ(0, instance1->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Instance, 1, 0, 0, 0, instance1->getID(), 0), instance1->getSNLID());
  EXPECT_EQ(SNLID::DesignObjectReference(1, 0, 0, 0), instance1->getReference());
  EXPECT_EQ(instance1, SNLUniverse::get()->getInstance(SNLID::DesignObjectReference(1, 0, 0, 0)));
  EXPECT_EQ(SNLID::DesignReference(1, 0, 0), instance1->getReference().getDesignReference());
  EXPECT_NE(SNLID::DesignObjectReference(1, 0, 0, 1), instance1->getReference());

  using InstTermsVector = std::vector<SNLInstTerm*>;
  {
    InstTermsVector instTermsVector(
      instance1->getInstTerms().begin(),
      instance1->getInstTerms().end());
    EXPECT_EQ(7, instTermsVector.size());

    auto instTermsBegin = instance1->getInstTerms().begin();
    auto instTermsEnd = instance1->getInstTerms().end();
    instTermsVector = InstTermsVector(instTermsBegin, instTermsEnd);
    EXPECT_EQ(7, instTermsVector.size());

    EXPECT_EQ(termsVector[0], instTermsVector[0]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0), instTermsVector[1]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1), instTermsVector[2]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2), instTermsVector[3]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3), instTermsVector[4]->getTerm());
    EXPECT_EQ(termsVector[2], instTermsVector[5]->getTerm());
    EXPECT_EQ(termsVector[3], instTermsVector[6]->getTerm());

    EXPECT_THAT(instTermsVector,
      ElementsAre(
        instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[0])),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3)),
        instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[2])),
        instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[3]))
      )
    );

    EXPECT_THAT(std::vector(instance1->getInstScalarTerms().begin(), instance1->getInstScalarTerms().end()),
      ElementsAre(
        instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[0])),
        instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[2])),
        instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[3]))
      )
    );

    EXPECT_THAT(std::vector(instance1->getInstBusTermBits().begin(), instance1->getInstBusTermBits().end()),
      ElementsAre(
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)),
        instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3))
      )
    );

    EXPECT_EQ(0, termsVector[0]->getFlatID());
    EXPECT_EQ(1, termsVector[1]->getFlatID());
    EXPECT_EQ(1, dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)->getFlatID());
    EXPECT_EQ(2, dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)->getFlatID());
    EXPECT_EQ(3, dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)->getFlatID());
    EXPECT_EQ(4, dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3)->getFlatID());
    EXPECT_EQ(5, termsVector[2]->getFlatID());
    EXPECT_EQ(6, termsVector[3]->getFlatID());

    //i0
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 0, 0, 0), instTermsVector[0]->getSNLID());
    //anon bus[0,3]
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 0), instTermsVector[1]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 1), instTermsVector[2]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 2), instTermsVector[3]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 3), instTermsVector[4]->getSNLID());
    //i1
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 2, 0, 0), instTermsVector[5]->getSNLID());
    //i2
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 3, 0, 0), instTermsVector[6]->getSNLID());

    //Verify getInstTerm
    EXPECT_EQ(instTermsVector[0], instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[0])));
    EXPECT_EQ(instTermsVector[1], instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)));
    EXPECT_EQ(instTermsVector[2], instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)));
    EXPECT_EQ(instTermsVector[3], instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)));
    EXPECT_EQ(instTermsVector[4], instance1->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3)));
    EXPECT_EQ(instTermsVector[5], instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[2])));
    EXPECT_EQ(instTermsVector[6], instance1->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[3])));

    for (auto instTerm: instTermsVector) {
      EXPECT_EQ(instTerm, SNLUniverse::get()->getInstTerm(instTerm->getSNLID()));
      EXPECT_EQ(instTerm, SNLUniverse::get()->getObject(instTerm->getSNLID()));
    }
  }

  SNLInstance* instance2 = SNLInstance::create(design, model, SNLName("instance2"));
  ASSERT_NE(instance2, nullptr);
  EXPECT_EQ(SNLName("instance2"), instance2->getName());
  EXPECT_EQ(design, instance2->getDesign());
  EXPECT_EQ(model, instance2->getModel());

  {
    InstTermsVector instTermsVector(instance2->getInstTerms().begin(), instance2->getInstTerms().end());
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
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 0, 1, 0), instTermsVector[0]->getSNLID());
    //anon bus[0,3]
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 0), instTermsVector[1]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 1), instTermsVector[2]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 2), instTermsVector[3]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 3), instTermsVector[4]->getSNLID());
    //i1
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 2, 1, 0), instTermsVector[5]->getSNLID());
    //i2
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 3, 1, 0), instTermsVector[6]->getSNLID());

    //Verify getInstTerm
    EXPECT_EQ(instTermsVector[0], instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[0])));
    EXPECT_EQ(instTermsVector[1], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)));
    EXPECT_EQ(instTermsVector[2], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)));
    EXPECT_EQ(instTermsVector[3], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)));
    EXPECT_EQ(instTermsVector[4], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3)));
    EXPECT_EQ(instTermsVector[5], instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[2])));
    EXPECT_EQ(instTermsVector[6], instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[3])));

    for (auto instTerm: instTermsVector) {
      EXPECT_EQ(instTerm, SNLUniverse::get()->getInstTerm(instTerm->getSNLID()));
      EXPECT_EQ(instTerm, SNLUniverse::get()->getObject(instTerm->getSNLID()));
    }
    EXPECT_EQ(nullptr, SNLUniverse::get()->getObject(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 4, 1, 0)));
  }

  SNLInstance* instance1Test = design->getInstance(SNLName("instance1"));
  EXPECT_EQ(instance1Test, instance1);
  EXPECT_EQ(instance1, design->getInstance(0));

  SNLInstance* instance2Test = design->getInstance(SNLName("instance2"));
  EXPECT_EQ(instance2Test, instance2);
  EXPECT_EQ(instance2, design->getInstance(1));

  EXPECT_EQ(nullptr, design->getInstance(SNLName("NON_EXIST")));
  EXPECT_EQ(nullptr, design->getInstance(3));

  EXPECT_EQ(2, design->getInstances().size());
  EXPECT_THAT(std::vector(design->getInstances().begin(), design->getInstances().end()),
    ElementsAre(instance1, instance2));
  EXPECT_TRUE(design->getSlaveInstances().empty());
  EXPECT_EQ(2, model->getSlaveInstances().size());
  EXPECT_THAT(std::vector(model->getSlaveInstances().begin(), model->getSlaveInstances().end()),
    ElementsAre(instance1, instance2));
  EXPECT_TRUE(model->getInstances().empty());

  //create new terminals on model and verify corresponding instance terminals creation
  auto term4 =  SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i3"));
  EXPECT_EQ(5, model->getTerms().size());
  EXPECT_THAT(std::vector(model->getTerms().begin(), model->getTerms().end()),
    ElementsAre(term0, term1, term2, term3, term4));
  auto termsBegin = model->getTerms().begin();
  auto termsEnd = model->getTerms().end();
  termsVector = TermsVector(termsBegin, termsEnd);
  EXPECT_EQ(5, termsVector.size());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 4, 0, 0), termsVector[4]->getSNLID());
  EXPECT_FALSE(termsVector[4]->isAnonymous());
  EXPECT_EQ(8, instance1->getInstTerms().size());
  EXPECT_EQ(8, instance2->getInstTerms().size());
  EXPECT_EQ(0, instance1->getConnectedInstTerms().size());
  EXPECT_EQ(0, instance2->getConnectedInstTerms().size());

  auto term5 = SNLBusTerm::create(model, SNLTerm::Direction::Output, -2, 3, SNLName("o"));
  EXPECT_EQ(6, model->getTerms().size());
  EXPECT_THAT(std::vector(model->getTerms().begin(), model->getTerms().end()),
    ElementsAre(term0, term1, term2, term3, term4, term5));
  termsVector = TermsVector(model->getTerms().begin(), model->getTerms().end());
  EXPECT_EQ(6, termsVector.size());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 5, 0, 0), termsVector[5]->getSNLID());
  EXPECT_FALSE(termsVector[5]->isAnonymous());
  EXPECT_EQ(14, instance1->getInstTerms().size());
  EXPECT_EQ(14, instance2->getInstTerms().size());
  EXPECT_EQ(0, instance1->getConnectedInstTerms().size());
  EXPECT_EQ(0, instance2->getConnectedInstTerms().size());

  {
    InstTermsVector instTermsVector(instance2->getInstTerms().begin(), instance2->getInstTerms().end());
    EXPECT_EQ(14, instTermsVector.size());

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
    EXPECT_EQ(termsVector[4], instTermsVector[7]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(-2), instTermsVector[8]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(-1), instTermsVector[9]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(0), instTermsVector[10]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(1), instTermsVector[11]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(2), instTermsVector[12]->getTerm());
    EXPECT_EQ(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(3), instTermsVector[13]->getTerm());

    EXPECT_FALSE(instTermsVector[0]->isAnonymous());
    EXPECT_TRUE(instTermsVector[1]->isAnonymous());
    EXPECT_TRUE(instTermsVector[2]->isAnonymous());
    EXPECT_TRUE(instTermsVector[3]->isAnonymous());
    EXPECT_TRUE(instTermsVector[4]->isAnonymous());
    EXPECT_FALSE(instTermsVector[5]->isAnonymous());
    EXPECT_FALSE(instTermsVector[6]->isAnonymous());
    EXPECT_FALSE(instTermsVector[7]->isAnonymous());
    EXPECT_FALSE(instTermsVector[8]->isAnonymous());
    EXPECT_FALSE(instTermsVector[9]->isAnonymous());
    EXPECT_FALSE(instTermsVector[10]->isAnonymous());
    EXPECT_FALSE(instTermsVector[11]->isAnonymous());
    EXPECT_FALSE(instTermsVector[12]->isAnonymous());
    EXPECT_FALSE(instTermsVector[13]->isAnonymous());

    EXPECT_EQ(0,  termsVector[0]->getFlatID());
    EXPECT_EQ(1,  termsVector[1]->getFlatID());
    EXPECT_EQ(1,  dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)->getFlatID());
    EXPECT_EQ(2,  dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)->getFlatID());
    EXPECT_EQ(3,  dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)->getFlatID());
    EXPECT_EQ(4,  dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3)->getFlatID());
    EXPECT_EQ(5,  termsVector[2]->getFlatID());
    EXPECT_EQ(6,  termsVector[3]->getFlatID());
    EXPECT_EQ(7,  termsVector[4]->getFlatID());
    EXPECT_EQ(8,  dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(-2)->getFlatID());
    EXPECT_EQ(9,  dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(-1)->getFlatID());
    EXPECT_EQ(10, dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(0)->getFlatID());
    EXPECT_EQ(11, dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(1)->getFlatID());
    EXPECT_EQ(12, dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(2)->getFlatID());
    EXPECT_EQ(13, dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(3)->getFlatID());

    //i0
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 0, 1, 0), instTermsVector[0]->getSNLID());
    //anon bus[0,3]
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 0), instTermsVector[1]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 1), instTermsVector[2]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 2), instTermsVector[3]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 1, 3), instTermsVector[4]->getSNLID());
    //i1
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 2, 1, 0), instTermsVector[5]->getSNLID());
    //i2
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 3, 1, 0), instTermsVector[6]->getSNLID());
    //i3
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 4, 1, 0), instTermsVector[7]->getSNLID());
    //o bus[-2,3]
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 5, 1, -2), instTermsVector[8]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 5, 1, -1), instTermsVector[9]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 5, 1, 0), instTermsVector[10]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 5, 1, 1), instTermsVector[11]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 5, 1, 2), instTermsVector[12]->getSNLID());
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 5, 1, 3), instTermsVector[13]->getSNLID());

    //Verify getInstTerm
    EXPECT_EQ(instTermsVector[0],  instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[0])));
    EXPECT_EQ(instTermsVector[1],  instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(0)));
    EXPECT_EQ(instTermsVector[2],  instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(1)));
    EXPECT_EQ(instTermsVector[3],  instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(2)));
    EXPECT_EQ(instTermsVector[4],  instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[1])->getBit(3)));
    EXPECT_EQ(instTermsVector[5],  instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[2])));
    EXPECT_EQ(instTermsVector[6],  instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[3])));
    EXPECT_EQ(instTermsVector[7],  instance2->getInstTerm(dynamic_cast<SNLScalarTerm*>(termsVector[4])));
    EXPECT_EQ(instTermsVector[8],  instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(-2)));
    EXPECT_EQ(instTermsVector[9],  instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(-1)));
    EXPECT_EQ(instTermsVector[10], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(0)));
    EXPECT_EQ(instTermsVector[11], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(1)));
    EXPECT_EQ(instTermsVector[12], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(2)));
    EXPECT_EQ(instTermsVector[13], instance2->getInstTerm(dynamic_cast<SNLBusTerm*>(termsVector[5])->getBit(3)));

     //Verify getDirection
    EXPECT_EQ(SNLTerm::Direction::Input, instTermsVector[0]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[1]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[2]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[3]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[4]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Input, instTermsVector[5]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::InOut, instTermsVector[6]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Input, instTermsVector[7]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[8]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[9]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[10]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[11]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[12]->getDirection());
    EXPECT_EQ(SNLTerm::Direction::Output, instTermsVector[13]->getDirection());

    for (auto instTerm: instTermsVector) {
      EXPECT_EQ(instTerm, SNLUniverse::get()->getInstTerm(instTerm->getSNLID()));
      EXPECT_EQ(instTerm, SNLUniverse::get()->getObject(instTerm->getSNLID()));
    }
  }

  //destroy some terminals and verify instance terminals
  term4->destroy();
  EXPECT_EQ(5, model->getTerms().size());
  EXPECT_THAT(std::vector(model->getTerms().begin(), model->getTerms().end()),
    ElementsAre(term0, term1, term2, term3, term5));
  termsVector = TermsVector(model->getTerms().begin(), model->getTerms().end());
  EXPECT_EQ(5, termsVector.size());
  EXPECT_EQ(13, instance1->getInstTerms().size());
  EXPECT_EQ(13, instance2->getInstTerms().size());

  term1->destroy();
  EXPECT_EQ(4, model->getTerms().size());
  EXPECT_THAT(std::vector(model->getTerms().begin(), model->getTerms().end()),
    ElementsAre(term0, term2, term3, term5));
  termsVector = TermsVector(model->getTerms().begin(), model->getTerms().end());
  EXPECT_EQ(4, termsVector.size());
  EXPECT_EQ(9, instance1->getInstTerms().size());
  EXPECT_EQ(9, instance2->getInstTerms().size());

  instance1Test->destroy();
  EXPECT_EQ(design->getInstance(SNLName("instance1")), nullptr);
  instance2Test->destroy();
  EXPECT_EQ(design->getInstance(SNLName("instance2")), nullptr);
}

TEST_F(SNLInstanceTest0, testInstTerm) {
  SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  SNLDesign* model0 = SNLDesign::create(library, SNLName("model0"));
  auto model0Term = SNLScalarTerm::create(model0, SNLTerm::Direction::Input, SNLName("i0"));
  EXPECT_EQ(1, model0->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Design, 1, 0, 1, 0, 0, 0), model0->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 1, 0, 0, 0), model0Term->getSNLID());

  SNLDesign* model1 = SNLDesign::create(library, SNLName("model1"));
  auto model1Term = SNLScalarTerm::create(model1, SNLTerm::Direction::Input, SNLName("i0"));
  EXPECT_EQ(2, model1->getID());
  EXPECT_EQ(SNLID(SNLID::Type::Design, 1, 0, 2, 0, 0, 0), model1->getSNLID());
  EXPECT_EQ(SNLID(SNLID::Type::Term, 1, 0, 2, 0, 0, 0), model1Term->getSNLID());

  SNLInstance* instance = SNLInstance::create(design, model0, SNLName("instance"));

  auto it0 = instance->getInstTerm(model0Term);
  EXPECT_NE(nullptr, it0);
  EXPECT_THROW(instance->getInstTerm(model1Term), SNLException);

  EXPECT_THROW(instance->getInstTerm(model0Term)->destroy(), SNLException);
}

TEST_F(SNLInstanceTest0, testModelDestroy) {
  SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  SNLDesign* model0 = SNLDesign::create(library, SNLName("model0"));
  SNLDesign* model1 = SNLDesign::create(library, SNLName("model1"));

  EXPECT_FALSE(library->getDesigns().empty());
  EXPECT_EQ(3, library->getDesigns().size());

  EXPECT_TRUE(design->getInstances().empty());
  EXPECT_TRUE(model0->getSlaveInstances().empty());
  EXPECT_TRUE(model1->getSlaveInstances().empty());

  for (int i=0; i<4; ++i) {
    auto inst0 = SNLInstance::create(design, model0);
    auto inst1 = SNLInstance::create(design, model1);
  }

  EXPECT_FALSE(design->getInstances().empty());
  EXPECT_FALSE(model0->getSlaveInstances().empty());
  EXPECT_FALSE(model1->getSlaveInstances().empty());
  EXPECT_EQ(8, design->getInstances().size());
  EXPECT_EQ(4, model0->getSlaveInstances().size());
  EXPECT_EQ(4, model1->getSlaveInstances().size());

  model0->destroy();
  EXPECT_EQ(2, library->getDesigns().size());
  EXPECT_EQ(4, design->getInstances().size());
  EXPECT_EQ(4, model1->getSlaveInstances().size());

  model1->destroy();
  EXPECT_EQ(1, library->getDesigns().size());
  EXPECT_EQ(0, design->getInstances().size());
  EXPECT_TRUE(design->getInstances().empty());
}

TEST_F(SNLInstanceTest0, testErrors) {
  SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
  SNLDesign* design = SNLDesign::create(library, SNLName("design"));
  SNLDesign* model = SNLDesign::create(library, SNLName("model"));

  EXPECT_THROW(SNLInstance::create(nullptr, model), SNLException);
  EXPECT_THROW(SNLInstance::create(design, nullptr), SNLException);

  SNLInstance::create(design, model, SNLName("name"));
  EXPECT_THROW(SNLInstance::create(design, model, SNLName("name")), SNLException);
  EXPECT_THROW(SNLInstance::create(design, model, SNLID::DesignObjectID(0)), SNLException);
}