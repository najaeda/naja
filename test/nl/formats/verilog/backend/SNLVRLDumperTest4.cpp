// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLVRLDumper.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

using namespace naja::SNL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif
#ifndef NAJA_DIFF
#define NAJA_DIFF "Undefined"
#endif

//Test large interface designs
class SNLVRLDumperTest4: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
      SNLDesign* design = SNLDesign::create(library, NLName("top"));

      auto term0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("term0"));
      auto term1 = SNLBusTerm::create(design, SNLTerm::Direction::Output, 31, 0, NLName("term1"));
      auto term2 = SNLBusTerm::create(design, SNLTerm::Direction::Input, 1, 1, NLName("term2"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB* db_;
};

TEST_F(SNLVRLDumperTest4, top_terms_same_nets) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);
  top->setName(NLName("top_terms_same_nets"));

  auto net0 = SNLScalarNet::create(top, NLName("term0"));
  auto net1 = SNLBusNet::create(top, 31, 0, NLName("term1"));
  auto net2 = SNLBusNet::create(top, 1, 1, NLName("term2"));
  top->getTerm(NLName("term0"))->setNet(net0);
  top->getTerm(NLName("term1"))->setNet(net1);
  top->getTerm(NLName("term2"))->setNet(net2);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test4_terms_same_nets";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test4" / "top_terms_same_nets.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest4, top_terms_connected_nets) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);
  top->setName(NLName("top_terms_connected_nets"));

  auto net0 = SNLScalarNet::create(top, NLName("net0"));
  auto net1 = SNLBusNet::create(top, 31, 0, NLName("net1"));
  auto net2 = SNLBusNet::create(top, 0, 0, NLName("net2"));
  top->getTerm(NLName("term0"))->setNet(net0);
  top->getTerm(NLName("term1"))->setNet(net1);
  top->getTerm(NLName("term2"))->setNet(net2);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test4_terms_connected_nets";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test4" / "top_terms_connected_nets.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest4, top_terms_erased_nets) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);
  top->setName(NLName("top_terms_erased_nets"));

  auto net0 = SNLScalarNet::create(top, NLName("term0"));
  auto net1 = SNLBusNet::create(top, 31, 0, NLName("term1"));
  auto net2 = SNLBusNet::create(top, 1, 1, NLName("term2"));
  top->getTerm(NLName("term0"))->setNet(net0);
  top->getTerm(NLName("term1"))->setNet(net1);
  top->getTerm(NLName("term2"))->setNet(net2);

  net0->destroy();
  net1->getBit(0)->destroy();
  net1->getBit(15)->destroy();
  net1->getBit(20)->destroy();
  net1->getBit(31)->destroy();
  net2->destroy();

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test4_terms_erased_nets";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
  
  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test4" / "top_terms_erased_nets.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest4, top_terms_connected_and_erased_nets) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);
  top->setName(NLName("top_terms_connected_and_erased_nets"));

  auto net0 = SNLScalarNet::create(top, NLName("net0"));
  auto net1 = SNLBusNet::create(top, 31, 0, NLName("net1"));
  auto net2 = SNLBusNet::create(top, 0, 0, NLName("net2"));
  top->getTerm(NLName("term0"))->setNet(net0);
  top->getTerm(NLName("term1"))->setNet(net1);
  top->getTerm(NLName("term2"))->setNet(net2);

  net0->destroy();
  net1->getBit(0)->destroy();
  net1->getBit(15)->destroy();
  net1->getBit(20)->destroy();
  net1->getBit(31)->destroy();
  net2->destroy();

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test4_terms_connected_and_erased_nets";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);

  outPath = outPath / (top->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "test4" / "top_terms_connected_and_erased_nets.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}
