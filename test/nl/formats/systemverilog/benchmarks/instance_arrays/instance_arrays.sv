// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module instance_array_leaf(
  input  logic a,
  output logic y
);
  assign y = a;
endmodule

interface instance_array_if;
  logic sig;
  modport sink(input sig);
endinterface

module instance_array_sink(
  instance_array_if.sink bus,
  output logic y
);
  assign y = bus.sig;
endmodule

module instance_arrays_top(
  input  logic [3:0] module_i,
  output logic [3:0] module_o,
  input  logic [5:0] matrix_i,
  output logic [5:0] matrix_o,
  input  logic [1:0] interface_i,
  output logic [1:0] interface_o
);
  instance_array_leaf u[4] (
    .a(module_i),
    .y(module_o)
  );

  instance_array_leaf matrix[2][3] (
    .a(matrix_i),
    .y(matrix_o)
  );

  for (genvar i = 0; i < 2; i++) begin : gen_interfaces
    instance_array_if buses[2] ();
    assign buses[0].sig = interface_i[i];
    instance_array_sink sink (
      .bus(buses[0]),
      .y(interface_o[i])
    );
  end
endmodule
