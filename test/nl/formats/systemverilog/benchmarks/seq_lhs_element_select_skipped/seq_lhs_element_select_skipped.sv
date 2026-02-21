// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_lhs_element_select_skipped(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Element-select LHS is currently not matched by sameLhs in sequential extraction.
  always_ff @(posedge clk) begin
    if (rst) q[0] <= 1'b0;
    else q[0] <= d[0];
  end
endmodule
