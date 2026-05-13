// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_enable_cond_unary_edge_cases_skipped(
  input logic clk,
  input logic rst,
  input logic [1:0] en_bus,
  input logic en_scalar,
  input logic [7:0] d0,
  input logic [7:0] d1,
  output logic [7:0] q0,
  output logic [7:0] q1
);
  // Unary bitwise-not with non-single-bit operand: condition net cannot be reduced to 1 bit.
  always_ff @(posedge clk) begin
    if (rst) q0 <= 8'h00;
    else if (~en_bus) q0 <= d0;
    else q0 <= q0;
  end

  // Unary-plus is not lowered by resolveConditionNet and falls back to resolveExpressionNet.
  always_ff @(posedge clk) begin
    if (rst) q1 <= 8'h00;
    else if (+en_scalar) q1 <= d1;
    else q1 <= q1;
  end
endmodule
