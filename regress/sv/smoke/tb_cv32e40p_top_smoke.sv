// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

`timescale 1ns/1ps

module tb_cv32e40p_top_smoke;
  localparam int unsigned TimeoutCycles = 2000;
  localparam int unsigned FetchPassCount = 4;

  reg clk_i;
  reg rst_ni;

  wire instr_req_o;
  reg instr_rvalid_i;
  wire [31:0] instr_addr_o;

  wire data_req_o;
  reg data_rvalid_i;
  wire data_we_o;
  wire [31:0] data_addr_o;
  wire [31:0] data_wdata_o;

  int unsigned cycle_count;
  int unsigned fetch_count;

  initial begin
    clk_i = 1'b0;
    forever #5 clk_i = ~clk_i;
  end

  initial begin
    rst_ni = 1'b0;
    repeat (8) @(posedge clk_i);
    rst_ni = 1'b1;
  end

  always @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      instr_rvalid_i <= 1'b0;
      data_rvalid_i <= 1'b0;
      cycle_count <= 0;
      fetch_count <= 0;
    end else begin
      instr_rvalid_i <= instr_req_o;
      data_rvalid_i <= data_req_o;
      cycle_count <= cycle_count + 1;
      if (instr_req_o) begin
        fetch_count <= fetch_count + 1;
      end

      if (fetch_count >= FetchPassCount) begin
        $display("CV32E40P_SMOKE_PASS fetch_count=%0d last_pc=0x%08x", fetch_count, instr_addr_o);
        $finish;
      end
      if (cycle_count >= TimeoutCycles) begin
        $fatal(1, "CV32E40P_SMOKE_TIMEOUT fetch_count=%0d", fetch_count);
      end
    end
  end

  cv32e40p_top dut (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .pulp_clock_en_i(1'b1),
    .scan_cg_en_i(1'b0),
    .boot_addr_i(32'h0000_0000),
    .mtvec_addr_i(32'h0000_0000),
    .dm_halt_addr_i(32'h0000_0000),
    .hart_id_i(32'h0),
    .dm_exception_addr_i(32'h0000_0000),
    .instr_req_o(instr_req_o),
    .instr_gnt_i(1'b1),
    .instr_rvalid_i(instr_rvalid_i),
    .instr_addr_o(instr_addr_o),
    .instr_rdata_i(32'h0000_0013),
    .data_req_o(data_req_o),
    .data_gnt_i(1'b1),
    .data_rvalid_i(data_rvalid_i),
    .data_we_o(data_we_o),
    .data_addr_o(data_addr_o),
    .data_wdata_o(data_wdata_o),
    .data_rdata_i(32'h0),
    .irq_i(32'h0),
    .debug_req_i(1'b0),
    .fetch_enable_i(1'b1)
  );
endmodule
