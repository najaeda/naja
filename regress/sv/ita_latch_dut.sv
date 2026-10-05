// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

// Synthesizable DUT for the ITA latch scheduling experiment. The read port
// loops back through the testbench so propagation delay stays outside Naja.
module ita_latch_dut(
  input clk,
  input write_enable,
  input write_bank,
  input [7:0] write_data,
  input [7:0] read_feedback,
  output wire [7:0] read_data,
  output logic [7:0] captured
);
  logic [7:0] sampled_write;
  logic [7:0] bank [2];
  logic global_enable;
  wire global_clock = clk & global_enable;
  logic [1:0] bank_enable;
  wire [1:0] bank_clock = {2{global_clock}} & bank_enable;

  always_latch if (!clk) global_enable = write_enable;
  for (genvar i = 0; i < 2; i++) begin : banks
    always_latch if (!global_clock)
      bank_enable[i] = write_enable && (write_bank == i);
    always_latch if (bank_clock[i]) bank[i] = sampled_write;
  end
  always @(posedge clk) begin
    if (write_enable) sampled_write <= write_data;
    captured <= read_feedback;
  end

  assign read_data = bank[0];
endmodule
