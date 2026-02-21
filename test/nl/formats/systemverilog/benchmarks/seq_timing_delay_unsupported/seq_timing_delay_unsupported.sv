// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_delay_unsupported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported timing extraction case: delay control is not a supported clock event.
  always begin
    #1 if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
