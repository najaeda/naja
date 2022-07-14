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

#include "SNLFBS.h"

using namespace naja::SNL;

#ifndef SNL_FBS_TEST_PATH
#define SNL_FBS_TEST_PATH "Undefined"
#endif

class SNLFBSTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
      SNLDesign* design = SNLDesign::create(library, SNLName("design"));

      SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("i0"));
      SNLBusTerm::create(design, SNLTerm::Direction::Input, 31, 0, SNLName("i1"));
      SNLScalarTerm::create(design, SNLTerm::Direction::Output, SNLName("o"));

      SNLScalarNet::create(design);
      SNLBusNet::create(design, 31, 0);
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

TEST_F(SNLFBSTest0, test0) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_FBS_TEST_PATH);
  outPath = outPath / "test0.snl";
  SNLFBS::dump(db_, outPath);
  db_->destroy();  
  db_ = nullptr;
  db_ = SNLFBS::load(outPath);
  ASSERT_TRUE(db_);
  EXPECT_EQ(SNLID::DBID(1), db_->getID());
  EXPECT_EQ(1, db_->getLibraries().size());
  auto library = *(db_->getLibraries().begin());
  ASSERT_TRUE(library);
  EXPECT_EQ(SNLID::LibraryID(0), library->getID());
  EXPECT_EQ(SNLName("MYLIB"), library->getName());
}