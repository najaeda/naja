// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_nested_begin_wrapper_supported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Nested begin/end wrapper around timed statement.
  always begin
    begin
      @(posedge clk) if (rst) q <= 8'h00;
      else q <= d;
    end
  end
endmodule
