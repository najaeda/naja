// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
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

TEST_F(SNLVRLConstructorTestAttributes, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_attributes.v");

  ASSERT_EQ(3, library_->getDesigns().size());
  auto simple_netlist = library_->getDesign(SNLName("simple_netlist"));
  ASSERT_NE(simple_netlist, nullptr);
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
  EXPECT_EQ(2, SNLAttributes::getAttributes(ins0).size());
  using Attributes = std::vector<SNLAttributes::SNLAttribute>;
  Attributes ins0Attributes(
    SNLAttributes::getAttributes(ins0).begin(),
    SNLAttributes::getAttributes(ins0).end());
  EXPECT_EQ(2, ins0Attributes.size());
  EXPECT_THAT(ins0Attributes,
    ElementsAre(
      SNLAttributes::SNLAttribute(SNLName("INSTANCE_ATTRIBUTE_AND"), "and2_inst"),
      SNLAttributes::SNLAttribute(SNLName("description"), "2-input AND gate instance")
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
      SNLAttributes::SNLAttribute(SNLName("INSTANCE_ATTRIBUTE_OR"), "or2_inst"),
      SNLAttributes::SNLAttribute(SNLName("description"), "2-input OR gate instance")
    )
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
  std::cerr << andWireAttributes[0].getString() << std::endl;
  EXPECT_EQ(
    SNLAttributes::SNLAttribute(SNLName("WIRE_ATTRIBUTE"), "Wire connecting AND gate output to top output"),
    andWireAttributes[0]
  );

#if 0
  EXPECT_EQ(1, ins0->getInstParameters().size());
  auto param = *(ins0->getInstParameters().begin());
  EXPECT_EQ("INIT", param->getName().getString());
  EXPECT_EQ("16'h5054", param->getValue());

  auto ins1 = ins_decode->getInstance(SNLName("decodes_in_0_a2_i_o3[8]"));
  ASSERT_NE(ins1, nullptr);
  EXPECT_TRUE(ins1->getInstParameters().empty());

  auto ins2 = ins_decode->getInstance(SNLName("decodes_RNO[6]"));
  ASSERT_NE(ins2, nullptr);
  EXPECT_EQ(1, ins2->getInstParameters().size());
  param = *(ins2->getInstParameters().begin());
  EXPECT_EQ("INIT", param->getName().getString());
  EXPECT_EQ("16'h0001", param->getValue());
#endif
}