// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <initializer_list>
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
#include "SNLSVConstructorException.h"

using namespace naja::NL;

#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

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

SNLNet* getSingleAssignInputDriving(SNLBitNet* drivenNet) {
  if (!drivenNet) {
    ADD_FAILURE() << "Missing driven net";
    return nullptr;
  }

  std::vector<SNLInstance*> assignDrivers;
  for (auto* instTerm : drivenNet->getInstTerms()) {
    if (!instTerm || instTerm->getBitTerm() != NLDB0::getAssignOutput()) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    if (instance && NLDB0::isAssign(instance->getModel())) {
      assignDrivers.push_back(instance);
    }
  }

  EXPECT_EQ(1u, assignDrivers.size()) << drivenNet->getString();
  if (assignDrivers.size() != 1) {
    return nullptr;
  }

  auto* inputTerm = assignDrivers.front()->getInstTerm(NLDB0::getAssignInput());
  if (!inputTerm) {
    ADD_FAILURE() << "Assign driver has no input term";
    return nullptr;
  }
  return inputTerm->getNet();
}

std::filesystem::path createTestDirectory(const char* name) {
  std::filesystem::path outPath(SNL_SV_DUMPER_TEST_PATH);
  outPath = outPath / name;
  if (std::filesystem::exists(outPath)) {
    std::filesystem::remove_all(outPath);
  }
  std::filesystem::create_directory(outPath);
  return outPath;
}

}

class SNLSVConstructorTestAlwaysComb: public ::testing::Test {
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

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConstantBitThenWholeVectorOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_constant_bit_then_whole_vector_override");

  const auto svPath =
    outPath / "always_comb_constant_bit_then_whole_vector_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_constant_bit_then_whole_vector_override(
  input  logic [3:0] a,
  output logic [3:0] y
);
  always_comb begin
    y[1] = 1'b1;
    y = a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_constant_bit_then_whole_vector_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(4, a->getWidth());
  ASSERT_EQ(4, y->getWidth());

  for (NLID::Bit bit = 0; bit < 4; ++bit) {
    ASSERT_NE(a->getBit(bit), nullptr);
    ASSERT_NE(y->getBit(bit), nullptr);
    EXPECT_EQ(a->getBit(bit), getSingleAssignInputDriving(y->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombWholeVectorThenConstantBitOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_whole_vector_then_constant_bit_override");

  const auto svPath =
    outPath / "always_comb_whole_vector_then_constant_bit_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_whole_vector_then_constant_bit_override(
  input  logic [3:0] a,
  output logic [3:0] y
);
  always_comb begin
    y = a;
    y[1] = 1'b1;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_whole_vector_then_constant_bit_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(4, a->getWidth());
  ASSERT_EQ(4, y->getWidth());

  for (NLID::Bit bit : {0, 2, 3}) {
    ASSERT_NE(a->getBit(bit), nullptr);
    ASSERT_NE(y->getBit(bit), nullptr);
    EXPECT_EQ(a->getBit(bit), getSingleAssignInputDriving(y->getBit(bit)))
      << "bit " << bit;
  }

  ASSERT_NE(a->getBit(1), nullptr);
  ASSERT_NE(y->getBit(1), nullptr);
  auto* y1Driver = getSingleAssignInputDriving(y->getBit(1));
  ASSERT_NE(y1Driver, nullptr);
  EXPECT_NE(a->getBit(1), y1Driver);
  EXPECT_TRUE(y1Driver->isAssign1());
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombOverlappingConstantRangesLastWriteWins) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_overlapping_constant_ranges_last_write_wins");

  const auto svPath =
    outPath / "always_comb_overlapping_constant_ranges_last_write_wins.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_overlapping_constant_ranges_last_write_wins(
  input  logic [2:0] a,
  input  logic [2:0] b,
  output logic [3:0] y
);
  always_comb begin
    y[3:1] = a;
    y[2:0] = b;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_overlapping_constant_ranges_last_write_wins"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* b = top->getBusNet(NLName("b"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  ASSERT_NE(y, nullptr);

  EXPECT_EQ(b->getBit(0), getSingleAssignInputDriving(y->getBit(0)));
  EXPECT_EQ(b->getBit(1), getSingleAssignInputDriving(y->getBit(1)));
  EXPECT_EQ(b->getBit(2), getSingleAssignInputDriving(y->getBit(2)));
  EXPECT_EQ(a->getBit(2), getSingleAssignInputDriving(y->getBit(3)));
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombCaseAssignmentFunctionCallSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_case_assignment_function_call_supported");

  const auto svPath = outPath / "always_comb_case_assignment_function_call_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_assignment_function_call_unsupported(
  input  logic [3:0] op_i,
  output logic [1:0] data_size_o
);
  function automatic logic [1:0] extract_transfer_size(input logic [3:0] op);
    unique case (op)
      4'h0: extract_transfer_size = 2'b00;
      4'h1: extract_transfer_size = 2'b01;
      default: extract_transfer_size = 2'b11;
    endcase
  endfunction

  always_comb begin
    data_size_o = extract_transfer_size(op_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(
    NLName("always_comb_case_assignment_function_call_unsupported"));
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getNet(NLName("data_size_o")), nullptr);
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseAlwaysCombConcatenationLHSSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory("always_comb_concatenation_lhs_supported");

  const auto svPath = outPath / "always_comb_concatenation_lhs_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_concatenation_lhs_supported(
  input  logic [1:0] d_i,
  output logic       a_o,
  output logic       b_o
);
  always_comb begin
    {a_o, b_o} = d_i;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto top = library_->getSNLDesign(NLName("always_comb_concatenation_lhs_supported"));
  ASSERT_NE(top, nullptr);
  EXPECT_FALSE(top->isBlackBox());
  EXPECT_NE(top->getNet(NLName("a_o")), nullptr);
  EXPECT_NE(top->getNet(NLName("b_o")), nullptr);
}

TEST_F(SNLSVConstructorTestAlwaysComb, parseAlwaysCombWhileStatementUnsupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory("always_comb_while_statement_unsupported");

  const auto svPath = outPath / "always_comb_while_statement_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_while_statement_unsupported(
  input  logic en_i,
  input  logic d_i,
  output logic y_o
);
  always_comb begin
    while (en_i) begin
      y_o = d_i;
    end
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"unsupported statement kind while collecting assignments"});
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombUnpackedStructMemberLHSBitsUnsupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_unpacked_struct_member_lhs_bits_unsupported");

  const auto svPath =
    outPath / "always_comb_unpacked_struct_member_lhs_bits_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_unpacked_struct_member_lhs_bits_unsupported(
  input  logic [1:0] d_i,
  output logic [7:0] q_o
);
  typedef struct {
    logic [1:0] a;
  } unpacked_s_t;
  unpacked_s_t s;

  always_comb begin
    s.a = d_i;
    q_o = {6'b0, s.a};
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to resolve always_comb assignment LHS bits"});
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombMemberAccessNonIntegralLHSWidthUnsupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_member_access_non_integral_lhs_width_unsupported");

  const auto svPath =
    outPath / "always_comb_member_access_non_integral_lhs_width_unsupported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_member_access_non_integral_lhs_width_unsupported;
  typedef struct {
    logic [1:0] a [0:1];
  } unpacked_s_t;

  unpacked_s_t s_q;
  unpacked_s_t s_n;

  always_comb begin
    s_n.a = s_q.a;
  end
endmodule
)";
  svFile.close();

  expectUnsupportedConstruct(
    constructor,
    svPath,
    {"failed to resolve always_comb assignment LHS bits"});
}
