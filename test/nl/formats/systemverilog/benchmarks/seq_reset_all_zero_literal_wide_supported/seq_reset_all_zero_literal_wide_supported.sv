// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_reset_all_zero_literal_wide_supported (
  input  logic         clk,
  input  logic         reset,
  input  logic [127:0] d,
  output logic [127:0] q
);
  always_ff @(posedge clk) begin
    if (reset) begin
      q <= '0;
    end else begin
      q <= d;
    end
  end
endmodule
