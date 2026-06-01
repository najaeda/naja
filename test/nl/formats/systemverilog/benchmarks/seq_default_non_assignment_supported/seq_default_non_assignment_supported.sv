// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_default_non_assignment_supported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Repeated same-LHS branch assignments follow last-assignment-wins semantics.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else begin
      q <= d;
      q <= d;
    end
  end
endmodule
