// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "ConstantPropagation.h"
#include "DNL.h"
#include "NetlistGraph.h"
#include "RemoveLoadlessLogic.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLException.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLUniverse.h"
#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"
#include "Reduction.h"
#include "SNLDesignTruthTable.h"

using namespace naja;
using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::NAJA_OPT;

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
    SNLUniverse::get()->destroy();
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
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  //Repeat 7 for all types in constant propagation code with paying atttention to the port names that are used in the ConstatPropagation.cpp
  // 7. create a and model for or
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("OR"));
  // add 2 inputs and 1 output to and
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::Output,
                                      SNLName("out"));  
  // 7. create a and model for xor
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("XOR"));
  // add 2 inputs and 1 output to and
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 7. create a and model for nand
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NAND"));
  // add 2 inputs and 1 output to and
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 7. create a and model for nor
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NOR"));
  // add 2 inputs and 1 output to and
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 7. create a and model for xnor
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("XNOR"));
  // add 2 inputs and 1 output to and
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 7. create a and model for inv
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("INV"));
  // add 1 input and 1 output to and
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      SNLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 7. create a and model for buf
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("BUF"));
  // add 1 input and 1 output to and
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      SNLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 7. create a and model for ha
  SNLDesign* haModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("HA"));

  // add 2 inputs and 2 outputs to and
  auto haIn1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto haIn2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto haOut1 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      SNLName("out1")); 
  auto haOut2 = SNLScalarTerm::create(haModel, SNLTerm::Direction::Output,
                                      SNLName("out2"));   
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      SNLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      SNLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      SNLName("Q"));    
  // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      SNLName("Y"));  
  // 7. create a and model for oai  
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      SNLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      SNLName("Y"));

  // create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, SNLName("and"));
  // create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, SNLName("or"));
  // create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, SNLName("xor"));
  // create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, SNLName("nand"));
  // create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, SNLName("nor"));
  // create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, SNLName("xnor"));
  // create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, SNLName("inv"));
  // create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, SNLName("buf"));
  // create a ha instance in top
  SNLInstance* inst11 = SNLInstance::create(top, haModel, SNLName("ha"));
  // create a dff instance in top
  SNLInstance* inst12 = SNLInstance::create(top, dffModel, SNLName("dff"));
  // create a mux instance in top
  SNLInstance* inst13 = SNLInstance::create(top, muxModel, SNLName("mux"));
  // create a oai instance in top
  SNLInstance* inst14 = SNLInstance::create(top, oaiModel, SNLName("oai"));

  // 9. connect all instances inputs 
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("and_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
  ConstantPropagation cp;
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("AND"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, SNLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, SNLName("and"));
  SNLInstance* inst4 = SNLInstance::create(top, andModel, SNLName("and2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("and2_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for OR
TEST_F(ConstantPropagationTests, TestConstantPropagationOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a or model
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("OR"));
  // add 2 inputs and 1 output to or
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::
                                      Output, SNLName("out"));  
  // 8. create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, SNLName("or"));
  SNLInstance* inst5 = SNLInstance::create(top, orModel, SNLName("or2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("or_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("or2_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XOR
TEST_F(ConstantPropagationTests, TestConstantPropagationXOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a xor model
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("XOR"));
  // add 2 inputs and 1 output to xor
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::
                                      Output, SNLName("out"));  
  // 8. create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, SNLName("xor"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("xor_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NAND
TEST_F(ConstantPropagationTests, TestConstantPropagationNAND) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a nand model
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NAND"));
  // add 2 inputs and 1 output to nand
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output, SNLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("nand_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("nand2_output_net"));
  // 8. create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, SNLName("nand"));
  SNLInstance* inst7 = SNLInstance::create(top, nandModel, SNLName("nand2"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NOR
TEST_F(ConstantPropagationTests, TestConstantPropagationNOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a nor model
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NOR"));
  // add 2 inputs and 1 output to nor
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output, SNLName("out"));  
  // 8. create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, SNLName("nor"));
  SNLInstance* inst8 = SNLInstance::create(top, norModel, SNLName("nor2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("nor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("nor2_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XNOR
TEST_F(ConstantPropagationTests, TestConstantPropagationXNOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a xnor model
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("XNOR"));
  // add 2 inputs and 1 output to xnor
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output, SNLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("xnor_output_net"));
  // 8. create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, SNLName("xnor"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for INV
TEST_F(ConstantPropagationTests, TestConstantPropagationINV) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a inv model
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("INV"));
  // add 1 input and 1 output to inv
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      SNLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output, SNLName("out"));
  // 8. create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, SNLName("inv"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , SNLName("logic_0_net"));    
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("inv_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationBUF) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("BUF"));
  // add 1 input and 1 output to buf
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      SNLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output, SNLName("out"));
  // 8. create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, SNLName("buf"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , SNLName("logic_0_net"));
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("buf_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationMUX) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      SNLName("Y"));  
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, muxModel, SNLName("mux"));
  SNLInstance* inst12 = SNLInstance::create(top, muxModel, SNLName("mux2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("mux_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("mux2_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationDFF) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      SNLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      SNLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      SNLName("Q"));    
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, dffModel, SNLName("dff"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("mux_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationOAI) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      SNLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      SNLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, SNLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("oai_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

TEST_F(ConstantPropagationTests, TestConstantPropagationNonDeffinedModel) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NonDeff"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      SNLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      SNLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, SNLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("oai_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for AND
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialAND) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("AND"));
  
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::
                                      Output, SNLName("out"));  
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, SNLName("and"));
  SNLInstance* inst4 = SNLInstance::create(top, andModel, SNLName("and2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("and_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("and_output_net2"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for OR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a or model
  SNLDesign* orModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("OR"));
  // add 2 inputs and 1 output to or
  auto orIn1 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto orIn2 = SNLScalarTerm::create(orModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto orOut = SNLScalarTerm::create(orModel, SNLTerm::Direction::
                                      Output, SNLName("out"));  
  // 8. create a or instance in top
  SNLInstance* inst4 = SNLInstance::create(top, orModel, SNLName("or"));
  SNLInstance* inst5 = SNLInstance::create(top, orModel, SNLName("or2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("or_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("or2_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialXOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a xor model
  SNLDesign* xorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("XOR"));
  // add 2 inputs and 1 output to xor
  auto xorIn1 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto xorIn2 = SNLScalarTerm::create(xorModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto xorOut = SNLScalarTerm::create(xorModel, SNLTerm::Direction::
                                      Output, SNLName("out"));  
  // 8. create a xor instance in top
  SNLInstance* inst5 = SNLInstance::create(top, xorModel, SNLName("xor"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("xor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NAND
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNAND) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a nand model
  SNLDesign* nandModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NAND"));
  // add 2 inputs and 1 output to nand
  auto nandIn1 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto nandIn2 = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto nandOut = SNLScalarTerm::create(nandModel, SNLTerm::Direction::Output, SNLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("nand_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("nand2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
  topIn->setNet(net4);
  // 8. create a nand instance in top
  SNLInstance* inst6 = SNLInstance::create(top, nandModel, SNLName("nand"));
  SNLInstance* inst7 = SNLInstance::create(top, nandModel, SNLName("nand2"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for NOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a nor model
  SNLDesign* norModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NOR"));
  // add 2 inputs and 1 output to nor
  auto norIn1 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto norIn2 = SNLScalarTerm::create(norModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto norOut = SNLScalarTerm::create(norModel, SNLTerm::Direction::Output, SNLName("out"));  
  // 8. create a nor instance in top
  SNLInstance* inst7 = SNLInstance::create(top, norModel, SNLName("nor"));
  SNLInstance* inst8 = SNLInstance::create(top, norModel, SNLName("nor2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("nor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("nor2_output_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for XNOR
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialXNOR) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a xnor model
  SNLDesign* xnorModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("XNOR"));
  // add 2 inputs and 1 output to xnor
  auto xnorIn1 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto xnorIn2 = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto xnorOut = SNLScalarTerm::create(xnorModel, SNLTerm::Direction::Output, SNLName("out"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("xnor_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
  topIn->setNet(net4);
  // 8. create a xnor instance in top
  SNLInstance* inst8 = SNLInstance::create(top, xnorModel, SNLName("xnor"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for INV
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialINV) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a inv model
  SNLDesign* invModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("INV"));
  // add 1 input and 1 output to inv
  auto invIn = SNLScalarTerm::create(invModel, SNLTerm::Direction::Input,
                                      SNLName("in"));
  auto invOut = SNLScalarTerm::create(invModel, SNLTerm::Direction::Output, SNLName("out"));
  // 8. create a inv instance in top
  SNLInstance* inst9 = SNLInstance::create(top, invModel, SNLName("inv"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , SNLName("logic_0_net"));    
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("inv_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialBUF) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
  SNLDesign* bufModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("BUF"));
  // add 1 input and 1 output to buf
  auto bufIn = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Input,
                                      SNLName("in"));
  auto bufOut = SNLScalarTerm::create(bufModel, SNLTerm::Direction::Output, SNLName("out"));
  // 8. create a buf instance in top
  SNLInstance* inst10 = SNLInstance::create(top, bufModel, SNLName("buf"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top
                                        , SNLName("logic_0_net"));
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("buf_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialMUX) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));    
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  SNLDesign* muxModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("MUX"));
  // create S, A, B, and Y ports as mentioned in const prop code
  auto muxS = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("S"));
  auto muxA = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("A"));  
  auto muxB = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Input,
                                      SNLName("B"));    
  auto muxY = SNLScalarTerm::create(muxModel, SNLTerm::Direction::Output,
                                      SNLName("Y"));  
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, muxModel, SNLName("mux"));
  SNLInstance* inst12 = SNLInstance::create(top, muxModel, SNLName("mux2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("mux_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("mux2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialDFF) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* dffModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("DFF"));
  // create D, CLK, and Q ports as mentioned in const prop code
  auto dffD = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      SNLName("D"));
  auto dffCLK = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Input,
                                      SNLName("CLK"));    
  auto dffQ = SNLScalarTerm::create(dffModel, SNLTerm::Direction::Output,
                                      SNLName("Q"));    
  // 8. create a mux instance in top
  SNLInstance* inst11 = SNLInstance::create(top, dffModel, SNLName("dff1"));
  SNLInstance* inst12 = SNLInstance::create(top, dffModel, SNLName("dff2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("dff_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("dff2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialOAI) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topOut2 =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("OAI"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      SNLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      SNLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, SNLName("oai"));
  SNLInstance* inst12 = SNLInstance::create(top, oaiModel, SNLName("oai2"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("oai_output_net"));
  SNLNet* net5 = SNLScalarNet::create(top, SNLName("oai2_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}

// Test constat propagation for BUF
TEST_F(ConstantPropagationTests, TestConstantPropagationPartialNonDefinedModel) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  auto topIn =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0,0));
  SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0,1));
  SNLLibraryTruthTables::construct(library);
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a buf model
   // 7. create a and model for mux
  // 7. create a and model for dff
  SNLDesign* oaiModel = SNLDesign::create(library, SNLDesign::Type::Primitive, SNLName("NON_DEFF"));
  // create A, B1, B2, and Y ports as mentioned in const prop code
  auto oaiA = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("A"));
  auto oaiB1 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,
                                      SNLName("B1"));   
  auto oaiB2 = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Input,   
                                      SNLName("B2"));   
  auto oaiY = SNLScalarTerm::create(oaiModel, SNLTerm::Direction::Output, 
                                      SNLName("Y"));
  // 8. create a oai instance in top
  SNLInstance* inst11 = SNLInstance::create(top, oaiModel, SNLName("oai"));
  // 9. connect all instances inputs
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  net1->setType(SNLNet::Type::Assign0);
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  net2->setType(SNLNet::Type::Assign1);
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("oai_output_net"));
  SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
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
    system(std::string(std::string("dot -Tsvg ") + dotFileName +
                       std::string(" -o ") + svgFileName)
               .c_str());
  }
}




