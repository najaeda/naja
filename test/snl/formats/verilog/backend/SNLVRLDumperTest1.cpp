// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
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
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

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

//Model with Bus terminal
class SNLVRLDumperTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
      SNLDesign* top = SNLDesign::create(library, NLName("top"));

      auto bus0 = SNLBusNet::create(top, 2, -2, NLName("bus0"));
      auto bus1 = SNLBusNet::create(top, -2, 2, NLName("bus1"));
      auto bus2 = SNLBusNet::create(top, 2, -2, NLName("bus2"));
      auto bus3 = SNLBusNet::create(top, -2, 2, NLName("bus3"));

      SNLDesign* model = SNLDesign::create(library, NLName("model"));
      SNLBusTerm::create(model, SNLTerm::Direction::Input, 2, -2, NLName("i0"));
      SNLBusTerm::create(model, SNLTerm::Direction::Input, -2, 2, NLName("i1"));
      SNLBusTerm::create(model, SNLTerm::Direction::Output, 2, -2, NLName("o0"));
      SNLBusTerm::create(model, SNLTerm::Direction::Output, -2, 2, NLName("o1"));
      SNLParameter::create(model, NLName("PARAM0"), SNLParameter::Type::String, "0000");
      SNLParameter::create(model, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
      SNLParameter::create(model, NLName("PARAM2"), SNLParameter::Type::Binary, "4h'0");
      SNLParameter::create(model, NLName("PARAM3"), SNLParameter::Type::Decimal, "10");

      SNLInstance* instance1 = SNLInstance::create(top, model, NLName("instance1"));
      SNLInstance* instance2 = SNLInstance::create(top, model, NLName("instance2"));

      //connections between instances
      //instance1->getInstTerm(model->getScalarTerm(NLName("o")))->setNet(design->getScalarNet(NLName("n1")));
      //instance2->getInstTerm(model->getScalarTerm(NLName("i")))->setNet(design->getScalarNet(NLName("n1")));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    NLDB* db_;
};

TEST_F(SNLVRLDumperTest1, test0) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);

  SNLInstance* instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  SNLInstance* instance2 = top->getInstance(NLName("instance2"));
  ASSERT_TRUE(instance2);
  SNLNet* bus0 = top->getNet(NLName("bus0"));
  ASSERT_TRUE(bus0);
  SNLNet* bus1 = top->getNet(NLName("bus1"));
  ASSERT_TRUE(bus1);

  //connect instance1.o0 to instance2.i0 with bus0
  instance1->setTermNet(instance1->getModel()->getTerm(NLName("o0")), bus0);
  instance2->setTermNet(instance2->getModel()->getTerm(NLName("i0")), bus0);

  //connect instance1.o1 to instance2.i1 with bus1
  instance1->setTermNet(instance1->getModel()->getTerm(NLName("o1")), bus1);
  instance2->setTermNet(instance2->getModel()->getTerm(NLName("i1")), bus1);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test1Test0";
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
  referencePath = referencePath / "test1Test0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTest1, test1) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);

  SNLInstance* instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  SNLInstance* instance2 = top->getInstance(NLName("instance2"));
  ASSERT_TRUE(instance2);
  SNLNet* bus0 = top->getNet(NLName("bus0"));
  ASSERT_TRUE(bus0);
  SNLNet* bus1 = top->getNet(NLName("bus1"));
  ASSERT_TRUE(bus1);

  //connect instance1.o0 to instance2.i0 with bus0
  instance1->setTermNet(instance1->getModel()->getTerm(NLName("o1")), bus0);
  instance2->setTermNet(instance2->getModel()->getTerm(NLName("i1")), bus0);

  //connect instance1.o1 to instance2.i1 with bus1
  instance1->setTermNet(instance1->getModel()->getTerm(NLName("o0")), bus1);
  instance2->setTermNet(instance2->getModel()->getTerm(NLName("i0")), bus1);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test1Test1";
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
  referencePath = referencePath / "test1Test1" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

//split busses in 2
TEST_F(SNLVRLDumperTest1, test2) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);

  SNLInstance* instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  SNLInstance* instance2 = top->getInstance(NLName("instance2"));
  ASSERT_TRUE(instance2);
  SNLNet* bus0 = top->getNet(NLName("bus0"));
  ASSERT_TRUE(bus0);
  SNLNet* bus2 = top->getNet(NLName("bus2"));
  ASSERT_TRUE(bus2);

  //connect instance1.o0 to instance2.i0 with bus0
  //instance1->getInstTerm(instance1->getModel()->getBusTerm());
  instance1->setTermNet(instance1->getModel()->getTerm(NLName("o0")), 2, 0, bus0, 2, 0);
  instance1->setTermNet(instance1->getModel()->getTerm(NLName("o0")), -1, -2, bus2, -1, -2);
  instance2->setTermNet(instance2->getModel()->getTerm(NLName("i0")), 2, 0, bus0, 2, 0);
  instance2->setTermNet(instance2->getModel()->getTerm(NLName("i0")), -1, -2, bus2, -1, -2);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test1Test2";
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
  referencePath = referencePath / "test1Test2" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

//connect with scalars concatenation
TEST_F(SNLVRLDumperTest1, test3) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);

  SNLInstance* instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  SNLInstance* instance2 = top->getInstance(NLName("instance2"));
  ASSERT_NE(nullptr, instance2);
  using Nets = std::vector<SNLBitNet*>;
  Nets nets;
  for (int i=0; i<5; ++i) {
    std::string netName = "net_" + std::to_string(i);
    nets.push_back(SNLScalarNet::create(top, NLName(netName)));
  }
  using Terms = std::vector<SNLBitTerm*>;
  auto model = instance1->getModel();
  auto o0BusTerm = model->getBusTerm(NLName("o0"));
  ASSERT_NE(nullptr, o0BusTerm);
  Terms terms(o0BusTerm->getBits().begin(), o0BusTerm->getBits().end());
  instance1->setTermsNets(terms, nets);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test1Test3";
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
  referencePath = referencePath / "test1Test3" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

//mix scalars with bus subrange
TEST_F(SNLVRLDumperTest1, test4) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);
  SNLBusNet* bus0 = top->getBusNet(NLName("bus0"));
  ASSERT_TRUE(bus0);

  SNLInstance* instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  SNLInstance* instance2 = top->getInstance(NLName("instance2"));
  ASSERT_NE(nullptr, instance2);
  using Nets = std::vector<SNLBitNet*>;
  Nets nets;
  nets.push_back(SNLScalarNet::create(top, NLName("net_0")));
  nets.push_back(bus0->getBit(-2));
  nets.push_back(SNLScalarNet::create(top, NLName("net_2")));
  nets.push_back(bus0->getBit(-1));
  nets.push_back(bus0->getBit(0));

  using Terms = std::vector<SNLBitTerm*>;
  auto model = instance1->getModel();
  auto o0BusTerm = model->getBusTerm(NLName("o0"));
  ASSERT_NE(nullptr, o0BusTerm);
  Terms terms(o0BusTerm->getBits().begin(), o0BusTerm->getBits().end());
  instance1->setTermsNets(terms, nets);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test1Test4";
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
  referencePath = referencePath / "test1Test4" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

//mix scalars with bus subrange and holes
TEST_F(SNLVRLDumperTest1, test5) {
  auto lib = db_->getLibrary(NLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(NLName("top"));
  ASSERT_TRUE(top);
  SNLBusNet* bus0 = top->getBusNet(NLName("bus0"));
  ASSERT_TRUE(bus0);

  SNLInstance* instance1 = top->getInstance(NLName("instance1"));
  ASSERT_TRUE(instance1);
  SNLInstance* instance2 = top->getInstance(NLName("instance2"));
  ASSERT_NE(nullptr, instance2);

  auto model = instance1->getModel();
  auto o0BusTerm = model->getBusTerm(NLName("o0"));
  ASSERT_NE(nullptr, o0BusTerm);
  instance1->setTermNet(o0BusTerm, -2, -2, SNLScalarNet::create(top, NLName("net_0")), 0, 0);
  instance1->setTermNet(o0BusTerm, 1, 2, bus0, -1, 0);

  auto i0BusTerm = model->getBusTerm(NLName("i0"));
  ASSERT_NE(nullptr, i0BusTerm);
  auto assign0Bus = SNLBusNet::create(top, 4, 0);
  assign0Bus->setType(naja::SNL::SNLNet::Type::Assign0);
  instance1->setTermNet(i0BusTerm, assign0Bus);
  
  auto param0 = instance1->getModel()->getParameter(NLName("PARAM0"));
  auto param1 = instance1->getModel()->getParameter(NLName("PARAM1"));
  auto param2 = instance1->getModel()->getParameter(NLName("PARAM2"));
  auto param3 = instance1->getModel()->getParameter(NLName("PARAM3"));
  ASSERT_NE(param0, nullptr);
  ASSERT_NE(param1, nullptr);
  ASSERT_NE(param2, nullptr);
  ASSERT_NE(param3, nullptr);
  SNLInstParameter::create(instance1, param0, "1111");
  SNLInstParameter::create(instance1, param1, "1");
  SNLInstParameter::create(instance1, param2, "4h'F");
  SNLInstParameter::create(instance1, param3, "152");

  SNLInstance::Nets nets;
  {
    auto n0 = SNLScalarNet::create(top);
    n0->setType(naja::SNL::SNLNet::Type::Assign1);
    auto n1 = SNLScalarNet::create(top);
    n1->setType(naja::SNL::SNLNet::Type::Assign0);
    auto n2 = SNLScalarNet::create(top);
    n2->setType(naja::SNL::SNLNet::Type::Assign0);
    auto n3 = SNLScalarNet::create(top);
    n3->setType(naja::SNL::SNLNet::Type::Assign1);
    auto n4 = SNLScalarNet::create(top);
    n4->setType(naja::SNL::SNLNet::Type::Assign1);
    nets = {n0, n1, n2, n3, n4};
  }
  SNLInstance::Terms terms(i0BusTerm->getBits().begin(), i0BusTerm->getBits().end()); 
  instance2->setTermsNets(terms, nets);

  terms.clear();
  nets.clear();
  auto i1BusTerm = model->getBusTerm(NLName("i1"));
  ASSERT_NE(nullptr, i1BusTerm);
  terms = SNLInstance::Terms(i1BusTerm->getBits().begin(), i1BusTerm->getBits().end());
  {
    auto n0 = SNLScalarNet::create(top);
    n0->setType(naja::SNL::SNLNet::Type::Assign0);
    auto n1 = SNLScalarNet::create(top);
    n1->setType(naja::SNL::SNLNet::Type::Assign1);
    auto n2 = SNLScalarNet::create(top, NLName("n2"));
    auto n3 = SNLScalarNet::create(top, NLName("n3"));
    auto n4 = SNLScalarNet::create(top, NLName("n4"));
    nets = {n0, n1, n2, n3, n4};
  }
  instance2->setTermsNets(terms, nets);

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "test1Test5";
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
  referencePath = referencePath / "test1Test5" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}
