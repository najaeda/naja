// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "NLDB0.h"
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
#include "SNLParameter.h"

using namespace naja::NL;

#ifndef SNL_VRL_DUMPER_TEST_PATH
#define SNL_VRL_DUMPER_TEST_PATH "Undefined"
#endif
#ifndef SNL_VRL_DUMPER_REFERENCES_PATH
#define SNL_VRL_DUMPER_REFERENCES_PATH "Undefined"
#endif

namespace {

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

}

class SNLVRLDumperTestParameters: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      NLDB* db = NLDB::create(universe);
      NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
      model_ = SNLDesign::create(library, NLName("model"));
      top_ = SNLDesign::create(library, NLName("top"));
    }

    SNLInstance* createMemoryInstance() {
      NLDB0::MemorySignature signature;
      signature.width = 8;
      signature.depth = 16;
      signature.abits = 4;
      signature.readPorts = 2;
      signature.writePorts = 2;
      signature.resetMode = NLDB0::MemoryResetMode::AsyncHigh;

      auto* memory = NLDB0::getOrCreateMemory(signature);
      if (nullptr == memory) {
        return nullptr;
      }

      auto* clk = SNLScalarNet::create(top_, NLName("clk"));
      auto* rst = SNLScalarNet::create(top_, NLName("rst"));
      auto* raddr = SNLBusNet::create(top_, 7, 0, NLName("raddr"));
      auto* rdata = SNLBusNet::create(top_, 15, 0, NLName("rdata"));
      auto* waddr = SNLBusNet::create(top_, 7, 0, NLName("waddr"));
      auto* wdata = SNLBusNet::create(top_, 15, 0, NLName("wdata"));
      auto* we = SNLBusNet::create(top_, 1, 0, NLName("we"));

      auto* ins = SNLInstance::create(top_, memory, NLName("mem0"));
      ins->setTermNet(memory->getScalarTerm(NLName("CLK")), clk);
      ins->setTermNet(memory->getScalarTerm(NLName("RST")), rst);
      ins->setTermNet(memory->getBusTerm(NLName("RADDR")), raddr);
      ins->setTermNet(memory->getBusTerm(NLName("RDATA")), rdata);
      ins->setTermNet(memory->getBusTerm(NLName("WADDR")), waddr);
      ins->setTermNet(memory->getBusTerm(NLName("WDATA")), wdata);
      ins->setTermNet(memory->getBusTerm(NLName("WE")), we);

      SNLInstParameter::create(ins, memory->getParameter(NLName("WIDTH")), "8");
      SNLInstParameter::create(ins, memory->getParameter(NLName("DEPTH")), "16");
      SNLInstParameter::create(ins, memory->getParameter(NLName("ABITS")), "4");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RD_PORTS")), "2");
      SNLInstParameter::create(ins, memory->getParameter(NLName("WR_PORTS")), "2");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RST_ENABLE")), "1");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RST_ASYNC")), "1");
      SNLInstParameter::create(ins, memory->getParameter(NLName("RST_ACTIVE_LOW")), "0");
      SNLInstParameter::create(
        ins,
        memory->getParameter(NLName("INIT")),
        "128'h00112233445566778899AABBCCDDEEFF");
      return ins;
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
    }
  protected:
    SNLDesign*  top_;
    SNLDesign*  model_;
};

TEST_F(SNLVRLDumperTestParameters, test0) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  SNLParameter::create(model_, NLName("PARAM0"), SNLParameter::Type::Decimal, "8");
  auto falseParam = SNLParameter::create(model_, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, NLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  SNLParameter::create(model_, NLName("PARAM3"), SNLParameter::Type::Binary, "4'hF");
  SNLParameter::create(model_, NLName("PARAM4"), SNLParameter::Type::Binary, "4'b0011");
  SNLParameter::create(model_, NLName("PARAM5"), SNLParameter::Type::String, "HELLO");
  auto ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, falseParam, "1");
  SNLInstParameter::create(ins, trueParam, "0");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParameters0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top_, outPath);

  outPath = outPath / (top_->getName().getString() + ".v");

  std::filesystem::path referencePath(SNL_VRL_DUMPER_REFERENCES_PATH);
  referencePath = referencePath / "testParameters0" / "top.v";
  ASSERT_TRUE(std::filesystem::exists(referencePath));
  std::string command = std::string(NAJA_DIFF) + " " + outPath.string() + " " + referencePath.string();
  EXPECT_FALSE(std::system(command.c_str()));
}

TEST_F(SNLVRLDumperTestParameters, testErrors0) {
  ASSERT_TRUE(top_);
  SNLParameter::create(top_, NLName("PARAM"), SNLParameter::Type::Boolean, "YY");
  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParametersErrors0";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestParameters, testErrors1) {
  ASSERT_TRUE(top_);
  ASSERT_TRUE(model_);
  auto falseParam = SNLParameter::create(model_, NLName("PARAM1"), SNLParameter::Type::Boolean, "0");
  auto trueParam = SNLParameter::create(model_, NLName("PARAM2"), SNLParameter::Type::Boolean, "1");
  auto ins = SNLInstance::create(top_, model_, NLName("ins"));
  SNLInstParameter::create(ins, falseParam, "Y");
  SNLInstParameter::create(ins, trueParam, "N");

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testParametersErrors1";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.setSingleFile(true);
  EXPECT_THROW(dumper.dumpDesign(top_, outPath), SNLVRLDumperException);
}

TEST_F(SNLVRLDumperTestParameters, testMemoryInstanceDump) {
  ASSERT_NE(nullptr, createMemoryInstance());

  std::ostringstream out;
  SNLVRLDumper dumper;
  dumper.dumpDesign(top_, out);
  const auto dumped = out.str();

  EXPECT_NE(std::string::npos, dumped.find("naja_mem #("));
  EXPECT_NE(std::string::npos, dumped.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, dumped.find(".DEPTH(16)"));
  EXPECT_NE(std::string::npos, dumped.find(".ABITS(4)"));
  EXPECT_NE(std::string::npos, dumped.find(".RD_PORTS(2)"));
  EXPECT_NE(std::string::npos, dumped.find(".WR_PORTS(2)"));
  EXPECT_NE(std::string::npos, dumped.find(".RST_ENABLE(1)"));
  EXPECT_NE(std::string::npos, dumped.find(".RST_ASYNC(1)"));
  EXPECT_NE(std::string::npos, dumped.find(".RST_ACTIVE_LOW(0)"));
  EXPECT_NE(
    std::string::npos,
    dumped.find(".INIT(128'h00112233445566778899AABBCCDDEEFF)"));
  EXPECT_EQ(std::string::npos, dumped.find("module naja_mem #("));
  EXPECT_EQ(std::string::npos, dumped.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
  EXPECT_EQ(std::string::npos, dumped.find("if (allow_write && addr_value < DEPTH)"));
}

TEST_F(SNLVRLDumperTestParameters, testMemoryPrimitiveFileDump) {
  ASSERT_NE(nullptr, createMemoryInstance());

  std::filesystem::path outPath(SNL_VRL_DUMPER_TEST_PATH);
  outPath = outPath / "testMemoryPrimitiveFileDump";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  dumper.setSingleFile(true);
  dumper.setTopFileName(top_->getName().getString() + ".v");
  dumper.dumpDesign(top_, outPath);

  const auto topDump = readTextFile(outPath / "top.v");
  EXPECT_NE(std::string::npos, topDump.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, topDump.find("module naja_mem #("));

  const auto primitivePath = outPath / "primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitivePath));
  const auto primitiveDump = readTextFile(primitivePath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
  EXPECT_NE(std::string::npos, primitiveDump.find("if (allow_write && addr_value < DEPTH)"));
}
