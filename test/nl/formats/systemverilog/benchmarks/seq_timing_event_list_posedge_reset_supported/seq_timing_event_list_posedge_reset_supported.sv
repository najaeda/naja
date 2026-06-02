// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_event_list_posedge_reset_supported(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Event-list timing extraction with clock and reset events.
  always_ff @(posedge clk or posedge rst) begin
    if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
