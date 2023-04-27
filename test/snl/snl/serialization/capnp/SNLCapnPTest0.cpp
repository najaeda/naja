#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "NajaDumpableProperty.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"

using namespace naja;
using namespace naja::SNL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapNpTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
      NajaDumpableProperty::create(library, "LIB_PROPERTY");
      SNLDesign* design = SNLDesign::create(library, SNLName("design"));

      db_->setTopDesign(design);
      NajaDumpableProperty::create(db_, "TEST_PROPERTY");

      auto iTerm = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("i"));
      auto o1Term = SNLBusTerm::create(design, SNLTerm::Direction::Output, 31, 0, SNLName("o1"));
      auto o2Term = SNLScalarTerm::create(design, SNLTerm::Direction::InOut, SNLName("o2"));

      auto iNet = SNLScalarNet::create(design);
      iTerm->setNet(iNet);

      auto o1Net = SNLBusNet::create(design, 31, 0);
      o1Term->setNet(o1Net);

      auto o2Net = SNLScalarNet::create(design);
      o2Term->setNet(o2Net);

      SNLScalarNet::create(design, SNLName("n1"));
      SNLBusNet::create(design, 31, 0, SNLName("n2"));

      SNLDesign* model0 = SNLDesign::create(library, SNLName("model0"));
      SNLScalarTerm::create(model0, SNLTerm::Direction::Input, SNLName("i"));
      SNLScalarTerm::create(model0, SNLTerm::Direction::Output, SNLName("o"));
      auto instance1 = SNLInstance::create(design, model0, SNLName("instance1"));
      auto instance2 = SNLInstance::create(design, model0, SNLName("instance2"));
      SNLParameter::create(model0, SNLName("Test1"), SNLParameter::Type::Decimal, "10");
      SNLParameter::create(model0, SNLName("Test2"), SNLParameter::Type::Binary, "4'hF");
      SNLParameter::create(model0, SNLName("Test3"), SNLParameter::Type::String, "Value2");

      //connections between instances
      instance1->getInstTerm(model0->getScalarTerm(SNLName("o")))->setNet(design->getScalarNet(SNLName("n1")));
      instance2->getInstTerm(model0->getScalarTerm(SNLName("i")))->setNet(design->getScalarNet(SNLName("n1")));

      SNLDesign* model1 = SNLDesign::create(library, SNLName("model1"));
      SNLBusTerm::create(model1, SNLTerm::Direction::Input, 31, 0, SNLName("i"));
      SNLBusTerm::create(model1, SNLTerm::Direction::Output, 31, 0, SNLName("o"));
      auto instance3 = SNLInstance::create(design, model1, SNLName("instance3"));
      auto instance4 = SNLInstance::create(design, model1, SNLName("instance4"));
      instance3->setTermNet(model1->getTerm(SNLName("i")), design->getNet(SNLName("n2")));
      instance4->setTermNet(model1->getTerm(SNLName("o")), design->getNet(SNLName("n2")));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
  protected:
    SNLDB*      db_;
};

TEST_F(SNLCapNpTest0, test0) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));
  ASSERT_TRUE(lib);
  EXPECT_EQ(1, lib->getProperties().size());
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "SNLCapNpTest0_test0.snl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);
  db_->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(SNLID::DBID(1), db_->getID());
  EXPECT_EQ(1, db_->getProperties().size());
  EXPECT_TRUE(db_->hasProperty("TEST_PROPERTY"));
  auto testProperty =
    dynamic_cast<NajaDumpableProperty*>(db_->getProperty("TEST_PROPERTY"));
  ASSERT_NE(nullptr, testProperty);
  EXPECT_EQ("TEST_PROPERTY", testProperty->getName());
  EXPECT_EQ(db_, testProperty->getOwner());
  testProperty->destroy();
  testProperty = nullptr;
  EXPECT_TRUE(db_->getProperties().empty());
  EXPECT_FALSE(db_->hasProperty("TEST_PROPERTY"));

  EXPECT_EQ(1, db_->getLibraries().size());
  auto library = *(db_->getLibraries().begin());
  ASSERT_TRUE(library);
  EXPECT_EQ(SNLID::LibraryID(0), library->getID());
  EXPECT_EQ(SNLName("MYLIB"), library->getName());
  EXPECT_EQ(SNLLibrary::Type::Standard, library->getType());

  EXPECT_EQ(1, library->getProperties().size());
  EXPECT_TRUE(library->hasProperty("LIB_PROPERTY"));
  auto libProperty =
    dynamic_cast<NajaDumpableProperty*>(library->getProperty("LIB_PROPERTY"));
  ASSERT_NE(nullptr, libProperty);
  EXPECT_EQ("LIB_PROPERTY", libProperty->getName());
  EXPECT_EQ(library, libProperty->getOwner());
  libProperty->destroy();
  libProperty = nullptr;
  EXPECT_TRUE(library->getProperties().empty());
  EXPECT_FALSE(library->hasProperty("LIB_PROPERTY"));

  EXPECT_EQ(3, library->getDesigns().size());
  using Designs = std::vector<SNLDesign*>;
  Designs designs(library->getDesigns().begin(), library->getDesigns().end());
  EXPECT_EQ(3, designs.size());
  
  auto design = designs[0];
  EXPECT_EQ(SNLID::DesignID(0), design->getID());
  EXPECT_EQ(SNLName("design"), design->getName());
  EXPECT_EQ(SNLDesign::Type::Standard, design->getType());
  EXPECT_TRUE(design->getParameters().empty());
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(5, design->getNets().size());
  EXPECT_EQ(4, design->getInstances().size());
  EXPECT_EQ(design, db_->getTopDesign());

  auto model = designs[1];
  EXPECT_EQ(SNLID::DesignID(1), model->getID());
  EXPECT_EQ(SNLName("model0"), model->getName());
  EXPECT_EQ(SNLDesign::Type::Standard, model->getType());
  EXPECT_EQ(2, model->getTerms().size());
  EXPECT_EQ(2, model->getScalarTerms().size());
  using ScalarTerms = std::vector<SNLScalarTerm*>;
  ScalarTerms scalarTerms(model->getScalarTerms().begin(), model->getScalarTerms().end());
  EXPECT_EQ(2, scalarTerms.size());
  auto scalarTerm0 = scalarTerms[0];
  EXPECT_EQ(SNLID::DesignObjectID(0), scalarTerm0->getID());
  EXPECT_EQ(SNLName("i"), scalarTerm0->getName());
  auto scalarTerm1 = scalarTerms[1];
  EXPECT_EQ(SNLName("o"), scalarTerm1->getName());
  EXPECT_EQ(SNLID::DesignObjectID(1), scalarTerm1->getID());
  EXPECT_TRUE(model->getBusTerms().empty());
  EXPECT_EQ(3, model->getParameters().size());
  using Parameters = std::vector<SNLParameter*>;
  Parameters parameters(model->getParameters().begin(), model->getParameters().end());
  EXPECT_EQ(3, parameters.size());
  EXPECT_EQ("Test1", parameters[0]->getName().getString());
  EXPECT_EQ(SNLParameter::Type::Decimal, parameters[0]->getType());
  EXPECT_EQ("10", parameters[0]->getValue());
  EXPECT_EQ("Test2", parameters[1]->getName().getString());
  EXPECT_EQ(SNLParameter::Type::Binary, parameters[1]->getType());
  EXPECT_EQ("4'hF", parameters[1]->getValue());
  EXPECT_EQ("Test3", parameters[2]->getName().getString());
  EXPECT_EQ(SNLParameter::Type::String, parameters[2]->getType());
  EXPECT_EQ("Value2", parameters[2]->getValue());

  using Instances = std::vector<SNLInstance*>;
  Instances instances(design->getInstances().begin(), design->getInstances().end());
  EXPECT_EQ(4, instances.size());
  auto instance1 = instances[0];
  EXPECT_EQ(SNLName("instance1"), instance1->getName());
  EXPECT_EQ(SNLID::DesignObjectID(0), instance1->getID());
  EXPECT_EQ(design, instance1->getDesign());
  EXPECT_EQ(model, instance1->getModel());
  auto instance2 = instances[1];
  EXPECT_EQ(SNLName("instance2"), instance2->getName());
  EXPECT_EQ(SNLID::DesignObjectID(1), instance2->getID());
  EXPECT_EQ(design, instance2->getDesign());
  EXPECT_EQ(model, instance2->getModel());
  using Nets = std::vector<SNLNet*>;
  using Terms = std::vector<SNLTerm*>;
  {
    Nets nets(design->getNets().begin(), design->getNets().end());
    EXPECT_EQ(5, nets.size());
    EXPECT_NE(nullptr, nets[0]);
    auto scalarNet0 = dynamic_cast<SNLScalarNet*>(nets[0]);
    EXPECT_NE(nullptr, scalarNet0);
    EXPECT_EQ(SNLID::DesignObjectID(0), scalarNet0->getID());
    EXPECT_TRUE(scalarNet0->isAnonymous());

    EXPECT_NE(nullptr, nets[1]);
    auto busNet1 = dynamic_cast<SNLBusNet*>(nets[1]);
    EXPECT_NE(nullptr, busNet1);
    EXPECT_EQ(SNLID::DesignObjectID(1), busNet1->getID());
    EXPECT_TRUE(busNet1->isAnonymous());
    EXPECT_EQ(31, busNet1->getMSB());
    EXPECT_EQ(0, busNet1->getLSB());

    EXPECT_NE(nullptr, nets[2]);
    auto scalarNet2 = dynamic_cast<SNLScalarNet*>(nets[2]);
    EXPECT_NE(nullptr, scalarNet2);
    EXPECT_EQ(SNLID::DesignObjectID(2), scalarNet2->getID());
    EXPECT_TRUE(scalarNet2->isAnonymous());

    EXPECT_NE(nullptr, nets[3]);
    auto scalarNet3 = dynamic_cast<SNLScalarNet*>(nets[3]);
    EXPECT_NE(nullptr, scalarNet3);
    EXPECT_EQ(SNLID::DesignObjectID(3), scalarNet3->getID());
    EXPECT_FALSE(scalarNet3->isAnonymous());
    EXPECT_EQ(SNLName("n1"), scalarNet3->getName());
    EXPECT_EQ(2, scalarNet3->getComponents().size());
    EXPECT_TRUE(scalarNet3->getBitTerms().empty());
    EXPECT_EQ(2, scalarNet3->getInstTerms().size());
    using InstTerms = std::vector<SNLInstTerm*>;
    InstTerms instTerms(scalarNet3->getInstTerms().begin(), scalarNet3->getInstTerms().end());
    EXPECT_EQ(2, instTerms.size());
    auto instTerm1 = instTerms[0];
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 0, 1, 0), instTerm1->getSNLID());
    EXPECT_EQ(instance2, instTerm1->getInstance());
    EXPECT_EQ(instance2->getModel()->getScalarTerm(SNLName("i")), instTerm1->getTerm());

    EXPECT_NE(nullptr, nets[4]);
    auto busNet4 = dynamic_cast<SNLBusNet*>(nets[4]);
    EXPECT_NE(nullptr, busNet4);
    EXPECT_EQ(SNLID::DesignObjectID(4), busNet4->getID());
    EXPECT_FALSE(busNet4->isAnonymous());
    EXPECT_EQ(SNLName("n2"), busNet4->getName());
    for (auto bit: busNet4->getBits()) {
      EXPECT_EQ(2, bit->getComponents().size());
      EXPECT_TRUE(bit->getBitTerms().empty());
      EXPECT_EQ(2, bit->getInstTerms().size());
    }

    auto instTerm2 = instTerms[1];
    EXPECT_EQ(SNLID(SNLID::Type::InstTerm, 1, 0, 0, 1, 0, 0), instTerm2->getSNLID());
    EXPECT_EQ(instance1, instTerm2->getInstance());
    EXPECT_EQ(instance1->getModel(), instTerm2->getTerm()->getDesign());
    EXPECT_EQ(instance1->getModel()->getScalarTerm(SNLName("o")), instTerm2->getTerm());

    Terms terms(design->getTerms().begin(), design->getTerms().end());
    EXPECT_EQ(3, terms.size());
    EXPECT_NE(nullptr, terms[0]);
    auto scalarTerm0 = dynamic_cast<SNLScalarTerm*>(terms[0]);
    EXPECT_NE(nullptr, scalarTerm0);
    EXPECT_EQ(SNLID::DesignObjectID(0), scalarTerm0->getID());
    EXPECT_EQ(SNLName("i"), scalarTerm0->getName());
    EXPECT_EQ(SNLTerm::Direction::Input, scalarTerm0->getDirection());
    EXPECT_NE(nullptr, scalarTerm0->getNet());
    EXPECT_EQ(scalarNet0, scalarTerm0->getNet());

    EXPECT_NE(nullptr, terms[1]);
    auto busTerm1 = dynamic_cast<SNLBusTerm*>(terms[1]);
    EXPECT_NE(nullptr, busTerm1);
    EXPECT_EQ(SNLID::DesignObjectID(1), busTerm1->getID());
    EXPECT_EQ(SNLName("o1"), busTerm1->getName());
    EXPECT_EQ(SNLTerm::Direction::Output, busTerm1->getDirection());
    EXPECT_EQ(31, busTerm1->getMSB());
    EXPECT_EQ(0, busTerm1->getLSB());
    for (size_t bit=0; bit<32; bit++) {
      EXPECT_EQ(busNet1->getBit(bit), busTerm1->getBit(bit)->getNet());
    }

    EXPECT_NE(nullptr, terms[2]);
    auto scalarTerm1 = dynamic_cast<SNLScalarTerm*>(terms[2]);
    EXPECT_NE(nullptr, scalarTerm1);
    EXPECT_EQ(SNLID::DesignObjectID(2), scalarTerm1->getID());
    EXPECT_EQ(SNLName("o2"), scalarTerm1->getName());
    EXPECT_EQ(SNLTerm::Direction::InOut, scalarTerm1->getDirection());
    EXPECT_EQ(scalarNet2, scalarTerm1->getNet());
  }
}
