// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_preincrement_supported(
  input logic clk,
  input logic rst,
  output logic [7:0] q
);
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else ++q;
  end
endmodule
