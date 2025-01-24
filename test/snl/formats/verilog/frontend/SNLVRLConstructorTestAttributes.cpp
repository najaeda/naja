// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLAttributes.h"

#include "SNLVRLConstructor.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestAttributes: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestAttributes, test0) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_attributes.v");

  ASSERT_EQ(3, library_->getDesigns().size());
  auto simple_netlist = library_->getDesign(SNLName("simple_netlist"));
  ASSERT_NE(simple_netlist, nullptr);

  ASSERT_EQ(3, SNLAttributes::getAttributes(simple_netlist).size());
  using Attributes = std::vector<SNLAttribute>;
  Attributes simple_netlistAttributes(
    SNLAttributes::getAttributes(simple_netlist).begin(),
    SNLAttributes::getAttributes(simple_netlist).end());
  EXPECT_EQ(3, simple_netlistAttributes.size());
  EXPECT_THAT(simple_netlistAttributes,
    ElementsAre(
      SNLAttribute(
        SNLName("MODULE_ATTRIBUTE"),
        SNLAttributeValue("Top level simple_netlist module")),
      SNLAttribute(
        SNLName("MODULE_VERSION"),
        SNLAttributeValue("1.0")),
      SNLAttribute(
        SNLName("VERSION"),
        SNLAttributeValue(
          SNLAttributeValue::Type::NUMBER,
          "3"))
    )
  );

  //2 standard instances, 2 assigns
  ASSERT_EQ(4, simple_netlist->getInstances().size());
  using Instances = std::vector<SNLInstance*>;
  Instances instances(simple_netlist->getInstances().begin(), simple_netlist->getInstances().end());
  EXPECT_THAT(instances,
    ElementsAre(
      simple_netlist->getInstance(SNLName("and2_inst")),
      simple_netlist->getInstance(SNLName("or2_inst")),
      simple_netlist->getInstance(2),
      simple_netlist->getInstance(3)
    )
  );

  auto ins0 = simple_netlist->getInstance(SNLName("and2_inst"));
  ASSERT_NE(ins0, nullptr);
  EXPECT_EQ(3, SNLAttributes::getAttributes(ins0).size());
  Attributes ins0Attributes(
    SNLAttributes::getAttributes(ins0).begin(),
    SNLAttributes::getAttributes(ins0).end());
  EXPECT_EQ(3, ins0Attributes.size());
  for (const auto& attribute: ins0Attributes) {
    std::cout << attribute.getString() << std::endl;
  }
  EXPECT_THAT(ins0Attributes,
    ElementsAre(
      SNLAttribute(
        SNLName("INSTANCE_ATTRIBUTE_AND"),
        SNLAttributeValue("and2_inst")),
      SNLAttribute(
        SNLName("description"),
        SNLAttributeValue("2-input AND gate instance")),
      SNLAttribute(
        SNLName("VERSION"),
        SNLAttributeValue(
          SNLAttributeValue::Type::NUMBER,
          "3"))
    )
  );

  auto ins1 = simple_netlist->getInstance(SNLName("or2_inst"));
  ASSERT_NE(ins1, nullptr);
  EXPECT_EQ(2, SNLAttributes::getAttributes(ins1).size());
  Attributes ins1Attributes(
    SNLAttributes::getAttributes(ins1).begin(),
    SNLAttributes::getAttributes(ins1).end());
  EXPECT_EQ(2, ins1Attributes.size());
  EXPECT_THAT(ins1Attributes,
    ElementsAre(
      SNLAttribute(
        SNLName("INSTANCE_ATTRIBUTE_OR"),
        SNLAttributeValue("or2_inst")),
      SNLAttribute(
        SNLName("description"),
        SNLAttributeValue("2-input OR gate instance"))
    )
  );

  ASSERT_EQ(4, simple_netlist->getTerms().size());
  ASSERT_EQ(4, simple_netlist->getScalarTerms().size());

  using Terms = std::vector<SNLScalarTerm*>;
  Terms terms(
    simple_netlist->getScalarTerms().begin(),
    simple_netlist->getScalarTerms().end()
  );

  EXPECT_THAT(terms,
    ElementsAre(
      simple_netlist->getScalarTerm(SNLName("a")),
      simple_netlist->getScalarTerm(SNLName("b")),
      simple_netlist->getScalarTerm(SNLName("and_out")),
      simple_netlist->getScalarTerm(SNLName("or_out"))
    )
  );

  auto aTerm = simple_netlist->getScalarTerm(SNLName("a"));
  ASSERT_NE(aTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(aTerm).size());
  Attributes aTermAttributes(
    SNLAttributes::getAttributes(aTerm).begin(),
    SNLAttributes::getAttributes(aTerm).end());
  EXPECT_EQ(1, aTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      SNLName("INPUT_ATTRIBUTE_A"),
      SNLAttributeValue("Input signal A")),
    aTermAttributes[0]
  );

  auto bTerm = simple_netlist->getScalarTerm(SNLName("b"));
  ASSERT_NE(bTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(bTerm).size());
  Attributes bTermAttributes(
    SNLAttributes::getAttributes(bTerm).begin(),
    SNLAttributes::getAttributes(bTerm).end());
  EXPECT_EQ(1, bTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      SNLName("INPUT_ATTRIBUTE_B"),
      SNLAttributeValue("Input signal B")),
    bTermAttributes[0]
  );

  auto andOutTerm = simple_netlist->getScalarTerm(SNLName("and_out"));
  ASSERT_NE(andOutTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(andOutTerm).size());
    Attributes andOutTermAttributes(
    SNLAttributes::getAttributes(andOutTerm).begin(),
    SNLAttributes::getAttributes(andOutTerm).end());
  EXPECT_EQ(1, andOutTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      SNLName("OUTPUT_ATTRIBUTE_AND"),
      SNLAttributeValue("Output of AND gate")),
    andOutTermAttributes[0]
  );

  auto orOutTerm = simple_netlist->getScalarTerm(SNLName("or_out"));
  ASSERT_NE(orOutTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(orOutTerm).size());
    EXPECT_EQ(1, SNLAttributes::getAttributes(orOutTerm).size());
    Attributes orOutTermAttributes(
    SNLAttributes::getAttributes(orOutTerm).begin(),
    SNLAttributes::getAttributes(orOutTerm).end());
  EXPECT_EQ(1, orOutTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      SNLName("OUTPUT_ATTRIBUTE_OR"),
      SNLAttributeValue("Output of OR gate")),
    orOutTermAttributes[0]
  );

  //2 assign nets (1'b0, 1'b1) and 6 nets
  ASSERT_EQ(8, simple_netlist->getNets().size());
  ASSERT_EQ(8, simple_netlist->getScalarNets().size());

  auto andWire = simple_netlist->getNet(SNLName("and_wire"));
  ASSERT_NE(andWire, nullptr);
  Attributes andWireAttributes(
    SNLAttributes::getAttributes(andWire).begin(),
    SNLAttributes::getAttributes(andWire).end());
  EXPECT_EQ(1, andWireAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      SNLName("WIRE_ATTRIBUTE"),
      SNLAttributeValue("Wire connecting AND gate output to top output")),
    andWireAttributes[0]
  );

  auto orWire = simple_netlist->getNet(SNLName("or_wire"));
  ASSERT_NE(orWire, nullptr);
  Attributes orWireAttributes(
    SNLAttributes::getAttributes(orWire).begin(),
    SNLAttributes::getAttributes(orWire).end());
  EXPECT_EQ(1, orWireAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      SNLName("WIRE_ATTRIBUTE"),
      SNLAttributeValue("Wire connecting OR gate output to top output")),
    orWireAttributes[0]
  );
}

TEST_F(SNLVRLConstructorTestAttributes, testDisableAttributes) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.setParseAttributes(false); //disable attributes
  constructor.construct(benchmarksPath/"test_attributes.v");

  ASSERT_EQ(3, library_->getDesigns().size());
  auto simple_netlist = library_->getDesign(SNLName("simple_netlist"));
  ASSERT_NE(simple_netlist, nullptr);
  ASSERT_TRUE(SNLAttributes::getAttributes(simple_netlist).empty());

  auto ins0 = simple_netlist->getInstance(SNLName("and2_inst"));
  ASSERT_NE(ins0, nullptr);
  EXPECT_TRUE(SNLAttributes::getAttributes(ins0).empty());
  auto ins1 = simple_netlist->getInstance(SNLName("or2_inst"));
  EXPECT_TRUE(SNLAttributes::getAttributes(ins1).empty());
}
