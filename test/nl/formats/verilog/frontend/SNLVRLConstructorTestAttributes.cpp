// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include <filesystem>
#include <fstream>

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLAttributes.h"
#include "SNLDesignModeling.h"
#include "SNLRTLInfos.h"

#include "SNLVRLConstructor.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestAttributes: public ::testing::Test {
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

TEST_F(SNLVRLConstructorTestAttributes, test0) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"test_attributes.v");

  ASSERT_EQ(3, library_->getSNLDesigns().size());
  auto simple_netlist = library_->getSNLDesign(NLName("simple_netlist"));
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
        NLName("MODULE_ATTRIBUTE"),
        SNLAttributeValue("Top level simple_netlist module")),
      SNLAttribute(
        NLName("MODULE_VERSION"),
        SNLAttributeValue("1.0")),
      SNLAttribute(
        NLName("VERSION"),
        SNLAttributeValue(
          SNLAttributeValue::Type::NUMBER,
          "3"))
    )
  );
  //get from design
  Attributes attributesFromDesign(
    simple_netlist->getAttributes().begin(),
    simple_netlist->getAttributes().end()
  );
  EXPECT_EQ(3, attributesFromDesign.size());
  EXPECT_EQ(attributesFromDesign, simple_netlistAttributes);

  //2 standard instances, 2 assigns
  ASSERT_EQ(4, simple_netlist->getInstances().size());
  using Instances = std::vector<SNLInstance*>;
  Instances instances(simple_netlist->getInstances().begin(), simple_netlist->getInstances().end());
  EXPECT_THAT(instances,
    ElementsAre(
      simple_netlist->getInstance(NLName("and2_inst")),
      simple_netlist->getInstance(NLName("or2_inst")),
      simple_netlist->getInstance(2),
      simple_netlist->getInstance(3)
    )
  );

  auto ins0 = simple_netlist->getInstance(NLName("and2_inst"));
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
        NLName("INSTANCE_ATTRIBUTE_AND"),
        SNLAttributeValue("and2_inst")),
      SNLAttribute(
        NLName("description"),
        SNLAttributeValue("2-input AND gate instance")),
      SNLAttribute(
        NLName("VERSION"),
        SNLAttributeValue(
          SNLAttributeValue::Type::NUMBER,
          "3"))
    )
  );

  auto ins1 = simple_netlist->getInstance(NLName("or2_inst"));
  ASSERT_NE(ins1, nullptr);
  EXPECT_EQ(2, SNLAttributes::getAttributes(ins1).size());
  Attributes ins1Attributes(
    SNLAttributes::getAttributes(ins1).begin(),
    SNLAttributes::getAttributes(ins1).end());
  EXPECT_EQ(2, ins1Attributes.size());
  EXPECT_THAT(ins1Attributes,
    ElementsAre(
      SNLAttribute(
        NLName("INSTANCE_ATTRIBUTE_OR"),
        SNLAttributeValue("or2_inst")),
      SNLAttribute(
        NLName("description"),
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
      simple_netlist->getScalarTerm(NLName("a")),
      simple_netlist->getScalarTerm(NLName("b")),
      simple_netlist->getScalarTerm(NLName("and_out")),
      simple_netlist->getScalarTerm(NLName("or_out"))
    )
  );

  auto aTerm = simple_netlist->getScalarTerm(NLName("a"));
  ASSERT_NE(aTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(aTerm).size());
  Attributes aTermAttributes(
    SNLAttributes::getAttributes(aTerm).begin(),
    SNLAttributes::getAttributes(aTerm).end());
  EXPECT_EQ(1, aTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      NLName("INPUT_ATTRIBUTE_A"),
      SNLAttributeValue("Input signal A")),
    aTermAttributes[0]
  );

  auto bTerm = simple_netlist->getScalarTerm(NLName("b"));
  ASSERT_NE(bTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(bTerm).size());
  Attributes bTermAttributes(
    SNLAttributes::getAttributes(bTerm).begin(),
    SNLAttributes::getAttributes(bTerm).end());
  EXPECT_EQ(1, bTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      NLName("INPUT_ATTRIBUTE_B"),
      SNLAttributeValue("Input signal B")),
    bTermAttributes[0]
  );

  auto andOutTerm = simple_netlist->getScalarTerm(NLName("and_out"));
  ASSERT_NE(andOutTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(andOutTerm).size());
    Attributes andOutTermAttributes(
    SNLAttributes::getAttributes(andOutTerm).begin(),
    SNLAttributes::getAttributes(andOutTerm).end());
  EXPECT_EQ(1, andOutTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      NLName("OUTPUT_ATTRIBUTE_AND"),
      SNLAttributeValue("Output of AND gate")),
    andOutTermAttributes[0]
  );

  auto orOutTerm = simple_netlist->getScalarTerm(NLName("or_out"));
  ASSERT_NE(orOutTerm, nullptr);
  EXPECT_EQ(1, SNLAttributes::getAttributes(orOutTerm).size());
    EXPECT_EQ(1, SNLAttributes::getAttributes(orOutTerm).size());
    Attributes orOutTermAttributes(
    SNLAttributes::getAttributes(orOutTerm).begin(),
    SNLAttributes::getAttributes(orOutTerm).end());
  EXPECT_EQ(1, orOutTermAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      NLName("OUTPUT_ATTRIBUTE_OR"),
      SNLAttributeValue("Output of OR gate")),
    orOutTermAttributes[0]
  );

  //2 assign nets (1'b0, 1'b1) and 6 nets
  ASSERT_EQ(8, simple_netlist->getNets().size());
  ASSERT_EQ(8, simple_netlist->getScalarNets().size());

  auto andWire = simple_netlist->getNet(NLName("and_wire"));
  ASSERT_NE(andWire, nullptr);
  Attributes andWireAttributes(
    SNLAttributes::getAttributes(andWire).begin(),
    SNLAttributes::getAttributes(andWire).end());
  EXPECT_EQ(1, andWireAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      NLName("WIRE_ATTRIBUTE"),
      SNLAttributeValue("Wire connecting AND gate output to top output")),
    andWireAttributes[0]
  );

  auto orWire = simple_netlist->getNet(NLName("or_wire"));
  ASSERT_NE(orWire, nullptr);
  Attributes orWireAttributes(
    SNLAttributes::getAttributes(orWire).begin(),
    SNLAttributes::getAttributes(orWire).end());
  EXPECT_EQ(1, orWireAttributes.size());
  EXPECT_EQ(
    SNLAttribute(
      NLName("WIRE_ATTRIBUTE"),
      SNLAttributeValue("Wire connecting OR gate output to top output")),
    orWireAttributes[0]
  );
}

TEST_F(SNLVRLConstructorTestAttributes,
       testAttributedEmptyPortOnlyModuleIsSafe) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath /= "attributed_empty_port_only_blackbox";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto verilogPath = outPath / "example_rf.v";
  std::ofstream verilogFile(verilogPath);
  ASSERT_TRUE(verilogFile.good());
  verilogFile << "(* blackbox = 1 *)\n"
              << "module example_rf(q_o, d_i);\n"
              << "  output q_o;\n"
              << "  input d_i;\n"
              << "endmodule\n";
  verilogFile.close();

  constructor.construct(verilogPath);

  auto exampleRF = library_->getSNLDesign(NLName("example_rf"));
  ASSERT_NE(exampleRF, nullptr);
  EXPECT_TRUE(exampleRF->isUserBlackBox());
  EXPECT_TRUE(exampleRF->isBlackBox());

  const std::vector<SNLAttribute> attributes(
      SNLAttributes::getAttributes(exampleRF).begin(),
      SNLAttributes::getAttributes(exampleRF).end());
  ASSERT_EQ(1, attributes.size());
  EXPECT_EQ(NLName("blackbox"), attributes[0].getName());
  EXPECT_TRUE(attributes[0].getValue().isNumber());
  EXPECT_EQ("1", attributes[0].getValue().getString());

  auto output = exampleRF->getScalarTerm(NLName("q_o"));
  ASSERT_NE(output, nullptr);
  EXPECT_EQ(0, SNLDesignModeling::getTruthTableCount(exampleRF));
  EXPECT_FALSE(SNLDesignModeling::getTruthTable(exampleRF).isInitialized());
  EXPECT_FALSE(
      SNLDesignModeling::getTruthTable(exampleRF, output->getOrderID())
          .isInitialized());
}

TEST_F(SNLVRLConstructorTestAttributes, testDisableAttributes) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.setParseAttributes(false); //disable attributes
  constructor.construct(benchmarksPath/"test_attributes.v");

  ASSERT_EQ(3, library_->getSNLDesigns().size());
  auto simple_netlist = library_->getSNLDesign(NLName("simple_netlist"));
  ASSERT_NE(simple_netlist, nullptr);
  ASSERT_TRUE(SNLAttributes::getAttributes(simple_netlist).empty());

  auto ins0 = simple_netlist->getInstance(NLName("and2_inst"));
  ASSERT_NE(ins0, nullptr);
  EXPECT_TRUE(SNLAttributes::getAttributes(ins0).empty());
  auto ins1 = simple_netlist->getInstance(NLName("or2_inst"));
  EXPECT_TRUE(SNLAttributes::getAttributes(ins1).empty());
}

TEST_F(SNLVRLConstructorTestAttributes, sourcePopulation) {
  auto path = std::filesystem::path(SNL_VRL_DUMPER_TEST_PATH) / "source_population.v";
  {
    std::ofstream out(path);
    out << R"((* src = "rtl.sv:2.70000-3.80000|C:\\rtl\\unit.sv:9.1-9.4|rtl.sv:2.70000-3.80000|bad|x:4294967296.1-2.1|partial:7|unknown:0.0-0.0" *)
module cell(input a, output y);
endmodule
module top(a, y);
(* src = "port.sv:1.2-1.3" *) input a;
output y;
(* src = "net.sv:4.5-4.6" *) wire n, m;
(* src = "gate.sv:8.9-8.10" *) cell u(a, n), v(a, m);
not g(y, n);
endmodule
)";
  }
  SNLVRLConstructor constructor(library_);
  constructor.config_.yosysSourceAttributes_ = true;
  constructor.construct(path);
  auto cell = library_->getSNLDesign(NLName("cell"));
  ASSERT_NE(nullptr, cell->getRTLInfos());
  const auto& origins = cell->getRTLInfos()->getSourceOrigins();
  ASSERT_EQ(4, origins.size());
  EXPECT_EQ(70000, origins[0].range.column);
  EXPECT_EQ(80000, origins[0].range.endColumn);
  EXPECT_EQ("C:\\rtl\\unit.sv", origins[1].range.file.getString());
  EXPECT_EQ("yosys", origins[0].provider.getString());
  EXPECT_TRUE(origins[0].language.empty());
  EXPECT_TRUE(origins[0].representation.empty());
  EXPECT_EQ(0, origins[2].range.endLine);
  EXPECT_EQ(0, origins[3].range.line);
  auto declaration = cell->getRTLInfos()->getSourceDeclaration();
  ASSERT_TRUE(declaration);
  EXPECT_EQ(2, declaration->range.line);
  EXPECT_EQ(1, declaration->range.column);
  EXPECT_EQ(11, declaration->range.endColumn);
  EXPECT_FALSE(cell->getRTLInfos()->getSourceLoc());
  auto top = library_->getSNLDesign(NLName("top"));
  for (auto name: {"u", "v"}) {
    auto infos = top->getInstance(NLName(name))->getRTLInfos();
    ASSERT_NE(nullptr, infos);
    ASSERT_TRUE(infos->getSourceDeclaration());
    EXPECT_EQ(8, infos->getSourceDeclaration()->range.line);
    EXPECT_EQ(1, infos->getSourceOrigins().size());
  }
  for (auto name: {"n", "m"}) {
    auto infos = top->getNet(NLName(name))->getRTLInfos();
    ASSERT_NE(nullptr, infos);
    EXPECT_EQ(7, infos->getSourceDeclaration()->range.line);
    EXPECT_EQ(1, infos->getSourceOrigins().size());
    EXPECT_EQ(1, SNLAttributes::getAttributes(top->getNet(NLName(name))).size());
  }
  auto port = top->getTerm(NLName("a"))->getRTLInfos();
  ASSERT_NE(nullptr, port);
  EXPECT_EQ(5, port->getSourceDeclaration()->range.line);
  EXPECT_EQ(1, port->getSourceOrigins().size());
  auto gate = top->getInstance(NLName("g"))->getRTLInfos();
  ASSERT_NE(nullptr, gate);
  EXPECT_EQ(9, gate->getSourceDeclaration()->range.line);
  std::filesystem::remove(path);
}

TEST_F(SNLVRLConstructorTestAttributes, sourcePathOctalEscapes) {
  auto path = std::filesystem::path(SNL_VRL_DUMPER_TEST_PATH) / "source_octal.v";
  {
    std::ofstream out(path);
    ASSERT_TRUE(out.good());
    out << R"((* src = "one\7.sv:2|two\40.sv:3|three\141.sv:4|limit\377.sv:5" *)
module cell();
endmodule
)";
  }
  SNLVRLConstructor constructor(library_);
  constructor.config_.yosysSourceAttributes_ = true;
  constructor.construct(path);
  auto cell = library_->getSNLDesign(NLName("cell"));
  ASSERT_NE(nullptr, cell);
  ASSERT_NE(nullptr, cell->getRTLInfos());
  const auto& origins = cell->getRTLInfos()->getSourceOrigins();
  ASSERT_EQ(4, origins.size());
  const std::vector<std::string> files = {
    "one\a.sv", "two .sv", "threea.sv", std::string("limit") + static_cast<char>(255) + ".sv"
  };
  for (size_t i = 0; i < files.size(); ++i) {
    EXPECT_EQ(files[i], origins[i].range.file.getString());
    EXPECT_EQ(i + 2, origins[i].range.line);
    EXPECT_EQ("yosys", origins[i].provider.getString());
  }
  std::filesystem::remove(path);
}

TEST_F(SNLVRLConstructorTestAttributes, genericSrcStaysOpaque) {
  SNLVRLConstructor constructor(library_);
  constructor.construct(std::filesystem::path(SNL_VRL_BENCHMARKS_PATH) / "test_attributes.v");
  for (auto design: library_->getSNLDesigns()) {
    ASSERT_NE(nullptr, design->getRTLInfos());
    EXPECT_TRUE(design->getRTLInfos()->getSourceOrigins().empty());
    EXPECT_TRUE(design->getRTLInfos()->getSourceDeclaration());
  }
}
