// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_reset_struct_default_zero_supported (
  input  logic        clk,
  input  logic        rst_ni,
  input  logic [11:0] d,
  input  logic        en,
  output logic [11:0] q_out,
  output logic        valid_out
);
  struct packed {
    logic [11:0] csr_address;
    logic        valid;
  } q, n;

  always_comb begin
    n = q;
    if (en) begin
      n.csr_address = d;
      n.valid = 1'b1;
    end
  end

  always_ff @(posedge clk or negedge rst_ni) begin
    if (~rst_ni) begin
      q <= '{default: 0};
    end else begin
      q <= n;
    end
  end

  assign q_out = q.csr_address;
  assign valid_out = q.valid;
endmodule
