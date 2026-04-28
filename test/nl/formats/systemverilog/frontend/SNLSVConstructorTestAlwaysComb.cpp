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

size_t countOutputInstTermDrivers(SNLBitNet* drivenNet) {
  if (!drivenNet) {
    ADD_FAILURE() << "Missing driven net";
    return 0;
  }

  size_t drivers = 0;
  for (auto* instTerm : drivenNet->getInstTerms()) {
    if (instTerm &&
        instTerm->getDirection() == SNLTerm::Direction::Output) {
      ++drivers;
    }
  }
  return drivers;
}

size_t countMux2InputUses(SNLBitNet* net) {
  if (!net) {
    ADD_FAILURE() << "Missing net";
    return 0;
  }

  size_t uses = 0;
  for (auto* instTerm : net->getInstTerms()) {
    if (!instTerm || instTerm->getDirection() != SNLTerm::Direction::Input) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    if (instance && NLDB0::isMux2(instance->getModel())) {
      ++uses;
    }
  }
  return uses;
}

size_t countInputInstTermUses(SNLBitNet* net) {
  if (!net) {
    ADD_FAILURE() << "Missing net";
    return 0;
  }

  size_t uses = 0;
  for (auto* instTerm : net->getInstTerms()) {
    if (instTerm &&
        instTerm->getDirection() == SNLTerm::Direction::Input) {
      ++uses;
    }
  }
  return uses;
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
  parseAlwaysCombNestedPackedElementBasesThenWholeVectorOverride) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_nested_packed_element_bases_then_whole_vector_override");

  const auto svPath =
    outPath / "always_comb_nested_packed_element_bases_then_whole_vector_override.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_nested_packed_element_bases_then_whole_vector_override(
  input  logic [1:0][3:0] a,
  output logic [1:0][3:0] y
);
  always_comb begin
    y[1][0] = 1'b1;
    y[0][2] = 1'b0;
    y = a;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_nested_packed_element_bases_then_whole_vector_override"));
  ASSERT_NE(top, nullptr);
  auto* a = top->getBusNet(NLName("a"));
  auto* y = top->getBusNet(NLName("y"));
  ASSERT_NE(a, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(8, a->getWidth());
  ASSERT_EQ(8, y->getWidth());

  for (NLID::Bit bit = 0; bit < 8; ++bit) {
    ASSERT_NE(a->getBit(bit), nullptr);
    ASSERT_NE(y->getBit(bit), nullptr);
    EXPECT_EQ(a->getBit(bit), getSingleAssignInputDriving(y->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructFieldCompoundAssignmentSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_field_compound_assignment_supported");

  const auto svPath =
    outPath / "always_comb_packed_struct_field_compound_assignment_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_packed_struct_field_compound_assignment_supported(
  input  logic [4:0] seed_i,
  input  logic [3:0] data_mask_i,
  input  logic       valid_mask_i,
  output logic [4:0] y_o
);
  typedef struct packed {
    logic [3:0] data;
    logic       valid;
  } pkt_s;

  pkt_s pkt_n;

  always_comb begin
    pkt_n = seed_i;
    pkt_n.data |= data_mask_i;
    pkt_n.valid &= valid_mask_i;
    y_o = pkt_n;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_field_compound_assignment_supported"));
  ASSERT_NE(top, nullptr);
  auto* pkt = top->getBusNet(NLName("pkt_n"));
  ASSERT_NE(pkt, nullptr);
  ASSERT_EQ(5, pkt->getWidth());
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructWholeThenFieldBranchOverridesSingleDriver) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_whole_then_field_branch_overrides_single_driver");

  const auto svPath =
    outPath / "always_comb_packed_struct_whole_then_field_branch_overrides_single_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_packed_struct_whole_then_field_branch_overrides_single_driver(
  input  logic       sel_i,
  input  logic [2:0] miss_i,
  input  logic [1:0] dirty_i,
  input  logic       lru_i,
  output logic [2:0] y_o
);
  typedef struct packed {
    logic [1:0] dirty;
    logic       lru;
  } stat_s;

  stat_s s;

  always_comb begin
    if (sel_i) begin
      s = miss_i;
    end else begin
      s.dirty = dirty_i;
      s.lru = lru_i;
    end
    y_o = s;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_whole_then_field_branch_overrides_single_driver"));
  ASSERT_NE(top, nullptr);
  auto* s = top->getBusNet(NLName("s"));
  ASSERT_NE(s, nullptr);
  ASSERT_EQ(3, s->getWidth());

  for (NLID::Bit bit = 0; bit < 3; ++bit) {
    ASSERT_NE(s->getBit(bit), nullptr);
    EXPECT_NE(nullptr, getSingleAssignInputDriving(s->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombNestedPackedStructPayloadThenSubfieldsSingleDriver) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_nested_packed_struct_payload_then_subfields_single_driver");

  const auto svPath =
    outPath / "always_comb_nested_packed_struct_payload_then_subfields_single_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_nested_packed_struct_payload_then_subfields_single_driver(
  input  logic [3:0] msg_i,
  input  logic [1:0] src_i,
  input  logic       dst_i,
  input  logic [2:0] did_i,
  output logic [9:0] y_o
);
  typedef struct packed {
    logic [1:0] src_id;
    logic       dst_id;
    logic [2:0] src_did;
  } payload_s;

  typedef struct packed {
    logic [3:0] msg_type;
    payload_s   payload;
  } header_s;

  header_s h;

  always_comb begin
    h.msg_type = msg_i;
    h.payload = '0;
    h.payload.src_id = src_i;
    h.payload.dst_id = dst_i;
    h.payload.src_did = did_i;
    y_o = h;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_nested_packed_struct_payload_then_subfields_single_driver"));
  ASSERT_NE(top, nullptr);
  auto* h = top->getBusNet(NLName("h"));
  ASSERT_NE(h, nullptr);
  ASSERT_EQ(10, h->getWidth());

  for (NLID::Bit bit = 0; bit < 10; ++bit) {
    ASSERT_NE(h->getBit(bit), nullptr);
    EXPECT_NE(nullptr, getSingleAssignInputDriving(h->getBit(bit)))
      << "bit " << bit;
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombPackedStructConditionLeavesExternalFieldDriverSingleDriver) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_packed_struct_condition_leaves_external_field_driver_single_driver");

  const auto svPath =
    outPath / "always_comb_packed_struct_condition_leaves_external_field_driver_single_driver.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module ready_source(output logic ready_o);
  assign ready_o = 1'b1;
endmodule

module always_comb_packed_struct_condition_leaves_external_field_driver_single_driver(
  input  logic       sel_i,
  input  logic [1:0] data_i,
  output logic [3:0] y_o
);
  typedef struct packed {
    logic       v;
    logic       ready;
    logic [1:0] data;
  } link_s;

  link_s link;

  ready_source ready_i(.ready_o(link.ready));

  always_comb begin
    link.v = 1'b0;
    link.data = data_i;
    if (sel_i) begin
      link.v = 1'b1;
    end else begin
      link.data = 2'b10;
    end
    y_o = link;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_packed_struct_condition_leaves_external_field_driver_single_driver"));
  ASSERT_NE(top, nullptr);
  auto* link = top->getBusNet(NLName("link"));
  ASSERT_NE(link, nullptr);
  ASSERT_EQ(4, link->getWidth());
  ASSERT_NE(link->getBit(2), nullptr);
  EXPECT_EQ(1u, countOutputInstTermDrivers(link->getBit(2)));
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

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombRHSCurrentLHSUsesReplayValueNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_rhs_current_lhs_uses_replay_value_no_feedback");

  const auto svPath =
    outPath / "always_comb_rhs_current_lhs_uses_replay_value_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_rhs_current_lhs_uses_replay_value_no_feedback(
  input  logic [3:0] a_i,
  input  logic       sel_i,
  input  logic       kill_i,
  output logic [3:0] y_o
);
  always_comb begin
    y_o = a_i;
    if (sel_i) begin
      y_o = kill_i ? 4'b0000 : y_o;
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_rhs_current_lhs_uses_replay_value_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* y = top->getBusNet(NLName("y_o"));
  ASSERT_NE(y, nullptr);
  ASSERT_EQ(4, y->getWidth());
  for (auto* bit : y->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(0u, countMux2InputUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombConditionCurrentLHSUsesReplayValueNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_condition_current_lhs_uses_replay_value_no_feedback");

  const auto svPath =
    outPath / "always_comb_condition_current_lhs_uses_replay_value_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_condition_current_lhs_uses_replay_value_no_feedback(
  input  logic [4:0] q_i,
  input  logic [4:0] init_i,
  input  logic       op_i,
  output logic [4:0] r_o,
  output logic       y_o
);
  always_comb begin
    r_o = q_i;
    y_o = 1'b0;
    if (op_i) begin
      r_o = init_i;
      y_o = 1'b1;
      if (r_o == 5'd4) begin
        r_o -= 5'd1;
      end
    end
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_condition_current_lhs_uses_replay_value_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* r = top->getBusNet(NLName("r_o"));
  ASSERT_NE(r, nullptr);
  ASSERT_EQ(5, r->getWidth());
  for (auto* bit : r->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(0u, countInputInstTermUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombTemporaryUsesCurrentLHSReplayValueNoFeedback) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_temporary_uses_current_lhs_replay_value_no_feedback");

  const auto svPath =
    outPath / "always_comb_temporary_uses_current_lhs_replay_value_no_feedback.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_temporary_uses_current_lhs_replay_value_no_feedback(
  input  logic [3:0] ext_i,
  input  logic       sel_i,
  output logic [3:0] result_o
);
  logic [3:0] rev;

  always_comb begin
    result_o = ext_i;
    for (int i = 0; i < 4; i++) begin
      rev[i] = result_o[3-i];
    end
    result_o = sel_i ? rev : result_o;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_temporary_uses_current_lhs_replay_value_no_feedback"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
    EXPECT_EQ(0u, countInputInstTermUses(bit)) << bit->getString();
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombTemporarySelectionReplaySupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_temporary_selection_replay_supported");

  const auto svPath =
    outPath / "always_comb_temporary_selection_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_temporary_selection_replay_supported(
  input  logic [3:0] seed_i,
  input  logic [1:0] lo_i,
  input  logic       hi_i,
  output logic [3:0] result_o
);
  logic [3:0] tmp;

  always_comb begin
    tmp = seed_i;
    tmp[1:0] = lo_i;
    tmp[3] = hi_i;
    result_o = {tmp[0], tmp[3:1]};
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_temporary_selection_replay_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombCaseTemporaryReplayDependenciesSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_case_temporary_replay_dependencies_supported");

  const auto svPath =
    outPath / "always_comb_case_temporary_replay_dependencies_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_case_temporary_replay_dependencies_supported(
  input  logic [3:0] seed_i,
  input  logic [1:0] sel_i,
  input  logic       a_i,
  input  logic       b_i,
  output logic [3:0] result_o
);
  logic [3:0] tmp;

  always_comb begin
    tmp = seed_i;
    unique case (sel_i)
      2'd0: tmp[0] = a_i;
      2'd1: tmp[1] = b_i;
      default: tmp[2] = a_i ^ b_i;
    endcase
    result_o = tmp;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_case_temporary_replay_dependencies_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseAlwaysCombTemporarySelectionCompoundReplaySupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "always_comb_temporary_selection_compound_replay_supported");

  const auto svPath =
    outPath / "always_comb_temporary_selection_compound_replay_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module always_comb_temporary_selection_compound_replay_supported(
  input  logic [3:0] seed_i,
  input  logic [2:0] inc_i,
  output logic [3:0] result_o
);
  logic [3:0] tmp;

  always_comb begin
    tmp = seed_i;
    tmp[2:0] += inc_i;
    result_o = tmp;
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("always_comb_temporary_selection_compound_replay_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
}

TEST_F(
  SNLSVConstructorTestAlwaysComb,
  parseFunctionDirectReturnLocalAssignmentsSupported) {
  SNLSVConstructor constructor(library_);
  auto outPath = createTestDirectory(
    "function_direct_return_local_assignments_supported");

  const auto svPath =
    outPath / "function_direct_return_local_assignments_supported.sv";
  std::ofstream svFile(svPath);
  ASSERT_TRUE(svFile.good());
  svFile
    << R"(module function_direct_return_local_assignments_supported(
  input  logic [3:0] seed_i,
  input  logic       hi_i,
  output logic [3:0] result_o
);
  function automatic logic [3:0] build(input logic [3:0] seed, input logic hi);
    logic [3:0] tmp;
    tmp = seed;
    tmp[3] = hi;
    return {tmp[0], tmp[3:1]};
  endfunction

  always_comb begin
    result_o = build(seed_i, hi_i);
  end
endmodule
)";
  svFile.close();

  constructor.construct(svPath);

  auto* top = library_->getSNLDesign(
    NLName("function_direct_return_local_assignments_supported"));
  ASSERT_NE(top, nullptr);
  auto* result = top->getBusNet(NLName("result_o"));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(4, result->getWidth());
  for (auto* bit : result->getBits()) {
    ASSERT_NE(bit, nullptr);
    EXPECT_EQ(1u, countOutputInstTermDrivers(bit));
  }
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
    {"unsupported always_comb assignment LHS: NamedValue base=s"});
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
    {"unsupported always_comb assignment LHS: NamedValue base=s_n"});
}
