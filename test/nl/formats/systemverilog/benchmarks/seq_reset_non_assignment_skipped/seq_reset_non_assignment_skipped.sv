// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_reset_non_assignment_skipped(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported extraction case: reset branch is not a single assignment.
  always_ff @(posedge clk) begin
    if (rst) begin
      q <= d;
      q <= d;
    end else q <= d;
  end
endmodule
