// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLDesignModeling.h"
#include "SNLScalarTerm.h"
using namespace naja::NL;

class SNLDesignModelingTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignModelingTest0, testCombinatorial) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto lut = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("LUT"));
  auto luti0 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I0"));
  auto luti1 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I1"));
  auto luti2 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I2"));
  auto luti3 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, NLName("I3"));
  auto luto = SNLScalarTerm::create(lut, SNLTerm::Direction::Output, NLName("O"));
  EXPECT_FALSE(SNLDesignModeling::hasModeling(lut));
  SNLDesignModeling::addCombinatorialArcs({luti0, luti1, luti2, luti3}, {luto});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(lut));
  EXPECT_FALSE(SNLDesignModeling::isSequential(lut));
  auto lutIns0 = SNLInstance::create(top, lut, NLName("ins0"));
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
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("REG"));
  auto regD = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("D"));
  auto regQ = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("Q"));
  auto regC = SNLScalarTerm::create(reg, SNLTerm::Direction::Input, NLName("C"));

  EXPECT_FALSE(SNLDesignModeling::hasModeling(reg));
  SNLDesignModeling::addInputsToClockArcs({regD}, regC);
  SNLDesignModeling::addClockToOutputsArcs(regC, {regQ});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(reg));
  EXPECT_TRUE(SNLDesignModeling::isSequential(reg));

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

  auto regIns = SNLInstance::create(top, reg, NLName("regIns"));
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
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto gate = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("LUT"));
  auto mode = SNLParameter::create(gate, NLName("MODE"), SNLParameter::Type::String, "NORMAL");
  auto luti0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("I0"));
  auto luti1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Input, NLName("I1"));
  auto luto0 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, NLName("O0"));
  auto luto1 = SNLScalarTerm::create(gate, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::setParameter(gate, "MODE", "NORMAL");
  //no parameter arg so default mode
  SNLDesignModeling::addCombinatorialArcs({luti0}, {luto0});
  SNLDesignModeling::addCombinatorialArcs({luti1}, {luto1});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti1}, {luto0});
  SNLDesignModeling::addCombinatorialArcs("CROSS", {luti0}, {luto1});
  EXPECT_TRUE(SNLDesignModeling::hasModeling(gate));

  //Default parameter
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti0).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(luti1).size());
  EXPECT_EQ(luto0, *SNLDesignModeling::getCombinatorialOutputs(luti0).begin());
  EXPECT_EQ(luto1, *SNLDesignModeling::getCombinatorialOutputs(luti1).begin());

  //instance with no parameter => default parameter
  auto ins0 = SNLInstance::create(top, gate, NLName("ins0")); 
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).size());
  EXPECT_EQ(ins0->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins0->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(luti1)).begin());

  //instance with default parameter value
  auto ins1 = SNLInstance::create(top, gate, NLName("ins1")); 
  SNLInstParameter::create(ins1, mode, "NORMAL");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).size());
  EXPECT_EQ(ins1->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins1->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins1->getInstTerm(luti1)).begin());

  //instance with non default parameter value
  auto ins2 = SNLInstance::create(top, gate, NLName("ins2")); 
  SNLInstParameter::create(ins2, mode, "CROSS");
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).size());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).size());
  EXPECT_EQ(ins2->getInstTerm(luto1), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti0)).begin());
  EXPECT_EQ(ins2->getInstTerm(luto0), *SNLDesignModeling::getCombinatorialOutputs(ins2->getInstTerm(luti1)).begin());

  //Error for non existing parameter
  auto ins3 = SNLInstance::create(top, gate, NLName("ins3")); 
  SNLInstParameter::create(ins3, mode, "UNKNOWN");
  EXPECT_THROW(SNLDesignModeling::getCombinatorialOutputs(ins3->getInstTerm(luti0)), NLException);
}

TEST_F(SNLDesignModelingTest0, testErrors0) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto designs = NLLibrary::create(db);
  //not a primitive
  auto design = SNLDesign::create(designs, NLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("C"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({designD}, designC), NLException);
}

TEST_F(SNLDesignModelingTest0, testErrors1) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto designD = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("D"));
  auto designC = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("C"));
  auto designA = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("A"));
  auto designB = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("B"));
  EXPECT_THROW(SNLDesignModeling::addInputsToClockArcs({}, designC), NLException);
  EXPECT_THROW(SNLDesignModeling::addClockToOutputsArcs(designC, {}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({}, {designB}), NLException);

  auto design1 = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design1"));
  auto design1A = SNLScalarTerm::create(design1, SNLTerm::Direction::Input, NLName("A"));
  auto design1B = SNLScalarTerm::create(design1, SNLTerm::Direction::Output, NLName("B"));
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA}, {design1B}), NLException);
  EXPECT_THROW(SNLDesignModeling::addCombinatorialArcs({designA, design1A}, {design1B}), NLException);
}

TEST_F(SNLDesignModelingTest0, testNonExistingParameterError) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto prim = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("prim"));
  EXPECT_THROW(SNLDesignModeling::setParameter(prim, "MODE", "NORMAL"), NLException);
}

TEST_F(SNLDesignModelingTest0, testGetCombiDepsFromTT) {
  //Create primitives
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto prims = NLLibrary::create(db, NLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, NLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  //set truth table
  SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x5));
  EXPECT_THROW(SNLDesignModeling::setTruthTable(design, SNLTruthTable(2, 0x1)), NLException);
  auto inputArcs = SNLDesignModeling::getCombinatorialInputs(o);
  EXPECT_EQ(inputArcs.size(), 2);
  auto outputArcs = SNLDesignModeling::getCombinatorialOutputs(i0);
  EXPECT_EQ(outputArcs.size(), 1);
  auto designs = NLLibrary::create(db);
  auto top = SNLDesign::create(designs, NLName("top"));
  auto ins0 = SNLInstance::create(top, design, NLName("ins0"));
  auto insInputArcs = SNLDesignModeling::getCombinatorialInputs(ins0->getInstTerm(o));
  EXPECT_EQ(insInputArcs.size(), 2);
  EXPECT_THAT(
    std::vector(insInputArcs.begin(), insInputArcs.end()),
    ElementsAre(ins0->getInstTerm(i0), ins0->getInstTerm(i1)));
  auto insOutputArcs = SNLDesignModeling::getCombinatorialOutputs(ins0->getInstTerm(i0));
  EXPECT_EQ(insOutputArcs.size(), 1);
  EXPECT_THAT(
    std::vector(insOutputArcs.begin(), insOutputArcs.end()),
    ElementsAre(ins0->getInstTerm(o)));
}