// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "NLDB0.h"
#include "NLDB.h"
#include "NLUniverse.h"
#include "NLLibrary.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLNet.h"
#include "SNLScalarTerm.h"

#include "SNLSVConstructor.h"

using namespace naja::NL;

#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

std::filesystem::path createTestDirectory(const char* name) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / name;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  return outPath;
}

SNLNet* getSingleDFFInputDriving(SNLBitNet* drivenNet) {
  if (!drivenNet) {
    ADD_FAILURE() << "Missing driven net";
    return nullptr;
  }

  std::vector<SNLInstance*> dffDrivers;
  for (auto* instTerm : drivenNet->getInstTerms()) {
    if (!instTerm || instTerm->getBitTerm() != NLDB0::getDFFOutput()) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    if (instance && instance->getModel() == NLDB0::getDFF()) {
      dffDrivers.push_back(instance);
    }
  }

  EXPECT_EQ(1u, dffDrivers.size()) << drivenNet->getString();
  if (dffDrivers.size() != 1) {
    return nullptr;
  }

  auto* inputTerm = dffDrivers.front()->getInstTerm(NLDB0::getDFFData());
  if (!inputTerm) {
    ADD_FAILURE() << "DFF driver has no data term";
    return nullptr;
  }
  return inputTerm->getNet();
}

SNLNet* getSingleDFFRNInputDriving(SNLBitNet* drivenNet) {
  if (!drivenNet) {
    ADD_FAILURE() << "Missing driven net";
    return nullptr;
  }

  std::vector<SNLInstance*> dffrnDrivers;
  for (auto* instTerm : drivenNet->getInstTerms()) {
    if (!instTerm || instTerm->getBitTerm() != NLDB0::getDFFRNOutput()) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    if (instance && NLDB0::isDFFRN(instance->getModel())) {
      dffrnDrivers.push_back(instance);
    }
  }

  EXPECT_EQ(1u, dffrnDrivers.size()) << drivenNet->getString();
  if (dffrnDrivers.size() != 1) {
    return nullptr;
  }

  auto* inputTerm = dffrnDrivers.front()->getInstTerm(NLDB0::getDFFRNData());
  if (!inputTerm) {
    ADD_FAILURE() << "DFFRN driver has no data term";
    return nullptr;
  }
  return inputTerm->getNet();
}

} // namespace

class SNLSVConstructorTestSequentialOverlap: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto* db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }

    NLLibrary* library_ {nullptr};
};

TEST_F(
  SNLSVConstructorTestSequentialOverlap,
  parseSequentialWholeVectorThenConstantBitOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "sequential_whole_vector_then_constant_bit_override");

  const auto svPath =
    outPath / "sequential_whole_vector_then_constant_bit_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_whole_vector_then_constant_bit_override(
  input  logic       clk_i,
  input  logic [3:0] a,
  output logic [3:0] y
);
  always_ff @(posedge clk_i) begin
    y <= a;
    y[1] <= 1'b1;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("sequential_whole_vector_then_constant_bit_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);

  EXPECT_EQ(a->getBit(0), getSingleDFFInputDriving(y->getBit(0)));
  EXPECT_EQ(a->getBit(2), getSingleDFFInputDriving(y->getBit(2)));
  EXPECT_EQ(a->getBit(3), getSingleDFFInputDriving(y->getBit(3)));

  auto* y1Driver = getSingleDFFInputDriving(y->getBit(1));
  ASSERT_NE(y1Driver, nullptr);
  EXPECT_NE(a->getBit(1), y1Driver);
  EXPECT_TRUE(y1Driver->isAssign1());
}

TEST_F(
  SNLSVConstructorTestSequentialOverlap,
  parseSequentialOverlappingConstantRangesLastWriteWins) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "sequential_overlapping_constant_ranges_last_write_wins");

  const auto svPath =
    outPath / "sequential_overlapping_constant_ranges_last_write_wins.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_overlapping_constant_ranges_last_write_wins(
  input  logic       clk_i,
  input  logic [2:0] a,
  input  logic [2:0] b,
  output logic [3:0] y
);
  always_ff @(posedge clk_i) begin
    y[3:1] <= a;
    y[2:0] <= b;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("sequential_overlapping_constant_ranges_last_write_wins"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* b = top->getBusNet(NLName("b"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(y, nullptr);

  EXPECT_EQ(b->getBit(0), getSingleDFFInputDriving(y->getBit(0)));
  EXPECT_EQ(b->getBit(1), getSingleDFFInputDriving(y->getBit(1)));
  EXPECT_EQ(b->getBit(2), getSingleDFFInputDriving(y->getBit(2)));
  EXPECT_EQ(a->getBit(2), getSingleDFFInputDriving(y->getBit(3)));
}

TEST_F(
  SNLSVConstructorTestSequentialOverlap,
  parseSequentialResetOverlappingConstantRangesLastWriteWins) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "sequential_reset_overlapping_constant_ranges_last_write_wins");

  const auto svPath =
    outPath / "sequential_reset_overlapping_constant_ranges_last_write_wins.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module sequential_reset_overlapping_constant_ranges_last_write_wins(
  input  logic       clk_i,
  input  logic       rst_ni,
  input  logic [2:0] a,
  input  logic [2:0] b,
  output logic [3:0] y
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (~rst_ni) begin
      y[3:1] <= '0;
      y[2:0] <= '0;
    end else begin
      y[3:1] <= a;
      y[2:0] <= b;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("sequential_reset_overlapping_constant_ranges_last_write_wins"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* b = top->getBusNet(NLName("b"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(y, nullptr);

  EXPECT_EQ(b->getBit(0), getSingleDFFRNInputDriving(y->getBit(0)));
  EXPECT_EQ(b->getBit(1), getSingleDFFRNInputDriving(y->getBit(1)));
  EXPECT_EQ(b->getBit(2), getSingleDFFRNInputDriving(y->getBit(2)));
  EXPECT_EQ(a->getBit(2), getSingleDFFRNInputDriving(y->getBit(3)));
}
