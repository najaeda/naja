// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

typedef enum logic [1:0] {
  NONE  = 2'b00,
  CVXIF = 2'b11
} fu_t;

typedef struct packed {
  fu_t fu;
  logic valid;
} scoreboard_entry_t;

module continuous_resolve_expression_bits_failure_paths_unsupported_top(
  input logic [3:0] a,
  input logic [3:0] b,
  input logic idx,
  input scoreboard_entry_t [1:0] issue_instr_i,
  output logic [3:0] y_unknown_const,
  output logic [3:0] y_call_cast,
  output logic [3:0] y_nested_binary,
  output logic [7:0] y_concat_add,
  output logic y_member_eq
);
  localparam logic [3:0] BAD_CONST = 4'bx001;

  // Unknown integer constant path in resolveExpressionBits.
  assign y_unknown_const = a + BAD_CONST;
  // Signed call-cast argument recursion failure path.
  assign y_call_cast = $signed({a, BAD_CONST}) + b;
  // Nested binary expression recursion failure path.
  assign y_nested_binary = (a & BAD_CONST) + b;
  // Concatenation operand recursion failure path.
  assign y_concat_add = {a, BAD_CONST} + 8'h00;
  // Member-access fast path where the base element-select cannot resolve.
  assign y_member_eq = (issue_instr_i[idx].fu == CVXIF);
endmodule
