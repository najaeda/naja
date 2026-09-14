// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

`timescale 1ns/1ps

module tb_ibex_top_smoke;
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
  int unsigned progress_count;
  reg seen_fetch;
  reg [31:0] last_fetch_addr;

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
      progress_count <= 0;
      seen_fetch <= 1'b0;
      last_fetch_addr <= 32'h0;
    end else begin
      instr_rvalid_i <= instr_req_o;
      data_rvalid_i <= data_req_o;
      cycle_count <= cycle_count + 1;
      if (instr_req_o) begin
        fetch_count <= fetch_count + 1;
        if ((^instr_addr_o) === 1'bx) begin
          $fatal(1, "IBEX_SMOKE_X_PC fetch_count=%0d", fetch_count);
        end
        if (!seen_fetch) begin
          seen_fetch <= 1'b1;
          progress_count <= 1;
          last_fetch_addr <= instr_addr_o;
        end else if (instr_addr_o != last_fetch_addr) begin
          if (instr_addr_o <= last_fetch_addr) begin
            $fatal(
              1,
              "IBEX_SMOKE_NON_FORWARD_PC previous=0x%08x current=0x%08x",
              last_fetch_addr,
              instr_addr_o);
          end
          progress_count <= progress_count + 1;
          last_fetch_addr <= instr_addr_o;
        end
      end

      if (progress_count >= FetchPassCount) begin
        $display(
          "IBEX_SMOKE_PASS fetch_count=%0d progress_count=%0d last_pc=0x%08x",
          fetch_count,
          progress_count,
          last_fetch_addr);
        $finish;
      end
      if (cycle_count >= TimeoutCycles) begin
        $fatal(
          1,
          "IBEX_SMOKE_TIMEOUT fetch_count=%0d progress_count=%0d",
          fetch_count,
          progress_count);
      end
    end
  end

  ibex_top dut (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .test_en_i(1'b0),
    .ram_cfg_icache_tag_i(12'h0),
    .ram_cfg_icache_data_i(12'h0),
    .hart_id_i(32'h0),
    .boot_addr_i(32'h0000_0000),
    .instr_req_o(instr_req_o),
    .instr_gnt_i(1'b1),
    .instr_rvalid_i(instr_rvalid_i),
    .instr_addr_o(instr_addr_o),
    .instr_rdata_i(32'h0000_0013),
    .instr_rdata_intg_i(7'h0),
    .instr_err_i(1'b0),
    .data_req_o(data_req_o),
    .data_gnt_i(1'b1),
    .data_rvalid_i(data_rvalid_i),
    .data_we_o(data_we_o),
    .data_addr_o(data_addr_o),
    .data_wdata_o(data_wdata_o),
    .data_rdata_i(32'h0),
    .data_rdata_intg_i(7'h0),
    .data_err_i(1'b0),
    .irq_software_i(1'b0),
    .irq_timer_i(1'b0),
    .irq_external_i(1'b0),
    .irq_fast_i(15'h0),
    .irq_nm_i(1'b0),
    .scramble_key_valid_i(1'b0),
    .scramble_key_i(128'h0),
    .scramble_nonce_i(64'h0),
    .debug_req_i(1'b0),
    .fetch_enable_i(4'b0101),
    .scan_rst_ni(1'b1)
  );
endmodule
