// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

typedef struct packed {
  logic [3:0] a;
  logic b;
} st_t;

typedef enum logic [1:0] {
  IDLE = 2'b00,
  BUSY = 2'b01
} state_t;

module packed_struct_enum_ports_top(
  input st_t in_s,
  input state_t st_i,
  output st_t out_s,
  output state_t st_o
);
  assign out_s = in_s;
  assign st_o = st_i;
endmodule
