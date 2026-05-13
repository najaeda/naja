// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_enable_cond_unary_not_supported(
  input logic clk,
  input logic rst_n,
  input logic en,
  input logic [7:0] d0,
  input logic [7:0] d1,
  output logic [7:0] q0,
  output logic [7:0] q1
);
  // Exercise unary condition lowering in resolveConditionNet:
  // logical not and bitwise not on a single-bit predicate.
  always_ff @(posedge clk) begin
    if (!rst_n) q0 <= 8'h00;
    else if (!en) q0 <= d0;
    else q0 <= q0;
  end

  always_ff @(posedge clk) begin
    if (!rst_n) q1 <= 8'h00;
    else if (~en) q1 <= d1;
    else q1 <= q1;
  end
endmodule
