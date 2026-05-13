// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_event_list_negedge_reset_supported(
  input logic clk,
  input logic rst_n,
  input logic [7:0] d,
  output logic [7:0] q
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) q <= 8'h00;
    else q <= d;
  end
endmodule
