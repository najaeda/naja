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

using namespace naja;
using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::NAJA_OPT;

class ConstatPropagationTests : public ::testing::Test {
 protected:
  ConstatPropagationTests() {
    // You can do set-up work for each test here
  }
  ~ConstatPropagationTests() override {
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
TEST_F(ConstatPropagationTests, TestConstantPropagation) {
  // 1. Create SNL
  SNLUniverse* univ = SNLUniverse::create();
  SNLDB* db = SNLDB::create(univ);
  SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
  // 2. Create a top model with one output
  SNLDesign* top = SNLDesign::create(library, SNLName("top"));
  univ->setTopDesign(top);
  auto topOut =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
  // 3. create a logic_0 model
  SNLDesign* logic0 = SNLDesign::create(library, SNLName("LOGIC0"));
  // add output to logic0
  auto logic0Out =
      SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("out"));
  // 4. create a logic_1 model
  SNLDesign* logic1 = SNLDesign::create(library, SNLName("LOGIC1"));
  // add output to logic0
  auto logic1Out =
      SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("out"));
  // 5. create a logic_0 instace in top
  SNLInstance* inst1 = SNLInstance::create(top, logic0, SNLName("logic0"));
  // 6. create a logic_1 instace in top
  SNLInstance* inst2 = SNLInstance::create(top, logic1, SNLName("logic1"));
  // 7. create a and model
  SNLDesign* andModel = SNLDesign::create(library, SNLName("AND"));
  // add 2 inputs and 1 output to and
  auto andIn1 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in1"));
  auto andIn2 = SNLScalarTerm::create(andModel, SNLTerm::Direction::Input,
                                      SNLName("in2"));
  auto andOut = SNLScalarTerm::create(andModel, SNLTerm::Direction::Output,
                                      SNLName("out"));
  // 8. create a and instance in top
  SNLInstance* inst3 = SNLInstance::create(top, andModel, SNLName("and"));
  // 9. connect the logic_0 and logic_1 to the and instance
  SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
  SNLNet* net2 = SNLScalarNet::create(top, SNLName("logic_1_net"));
  SNLNet* net3 = SNLScalarNet::create(top, SNLName("and_output_net"));
  // connect logic0 to and
  inst1->getInstTerm(logic0Out)->setNet(net1);
  inst3->getInstTerm(andIn1)->setNet(net1);
  // connect logic1 to and
  inst2->getInstTerm(logic1Out)->setNet(net2);
  inst3->getInstTerm(andIn2)->setNet(net2);
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
  EXPECT_EQ(cp.getConstants1().size(), 1);
  EXPECT_EQ(cp.getConstants0().size(), 1);
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
  EXPECT_EQ(cp.getConstants1().size(), 1);
  EXPECT_EQ(cp.getConstants0().size(), 2);
  EXPECT_EQ(topOut->getNet()->getInstTerms().size(), 0);
  EXPECT_EQ(topOut->getNet()->getType(), naja::SNL::SNLNet::Type::Assign0);
}
