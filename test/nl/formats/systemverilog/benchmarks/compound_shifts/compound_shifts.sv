// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module compound_shifts_top(
  input logic [7:0] value,
  input logic [9:0] amount,
  output logic [7:0] left_o, right_o, arithmetic_left_o, unsigned_arithmetic_o,
  output logic signed [7:0] signed_right_o,
  output logic [7:0] replay_o,
  output logic [3:0] field_o,
  output logic [7:0] constant_o, giant_o
);
  typedef struct packed { logic [3:0] high; logic [3:0] low; } pair_t;
  pair_t pair;
  logic [69:0] wide_amount;
  always_comb begin
    left_o = value;
    left_o <<= amount;
    right_o = value;
    right_o >>= amount;
    arithmetic_left_o = value;
    arithmetic_left_o <<<= amount;
    unsigned_arithmetic_o = value;
    unsigned_arithmetic_o >>>= amount;
    signed_right_o = value;
    signed_right_o >>>= amount;
    replay_o = value;
    replay_o <<= 1;
    replay_o >>= amount;
    pair = value;
    pair.low <<= amount;
    field_o = pair.low;
    wide_amount = {value[0], 59'b0, amount};
    giant_o = value;
    giant_o <<= wide_amount;
    constant_o = value;
    constant_o >>= 2;
  end
endmodule
