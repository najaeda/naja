// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module latch_loops_top(
  input logic [3:0] en,
  input logic [7:0] d, other,
  input logic override_en,
  output logic [31:0] array_o, nested_o,
  output logic [7:0] priority_o, partial_o, nb_priority_o, branch_o,
  output logic [3:0] zero_o
);
  logic [7:0] words [0:3];
  logic [1:0][3:0] bytes [0:3];
  integer k;
  always_latch begin
    for (k = 0; k < 4; k++)
      if (en[k]) words[k] <= d;
  end
  always_latch begin
    zero_o = '0;
    for (int i = 0; i < 4; i++) begin
      for (int j = 0; j < 2; j++) begin
        if (en[i]) bytes[i][j] = d[j*4 +: 4];
      end
    end
  end
  always_latch begin
    if (en[0]) priority_o = d;
    if (override_en) priority_o = other;
    if (en[1]) partial_o[3:0] = d[3:0];
    if (en[2]) partial_o[7:4] = d[7:4];
    if (en[3]) begin
      if (override_en) partial_o[5:2] = other[3:0];
      else partial_o[5:2] = other[7:4];
    end
  end
  always_latch begin
    if (en[0]) nb_priority_o <= d;
    if (override_en) nb_priority_o <= other;
    if (en[1]) branch_o[3:0] <= d[3:0];
    else if (en[2]) branch_o[7:4] <= other[7:4];
  end
  for (genvar i = 0; i < 4; i++) begin
    assign array_o[8*i +: 8] = words[i];
    assign nested_o[8*i +: 8] = bytes[i];
  end
endmodule
