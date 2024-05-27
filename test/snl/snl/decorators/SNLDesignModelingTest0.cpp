// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLDesignModeling.h"
#include "SNLDesignTruthTable.h"
#include "SNLScalarTerm.h"
#include "SNLException.h"
using namespace naja::SNL;

class SNLDesignModelingTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignModelingTest0, testCombinatorial) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto designs = SNLLibrary::create(db);
  auto top = SNLDesign::create(designs, SNLName("top"));
  auto lut = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("LUT"));
  auto luti0 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I0"));
  auto luti1 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I1"));
  auto luti2 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I2"));
  auto luti3 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I3"));
  auto luto = SNLScalarTerm::create(lut, SNLTerm::Direction::Output, SNLName("O"));
  SNLDesignModeling::addCombinatorialArcs({luti0, luti1, luti2, luti3}, {luto});
  auto lutIns0 = SNLInstance::create(top, lut, SNLName("ins0"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(luto).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(luto).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(luto).begin(),
      SNLDesignModeling::getCombinatorialInputs(luto).end()),
    ElementsAre(luti0, luti1, luti2, luti3));
  //lutIns0/luto
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luto)).empty());
  ASSERT_EQ(4, SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).begin(),
      SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luto)).end()),
    ElementsAre(
      lutIns0->getInstTerm(luti0),
      lutIns0->getInstTerm(luti1),
      lutIns0->getInstTerm(luti2),
      lutIns0->getInstTerm(luti3)));
  
  //luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti0).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti0).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti0).end()),
    ElementsAre(luto));
  //lutIns0/luti0
  ASSERT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).size());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(lutIns0->getInstTerm(luti0)).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).begin(),
      SNLDesignModeling::getCombinatorialOutputs(lutIns0->getInstTerm(luti0)).end()),
    ElementsAre(lutIns0->getInstTerm(luto)));

  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti1).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti1).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti1).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti2).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti2).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti2).end()),
    ElementsAre(luto));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(luti3).empty());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(luti3).begin(),
      SNLDesignModeling::getCombinatorialOutputs(luti3).end()),
    ElementsAre(luto));
}

TEST_F(SNLDesignModelingTest0, testSequential) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto designs = SNLLibrary::create(db);
  auto top = SNLDesign::create(designs, SNLName("top"));
  auto reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("REG"));
  auto regD = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, SNLName("D"));
  auto regQ = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, SNLName("Q"));
  auto regC = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, SNLName("C"));
  SNLDesignModeling::addInputsToClockArcs({regD}, regC);
  SNLDesignModeling::addClockToOutputsArcs(regC, {regQ});

  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regC).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedInputs(regD).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(regQ).empty());
  EXPECT_TRUE(SNLDesignModeling::getClockRelatedOutputs(regD).empty());

  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedInputs(regC).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedInputs(regC).begin(),
      SNLDesignModeling::getClockRelatedInputs(regC).end()),
    ElementsAre(regD));
  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedOutputs(regC).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedOutputs(regC).begin(),
      SNLDesignModeling::getClockRelatedOutputs(regC).end()),
    ElementsAre(regQ));
  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(regD).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getInputRelatedClocks(regD).begin(),
      SNLDesignModeling::getInputRelatedClocks(regD).end()),
    ElementsAre(regC));
  EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(regQ).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getOutputRelatedClocks(regQ).begin(),
      SNLDesignModeling::getOutputRelatedClocks(regQ).end()),
    ElementsAre(regC));

  auto regIns = SNLInstance::create(top, reg, SNLName("regIns"));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regD)).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regQ)).empty());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regD)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regC)).empty());
  EXPECT_TRUE(SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regQ)).empty());

  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).begin(),
      SNLDesignModeling::getClockRelatedInputs(regIns->getInstTerm(regC)).end()),
    ElementsAre(regIns->getInstTerm(regD)));
  EXPECT_EQ(1, SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).begin(),
      SNLDesignModeling::getClockRelatedOutputs(regIns->getInstTerm(regC)).end()),
    ElementsAre(regIns->getInstTerm(regQ)));
  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).begin(),
      SNLDesignModeling::getInputRelatedClocks(regIns->getInstTerm(regD)).end()),
    ElementsAre(regIns->getInstTerm(regC)));
  EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).begin(),
      SNLDesignModeling::getOutputRelatedClocks(regIns->getInstTerm(regQ)).end()),
    ElementsAre(regIns->getInstTerm(regC)));
}

TEST_F(SNLDesignModelingTest0, testCombiWithParameter) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto designs = SNLLibrary::create(db);
  auto top = SNLDesign::create(designs, SNLName("top"));
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto gate = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("LUT"));
  auto mode = SNLParameter::create(gate, SNLName("MODE"), SNLParameter::Type::String, "NORMAL");
  auto luti0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, SNLName("I0"));
  auto luti1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, SNLName("I1"));
  auto luto0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, SNLName("O0"));
  auto luto1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, SNLName("O1"));
  SNLDesignModeling::setParameter(gate, "MODE", "NORMAL");
  //no parameter arg so default mode
  SNLDesignModeling::addCombinatorialArcs({luti0}, {luto0});
  SNLDesignModeling::addCombinatorialArcs({luti1}, {luto1});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti1}, {luto0});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti0}, {luto1});

  //Default parameter
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti1).size());
  EXPECT_EQ(luto0, *SNLDesignModeling::getCombinatorialOutputs(luti0).begin());
  EXPECT_EQ(luto1, *SNLDesignModeling::getCombinatorialOutputs(luti1).begin());

  //instance with no parameter => default parameter
  auto ins0 = SNLInstance::create(top, gate, SNLName("ins0")); 
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).size());
  EXPECT_EQ(ins0->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins0->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).begin());

  //instance with default parameter value
  auto ins1 = SNLInstance::create(top, gate, SNLName("ins1")); 
  SNLInstParameter::create(ins1, mode, "NORMAL");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).size());
  EXPECT_EQ(ins1->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins1->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).begin());

  //instance with non default parameter value
  auto ins2 = SNLInstance::create(top, gate, SNLName("ins2")); 
  SNLInstParameter::create(ins2, mode, "CROSS");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).size());
  EXPECT_EQ(ins2->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins2->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).begin());

  //Error for non existing parameter
  auto ins3 = SNLInstance::create(top, gate, SNLName("ins3")); 
  SNLInstParameter::create(ins3, mode, "UNKNOWN");
  EXPECT_THROW(SNLDesignModeling::getCombinatorialOutputs(ins3->getInstTerm(luti0)), SNLException);
}

TEST_F(SNLDesignModelingTest0, testErrors0) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto designs = SNLLibrary::create(db);
  //not a primitive
  auto design = SNLDesign::create(designs, SNLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("C"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({designD}, designC), SNLException);
}

TEST_F(SNLDesignModelingTest0, testErrors1) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("C"));
  auto designA = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("A"));
  auto designB = SNLScalarTerm::create(design, SNLTerm::Direction::Output, SNLName("B"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({}, designC), SNLException);
  EXPECT_THROW(SNLDesignModeling::addClockToOutputsArcs(designC, {}), SNLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {}), SNLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({}, {designB}), SNLException);

  auto design1 = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("design1"));
  auto design1A = SNLScalarTerm::create(design1, SNLTerm::Direction::Input, SNLName("A"));
  auto design1B = SNLScalarTerm::create(design1, SNLTerm::Direction::Output, SNLName("B"));
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {design1B}), SNLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA, design1A}, {design1B}), SNLException);
}

TEST_F(SNLDesignModelingTest0, testNonExistingParameterError) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto prim = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("prim"));
  EXPECT_THROW(SNLDesignModeling::setParameter(prim, "MODE", "NORMAL"), SNLException);
}

TEST_F(SNLDesignModelingTest0, testTruthTablesError) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("I1"));
  auto o0 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, SNLName("O0"));
  //size discrepancy error
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(design, SNLTruthTable(3, 0x5)), SNLException);
}