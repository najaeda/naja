// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_rhs_wide_const_supported (
  input  logic         clk,
  input  logic         rst,
  output logic [127:0] q
);
  always_ff @(posedge clk) begin
    if (rst) begin
      q <= 128'hFEDCBA98765432100123456789ABCDEF;
    end else begin
      q <= 128'h0123456789ABCDEFFEDCBA9876543210;
    end
  end
endmodule
