// SPDX-FileCopyrightText: 2026 The Naja authors
//
// SPDX-License-Identifier: Apache-2.0

// Primitive behavioral shims used to simulate najaeda-generated Verilog.
// Canonical module names are naja_* to match NLDB0 primitive names.

module naja_fa(
  input wire A,
  input wire B,
  input wire CI,
  output wire S,
  output wire CO
);
  assign {CO, S} = A + B + CI;
endmodule

module naja_mux2 #(
  parameter WIDTH = 1
) (
  input wire [WIDTH-1:0] A,
  input wire [WIDTH-1:0] B,
  input wire S,
  output wire [WIDTH-1:0] Y
);
  assign Y = S ? B : A;
endmodule

module naja_dff(
  input wire C,
  input wire D,
  output reg Q
);
  always @(posedge C) begin
    Q <= D;
  end
endmodule

/* verilator lint_off LATCH */
module naja_dlatch(
  input wire E,
  input wire D,
  output reg Q
);
  always @* begin
    if (E) Q = D;
  end
endmodule
/* verilator lint_on LATCH */

module naja_dffn(
  input wire C,
  input wire D,
  output reg Q
);
  always @(negedge C) begin
    Q <= D;
  end
endmodule

module naja_dffrn(
  input wire C,
  input wire D,
  input wire RN,
  output reg Q
);
  always @(posedge C or negedge RN) begin
    if (!RN) Q <= 1'b0;
    else Q <= D;
  end
endmodule

module naja_dffe(
  input wire C,
  input wire D,
  input wire E,
  output reg Q
);
  always @(posedge C) begin
    if (E) Q <= D;
  end
endmodule

module naja_dffre(
  input wire C,
  input wire D,
  input wire E,
  input wire R,
  output reg Q
);
  always @(posedge C or posedge R) begin
    if (R) Q <= 1'b0;
    else if (E) Q <= D;
  end
endmodule

module naja_dffse(
  input wire C,
  input wire D,
  input wire E,
  input wire S,
  output reg Q
);
  always @(posedge C or posedge S) begin
    if (S) Q <= 1'b1;
    else if (E) Q <= D;
  end
endmodule

module naja_mem #(
  parameter WIDTH = 1,
  parameter DEPTH = 1,
  parameter ABITS = 1,
  parameter RD_PORTS = 1,
  parameter WR_PORTS = 1,
  parameter RST_ENABLE = 0,
  parameter RST_ASYNC = 0,
  parameter RST_ACTIVE_LOW = 0,
  parameter INIT = 1'b0
) (
  input CLK,
  input RST,
  input [RD_PORTS*ABITS-1:0] RADDR,
  output reg [RD_PORTS*WIDTH-1:0] RDATA,
  input [WR_PORTS*ABITS-1:0] WADDR,
  input [WR_PORTS*WIDTH-1:0] WDATA,
  input [WR_PORTS-1:0] WE
);

  reg [WIDTH-1:0] mem [0:DEPTH-1];
  integer rp;
  integer wp;
  integer later;
  reg allow_write;
  reg [ABITS-1:0] addr_value;

  task automatic load_init;
    integer init_idx;
    begin
      for (init_idx = 0; init_idx < DEPTH; init_idx = init_idx + 1)
        mem[init_idx] = INIT[init_idx*WIDTH +: WIDTH];
    end
  endtask

  wire reset_active = RST_ENABLE && (RST_ACTIVE_LOW ? ~RST : RST);

  always @* begin
    for (rp = 0; rp < RD_PORTS; rp = rp + 1) begin
      addr_value = RADDR[rp*ABITS +: ABITS];
      if (addr_value < DEPTH)
        RDATA[rp*WIDTH +: WIDTH] = mem[addr_value];
      else
        RDATA[rp*WIDTH +: WIDTH] = {WIDTH{1'b0}};
    end
  end

  always @(posedge CLK or posedge RST or negedge RST) begin
    if (RST_ASYNC && reset_active) begin
      load_init();
    end else begin
      if (!RST_ASYNC && reset_active)
        load_init();
      else begin
        for (wp = 0; wp < WR_PORTS; wp = wp + 1) begin
          allow_write = WE[wp];
          addr_value = WADDR[wp*ABITS +: ABITS];
          for (later = wp + 1; later < WR_PORTS; later = later + 1) begin
            if (WE[later] && WADDR[later*ABITS +: ABITS] == addr_value)
              allow_write = 1'b0;
          end
          if (allow_write && addr_value < DEPTH)
            mem[addr_value] <= WDATA[wp*WIDTH +: WIDTH];
        end
      end
    end
  end
endmodule
