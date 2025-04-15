// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NetlistGraph.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "NLLibraryTruthTables.h"
#include "SNLDesignTruthTable.h"
#include "ConstantPropagation.h"

using namespace naja;
using namespace naja::NL;
using namespace naja::NAJA_OPT;

namespace {

void executeCommand(const std::string& command) {
  int result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Command execution failed." << std::endl;
  }
}

}

class ConstantPropagationTests : public ::testing::Test {
 protected:
  ConstantPropagationTests() {
    // You can do set-up work for each test here
  }
  ~ConstantPropagationTests() override {
    // You can do clean-up work that doesn't throw exceptions here
  }
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }
  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    // Destroy the SNL
    NLUniverse::get()->destroy();
  }
};

// Test constat propagation
// 1. Create SNL
// 2. Create a top model with one output
// 3. create a logic_0 model
// 4. create a logic_1 model
// 5. create a logic_0 instace in top
// 6. create a logic_1 instace in top
// 7. create a and model
// 8. create a and instance in top
// 9. connect the logic_0 and logic_1 to the and instance
// 10. connect the and instance output to the top output
// 11. create DNL
// 12. create a constant propagation object
// 13. collect the constants
// 14. run the constant propagation
// 15. check the output value of the top instance
TEST_F(ConstantPropagationTests, TestConstantPropagation) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  // need to create additional top outputs for every logic gate (EILON)
  
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  
  // 5. create a logic_0 instace in top
  SNLInstance* instlogic0 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* instlogic1 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // 7. create a and model for or
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to and
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::Output,
                                      NLName("out"));  
  // 7. create a and model for xor
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to and
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nand
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to and
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nor
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to and
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for xnor
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to and
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for inv
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to and
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for buf
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to and
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for ha
  SNLDesign* haModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("HA"));

  // add 2 inputs and 2 outputs to and
  auto haIn1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto haIn2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto haOut1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out1")); 
  auto haOut2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out2"));   
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 7. create a and model for oai  
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));

  // create a and instance in top
  SNLInstance* instand = SNLInstance::create(top, andModel, NLName("and"));
  // create a or instance in top
  SNLInstance* instor = SNLInstance::create(top, orModel, NLName("or"));
  // create a xor instance in top
  SNLInstance* instxor = SNLInstance::create(top, xorModel, NLName("xor"));
  // create a nand instance in top
  SNLInstance* instnand = SNLInstance::create(top, nandModel, NLName("nand"));
  // create a nor instance in top
  SNLInstance* instnor = SNLInstance::create(top, norModel, NLName("nor"));
  // create a xnor instance in top
  SNLInstance* instxnor = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // create a inv instance in top
  SNLInstance* instinv = SNLInstance::create(top, invModel, NLName("inv"));
  // create a buf instance in top
  SNLInstance* instbuf = SNLInstance::create(top, bufModel, NLName("buf"));
  // create a ha instance in top
  SNLInstance* instha = SNLInstance::create(top, haModel, NLName("ha"));
  // create a dff instance in top
  SNLInstance* instdff = SNLInstance::create(top, dffModel, NLName("dff"));
  // create a mux instance in top
  SNLInstance* instmux = SNLInstance::create(top, muxModel, NLName("mux"));
  // create a oai instance in top
  SNLInstance* instoai = SNLInstance::create(top, oaiModel, NLName("oai"));

  // 9. connect all instances inputs 
  SNLNet* netlogic0 = SNLScalarNet::create(top, NLName("logic_0_net"));
  netlogic0->setType(SNLNet::Type::Assign0);
  SNLNet* netlogic1 = SNLScalarNet::create(top, NLName("logic_1_net"));
  netlogic1->setType(SNLNet::Type::Assign1);
  SNLNet* netand = SNLScalarNet::create(top, NLName("and_output_net"));
  // connect logic0 to and
  instlogic0->getInstTerm(logic0Out)->setNet(netlogic0);
  instand->getInstTerm(andIn1)->setNet(netlogic0);
  instor->getInstTerm(orIn1)->setNet(netlogic0);
  instxor->getInstTerm(xorIn1)->setNet(netlogic0);
  instnand->getInstTerm(nandIn1)->setNet(netlogic0);
  instnor->getInstTerm(norIn1)->setNet(netlogic0);
  instxnor->getInstTerm(xnorIn1)->setNet(netlogic0);
  instinv->getInstTerm(invIn)->setNet(netlogic0);
  instbuf->getInstTerm(bufIn)->setNet(netlogic0);
  instha->getInstTerm(haIn1)->setNet(netlogic0);
  instdff->getInstTerm(dffD)->setNet(netlogic0);
  instmux->getInstTerm(muxS)->setNet(netlogic0);
  instoai->getInstTerm(oaiA)->setNet(netlogic0);

  // connect logic1 to all gate inputs
  instlogic1->getInstTerm(logic1Out)->setNet(netlogic1);
  instand->getInstTerm(andIn2)->setNet(netlogic1);
  instor->getInstTerm(orIn2)->setNet(netlogic1);
  instxor->getInstTerm(xorIn2)->setNet(netlogic1);
  instnand->getInstTerm(nandIn2)->setNet(netlogic1);
  instnor->getInstTerm(norIn2)->setNet(netlogic1);
  instxnor->getInstTerm(xnorIn2)->setNet(netlogic1);
  instinv->getInstTerm(invIn)->setNet(netlogic1);
  instbuf->getInstTerm(bufIn)->setNet(netlogic1);
  instha->getInstTerm(haIn2)->setNet(netlogic1);
  instdff->getInstTerm(dffCLK)->setNet(netlogic1);
  instmux->getInstTerm(muxA)->setNet(netlogic1);
  instmux->getInstTerm(muxB)->setNet(netlogic1);
  instoai->getInstTerm(oaiB1)->setNet(netlogic1);
  instoai->getInstTerm(oaiB2)->setNet(netlogic1);

  // 10. connect the and instance output to the top output
  instand->getInstTerm(andOut)->setNet(netand);
  topOut->setNet(netand);
  //inst4->getInstTerm(orOut)->setNet(net4);
  //topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setTruthTableEngine(true);   
  // 13. collect the constants
  cp.collectConstants();
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}

TEST_F(ConstantPropagationTests, TestConstantPropagationNonBNE) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // 7. create a and model for or
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to and
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::Output,
                                      NLName("out"));  
  // 7. create a and model for xor
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to and
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nand
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to and
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for nor
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to and
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for xnor
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to and
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for inv
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to and
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for buf
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to and
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output,
                                      NLName("out"));
  // 7. create a and model for ha
  SNLDesign* haModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("HA"));

  // add 2 inputs and 2 outputs to and
  auto haIn1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto haIn2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto haOut1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out1")); 
  auto haOut2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      NLName("out2"));   
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 7. create a and model for oai  
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));

  // create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, NLName("and"));
  // create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, NLName("or"));
  // create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, NLName("xor"));
  // create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, NLName("nand"));
  // create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, NLName("nor"));
  // create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, NLName("inv"));
  // create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, NLName("buf"));
  // create a ha instance in top
  SNLInstance* inst11 = SNLInstance::create(top, haModel, NLName("ha"));
  // create a dff instance in top
  SNLInstance* inst12 = SNLInstance::create(top, dffModel, NLName("dff"));
  // create a mux instance in top
  SNLInstance* inst13 = SNLInstance::create(top, muxModel, NLName("mux"));
  // create a oai instance in top
  SNLInstance* inst14 = SNLInstance::create(top, oaiModel, NLName("oai"));

  // 9. connect all instances inputs 
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("and_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net1);
  inst4->getInstTerm(orIn1)->setNet(net1);
  inst5->getInstTerm(xorIn1)->setNet(net1);
  inst6->getInstTerm(nandIn1)->setNet(net1);
  inst7->getInstTerm(norIn1)->setNet(net1);
  inst8->getInstTerm(xnorIn1)->setNet(net1);
  inst9->getInstTerm(invIn)->setNet(net1);
  inst10->getInstTerm(bufIn)->setNet(net1);
  inst11->getInstTerm(haIn1)->setNet(net1);
  inst12->getInstTerm(dffD)->setNet(net1);
  inst13->getInstTerm(muxS)->setNet(net1);
  inst14->getInstTerm(oaiA)->setNet(net1);

  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net2);
  inst4->getInstTerm(orIn2)->setNet(net2);
  inst5->getInstTerm(xorIn2)->setNet(net2);
  inst6->getInstTerm(nandIn2)->setNet(net2);
  inst7->getInstTerm(norIn2)->setNet(net2);
  inst8->getInstTerm(xnorIn2)->setNet(net2);
  inst9->getInstTerm(invIn)->setNet(net2);
  inst10->getInstTerm(bufIn)->setNet(net2);
  inst11->getInstTerm(haIn2)->setNet(net2);
  inst12->getInstTerm(dffCLK)->setNet(net2);
  inst13->getInstTerm(muxA)->setNet(net2);
  inst13->getInstTerm(muxB)->setNet(net2);
  inst14->getInstTerm(oaiB1)->setNet(net2);
  inst14->getInstTerm(oaiB2)->setNet(net2);

  // 10. connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  cp.collectConstants();
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 1);
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  //EXPECT_EQ(cp.getConstants1().size(), 1);
  //EXPECT_EQ(cp.getConstants0().size(), 2);
  //EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  //EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}

//Seperate the test case above for tests for each type in the constant propagation code

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(top, andModel, NLName("and2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("and2_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  topOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for OR
TEST_F(ConstantPropagationTests, TestConstantPropagationOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a or model
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to or
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, NLName("or"));
  SNLInstance* inst5 = SNLInstance::create(top, orModel, NLName("or2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("or_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("or2_output_net"));
  // connect logic0 to or
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst5->getInstTerm(orIn1)->setNet(net1);
  inst5->getInstTerm(orIn2)->setNet(net1);
  // connect logic1 to or
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst4->getInstTerm(orIn2)->setNet(net2);
  inst4->getInstTerm(orIn1)->setNet(net4);
  // connect the or instance output to the top output
  inst4->getInstTerm(orOut)->setNet(net3);
  topOut->setNet(net3);
  inst5->getInstTerm(orOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XOR
TEST_F(ConstantPropagationTests, TestConstantPropagationXOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xor model
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to xor
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, NLName("xor"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xor_output_net"));
  // connect logic0 to xor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst5->getInstTerm(xorIn1)->setNet(net1);
  // connect logic1 to xor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst5->getInstTerm(xorIn2)->setNet(net2);
  // connect the xor instance output to the top output
  inst5->getInstTerm(xorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NAND
TEST_F(ConstantPropagationTests, TestConstantPropagationNAND) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nand model
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to nand
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nand_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("nand2_output_net"));
  // 8. create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, NLName("nand"));
  SNLInstance* inst7 = SNLInstance::create(top, nandModel, NLName("nand2"));
  // connect logic0 to nand
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst6->getInstTerm(nandIn1)->setNet(net1);
  // connect logic1 to nand
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst6->getInstTerm(nandIn2)->setNet(net2);
  inst7->getInstTerm(nandIn1)->setNet(net2);
  inst7->getInstTerm(nandIn2)->setNet(net2);
  // connect the nand instance output to the top output
  inst6->getInstTerm(nandOut)->setNet(net3);
  topOut->setNet(net3);
  inst7->getInstTerm(nandOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NOR
TEST_F(ConstantPropagationTests, TestConstantPropagationNOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nor model
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to nor
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output, NLName("out"));  
  // 8. create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, NLName("nor"));
  SNLInstance* inst8 = SNLInstance::create(top, norModel, NLName("nor2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("nor2_output_net"));
  // connect logic0 to nor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst7->getInstTerm(norIn1)->setNet(net1);
  inst8->getInstTerm(norIn1)->setNet(net1);
  inst8->getInstTerm(norIn2)->setNet(net1);
  // connect logic1 to nor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst7->getInstTerm(norIn2)->setNet(net2);
  // connect the nor instance output to the top output
  inst7->getInstTerm(norOut)->setNet(net3);
  topOut->setNet(net3);
  inst8->getInstTerm(norOut)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XNOR
TEST_F(ConstantPropagationTests, TestConstantPropagationXNOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xnor model
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to xnor
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xnor_output_net"));
  // 8. create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // connect logic0 to xnor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst8->getInstTerm(xnorIn1)->setNet(net1);
  // connect logic1 to xnor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst8->getInstTerm(xnorIn2)->setNet(net2);
  // connect the xnor instance output to the top output
  inst8->getInstTerm(xnorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for INV
TEST_F(ConstantPropagationTests, TestConstantPropagationINV) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a inv model
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to inv
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, NLName("inv"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));    
  SNLNet* net3 = SNLScalarNet::create(top, NLName("inv_output_net"));
  // connect logic0 to inv
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst9->getInstTerm(invIn)->setNet(net1);
  // connect the inv instance output to the top output
  inst9->getInstTerm(invOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationBUF) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to buf
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, NLName("buf"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));
  SNLNet* net3 = SNLScalarNet::create(top, NLName("buf_output_net"));
  // connect logic0 to buf
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst10->getInstTerm(bufIn)->setNet(net1);
  // connect the buf instance output to the top output
  inst10->getInstTerm(bufOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationMUX) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, muxModel, NLName("mux"));
  SNLInstance* inst12 = SNLInstance::create(top, muxModel, NLName("mux2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("mux2_output_net"));
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(muxA)->setNet(net1);
  inst12->getInstTerm(muxB)->setNet(net1);
  inst12->getInstTerm(muxS)->setNet(net1);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(muxB)->setNet(net2);
  inst12->getInstTerm(muxA)->setNet(net2);
  inst11->getInstTerm(muxS)->setNet(net2);
  // connect the mux instance output to the top output
  inst11->getInstTerm(muxY)->setNet(net3);
  topOut->setNet(net3);
  inst12->getInstTerm(muxY)->setNet(net4);
  topOut2->setNet(net4);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationDFF) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, dffModel, NLName("dff"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(dffD)->setNet(net1);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(dffCLK)->setNet(net2);
  // connect the mux instance output to the top output
  inst11->getInstTerm(dffQ)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationOAI) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(oaiB1)->setNet(net2);
  inst11->getInstTerm(oaiB2)->setNet(net2);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

TEST_F(ConstantPropagationTests, TestConstantPropagationNonDefinedModel) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NonDeff"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(oaiB1)->setNet(net2);
  inst11->getInstTerm(oaiB2)->setNet(net2);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialAND) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("AND"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(top, andModel, NLName("and2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("and_output_net2"));
  topIn->setNet(net4);
  topOut2->setNet(net5);
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst4->getInstTerm(andIn1)->setNet(net4);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net2);
  inst4->getInstTerm(andIn1)->setNet(net1);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for OR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a or model
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OR"));
  // add 2 inputs and 1 output to or
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, NLName("or"));
  SNLInstance* inst5 = SNLInstance::create(top, orModel, NLName("or2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("or_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("or2_output_net"));
  topIn->setNet(net4);
  // connect logic0 to or
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst4->getInstTerm(orIn1)->setNet(net1);
  // connect logic1 to or
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst5->getInstTerm(orIn1)->setNet(net2);
  inst4->getInstTerm(orIn2)->setNet(net4);
  inst5->getInstTerm(orIn2)->setNet(net4);
  // connect the or instance output to the top output
  inst4->getInstTerm(orOut)->setNet(net3);
  topOut->setNet(net3);
  inst5->getInstTerm(orOut)->setNet(net5);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialXOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xor model
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XOR"));
  // add 2 inputs and 1 output to xor
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, NLName("xor"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to xor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst5->getInstTerm(xorIn1)->setNet(net1);
  // connect logic1 to xor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst5->getInstTerm(xorIn2)->setNet(net4);
  // connect the xor instance output to the top output
  inst5->getInstTerm(xorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NAND
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNAND) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nand model
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NAND"));
  // add 2 inputs and 1 output to nand
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nand_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("nand2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // 8. create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, NLName("nand"));
  SNLInstance* inst7 = SNLInstance::create(top, nandModel, NLName("nand2"));
  // connect logic0 to nand
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst6->getInstTerm(nandIn1)->setNet(net1);
  // connect logic1 to nand
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst7->getInstTerm(nandIn1)->setNet(net2);
  inst6->getInstTerm(nandIn2)->setNet(net4);
  inst7->getInstTerm(nandIn2)->setNet(net4);
  // connect the nand instance output to the top output
  inst6->getInstTerm(nandOut)->setNet(net3);
  topOut->setNet(net3);
  inst7->getInstTerm(nandOut)->setNet(net5);
  topOut->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a nor model
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NOR"));
  // add 2 inputs and 1 output to nor
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output, NLName("out"));  
  // 8. create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, NLName("nor"));
  SNLInstance* inst8 = SNLInstance::create(top, norModel, NLName("nor2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("nor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("nor2_output_net"));
  topIn->setNet(net4);
  // connect logic0 to nor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst7->getInstTerm(norIn1)->setNet(net1);
  // connect logic1 to nor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst7->getInstTerm(norIn2)->setNet(net4);
  // connect the nor instance output to the top output
  inst7->getInstTerm(norOut)->setNet(net3);
  topOut->setNet(net3);

  inst8->getInstTerm(norIn1)->setNet(net2);
  inst8->getInstTerm(norIn2)->setNet(net4);
  inst8->getInstTerm(norOut)->setNet(net5);
  topOut2->setNet(net5);

  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XNOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialXNOR) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a xnor model
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("XNOR"));
  // add 2 inputs and 1 output to xnor
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output, NLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("xnor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // 8. create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, NLName("xnor"));
  // connect logic0 to xnor
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst8->getInstTerm(xnorIn1)->setNet(net1);
  // connect logic1 to xnor
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst8->getInstTerm(xnorIn2)->setNet(net4);
  // connect the xnor instance output to the top output
  inst8->getInstTerm(xnorOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for INV
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialINV) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a inv model
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("INV"));
  // add 1 input and 1 output to inv
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, NLName("inv"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));    
  SNLNet* net3 = SNLScalarNet::create(top, NLName("inv_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to inv
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst9->getInstTerm(invIn)->setNet(net4);
  // connect the inv instance output to the top output
  inst9->getInstTerm(invOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialBUF) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("BUF"));
  // add 1 input and 1 output to buf
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      NLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output, NLName("out"));
  // 8. create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, NLName("buf"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , NLName("logic_0_net"));
  SNLNet* net3 = SNLScalarNet::create(top, NLName("buf_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to buf
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst10->getInstTerm(bufIn)->setNet(net4);
  // connect the buf instance output to the top output
  inst10->getInstTerm(bufOut)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialMUX) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));    
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      NLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      NLName("Y"));  
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, muxModel, NLName("mux"));
  SNLInstance* inst12 = SNLInstance::create(top, muxModel, NLName("mux2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("mux2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(muxA)->setNet(net4);
  inst12->getInstTerm(muxA)->setNet(net1);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(muxB)->setNet(net2);
  inst12->getInstTerm(muxB)->setNet(net4);

  inst11->getInstTerm(muxS)->setNet(net2);
  inst11->getInstTerm(muxS)->setNet(net2);
  // connect the mux instance output to the top output
  inst11->getInstTerm(muxY)->setNet(net3);
  topOut->setNet(net3);
  inst12->getInstTerm(muxY)->setNet(net5);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialDFF) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      NLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      NLName("Q"));    
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, dffModel, NLName("dff1"));
  SNLInstance* inst12 = SNLInstance::create(top, dffModel, NLName("dff2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("dff_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("dff2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to mux
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(dffCLK)->setNet(net4);
  inst12->getInstTerm(dffCLK)->setNet(net4);
  // connect logic1 to mux
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(dffD)->setNet(net2);
  inst12->getInstTerm(dffD)->setNet(net1);
  // connect the mux instance output to the top output
  inst11->getInstTerm(dffQ)->setNet(net3);
  inst12->getInstTerm(dffQ)->setNet(net5);
  topOut->setNet(net3);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialOAI) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  SNLInstance* inst12 = SNLInstance::create(top, oaiModel, NLName("oai2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, NLName("oai2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  inst12->getInstTerm(oaiB2)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst11->getInstTerm(oaiA)->setNet(net2);
  inst11->getInstTerm(oaiB1)->setNet(net2);

  inst11->getInstTerm(oaiB2)->setNet(net4);
  inst12->getInstTerm(oaiB1)->setNet(net4);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  inst12->getInstTerm(oaiY)->setNet(net5);
  topOut2->setNet(net5);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNonDefinedModel) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, NLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName("NON_DEFF"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      NLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      NLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      NLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, NLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, NLName("oai_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
  topIn->setNet(net4);
  // connect logic0 to oai
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst11->getInstTerm(oaiA)->setNet(net1);
  // connect logic1 to oai
  inst2->getInstTerm(logic1Out)->setNet(net2);
  
  inst11->getInstTerm(oaiB1)->setNet(net2);
  inst11->getInstTerm(oaiB2)->setNet(net4);
  // connect the oai instance output to the top output
  inst11->getInstTerm(oaiY)->setNet(net3);
  topOut->setNet(net3);
  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
               
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND_Hierarchical_duplicated_nested_actions) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Standard, NLName("standard"));
  NLLibrary* libraryp = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Standard, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topOut3 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
  auto topOut4 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
  auto topOut5 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
  auto topOut6 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
  auto topOut7 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
  auto topOut8 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(libraryp);
  // 5. create a logic_0 instace in top
  SNLDesign* mod = SNLDesign::create(library, SNLDesign::Type::Standard, NLName("mod"));
  SNLInstance* inst1 = SNLInstance::create(mod, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(mod, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("AND"));
  
  auto modOut =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  auto modOut2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
  auto modOut3 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
  auto modOut4 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(mod, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(mod, andModel, NLName("and2"));
  SNLInstance* inst5 = SNLInstance::create(mod, andModel, NLName("and3"));
  SNLInstance* inst6 = SNLInstance::create(mod, andModel, NLName("and4"));

  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(mod, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(mod, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(mod, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(mod, NLName("and2_output_net"));
  SNLNet* net5 = SNLScalarNet::create(mod, NLName("and3_output_net"));
  SNLNet* net6 = SNLScalarNet::create(mod, NLName("and4_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  inst6->getInstTerm(andIn1)->setNet(net2);
  inst6->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst6->getInstTerm(andIn2)->setNet(net1);
  inst6->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  modOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  modOut2->setNet(net4);
  inst5->getInstTerm(andOut)->setNet(net5);
  modOut3->setNet(net3);
  inst6->getInstTerm(andOut)->setNet(net6);
  modOut4->setNet(net4);

  //Create 2 mod instances in top and connect the outputs with nets to the top outptus

  SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
  SNLInstance* modIns2 = SNLInstance::create(top, mod, NLName("mod2"));

  //Create 8 nets on top
  SNLNet* netTopOut1 = SNLScalarNet::create(top, NLName("top_out_1"));
  SNLNet* netTopOut2 = SNLScalarNet::create(top, NLName("top_out_2"));
    SNLNet* netTopOut3 = SNLScalarNet::create(top, NLName("top_out_3"));
    SNLNet* netTopOut4 = SNLScalarNet::create(top, NLName("top_out_4"));
    SNLNet* netTopOut5 = SNLScalarNet::create(top, NLName("top_out_5"));
    SNLNet* netTopOut6 = SNLScalarNet::create(top, NLName("top_out_6"));
    SNLNet* netTopOut7 = SNLScalarNet::create(top, NLName("top_out_7"));
    SNLNet* netTopOut8 = SNLScalarNet::create(top, NLName("top_out_8"));

//connect all the outputs of mod insts to the top outputs
    modInst->getInstTerm(modOut)->setNet(netTopOut1);
    topOut->setNet(netTopOut1);
    modInst->getInstTerm(modOut2)->setNet(netTopOut2);
    topOut2->setNet(netTopOut2);
    modInst->getInstTerm(modOut3)->setNet(netTopOut3);
    topOut3->setNet(netTopOut3);
    modInst->getInstTerm(modOut4)->setNet(netTopOut4);
    topOut4->setNet(netTopOut4);
    modIns2->getInstTerm(modOut)->setNet(netTopOut5);
    topOut5->setNet(netTopOut5);
    modIns2->getInstTerm(modOut2)->setNet(netTopOut6);
    topOut6->setNet(netTopOut6);
    modIns2->getInstTerm(modOut3)->setNet(netTopOut7);
    topOut7->setNet(netTopOut7);
    modIns2->getInstTerm(modOut4)->setNet(netTopOut8);
    topOut8->setNet(netTopOut8);


  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(true);
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND_Hierarchical_duplicated_nested_actionsNonBNE) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Standard, NLName("standard"));
  NLLibrary* libraryp = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Standard, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topOut3 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
  auto topOut4 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
  auto topOut5 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
  auto topOut6 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
  auto topOut7 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
  auto topOut8 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(libraryp);
  // 5. create a logic_0 instace in top
  SNLDesign* mod = SNLDesign::create(library, SNLDesign::Type::Standard, NLName("mod"));
  SNLInstance* inst1 = SNLInstance::create(mod, logic0, NLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(mod, logic1, NLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("AND"));
  
  auto modOut =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  auto modOut2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
  auto modOut3 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
  auto modOut4 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(mod, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(mod, andModel, NLName("and2"));
  SNLInstance* inst5 = SNLInstance::create(mod, andModel, NLName("and3"));
  SNLInstance* inst6 = SNLInstance::create(mod, andModel, NLName("and4"));

  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(mod, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(mod, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(mod, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(mod, NLName("and2_output_net"));
  SNLNet* net5 = SNLScalarNet::create(mod, NLName("and3_output_net"));
  SNLNet* net6 = SNLScalarNet::create(mod, NLName("and4_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  inst6->getInstTerm(andIn1)->setNet(net2);
  inst6->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst6->getInstTerm(andIn2)->setNet(net1);
  inst6->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  modOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  modOut2->setNet(net4);
  inst5->getInstTerm(andOut)->setNet(net5);
  modOut3->setNet(net3);
  inst6->getInstTerm(andOut)->setNet(net6);
  modOut4->setNet(net4);

  //Create 2 mod instances in top and connect the outputs with nets to the top outptus

  SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
  SNLInstance* modIns2 = SNLInstance::create(top, mod, NLName("mod2"));

  //Create 8 nets on top
  SNLNet* netTopOut1 = SNLScalarNet::create(top, NLName("top_out_1"));
  SNLNet* netTopOut2 = SNLScalarNet::create(top, NLName("top_out_2"));
    SNLNet* netTopOut3 = SNLScalarNet::create(top, NLName("top_out_3"));
    SNLNet* netTopOut4 = SNLScalarNet::create(top, NLName("top_out_4"));
    SNLNet* netTopOut5 = SNLScalarNet::create(top, NLName("top_out_5"));
    SNLNet* netTopOut6 = SNLScalarNet::create(top, NLName("top_out_6"));
    SNLNet* netTopOut7 = SNLScalarNet::create(top, NLName("top_out_7"));
    SNLNet* netTopOut8 = SNLScalarNet::create(top, NLName("top_out_8"));

//connect all the outputs of mod insts to the top outputs
    modInst->getInstTerm(modOut)->setNet(netTopOut1);
    topOut->setNet(netTopOut1);
    modInst->getInstTerm(modOut2)->setNet(netTopOut2);
    topOut2->setNet(netTopOut2);
    modInst->getInstTerm(modOut3)->setNet(netTopOut3);
    topOut3->setNet(netTopOut3);
    modInst->getInstTerm(modOut4)->setNet(netTopOut4);
    topOut4->setNet(netTopOut4);
    modIns2->getInstTerm(modOut)->setNet(netTopOut5);
    topOut5->setNet(netTopOut5);
    modIns2->getInstTerm(modOut2)->setNet(netTopOut6);
    topOut6->setNet(netTopOut6);
    modIns2->getInstTerm(modOut3)->setNet(netTopOut7);
    topOut7->setNet(netTopOut7);
    modIns2->getInstTerm(modOut4)->setNet(netTopOut8);
    topOut8->setNet(netTopOut8);


  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationAND_Hierarchical_duplicated_nested_actionsNonBNE2) {
  // 1. Create SNL
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLLibrary::Type::Standard, NLName("standard"));
  NLLibrary* libraryp = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Standard, NLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
  auto topOut3 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
  auto topOut4 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
  auto topOut5 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
  auto topOut6 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
  auto topOut7 = 
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
  auto topOut8 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  NLLibraryTruthTables::construct(libraryp);
  // 5. create a logic_0 instace in top
  SNLDesign* mod = SNLDesign::create(library, SNLDesign::Type::Standard, NLName("mod"));
  SNLInstance* inst1 = SNLInstance::create(mod, logic0, NLName("logic0a"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(mod, logic0, NLName("logic0b"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(libraryp, SNLDesign::Type::Primitive, NLName("AND"));
  
  auto modOut =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
  auto modOut2 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
  auto modOut3 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
  auto modOut4 =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      NLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, NLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(mod, andModel, NLName("and"));
  SNLInstance* inst4 = SNLInstance::create(mod, andModel, NLName("and2"));
  SNLInstance* inst5 = SNLInstance::create(mod, andModel, NLName("and3"));
  SNLInstance* inst6 = SNLInstance::create(mod, andModel, NLName("and4"));

  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(mod, NLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(mod, NLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(mod, NLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(mod, NLName("and2_output_net"));
  SNLNet* net5 = SNLScalarNet::create(mod, NLName("and3_output_net"));
  SNLNet* net6 = SNLScalarNet::create(mod, NLName("and4_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  
  inst4->getInstTerm(andIn1)->setNet(net2);
  inst4->getInstTerm(andIn2)->setNet(net2);
  inst6->getInstTerm(andIn1)->setNet(net2);
  inst6->getInstTerm(andIn2)->setNet(net2);
  // connect logic1 to and
  inst2->getInstTerm(logic0Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net4);
  inst6->getInstTerm(andIn2)->setNet(net1);
  inst6->getInstTerm(andIn1)->setNet(net4);
  // connect the and instance output to the top output
  inst3->getInstTerm(andOut)->setNet(net3);
  modOut->setNet(net3);
  inst4->getInstTerm(andOut)->setNet(net4);
  modOut2->setNet(net4);
  inst5->getInstTerm(andOut)->setNet(net5);
  modOut3->setNet(net3);
  inst6->getInstTerm(andOut)->setNet(net6);
  modOut4->setNet(net4);

  //Create 2 mod instances in top and connect the outputs with nets to the top outptus

  SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
  SNLInstance* modIns2 = SNLInstance::create(top, mod, NLName("mod2"));

  //Create 8 nets on top
  SNLNet* netTopOut1 = SNLScalarNet::create(top, NLName("top_out_1"));
  SNLNet* netTopOut2 = SNLScalarNet::create(top, NLName("top_out_2"));
    SNLNet* netTopOut3 = SNLScalarNet::create(top, NLName("top_out_3"));
    SNLNet* netTopOut4 = SNLScalarNet::create(top, NLName("top_out_4"));
    SNLNet* netTopOut5 = SNLScalarNet::create(top, NLName("top_out_5"));
    SNLNet* netTopOut6 = SNLScalarNet::create(top, NLName("top_out_6"));
    SNLNet* netTopOut7 = SNLScalarNet::create(top, NLName("top_out_7"));
    SNLNet* netTopOut8 = SNLScalarNet::create(top, NLName("top_out_8"));

//connect all the outputs of mod insts to the top outputs
    modInst->getInstTerm(modOut)->setNet(netTopOut1);
    topOut->setNet(netTopOut1);
    modInst->getInstTerm(modOut2)->setNet(netTopOut2);
    topOut2->setNet(netTopOut2);
    modInst->getInstTerm(modOut3)->setNet(netTopOut3);
    topOut3->setNet(netTopOut3);
    modInst->getInstTerm(modOut4)->setNet(netTopOut4);
    topOut4->setNet(netTopOut4);
    modIns2->getInstTerm(modOut)->setNet(netTopOut5);
    topOut5->setNet(netTopOut5);
    modIns2->getInstTerm(modOut2)->setNet(netTopOut6);
    topOut6->setNet(netTopOut6);
    modIns2->getInstTerm(modOut3)->setNet(netTopOut7);
    topOut7->setNet(netTopOut7);
    modIns2->getInstTerm(modOut4)->setNet(netTopOut8);
    topOut8->setNet(netTopOut8);


  // 11. create DNL
  get();
  // 12. create a constant propagation object
  {
    std::string dotFileName(
        std::string(std::string("./beforeCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./beforeCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
  cp.setNormalizedUniquification(false);
  // 13. collect the constants
  cp.collectConstants();
  // 14. run the constant propagation
  cp.run();
  // 15. check the output value of the top instance
  {
    std::string dotFileName(
        std::string(std::string("./afterCP") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./afterCP") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}
