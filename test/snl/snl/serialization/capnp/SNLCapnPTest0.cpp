#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

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
      SNLDesign* design = SNLDesign::create(library, SNLName("design"));

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

      SNLDesign* model = SNLDesign::create(library, SNLName("model"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Output, SNLName("o"));
      SNLInstance* instance1 = SNLInstance::create(design, model, SNLName("instance1"));
      SNLInstance* instance2 = SNLInstance::create(design, model, SNLName("instance2"));
      SNLParameter::create(model, SNLName("Test1"), "Value1");
      SNLParameter::create(model, SNLName("Test2"), "Value2");

      //connections between instances
      instance1->getInstTerm(model->getScalarTerm(SNLName("o")))->setNet(design->getScalarNet(SNLName("n1")));
      instance2->getInstTerm(model->getScalarTerm(SNLName("i")))->setNet(design->getScalarNet(SNLName("n1")));
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
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_CAPNP_TEST_PATH);
  outPath /= "test0.snl";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }

  SNLCapnP::dump(db_, outPath);
  db_->destroy();  
  db_ = nullptr;
  db_ = SNLCapnP::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(SNLID::DBID(1), db_->getID());
  EXPECT_EQ(1, db_->getLibraries().size());
  auto library = *(db_->getLibraries().begin());
  ASSERT_TRUE(library);
  EXPECT_EQ(SNLID::LibraryID(0), library->getID());
  EXPECT_EQ(SNLName("MYLIB"), library->getName());
  EXPECT_EQ(SNLLibrary::Type::Standard, library->getType());
  EXPECT_EQ(2, library->getDesigns().size());
  using Designs = std::vector<SNLDesign*>;
  Designs designs(library->getDesigns().begin(), library->getDesigns().end());
  EXPECT_EQ(2, designs.size());
  auto design0 = designs[0];
  EXPECT_EQ(SNLID::DesignID(0), design0->getID());
  EXPECT_EQ(SNLName("design"), design0->getName());
  EXPECT_EQ(SNLDesign::Type::Standard, design0->getType());
  EXPECT_TRUE(design0->getParameters().empty());
  EXPECT_EQ(3, design0->getTerms().size());
  EXPECT_EQ(4, design0->getNets().size());
  EXPECT_EQ(2, design0->getInstances().size());
  using Nets = std::vector<SNLNet*>;
  using Terms = std::vector<SNLTerm*>;
  {
    Nets nets(design0->getNets().begin(), design0->getNets().end());
    EXPECT_EQ(4, nets.size());
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

    Terms terms(design0->getTerms().begin(), design0->getTerms().end());
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
  }

  auto design1 = designs[1];
  EXPECT_EQ(SNLID::DesignID(1), design1->getID());
  EXPECT_EQ(SNLName("model"), design1->getName());
  EXPECT_EQ(SNLDesign::Type::Standard, design1->getType());
  EXPECT_EQ(2, design1->getTerms().size());
  {
    EXPECT_EQ(2, design1->getParameters().size());
    using Parameters = std::vector<SNLParameter*>;
    Parameters parameters(design1->getParameters().begin(), design1->getParameters().end());
    EXPECT_EQ(2, parameters.size());
    EXPECT_EQ("Test1", parameters[0]->getName().getString());
    EXPECT_EQ("Value1", parameters[0]->getValue());
    EXPECT_EQ("Test2", parameters[1]->getName().getString());
    EXPECT_EQ("Value2", parameters[1]->getValue());
  }

}