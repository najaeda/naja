// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_list_single_wrapper_supported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Single-statement begin/end wrapper to exercise StatementList unwrap path.
  always begin
    @(posedge clk) if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
