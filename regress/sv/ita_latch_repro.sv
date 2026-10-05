// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

// Reduced ITA weight-buffer scheduling case: a flop reads a latch bank on
// the same edge that opens that bank for a new write. A shared write-data
// register still holds the other bank's previous data before its NBA update.
module ita_latch_repro #(
  parameter integer READ_DELAY_PS = 1
);
  timeunit 1ns;
  timeprecision 1ps;
  logic clk = 0;
  logic write_enable = 0;
  logic write_bank = 0;
  logic [7:0] write_data = 0;
  wire [7:0] captured, read_data, read_feedback;
  ita_latch_dut dut(.*);
  always #5 clk = ~clk;
  if (READ_DELAY_PS == 0) begin : zero_delay
    assign read_feedback = read_data;
  end else begin : propagation_delay
    assign #(READ_DELAY_PS * 1ps) read_feedback = read_data;
  end

  initial begin
    #11; write_enable = 1; write_bank = 0; write_data = 8'h11;
    #10; write_bank = 1; write_data = 8'h22;
    #10; write_bank = 0; write_data = 8'h33;
    #5;
    $display("captured=%h read_data=%h", captured, read_data);
    if (captured !== 8'h11 || read_data !== 8'h33)
      $fatal(1, "Latch bank reader did not capture the pre-edge value");
    $finish;
  end
endmodule
