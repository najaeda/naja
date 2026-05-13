// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_add_const_two_unsupported(
  input logic clk,
  input logic rst,
  output logic [7:0] q
);
  // Unsupported non-increment add constant.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else q <= q + 8'd2;
  end
endmodule
