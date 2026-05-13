// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_rhs_wide_unknown_const_unsupported (
  input  logic         clk,
  input  logic         rst,
  output logic [127:0] q
);
  localparam logic [127:0] BAD_CONST = 128'hxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;

  always_ff @(posedge clk) begin
    if (rst) begin
      q <= 128'h0;
    end else begin
      q <= BAD_CONST;
    end
  end
endmodule
