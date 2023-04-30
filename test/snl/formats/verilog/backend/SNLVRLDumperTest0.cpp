#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLInstTerm.h"

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

class SNLVRLDumperTest0: public ::testing::Test {
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
      SNLScalarNet::create(design, SNLName("n2"));

      SNLDesign* model = SNLDesign::create(library, SNLName("model"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Input, SNLName("i"));
      SNLScalarTerm::create(model, SNLTerm::Direction::Output, SNLName("o"));
      SNLScalarTerm::create(model, SNLTerm::Direction::InOut, SNLName("io"));
      SNLInstance* instance1 = SNLInstance::create(design, model, SNLName("instance1"));
      SNLInstance* instance2 = SNLInstance::create(design, model, SNLName("instance2"));

      //connections between instances
      instance1->getInstTerm(model->getScalarTerm(SNLName("o")))->setNet(design->getScalarNet(SNLName("n1")));
      instance2->getInstTerm(model->getScalarTerm(SNLName("i")))->setNet(design->getScalarNet(SNLName("n1")));
      instance1->getInstTerm(model->getScalarTerm(SNLName("io")))->setNet(design->getScalarNet(SNLName("n2")));
      instance2->getInstTerm(model->getScalarTerm(SNLName("io")))->setNet(design->getScalarNet(SNLName("n2")));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDB*      db_;
};

TEST_F(SNLVRLDumperTest0, testNonExistingPath) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "ERROR";
  SNLVRLDumper dumper;
  ASSERT_THROW(dumper.dumpDesign(top, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTest0, testSingleFile) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test0SingleFile";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test0SingleFile" / "design.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = "diff " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest0, testMultipleFiles) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test0MultipleFiles";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setSingleFile(false);
  dumper.dumpDesign(top, outPath);
}