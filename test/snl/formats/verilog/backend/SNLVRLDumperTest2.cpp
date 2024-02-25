// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLDB0.h"

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

//Test assigns
class SNLVRLDumperTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      SNLLibrary* library = SNLLibrary::create(db_, SNLName("MYLIB"));
      SNLDesign* top = SNLDesign::create(library, SNLName("top"));

      auto const0 = SNLScalarNet::create(top);
      const0->setType(SNLNet::Type::Assign0);
      auto const1 = SNLScalarNet::create(top);
      const1->setType(SNLNet::Type::Assign1);
      auto bus = SNLBusNet::create(top, -2, 2, SNLName("bus"));
      auto scalar = SNLScalarNet::create(top, SNLName("scalar"));

      auto assign0 = SNLInstance::create(top, SNLDB0::getAssign());
      auto assign1 = SNLInstance::create(top, SNLDB0::getAssign());
      auto assign2 = SNLInstance::create(top, SNLDB0::getAssign());
      auto assign3 = SNLInstance::create(top, SNLDB0::getAssign());
      auto assign4 = SNLInstance::create(top, SNLDB0::getAssign());

      assign0->getInstTerm(SNLDB0::getAssignInput())->setNet(const0);
      assign0->getInstTerm(SNLDB0::getAssignOutput())->setNet(bus->getBit(-2));
      assign1->getInstTerm(SNLDB0::getAssignInput())->setNet(scalar);
      assign1->getInstTerm(SNLDB0::getAssignOutput())->setNet(bus->getBit(-1));
      assign2->getInstTerm(SNLDB0::getAssignInput())->setNet(const1);
      assign2->getInstTerm(SNLDB0::getAssignOutput())->setNet(bus->getBit(0));
      assign3->getInstTerm(SNLDB0::getAssignInput())->setNet(scalar);
      assign3->getInstTerm(SNLDB0::getAssignOutput())->setNet(bus->getBit(1));
      assign4->getInstTerm(SNLDB0::getAssignInput())->setNet(const0);
      assign4->getInstTerm(SNLDB0::getAssignOutput())->setNet(bus->getBit(2));

    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
  protected:
    SNLDB*      db_;
};

TEST_F(SNLVRLDumperTest2, test0) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("top"));
  ASSERT_TRUE(top);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2Test0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2Test0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = "diff " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest2, testUnconnectedAssigns) {
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("top"));
  ASSERT_TRUE(top);

  //Destroy net: scalar
  auto scalar = top->getScalarNet(SNLName("scalar"));
  ASSERT_TRUE(scalar);
  scalar->destroy();

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test2TestUnconnectedAssigns";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test2TestUnconnectedAssigns" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = "diff " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}