#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
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
  EXPECT_EQ(3, design0->getTerms().size());
  using Terms = std::vector<SNLTerm*>;
  {
    Terms terms(design0->getTerms().begin(), design0->getTerms().end());
    EXPECT_EQ(3, terms.size());
    EXPECT_NE(nullptr, terms[0]);
    auto scalarTerm0 = dynamic_cast<SNLScalarTerm*>(terms[0]);
    EXPECT_NE(nullptr, scalarTerm0);
    EXPECT_EQ(SNLID::DesignObjectID(0), scalarTerm0->getID());
    EXPECT_EQ(SNLName("i"), scalarTerm0->getName());
    EXPECT_EQ(SNLTerm::Direction::Input, scalarTerm0->getDirection());

    EXPECT_NE(nullptr, terms[1]);
    auto busTerm1 = dynamic_cast<SNLBusTerm*>(terms[1]);
    EXPECT_NE(nullptr, busTerm1);
    EXPECT_EQ(SNLID::DesignObjectID(1), busTerm1->getID());
    EXPECT_EQ(SNLName("o1"), busTerm1->getName());
    EXPECT_EQ(SNLTerm::Direction::Output, busTerm1->getDirection());

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
}