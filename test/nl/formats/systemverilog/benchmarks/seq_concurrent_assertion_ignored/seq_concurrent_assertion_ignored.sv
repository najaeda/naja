// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_concurrent_assertion_ignored(
  input logic clk,
  input logic reset,
  input logic d,
  output logic q
);
`ifndef VERILATOR
  // Keep this assertion out of Verilator runs while preserving the
  // constructor path that sees procedural concurrent assertions.
  always assert property (@(posedge clk) d |-> d);
`endif

  always_ff @(posedge clk) begin
    if (reset) begin
      q <= 1'b0;
    end else begin
      q <= d;
    end
  end
endmodule
