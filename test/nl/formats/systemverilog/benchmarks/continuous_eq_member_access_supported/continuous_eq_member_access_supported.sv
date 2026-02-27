// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

typedef enum logic [1:0] {
  NONE  = 2'b00,
  CVXIF = 2'b11
} fu_t;

typedef struct packed {
  fu_t fu;
  logic valid;
} scoreboard_entry_t;

module continuous_eq_member_access_supported_top(
  input scoreboard_entry_t [1:0] issue_instr_i,
  output logic cvxif_req_allowed
);
  assign cvxif_req_allowed = (issue_instr_i[0].fu == CVXIF);
endmodule
