// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <sstream>
#include <unordered_set>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBitNet.h"
#include "SNLInstParameter.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLUtils.h"
#include "SNLVRLDumper.h"

#include "SNLSVConstructor.h"
#include "SNLSVConstructorTestDetail.h"
#include "SNLSVConstructorException.h"

using namespace naja::NL;

#ifndef SNL_SV_BENCHMARKS_PATH
#define SNL_SV_BENCHMARKS_PATH "Undefined"
#endif
#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

const SNLRTLInfos* getRTLInfos(const NLObject* object) {
  if (auto design = dynamic_cast<const SNLDesign*>(object)) {
    return design->getRTLInfos();
  }
  if (auto designObject = dynamic_cast<const SNLDesignObject*>(object)) {
    return designObject->getRTLInfos();
  }
  return nullptr;
}

bool hasRTLInfo(const NLObject* object, const std::string& name) {
  auto rtlInfos = getRTLInfos(object);
  return rtlInfos && rtlInfos->hasInfo(NLName(name));
}

std::string readTextFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::filesystem::path dumpTopAndGetVerilogPath(const SNLDesign* top,
                                               const std::string& outDirName,
                                               bool dumpRTLInfosAsAttributes = false) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath /= outDirName;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  SNLVRLDumper dumper;
  auto fileName = top->getName().getString() + ".v";
  dumper.setTopFileName(fileName);
  dumper.setSingleFile(true);
  dumper.setDumpRTLInfosAsAttributes(dumpRTLInfosAsAttributes);
  dumper.dumpDesign(top, outPath);
  return outPath / fileName;
}

size_t getMux2Width(const SNLInstance* instance) {
  if (!instance || !NLDB0::isMux2(instance->getModel())) {
    return 0;
  }
  if (auto* widthInstParam = instance->getInstParameter(NLName("WIDTH"))) {
    return static_cast<size_t>(std::stoull(widthInstParam->getValue()));
  }
  auto* widthParam = instance->getModel()->getParameter(NLName("WIDTH"));
  if (!widthParam) {
    return 0;
  }
  return static_cast<size_t>(std::stoull(widthParam->getValue()));
}

size_t countMux2Instances(const SNLDesign* design, size_t width = 0) {
  size_t count = 0;
  for (auto inst : design->getInstances()) {
    if (!NLDB0::isMux2(inst->getModel())) {
      continue;
    }
    if (width != 0 && getMux2Width(inst) != width) {
      continue;
    }
    ++count;
  }
  return count;
}

size_t countFAInstances(const SNLDesign* design) {
  size_t count = 0;
  for (auto inst : design->getInstances()) {
    if (NLDB0::isFA(inst->getModel())) {
      ++count;
    }
  }
  return count;
}

size_t countAssignDrivers(const SNLDesign* design, const SNLBitNet* net) {
  size_t count = 0;
  auto assignOutput = NLDB0::getAssignOutput();
  for (auto inst : design->getInstances()) {
    if (!NLDB0::isAssign(inst->getModel())) {
      continue;
    }
    auto instTerm = inst->getInstTerm(assignOutput);
    if (instTerm && instTerm->getNet() == net) {
      ++count;
    }
  }
  return count;
}

void expectUnsupportedConstruct(
  SNLSVConstructor& constructor,
  const std::filesystem::path& svPath,
  std::initializer_list<const char*> expectedSubstrings) {
  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported SystemVerilog element exception";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported SystemVerilog elements encountered"))
      << reason;
    for (const auto* expected : expectedSubstrings) {
      EXPECT_NE(std::string::npos, reason.find(expected)) << reason;
    }
  }
}

}

class SNLSVConstructorTestMemoryInference: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  size_t mux2Count = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
    if (NLDB0::isMux2(inst->getModel())) {
      ++mux2Count;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  EXPECT_EQ(0u, mux2Count);

  auto* model = memoryInst->getModel();
  ASSERT_NE(nullptr, model);
  EXPECT_TRUE(NLDB0::isMemory(model));
  EXPECT_EQ("4", model->getParameter(NLName("DEPTH"))->getValue());
  EXPECT_EQ("8", model->getParameter(NLName("WIDTH"))->getValue());

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  auto* abitsParam = memoryInst->getInstParameter(NLName("ABITS"));
  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  ASSERT_NE(nullptr, abitsParam);
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, rstEnableParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
  EXPECT_EQ("2", abitsParam->getValue());
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());
  EXPECT_EQ("0", rstEnableParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".WIDTH(8)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".DEPTH(4)"));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("naja_mux2"));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSharedSequentialKeepsScalarFlops) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_keeps_scalar_flops";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_keeps_scalar_flops.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_keeps_scalar_flops(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       inc_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] ptr_o,
  output logic [2:0] cnt_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [1:0] ptr_q;
  logic [1:0] ptr_n;
  logic [2:0] cnt_q;
  logic [2:0] cnt_n;

  always_comb begin
    mem_d = mem_q;
    ptr_n = ptr_q;
    cnt_n = cnt_q;
    if (inc_i) begin
      mem_d[addr_i] = data_i;
      ptr_n = ptr_q + 1'b1;
      cnt_n = cnt_q + 1'b1;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{default: '0};
      ptr_q <= '0;
      cnt_q <= '0;
    end else begin
      mem_q <= mem_d;
      ptr_q <= ptr_n;
      cnt_q <= cnt_n;
    end
  end

  assign data_o = mem_q[addr_i];
  assign ptr_o = ptr_q;
  assign cnt_o = cnt_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_keeps_scalar_flops"));
  ASSERT_NE(top, nullptr);

  auto dffrnModel = NLDB0::getDFFRN();
  ASSERT_NE(dffrnModel, nullptr);
  size_t memoryCount = 0;
  size_t dffrnCount = 0;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ++memoryCount;
    }
    if (inst->getModel() == dffrnModel) {
      ++dffrnCount;
    }
  }
  EXPECT_EQ(1u, memoryCount);
  EXPECT_EQ(5u, dffrnCount);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWriteOnlySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_write_only_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_write_only_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_write_only_supported(
  input  logic       clk_i,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = en_i;
    mem_d = mem_q;
    if (en_i) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_write_only_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rdPortsParam = memoryInst->getInstParameter(NLName("RD_PORTS"));
  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, rdPortsParam);
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", rdPortsParam->getValue());
  EXPECT_EQ("1", wrPortsParam->getValue());

  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_raddr")));
  EXPECT_NE(nullptr, top->getNet(NLName("mem_q_mem_dummy_rdata")));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNonBitstreamElementFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_non_bitstream_element_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_non_bitstream_element_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_non_bitstream_element_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct {
    logic [7:0] data;
    string      tag;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i].data = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i].data;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00110011001000100001000100000000", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(top, "qd_memory_inference_reset_init_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ENABLE(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ASYNC(1)"));
  EXPECT_NE(std::string::npos, dumpedText.find(".RST_ACTIVE_LOW(1)"));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00110011001000100001000100000000)"));

  const auto primitiveDumpPath = dumpedVerilog.parent_path() / "primitives.v";
  ASSERT_TRUE(std::filesystem::exists(primitiveDumpPath));
  std::string primitiveDump = readTextFile(primitiveDumpPath);
  EXPECT_NE(std::string::npos, primitiveDump.find("module naja_mem #("));
  EXPECT_NE(std::string::npos, primitiveDump.find("reg [WIDTH-1:0] mem [0:DEPTH-1];"));
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAsyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_async_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      mem_q <= '{8'hA0, 8'hB1, 8'hC2, 8'hD3};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_async_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b11010011110000101011000110100000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncHighResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_high_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sync_high_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_high_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (rst_i) begin
      mem_q <= '{8'h10, 8'h21, 8'h32, 8'h43};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_sync_high_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("0", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000011001100100010000100010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncLowBitwiseResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_low_bitwise_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_low_bitwise_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (~rst_ni) begin
      mem_q <= '{8'h55, 8'h66, 8'h77, 8'h88};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_sync_low_bitwise_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("0", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b10001000011101110110011001010101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceAsyncResetConditionMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_async_reset_condition_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_async_reset_condition_mismatch_fallback(
  input  logic       clk_i,
  input  logic       rst_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (~rst_i) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after async reset mismatch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSyncLogicalNotResetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sync_logical_not_reset_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_sync_logical_not_reset_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sync_logical_not_reset_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    if (!rst_ni) begin
      mem_q <= '{8'h00, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after sync logical-not reset";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitUnknownBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_unknown_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unknown_bits_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q <= '{8'hxx, 8'h11, 8'h22, 8'h33};
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after unknown-bit reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(std::string::npos, reason.find("Unsupported RHS in sequential assignment"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitNonConstantFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_non_constant_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_non_constant_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= data_i;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after non-constant reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitNarrowEntryZeroExtendSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_narrow_entry_zero_extend_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_narrow_entry_zero_extend_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 4'h1;
        end else if (i == 1) begin
          mem_q[i] <= 4'h2;
        end else if (i == 2) begin
          mem_q[i] <= 4'h3;
        end else begin
          mem_q[i] <= 4'h4;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_narrow_entry_zero_extend_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000100000000110000001000000001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitOversizedEntryFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_oversized_entry_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_oversized_entry_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 16'h1234;
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported sequential array assignment after oversized reset init";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceResetInitRealConstantsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_real_constants_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_real_constants_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_real_constants_supported(
  input  logic        clk_i,
  input  logic        rst_i,
  input  logic [1:0]  addr_i,
  input  logic [63:0] data_i,
  output logic [63:0] data_o
);
  logic [63:0] mem_q [0:3];
  logic [63:0] mem_d [0:3];

  localparam real ZERO_R  = 0.0;
  localparam real ONE_R   = 1.0;
  localparam real TWO_R   = 2.0;
  localparam real THREE_R = 3.0;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or posedge rst_i) begin
    if (rst_i) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= ZERO_R;
        end else if (i == 1) begin
          mem_q[i] <= ONE_R;
        end else if (i == 2) begin
          mem_q[i] <= TWO_R;
        end else begin
          mem_q[i] <= THREE_R;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_real_constants_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);

  const std::array<uint64_t, 4> values{0, 1, 2, 3};
  std::string expectedInit = "256'b";
  for (auto it = values.rbegin(); it != values.rend(); ++it) {
    for (int bit = 63; bit >= 0; --bit) {
      expectedInit += ((*it >> bit) & 1ULL) ? '1' : '0';
    }
  }
  EXPECT_EQ(expectedInit, initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitPackedArrayElementSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_packed_array_element_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_packed_array_element_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [0:3][7:0] INIT_PACKED = '{8'h11, 8'h22, 8'h33, 8'h44};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_PACKED[0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_PACKED[1];
        end else if (i == 2) begin
          mem_q[i] <= INIT_PACKED[2];
        end else begin
          mem_q[i] <= INIT_PACKED[3];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_packed_array_element_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitStructMemberAccessSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_struct_member_access_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_struct_member_access_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_struct_member_access_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [7:0] payload;
    logic       valid;
  } init_t;

  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam init_t INIT0 = '{payload: 8'h12, valid: 1'b0};
  localparam init_t INIT1 = '{payload: 8'h23, valid: 1'b1};
  localparam init_t INIT2 = '{payload: 8'h34, valid: 1'b0};
  localparam init_t INIT3 = '{payload: 8'h45, valid: 1'b1};

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT0.payload;
        end else if (i == 1) begin
          mem_q[i] <= INIT1.payload;
        end else if (i == 2) begin
          mem_q[i] <= INIT2.payload;
        end else begin
          mem_q[i] <= INIT3.payload;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_struct_member_access_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitRangeSelectSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_range_select_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_reset_init_range_select_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_range_select_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  localparam logic [31:0] INIT_WORD = 32'h44332211;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= INIT_WORD[7:0];
        end else if (i == 1) begin
          mem_q[i] <= INIT_WORD[15:8];
        end else if (i == 2) begin
          mem_q[i] <= INIT_WORD[23:16];
        end else begin
          mem_q[i] <= INIT_WORD[31:24];
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_reset_init_range_select_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
  ASSERT_NE(nullptr, memoryInst->getInstParameter(NLName("INIT")));
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitUnaryConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (!(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_unary_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitLiteralConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_literal_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_literal_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (1'b1) begin
          mem_q[i] <= 8'hA5;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_literal_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100101101001011010010110100101", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitBitwiseNotConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_bitwise_not_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_bitwise_not_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~(i < 2)) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_bitwise_not_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10101010101010100001000100010001", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitUnaryNonConstantConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_unary_non_constant_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_unary_non_constant_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (~addr_i[0]) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitComparisonConstantConditionsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_comparison_constant_conditions_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_comparison_constant_conditions_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i > 3) begin
          mem_q[i] <= 8'hF0;
        end else if (i >= 2) begin
          mem_q[i] <= 8'hA0;
        end else if (i == 1) begin
          mem_q[i] <= 8'hB0;
        end else if (i <= 0) begin
          mem_q[i] <= 8'hC0;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_comparison_constant_conditions_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b10100000101000001011000011000000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitInequalityConstantConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_inequality_constant_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_inequality_constant_condition_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if (i != 1) begin
          mem_q[i] <= 8'hD0;
        end else begin
          mem_q[i] <= 8'hE1;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_reset_init_inequality_constant_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b11010000110100001110000111010000", initParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference,
       parseQDMemoryInferenceResetInitLogicalAndConditionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_reset_init_logical_and_condition_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_reset_init_logical_and_condition_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        if ((i < 2) && 1'b1) begin
          mem_q[i] <= 8'hAA;
        end else begin
          mem_q[i] <= 8'h11;
        end
      end
    end else begin
      mem_q <= mem_d;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConditionalSideLogicSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_conditional_side_logic_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_conditional_side_logic_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_conditional_side_logic_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] alt_addr_i,
  input  logic [7:0] data_i,
  input  logic [7:0] alt_data_i,
  output logic [7:0] data_o,
  output logic       side_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    side_o = sel_i;
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[alt_addr_i] = alt_data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_conditional_side_logic_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("side_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSameGuardPartialWritesSingleWEDriver) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_guard_partial_writes_single_we_driver";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_same_guard_partial_writes_single_we_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_guard_partial_writes_single_we_driver(
  input  logic       clk_i,
  input  logic       valid_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] data_i,
  output logic [1:0] data_o
);
  logic [1:0] mem_q [0:3];
  logic [1:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (valid_i) begin
      mem_d[addr_i][0] = data_i[0];
      mem_d[addr_i][1] = data_i[1];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_guard_partial_writes_single_we_driver"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());

  size_t checkedWriteEnableNets = 0;
  for (auto net : top->getNets()) {
    auto* bitNet = dynamic_cast<SNLBitNet*>(net);
    if (!bitNet || bitNet->isUnnamed()) {
      continue;
    }
    const auto name = bitNet->getName().getString();
    if (name.find("mem_we_") == std::string::npos) {
      continue;
    }
    ++checkedWriteEnableNets;
    EXPECT_EQ(1u, countAssignDrivers(top, bitNet)) << name;
  }
  EXPECT_EQ(2u, checkedWriteEnableNets);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopConditionalElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_conditional_else_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_conditional_else_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_conditional_else_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_d[i] = data_i;
      end else begin
        mem_d[i] = 8'h00;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_conditional_else_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceLoopBreakSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_loop_break_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_loop_break_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_loop_break_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    for (int i = 0; i < 4; i++) begin
      mem_d[i] = data_i;
      break;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_loop_break_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCasezFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_casez_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_casez_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_casez_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    casez (mode_i)
      2'b1?: mem_d[0] = data_i;
      default: mem_d[addr_i] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIfFalseBranchFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_iffalse_branch_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_iffalse_branch_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i] = data_i;
    end else begin
      mem_d[addr_i] += data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseItemFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[addr_i] += data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseDefaultFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_default_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_default_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_default_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      2'd0: mem_d[addr_i] = data_i;
      2'd1: mem_d[0] = data_i;
      default: mem_d[addr_i] += data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConditionGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_condition_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_condition_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_condition_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if ($urandom_range(1, 0)) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceNoIndexedWritesAfterLateConstantFalseGuardFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_no_indexed_writes_after_late_constant_false_guard.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_no_indexed_writes_after_late_constant_false_guard(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (addr_i[0] && 1'b0) begin
      mem_d[addr_i] = data_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseItemGuardFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_item_guard_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_item_guard_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    case (mode_i)
      $urandom_range(1, 0): mem_d[addr_i] = data_i;
      default: mem_d[0] = data_i;
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCompoundShadowActionFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_compound_shadow_action_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_compound_shadow_action_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_compound_shadow_action_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported compound assignment in always_comb without current LHS bits",
     "unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNestedBaseCopyFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_base_copy_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_base_copy_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_base_copy_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d = mem_q;
    end
    mem_d[addr_i] = data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseAndForSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_and_for_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_and_for_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_and_for_supported(
  input  logic       clk_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  input  logic [1:0] token_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    flag_o = 1'b0;
    mem_d = mem_q;
    case (mode_i)
      2'b00: begin
        for (int i = 0; i < 2; i++) begin
          mem_d[i] = {7'b0, token_i[i]};
        end
      end
      2'b01: begin
        mem_d[addr_i] = data_i;
      end
      default: begin
        flag_o = 1'b1;
      end
    endcase
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_case_and_for_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceConstantConditionalsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_constant_conditionals_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_constant_conditionals_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_constant_conditionals_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (1'b0) begin
      mem_d[0] = 8'hAA;
    end
    if (1'b1) begin
      mem_d[1] = data_i;
    end else begin
      mem_d[1] = 8'h55;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_constant_conditionals_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSharedSequentialResetLoopSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_shared_sequential_reset_loop_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_loop_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        if (i == 0) begin
          mem_q[i] <= 8'h11;
        end else if (i == 1) begin
          mem_q[i] <= 8'h22;
        end else begin
          mem_q[i] <= 8'h00;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_loop_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000010001000010001", initParam->getValue());

  auto dumpedVerilog = dumpTopAndGetVerilogPath(
    top, "qd_memory_inference_shared_sequential_reset_loop_supported");
  std::string dumpedText = readTextFile(dumpedVerilog);
  EXPECT_NE(std::string::npos, dumpedText.find("naja_mem #("));
  EXPECT_EQ(std::string::npos, dumpedText.find("module naja_mem #("));
  EXPECT_NE(
    std::string::npos,
    dumpedText.find(".INIT(32'b00000000000000000010001000010001)"));
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresUnrelatedAlwaysFFChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      scratch_q <= 1'b0;
    else
      scratch_q <= en_i;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_unrelated_alwaysff_chain"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseGeneratedDirectSequentialMemoryWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "generated_direct_sequential_memory_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "generated_direct_sequential_memory_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module generated_direct_sequential_memory_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                    clk_i,
  input  logic                    w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]      w_data_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]      r_data_o
);
  if (width_p == 0 || els_p == 0) begin : z
    assign r_data_o = '0;
  end else begin : nz
    logic [width_p-1:0] mem [els_p-1:0];
    wire [addr_width_lp-1:0] r_addr_li = (els_p > 0) ? r_addr_i : '0;
    wire [addr_width_lp-1:0] w_addr_li = (els_p > 0) ? w_addr_i : '0;

    assign r_data_o = mem[r_addr_li];

    always_ff @(posedge clk_i) begin
      if (w_v_i) begin
        mem[w_addr_li] <= w_data_i;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("generated_direct_sequential_memory_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  auto* depthParam = memoryInst->getInstParameter(NLName("DEPTH"));
  ASSERT_NE(nullptr, widthParam);
  ASSERT_NE(nullptr, depthParam);
  EXPECT_EQ("8", widthParam->getValue());
  EXPECT_EQ("4", depthParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryMaskedBitWriteInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_masked_bit_write_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_masked_bit_write_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_masked_bit_write_inference_supported #(
  parameter int width_p = 8,
  parameter int els_p = 4,
  parameter int addr_width_lp = 2
) (
  input  logic                     clk_i,
  input  logic                     w_v_i,
  input  logic [addr_width_lp-1:0] w_addr_i,
  input  logic [width_p-1:0]       w_data_i,
  input  logic [width_p-1:0]       w_mask_i,
  input  logic [addr_width_lp-1:0] r_addr_i,
  output logic [width_p-1:0]       r_data_o
);
  logic [width_p-1:0] mem [els_p-1:0];

  assign r_data_o = mem[r_addr_i];

  always_ff @(posedge clk_i)
    if (w_v_i)
      for (integer i = 0; i < width_p; i=i+1)
        if (w_mask_i[i])
          mem[w_addr_i][i] <= w_data_i[i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_masked_bit_write_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseDirectSequentialMemoryAsyncLowResetInitSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "direct_sequential_memory_async_low_reset_init_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "direct_sequential_memory_async_low_reset_init_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module direct_sequential_memory_async_low_reset_init_supported(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       w_v_i,
  input  logic [1:0] w_addr_i,
  input  logic [7:0] w_data_i,
  output logic [7:0] r_data_o
);
  logic [7:0] mem [0:3];

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      mem <= '{default: '0};
    end else begin
      for (int i = 0; i < 4; i++) begin
        if (w_v_i && (w_addr_i == i[1:0])) begin
          mem[i] <= w_data_i;
        end
      end
    end
  end

  assign r_data_o = mem[w_addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("direct_sequential_memory_async_low_reset_init_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialWholeArrayCopyIgnoresUnaryExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    scratch_o = 2'b00;
    scratch_o++;
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_whole_array_copy_ignores_unary_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMissingDefaultChainFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_missing_default_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_missing_default_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_missing_default_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialInvalidDefaultCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_invalid_default_commit_target.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_invalid_default_commit_target(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_q;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor, svPath, {"Unsupported RHS in sequential assignment"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialSimpleResetChainSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_simple_reset_chain";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_simple_reset_chain.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_simple_reset_chain(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni)
      mem_q <= '{8'h11, 8'h22, 8'h33, 8'h44};
    else
      mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_simple_reset_chain"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* rstEnableParam = memoryInst->getInstParameter(NLName("RST_ENABLE"));
  auto* rstAsyncParam = memoryInst->getInstParameter(NLName("RST_ASYNC"));
  auto* rstActiveLowParam = memoryInst->getInstParameter(NLName("RST_ACTIVE_LOW"));
  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, rstEnableParam);
  ASSERT_NE(nullptr, rstAsyncParam);
  ASSERT_NE(nullptr, rstActiveLowParam);
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("1", rstEnableParam->getValue());
  EXPECT_EQ("1", rstAsyncParam->getValue());
  EXPECT_EQ("1", rstActiveLowParam->getValue());
  EXPECT_EQ("32'b01000100001100110010001000010001", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetConstantFalseNoElseSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_constant_false_no_else.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_constant_false_no_else(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
        if (1'b0) begin
          mem_q[i] <= 8'hAA;
        end
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_constant_false_no_else"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* initParam = memoryInst->getInstParameter(NLName("INIT"));
  ASSERT_NE(nullptr, initParam);
  EXPECT_EQ("32'b00000000000000000000000000000000", initParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetIgnoresNonAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_ignores_non_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_ignores_non_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      case (addr_i)
        default: ;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_reset_ignores_non_assignment"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetCaseAssignsTrackedLhsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_case_assigns_tracked_lhs(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o,
  output logic       other_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       other_q;
  logic       other_n;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    other_n = other_q;
    if (en_i) begin
      other_n = ~other_q;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      other_q <= 1'b0;
      case (addr_i)
        default: flag_q <= 1'b1;
      endcase
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      other_q <= other_n;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
  assign other_o = other_q;
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while lowering sequential block"});
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialDynamicResetIndexFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_dynamic_reset_index_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_dynamic_reset_index_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[addr_i] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected dynamic reset index fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetMissingTargetAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_missing_target_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_missing_target_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
    end else begin
      flag_q <= en_i;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q;
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected missing reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetPartialInitializationFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_partial_initialization.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_partial_initialization(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] <= 8'h00;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected partial reset initialization fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialResetCompoundAssignmentFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_reset_compound_assignment.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_reset_compound_assignment(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      mem_q[0] += 8'h01;
    end else begin
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected compound reset assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialSelfCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_self_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_self_commit_target_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_q;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected self commit target fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMismatchedCommitSignatureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_mismatched_commit_signature.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_mismatched_commit_signature(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [9:0] mem_next_bad [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
    for (int i = 0; i < 4; i++) begin
      mem_next_bad[i] = {2'b00, mem_d[i]};
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next_bad;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected mismatched commit signature fallback";
  } catch (const SNLSVConstructorException& e) {
    EXPECT_FALSE(std::string(e.what()).empty());
  }
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresTopLevelConditionalSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_top_level_conditional.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_top_level_conditional(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic       en_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic       flag_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic       flag_q;
  logic       scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      flag_q <= 1'b0;
      scratch_q <= 1'b0;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      flag_q <= en_i;
      if (en_i) begin
        scratch_q <= addr_i[0];
      end
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign flag_o = flag_q ^ scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_top_level_conditional"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("flag_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialIgnoresNonAssignmentExpressionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_ignores_non_assignment_expression.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_ignores_non_assignment_expression(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o,
  output logic [1:0] scratch_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];
  logic [1:0] scratch_q;

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      scratch_q <= 2'b00;
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      ++scratch_q;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
  assign scratch_o = scratch_q;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_shared_sequential_ignores_non_assignment_expression"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("scratch_o")), nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDMemoryInferenceSharedSequentialMultipleTargetAssignmentsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_shared_sequential_multiple_target_assignments_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_shared_sequential_multiple_target_assignments_fallback(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      for (int i = 0; i < 4; i++) begin
        mem_q[i] <= 8'h00;
      end
    end else begin
      mem_q <= mem_next;
      mem_q <= mem_next;
    end
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected shared sequential duplicate target assignment fallback";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_d;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitMemoryInferenceSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_memory_inference_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_memory_inference_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_memory_inference_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_memory_inference_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitElseGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_else_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_else_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_else_guard_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_indexed_commit_else_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (i == 0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitLateConstantFalseGuardSkipSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_late_constant_false_guard_skip_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_late_constant_false_guard_skip_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && 1'b0) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_late_constant_false_guard_skip_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitConstantSelectorFastPathsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_constant_selector_fast_paths_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_constant_selector_fast_paths_supported(
  input  logic       clk_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[1] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[i][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[1];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_constant_selector_fast_paths_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (mem_d[0][0]) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitRhsSelectorMismatchFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_rhs_selector_mismatch_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_rhs_selector_mismatch_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[0];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextDynamicIndexedCommitTargetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_dynamic_indexed_commit_target_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_dynamic_indexed_commit_target_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] commit_idx_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[commit_idx_i] = mem_d[commit_idx_i];
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed commit target";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIfTrueIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iftrue_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iftrue_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] += mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifTrue branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIfFalseIndexedCommitFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_iffalse_indexed_commit_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_iffalse_indexed_commit_failure_fallback(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (sel_i) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] += mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported indexed commit action in ifFalse branch";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIgnoresEmptyAndUnrelatedCommitCandidates) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_empty_and_unrelated_commit_candidates.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_empty_and_unrelated_commit_candidates(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    ;
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_empty_and_unrelated_commit_candidates"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIgnoresNonAssignmentCommitCandidate) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_ignores_non_assignment_commit_candidate";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_ignores_non_assignment_commit_candidate.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_ignores_non_assignment_commit_candidate(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic       scratch;
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    case (addr_i)
      default: ;
    endcase
  end

  always_comb begin
    scratch = addr_i[0];
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      if (i < 2) begin
        mem_next[i] = mem_d[i];
      end else begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_ignores_non_assignment_commit_candidate"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextCompoundCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_compound_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_compound_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_compound_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next[addr_i] += data_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextSelfCopyCommitBlockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_self_copy_commit_block_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_self_copy_commit_block_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_self_copy_commit_block_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    mem_next = mem_next;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitFieldFallbackSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_field_fallback_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_field_fallback_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_field_fallback_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i].mode == 2'b11) begin
        mem_next[i].mode = mem_q[i].mode;
      end
      if (mem_d[i].access == 2'b01) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_field_fallback_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialFieldWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_field_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_field_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_field_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [5:0] payload_i,
  output logic [5:0] payload_o,
  output logic [1:0] reserved_o
);
  typedef struct packed {
    logic [5:0] payload;
    logic [1:0] reserved;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].reserved = 2'b00;
    mem_d[addr_i].payload = payload_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign payload_o = mem_q[addr_i].payload;
  assign reserved_o = mem_q[addr_i].reserved;
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_field_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, wrPortsParam);
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialSliceWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_slice_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_slice_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_slice_writes_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] upper_i,
  input  logic [1:0] lower_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][7:4] = upper_i;
    mem_d[addr_i][1:0] = lower_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_partial_slice_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("2", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePackedElementWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_packed_element_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_packed_element_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_packed_element_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_packed_element_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDynamicPackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] bit_sel_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][bit_sel_i] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangePackedElementWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_packed_element_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_packed_element_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][8] = bit_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range packed element shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedUpRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_up_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_up_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_up_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][2 +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_up_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalOrSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_or_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_or_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_or_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] mode_i,
  input  logic [1:0] access_i,
  input  logic [3:0] payload_i,
  output logic [7:0] data_o
);
  typedef struct packed {
    logic [1:0] mode;
    logic [1:0] access;
    logic [3:0] payload;
  } entry_t;

  entry_t mem_q [0:3];
  entry_t mem_d [0:3];
  entry_t mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i].mode = mode_i;
    mem_d[addr_i].access = access_i;
    mem_d[addr_i].payload = payload_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if ((mem_d[i].mode == 2'b11) || (mem_d[i].access == 2'b01)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = {mem_q[addr_i].mode, mem_q[addr_i].access, mem_q[addr_i].payload};
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_or_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("3", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitSingleBitEqualitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_equality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_equality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_equality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_equality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitSingleBitInequalitySupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_single_bit_inequality_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_single_bit_inequality_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_single_bit_inequality_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] != mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_local_commit_single_bit_inequality_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitEqualityRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_equality_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_equality_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_equality_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic       bit_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][0] = bit_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] == mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_q[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_next_local_commit_logical_and_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndLhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_lhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_lhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[0][0] && mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextLocalCommitLogicalAndRhsFailureFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_local_commit_logical_and_rhs_failure_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_local_commit_logical_and_rhs_failure_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (mem_d[i][0] && mem_d[0][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDNextIndexedCommitUnaryLocalConditionSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_next_indexed_commit_unary_local_condition_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_local_condition_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_d[i];
      if (!mem_d[i][0]) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_next_indexed_commit_unary_local_condition_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(
  SNLSVConstructorTestMemoryInference,
  parseQDNextIndexedCommitUnaryConstantLocalConditionShortcutsSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_next_indexed_commit_unary_constant_local_condition_shortcuts.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_next_indexed_commit_unary_constant_local_condition_shortcuts(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  logic [7:0] mem_next [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always_comb begin
    for (int i = 0; i < 4; i++) begin
      mem_next[i] = mem_q[i];
      if (!(mem_d[i][0] && 1'b0)) begin
        mem_next[i] = mem_d[i];
      end
      if (!(mem_d[i][0] || 1'b1)) begin
        mem_next[i] = mem_q[i];
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_next;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_next_indexed_commit_unary_constant_local_condition_shortcuts"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceIndexedDownRangeWriteSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_indexed_down_range_write_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_indexed_down_range_write_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_indexed_down_range_write_supported(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][5 -: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top =
    library_->getSNLDesign(NLName("qd_memory_inference_indexed_down_range_write_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDynamicIndexedRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_dynamic_indexed_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_dynamic_indexed_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [2:0] start_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][start_i +: 2] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported dynamic indexed range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCurrentEntryBitsSelectorFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_current_entry_bits_selector_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_current_entry_bits_selector_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_current_entry_bits_selector_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[$urandom_range(3, 0)][3:0] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported current-entry selector lowering";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceOutOfRangeSimpleRangeWriteFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_out_of_range_simple_range_write_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_out_of_range_simple_range_write_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [1:0] slice_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][9:8] = slice_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  try {
    constructor.construct(svPath);
    FAIL() << "Expected unsupported out-of-range simple range shadow write";
  } catch (const SNLSVConstructorException& e) {
    const std::string reason = e.what();
    EXPECT_NE(
      std::string::npos,
      reason.find("unsupported statement pattern for sequential lowering"));
  }
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceWholeEntryWriteDataBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_whole_entry_write_data_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_whole_entry_write_data_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = $urandom_range(8'hFF, 8'h00);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferencePartialWriteAssignBitsFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_partial_write_assign_bits_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_partial_write_assign_bits_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i][3:0] = $urandom_range(4'hF, 4'h0);
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDuplicateDynamicSelectorPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath =
    outPath / "qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[addr_i][3:0] = lo_i;
      mem_d[addr_i][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_duplicate_dynamic_selector_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceDisjointConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_disjoint_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_disjoint_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_lo_i,
  input  logic       sel_hi_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_lo_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (sel_hi_i) begin
      mem_d[1][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_disjoint_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSameConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_same_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_same_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[0][3:0] = lo_i;
    mem_d[0][7:4] = hi_i;
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_same_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSymbolicConstantPartialWritesSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_symbolic_constant_partial_writes_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_symbolic_constant_partial_writes_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic       en_i,
  input  logic [3:0] lo_i,
  input  logic [3:0] hi_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      mem_d[0][3:0] = lo_i;
    end
    if (en_i) begin
      mem_d[0][7:4] = hi_i;
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[0];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_symbolic_constant_partial_writes_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceNestedDuplicateGuardSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_nested_duplicate_guard_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_nested_duplicate_guard_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      if (sel_i) begin
        mem_d[addr_i] = data_i;
      end
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("qd_memory_inference_nested_duplicate_guard_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* wrPortsParam = memoryInst->getInstParameter(NLName("WR_PORTS"));
  ASSERT_NE(nullptr, wrPortsParam);
  EXPECT_EQ("1", wrPortsParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceCaseGuardConstantTrueShortcutSupported) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_case_guard_constant_true_shortcut_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_case_guard_constant_true_shortcut_supported(
  input  logic       clk_i,
  input  logic       sel_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    if (sel_i) begin
      case (1'b0)
        1'b0: mem_d[addr_i] = data_i;
        default: mem_d[addr_i] = 8'h00;
      endcase
    end
  end

  always_ff @(posedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("qd_memory_inference_case_guard_constant_true_shortcut_supported"));
  ASSERT_NE(top, nullptr);

  SNLInstance* memoryInst = nullptr;
  for (auto inst : top->getInstances()) {
    if (NLDB0::isMemory(inst->getModel())) {
      ASSERT_EQ(nullptr, memoryInst);
      memoryInst = inst;
    }
  }
  ASSERT_NE(nullptr, memoryInst);

  auto* widthParam = memoryInst->getInstParameter(NLName("WIDTH"));
  ASSERT_NE(nullptr, widthParam);
  EXPECT_EQ("8", widthParam->getValue());
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceUntimedSequentialCandidateIgnored) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_untimed_sequential_candidate_ignored.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_untimed_sequential_candidate_ignored(
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"Unsupported statement while extracting sequential timing control",
     "unable to resolve a single-bit clock net"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSequentialNedgeClockFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_nedge_clock_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_nedge_clock_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(negedge clk_i) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement pattern for sequential lowering"});
}

TEST_F(SNLSVConstructorTestMemoryInference, parseQDMemoryInferenceSequentialClockNetFallback) {
  SNLSVConstructor constructor(library_);
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / "qd_memory_inference_sequential_clock_net_fallback";
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);

  const auto svPath = outPath / "qd_memory_inference_sequential_clock_net_fallback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module qd_memory_inference_sequential_clock_net_fallback(
  input  logic       clk_i,
  input  logic [1:0] addr_i,
  input  logic [7:0] data_i,
  output logic [7:0] data_o
);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];

  always_comb begin
    mem_d = mem_q;
    mem_d[addr_i] = data_i;
  end

  always @(posedge (clk_i + 2'd1)) begin
    mem_q <= mem_d;
  end

  assign data_o = mem_q[addr_i];
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unable to resolve a single-bit clock net"});
}
