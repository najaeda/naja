// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

interface data_if;
  logic sig;
endinterface

module interface_port_skip_top(
  data_if bus,
  input logic i,
  output logic o
);
  assign o = i;
endmodule
