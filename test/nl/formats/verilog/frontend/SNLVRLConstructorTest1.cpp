// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <istream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "NLUniverse.h"

#include "SNLBundleTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLUtils.h"

#define private public
#include "SNLVRLConstructor.h"
#undef private
#include "SNLVRLConstructorException.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*  library_;
};

TEST_F(SNLVRLConstructorTest1, test) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test0.v");
  ASSERT_EQ(3, library_->getSNLDesigns().size());
  auto mod0 = library_->getSNLDesign(NLName("mod0"));
  auto mod1 = library_->getSNLDesign(NLName("mod1"));
  auto test = library_->getSNLDesign(NLName("test")); 
  ASSERT_TRUE(mod0);
  ASSERT_TRUE(test);
  EXPECT_TRUE(mod0->getNets().empty());
  EXPECT_TRUE(test->getNets().empty());
  EXPECT_TRUE(mod0->getInstances().empty());
  EXPECT_TRUE(test->getInstances().empty());

  EXPECT_EQ(2, mod0->getTerms().size());
  auto mod0i0 = mod0->getScalarTerm(NLName("i0"));
  ASSERT_NE(mod0i0, nullptr);
  EXPECT_EQ(mod0i0->getDirection(), SNLTerm::Direction::Input);
  auto mod0o0 = mod0->getScalarTerm(NLName("o0"));
  EXPECT_NE(mod0o0, nullptr);
  EXPECT_EQ(mod0o0->getDirection(), SNLTerm::Direction::Output);
  
  EXPECT_EQ(2, mod1->getTerms().size());
  auto mod1i = mod1->getBusTerm(NLName("i"));
  ASSERT_NE(mod1i, nullptr);
  EXPECT_EQ(mod1i->getDirection(), SNLTerm::Direction::Input);
  EXPECT_EQ(5, mod1i->getWidth());
  EXPECT_EQ(4, mod1i->getMSB());
  EXPECT_EQ(0, mod1i->getLSB());

  {
    EXPECT_EQ(3, test->getTerms().size());
    auto i = test->getTerm(NLName("i"));
    EXPECT_NE(i, nullptr);
    EXPECT_EQ(i->getDirection(), SNLTerm::Direction::Input);
    auto o = test->getTerm(NLName("o"));
    EXPECT_NE(o, nullptr);
    EXPECT_EQ(o->getDirection(), SNLTerm::Direction::Output);
    auto io = test->getTerm(NLName("io"));
    EXPECT_NE(io, nullptr);
    EXPECT_EQ(io->getDirection(), SNLTerm::Direction::InOut);
  }
  
  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test0.v");

  EXPECT_TRUE(mod0->isBlackBox());
  EXPECT_EQ(4, mod0->getNets().size());
  {
    auto i0Net = mod0->getNet(NLName("i0"));
    ASSERT_NE(i0Net, nullptr);
    auto i0ScalarNet = dynamic_cast<SNLScalarNet*>(i0Net);
    EXPECT_EQ(SNLNet::Type::Standard, i0ScalarNet->getType());
    EXPECT_FALSE(i0ScalarNet->getBitTerms().empty());
    EXPECT_EQ(1, i0ScalarNet->getBitTerms().size());
    EXPECT_EQ(mod0i0, *(i0ScalarNet->getBitTerms().begin()));
  }
  EXPECT_TRUE(mod1->isBlackBox());
  EXPECT_EQ(4, mod1->getNets().size());
  
  auto top = SNLUtils::findTop(library_);
  EXPECT_EQ(top, test);
  //2 assign nets + 10 (3 terms) standard nets
  EXPECT_EQ(12, test->getNets().size());
  using Nets = std::vector<SNLNet*>;
  Nets nets(test->getNets().begin(), test->getNets().end());
  ASSERT_EQ(12, nets.size());
  EXPECT_TRUE(nets[0]->isUnnamed());
  EXPECT_TRUE(nets[1]->isUnnamed());
  for (size_t i=2; i<12; ++i) {
    EXPECT_FALSE(nets[i]->isUnnamed());
  }

  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[0]));
  EXPECT_EQ(SNLNet::Type::Assign0, dynamic_cast<SNLScalarNet*>(nets[0])->getType());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[1]));
  EXPECT_EQ(SNLNet::Type::Assign1, dynamic_cast<SNLScalarNet*>(nets[1])->getType());

  EXPECT_EQ("i", nets[2]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[2]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[2])->getType());

  EXPECT_EQ("o", nets[3]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[3]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[3])->getType());

  EXPECT_EQ("io", nets[4]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[4]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[4])->getType());

  EXPECT_EQ("net0", nets[5]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[5]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[5])->getType());

  EXPECT_EQ("net1", nets[6]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[6]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[6])->getType());

  EXPECT_EQ("net2", nets[7]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[7]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[7])->getType());

  EXPECT_EQ("net3", nets[8]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[8]));
  EXPECT_EQ(SNLNet::Type::Standard, dynamic_cast<SNLScalarNet*>(nets[8])->getType());

  EXPECT_EQ("net4", nets[9]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLBusNet*>(nets[9]));
  EXPECT_EQ(3, dynamic_cast<SNLBusNet*>(nets[9])->getMSB());
  EXPECT_EQ(-1, dynamic_cast<SNLBusNet*>(nets[9])->getLSB());
  for (auto bit: dynamic_cast<SNLBusNet*>(nets[9])->getBits()) {
    EXPECT_EQ(SNLNet::Type::Standard, bit->getType());
  }

  EXPECT_EQ("constant0", nets[10]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[10]));
  EXPECT_EQ(SNLNet::Type::Supply0, dynamic_cast<SNLScalarNet*>(nets[10])->getType());

  EXPECT_EQ("constant1", nets[11]->getName().getString());
  ASSERT_TRUE(dynamic_cast<SNLScalarNet*>(nets[11]));
  EXPECT_EQ(SNLNet::Type::Supply1, dynamic_cast<SNLScalarNet*>(nets[11])->getType());

  ASSERT_EQ(4, test->getInstances().size());

  using Instances = std::vector<SNLInstance*>;
  Instances instances(test->getInstances().begin(), test->getInstances().end());
  ASSERT_EQ(4, instances.size());
  EXPECT_EQ("inst0", instances[0]->getName().getString());
  EXPECT_EQ("inst1", instances[1]->getName().getString());
  EXPECT_EQ("inst2", instances[2]->getName().getString());
  EXPECT_EQ("inst3", instances[3]->getName().getString());
  EXPECT_EQ(mod0, instances[0]->getModel());
  EXPECT_EQ(mod0, instances[1]->getModel());
  EXPECT_EQ(mod1, instances[2]->getModel());
  EXPECT_EQ(mod1, instances[3]->getModel());

  auto inst1i0 = instances[1]->getInstTerm(mod0i0);
  ASSERT_NE(nullptr, inst1i0);
  EXPECT_EQ(nets[0], inst1i0->getNet());

  //5'h1A == 5'b11010 connected to inst2.i[4:0] 
  EXPECT_EQ(nets[0], instances[2]->getInstTerm(mod1i->getBit(0))->getNet());
  EXPECT_EQ(nets[1], instances[2]->getInstTerm(mod1i->getBit(1))->getNet());
  EXPECT_EQ(nets[0], instances[2]->getInstTerm(mod1i->getBit(2))->getNet());
  EXPECT_EQ(nets[1], instances[2]->getInstTerm(mod1i->getBit(3))->getNet());
  EXPECT_EQ(nets[1], instances[2]->getInstTerm(mod1i->getBit(4))->getNet());
}

TEST_F(SNLVRLConstructorTest1, testOrderedBundleConnections) {
  auto primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("cell_def"));
  auto ck = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("CK"));
  auto se = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("SE"));
  auto si = SNLScalarTerm::create(primitive, SNLTerm::Direction::Input, NLName("SI"));
  auto d = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  auto d0 = SNLScalarTerm::create(d, SNLTerm::Direction::Input, NLName("D0"));
  auto d1 = SNLScalarTerm::create(d, SNLTerm::Direction::Input, NLName("D1"));
  auto qn = SNLBundleTerm::create(primitive, SNLTerm::Direction::Output, NLName("QN"));
  auto qn0 = SNLScalarTerm::create(qn, SNLTerm::Direction::Output, NLName("QN0"));
  auto qn1 = SNLScalarTerm::create(qn, SNLTerm::Direction::Output, NLName("QN1"));

  auto testPath = std::filesystem::temp_directory_path() / "naja_vrl_ordered_bundle_connections.v";
  {
    std::ofstream stream(testPath);
    stream
      << "module top (clk, se, si, d0, d1, qn0, qn1);\n"
      << "  input clk;\n"
      << "  input se;\n"
      << "  input si;\n"
      << "  input d0;\n"
      << "  input d1;\n"
      << "  output qn0;\n"
      << "  output qn1;\n"
      << "  cell_def u0 (clk, se, si, d0, d1, qn0, qn1);\n"
      << "endmodule\n";
  }

  SNLVRLConstructor constructor(library_);
  constructor.parse(testPath);
  constructor.setFirstPass(false);
  constructor.parse(testPath);

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);
  auto instance = top->getInstance(NLName("u0"));
  ASSERT_NE(nullptr, instance);

  using InstTerms = std::vector<SNLInstTerm*>;
  auto instTerms = InstTerms(instance->getInstTerms().begin(), instance->getInstTerms().end());
  ASSERT_EQ(7, instTerms.size());
  EXPECT_EQ(ck, instTerms[0]->getBitTerm());
  EXPECT_EQ(se, instTerms[1]->getBitTerm());
  EXPECT_EQ(si, instTerms[2]->getBitTerm());
  EXPECT_EQ(d0, instTerms[3]->getBitTerm());
  EXPECT_EQ(d1, instTerms[4]->getBitTerm());
  EXPECT_EQ(qn0, instTerms[5]->getBitTerm());
  EXPECT_EQ(qn1, instTerms[6]->getBitTerm());

  EXPECT_EQ(top->getNet(NLName("clk")), instTerms[0]->getNet());
  EXPECT_EQ(top->getNet(NLName("se")), instTerms[1]->getNet());
  EXPECT_EQ(top->getNet(NLName("si")), instTerms[2]->getNet());
  EXPECT_EQ(top->getNet(NLName("d0")), instTerms[3]->getNet());
  EXPECT_EQ(top->getNet(NLName("d1")), instTerms[4]->getNet());
  EXPECT_EQ(top->getNet(NLName("qn0")), instTerms[5]->getNet());
  EXPECT_EQ(top->getNet(NLName("qn1")), instTerms[6]->getNet());

  std::filesystem::remove(testPath);
}

TEST_F(SNLVRLConstructorTest1, testEscapedPortNameInNamedPortConnection) {
  auto testPath = std::filesystem::temp_directory_path() / "naja_vrl_escaped_named_port_connection.v";
  {
    std::ofstream stream(testPath);
    stream
      << "// Test escaped identifier as port name in named port connection\n"
      << "module submod(\n"
      << "  \\rdata_o[21]_0 ,\n"
      << "  rdata_in\n"
      << ");\n"
      << "  input \\rdata_o[21]_0 ;\n"
      << "  output rdata_in;\n"
      << "endmodule\n"
      << "\n"
      << "module top(\n"
      << "  \\rdata_o[21]_0 ,\n"
      << "  rdata_o\n"
      << ");\n"
      << "  input \\rdata_o[21]_0 ;\n"
      << "  input [31:0] rdata_o;\n"
      << "\n"
      << "  submod inst (\n"
      << "    .\\rdata_o[21]_0 (rdata_o[21]),\n"
      << "    .rdata_in(\\rdata_o[21]_0 )\n"
      << "  );\n"
      << "endmodule\n";
  }

  SNLVRLConstructor constructor(library_);
  constructor.parse(testPath);
  constructor.setFirstPass(false);
  constructor.parse(testPath);

  auto submod = library_->getSNLDesign(NLName("submod"));
  ASSERT_NE(nullptr, submod);
  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);

  auto escapedSubmodTerm = submod->getScalarTerm(NLName("rdata_o[21]_0"));
  ASSERT_NE(nullptr, escapedSubmodTerm);
  EXPECT_EQ(SNLTerm::Direction::Input, escapedSubmodTerm->getDirection());

  auto rdataIn = submod->getScalarTerm(NLName("rdata_in"));
  ASSERT_NE(nullptr, rdataIn);
  EXPECT_EQ(SNLTerm::Direction::Output, rdataIn->getDirection());

  auto escapedTopTerm = top->getScalarTerm(NLName("rdata_o[21]_0"));
  ASSERT_NE(nullptr, escapedTopTerm);
  EXPECT_EQ(SNLTerm::Direction::Input, escapedTopTerm->getDirection());

  auto escapedTopNet = top->getScalarNet(NLName("rdata_o[21]_0"));
  ASSERT_NE(nullptr, escapedTopNet);
  ASSERT_EQ(1, escapedTopNet->getBitTerms().size());
  EXPECT_EQ(escapedTopTerm, *escapedTopNet->getBitTerms().begin());

  auto rdataBus = top->getBusNet(NLName("rdata_o"));
  ASSERT_NE(nullptr, rdataBus);
  EXPECT_EQ(31, rdataBus->getMSB());
  EXPECT_EQ(0, rdataBus->getLSB());

  auto instance = top->getInstance(NLName("inst"));
  ASSERT_NE(nullptr, instance);
  EXPECT_EQ(submod, instance->getModel());

  auto escapedInstTerm = instance->getInstTerm(escapedSubmodTerm);
  ASSERT_NE(nullptr, escapedInstTerm);
  EXPECT_EQ(rdataBus->getBit(21), escapedInstTerm->getNet());

  auto rdataInInstTerm = instance->getInstTerm(rdataIn);
  ASSERT_NE(nullptr, rdataInInstTerm);
  EXPECT_EQ(escapedTopNet, rdataInInstTerm->getNet());

  std::filesystem::remove(testPath);
}

TEST_F(SNLVRLConstructorTest1, testCurrentInstancePortConnectionNullTerm) {
  SNLVRLConstructor constructor(library_);
  constructor.currentPath_ = "synthetic.v";
  constructor.setCurrentLocation(7, 3);

  naja::verilog::Expression expression;
  expression.valid_ = true;
  expression.value_ = naja::verilog::RangeIdentifier(naja::verilog::Identifier("n"));

  try {
    constructor.currentInstancePortConnection(nullptr, expression);
    FAIL();
  } catch (const SNLVRLConstructorException& e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("null term in instance connection"));
  }
}

TEST_F(SNLVRLConstructorTest1, testCurrentInstancePortConnectionBundleTerm) {
  auto primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto primitive = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("cell_def"));
  auto bundle = SNLBundleTerm::create(primitive, SNLTerm::Direction::Input, NLName("D"));
  SNLScalarTerm::create(bundle, SNLTerm::Direction::Input, NLName("D0"));

  SNLVRLConstructor constructor(library_);
  constructor.currentPath_ = "synthetic.v";
  constructor.setCurrentLocation(8, 5);

  naja::verilog::Expression expression;
  expression.valid_ = true;
  expression.value_ = naja::verilog::RangeIdentifier(naja::verilog::Identifier("n"));

  try {
    constructor.currentInstancePortConnection(bundle, expression);
    FAIL();
  } catch (const SNLVRLConstructorException& e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("direct connection to bundle term"));
  }
}

TEST_F(SNLVRLConstructorTest1, testCurrentInstancePortConnectionUnsupportedExpression) {
  auto model = SNLDesign::create(library_, NLName("model"));
  auto term = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("A"));

  SNLVRLConstructor constructor(library_);
  constructor.currentPath_ = "synthetic.v";
  constructor.setCurrentLocation(9, 11);

  naja::verilog::Expression expression;
  expression.valid_ = true;
  expression.supported_ = false;
  expression.value_ = std::string("\"FOO\"");

  try {
    constructor.currentInstancePortConnection(term, expression);
    FAIL();
  } catch (const SNLVRLConstructorException& e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("\"FOO\" is not currently supported"));
  }
}

TEST_F(SNLVRLConstructorTest1, testOrderedInstanceConnectionPortIndexTooLarge) {
  auto model = SNLDesign::create(library_, NLName("model"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("A"));
  auto top = SNLDesign::create(library_, NLName("top"));
  auto instance = SNLInstance::create(top, model, NLName("u0"));

  SNLVRLConstructor constructor(library_);
  constructor.currentPath_ = "synthetic.v";
  constructor.setCurrentLocation(10, 13);
  constructor.setFirstPass(false);
  constructor.currentInstance_ = instance;

  naja::verilog::Expression expression;

  try {
    constructor.addOrderedInstanceConnection(1, expression);
    FAIL();
  } catch (const SNLVRLConstructorException& e) {
    EXPECT_NE(std::string::npos, std::string(e.what()).find("ordered port index 1 exceeds model interface size for model"));
  }
}

TEST_F(SNLVRLConstructorTest1, testMultipleFirstPassError) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test0.v");
  //Will throw NLException for library already containing module
  EXPECT_THROW(
    constructor.parse(benchmarksPath/"test0.v"),
    NLException
  );
}

TEST_F(SNLVRLConstructorTest1, testMultipleSecondPassError) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"test0.v");
  constructor.setFirstPass(false);
  constructor.parse(benchmarksPath/"test0.v");
  EXPECT_THROW(
    constructor.parse(benchmarksPath/"test0.v"),
    SNLVRLConstructorException
  );
}

TEST_F(SNLVRLConstructorTest1, testDirectSecondPassError) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.setFirstPass(false);
  EXPECT_THROW(
    constructor.parse(benchmarksPath/"test0.v"),
    SNLVRLConstructorException
  );
}
