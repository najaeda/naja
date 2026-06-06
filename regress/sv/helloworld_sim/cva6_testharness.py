#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Run CVA6's Verilator testharness with a Naja-generated cva6 netlist."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import time


PASS_MARKER = "CVA6_HELLOWORLD_SIM_PASS"
TARGET = "cv64a6_imafdc_sv39"
ISA = "rv64gc_zba_zbb_zbs_zbc_zbkb_zbkx_zkne_zknd_zknh"
MABI = "lp64d"
PROGRAM_SOURCES = {
    "hello_world": ["hello_world/hello_world.c"],
    "corev_dhrystone": ["dhrystone/dhrystone_main.c", "dhrystone/dhrystone.c"],
    "corev_return0": ["return0/return0.c"],
    "corev_custom_template": ["hello_world/custom_test_template.S"],
    "corev_isacov_branch_to_zero": ["isacov/branch_to_zero.S"],
    "corev_isacov_jump": ["isacov/jump_test.S"],
    "corev_isacov_isa": ["isacov/isa_test.S"],
    "corev_isacov_seq_hazard": ["isacov/seq_hazard.S"],
    "corev_pmp_exact_csrr": ["pmp/exact_csrr_test.S"],
    "corev_pmp_granularity": ["pmp/granularity_test.S"],
    "corev_pmp_lsu_tor": ["pmp/lsu_tor_test.S"],
}
PROGRAM_CFLAGS = {
    "corev_dhrystone": [
        "-std=gnu99",
        "-Wno-error=implicit-function-declaration",
        "-Wno-error=implicit-int",
    ],
}
PROGRAMS = tuple(PROGRAM_SOURCES)


def run(args: list[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    print("$ " + (" ".join(args) if cwd is None else f"(cd {cwd} && {' '.join(args)})"), flush=True)
    started = time.monotonic()
    try:
        subprocess.run(args, cwd=str(cwd) if cwd else None, env=env, check=True)
    finally:
        elapsed = time.monotonic() - started
        print(f"[cva6-sim] command elapsed_seconds={elapsed:.3f}", flush=True)


def capture(args: list[str], *, cwd: Path | None = None) -> str:
    print("$ " + (" ".join(args) if cwd is None else f"(cd {cwd} && {' '.join(args)})"), flush=True)
    return subprocess.run(
        args,
        cwd=str(cwd) if cwd else None,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
    ).stdout


def find_riscv_tool_prefix() -> str:
    for prefix in ("riscv64-unknown-elf-", "riscv-none-elf-", "riscv32-unknown-elf-"):
        if shutil.which(prefix + "gcc") and shutil.which(prefix + "objcopy") and shutil.which(prefix + "nm"):
            return prefix
    raise SystemExit(
        "missing RISC-V toolchain: expected riscv64-unknown-elf, riscv-none-elf, "
        "or riscv32-unknown-elf gcc/objcopy/nm in PATH"
    )


def default_jobs() -> str:
    env_value = os.environ.get("NUM_JOBS")
    if env_value:
        return env_value
    return str(min(2, os.cpu_count() or 1))


def normalize_jobs(value: str) -> str:
    try:
        jobs = int(value)
    except ValueError as exc:
        raise SystemExit(f"--jobs must be a positive integer, got {value!r}") from exc
    if jobs < 1:
        raise SystemExit(f"--jobs must be a positive integer, got {value!r}")
    return str(jobs)


def tool_root(tool: str) -> Path | None:
    path = shutil.which(tool)
    if not path:
        return None
    resolved = Path(path).resolve()
    if resolved.parent.name == "bin":
        return resolved.parent.parent
    return None


def find_spike_install_dir() -> Path:
    env_value = os.environ.get("SPIKE_INSTALL_DIR")
    candidates = []
    if env_value:
        candidates.append(Path(env_value))
    spike_root = tool_root("spike")
    if spike_root:
        candidates.append(spike_root)
    candidates.extend([
        Path("/opt/homebrew/opt/riscv-isa-sim"),
        Path("/usr/local/opt/riscv-isa-sim"),
    ])
    for candidate in candidates:
        if (candidate / "include" / "fesvr" / "dtm.h").exists() and (
            candidate / "lib" / "libfesvr.a"
        ).exists():
            return candidate
    raise SystemExit(
        "missing Spike/FESVR install: set SPIKE_INSTALL_DIR to a tree containing "
        "include/fesvr/dtm.h and lib/libfesvr.a"
    )


def find_verilator_install_dir() -> Path:
    env_value = os.environ.get("VERILATOR_INSTALL_DIR")
    candidates = []
    if env_value:
        candidates.append(Path(env_value))
    verilator_root = tool_root("verilator")
    if verilator_root:
        candidates.append(verilator_root)
    candidates.extend([
        Path("/opt/homebrew/opt/verilator"),
        Path("/usr/local/opt/verilator"),
    ])
    for candidate in candidates:
        if (candidate / "share" / "verilator" / "include").exists():
            return candidate
    raise SystemExit(
        "missing Verilator install tree: set VERILATOR_INSTALL_DIR to a tree containing "
        "share/verilator/include"
    )


def compile_program(repo: Path, artifacts: Path, prefix: str, program: str) -> Path:
    elf = artifacts / f"cva6_{program}.o"
    common = repo / "verif" / "tests" / "custom" / "common"
    custom = repo / "verif" / "tests" / "custom"
    env_dir = repo / "verif" / "tests" / "custom" / "env"
    linker = repo / "config" / "gen_from_riscv_config" / "linker" / "link.ld"
    if program not in PROGRAM_SOURCES:
        valid = ", ".join(PROGRAMS)
        raise SystemExit(f"unknown CVA6 program '{program}', valid programs: {valid}")
    alloc_shim = write_baremetal_alloc_shim(artifacts)

    command = [
        prefix + "gcc",
        *(str(custom / source) for source in PROGRAM_SOURCES[program]),
        str(alloc_shim),
        str(common / "syscalls.c"),
        str(common / "crt.S"),
        f"-I{env_dir}",
        f"-I{common}",
        f"-I{custom / 'dhrystone'}",
        *PROGRAM_CFLAGS.get(program, []),
        "-DNOPRINT=1",
        "-static",
        "-mcmodel=medany",
        "-fvisibility=hidden",
        "-nostdlib",
        "-nostartfiles",
        "-g",
        f"-T{linker}",
        f"-march={ISA}",
        f"-mabi={MABI}",
        "-Wl,--no-relax",
        "-o",
        str(elf),
        "-lgcc",
    ]
    print(f"[cva6-sim] compiling program={program} elf={elf}", flush=True)
    run(command, cwd=repo)
    return elf


def write_baremetal_alloc_shim(artifacts: Path) -> Path:
    path = artifacts / "cva6_baremetal_alloc.c"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        r'''#include <stddef.h>
#include <stdint.h>

#define NAJA_CVA6_HEAP_SIZE (64u * 1024u)

static unsigned char naja_cva6_heap[NAJA_CVA6_HEAP_SIZE] __attribute__((aligned(16)));
static uintptr_t naja_cva6_heap_offset;

void *malloc(size_t size)
{
  if (size == 0)
    size = 1;

  size = (size + 15u) & ~(size_t)15u;
  if (naja_cva6_heap_offset > NAJA_CVA6_HEAP_SIZE ||
      size > NAJA_CVA6_HEAP_SIZE - naja_cva6_heap_offset)
    return 0;

  void *ptr = &naja_cva6_heap[naja_cva6_heap_offset];
  naja_cva6_heap_offset += size;
  return ptr;
}

void free(void *ptr)
{
  (void)ptr;
}
''',
        encoding="utf-8",
    )
    return path


def tohost_address(elf: Path, prefix: str) -> str:
    output = capture([prefix + "nm", "-B", str(elf)])
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and fields[-1] == "tohost":
            return fields[0]
    raise SystemExit(f"could not find tohost symbol in {elf}")


def write_ariane_wrapper(path: Path) -> Path:
    path.write_text(
        r'''`include "cvxif_types.svh"

module ariane import ariane_pkg::*; #(
  parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty,
  parameter type rvfi_probes_instr_t = logic,
  parameter type rvfi_probes_csr_t = logic,
  parameter type rvfi_probes_t = struct packed {
    logic csr;
    logic instr;
  },
  localparam type readregflags_t      = `READREGFLAGS_T(CVA6Cfg),
  localparam type writeregflags_t     = `WRITEREGFLAGS_T(CVA6Cfg),
  localparam type id_t                = `ID_T(CVA6Cfg),
  localparam type hartid_t            = `HARTID_T(CVA6Cfg),
  localparam type x_compressed_req_t  = `X_COMPRESSED_REQ_T(CVA6Cfg, hartid_t),
  localparam type x_compressed_resp_t = `X_COMPRESSED_RESP_T(CVA6Cfg),
  localparam type x_issue_req_t       = `X_ISSUE_REQ_T(CVA6Cfg, hartid_t, id_t),
  localparam type x_issue_resp_t      = `X_ISSUE_RESP_T(CVA6Cfg, writeregflags_t, readregflags_t),
  localparam type x_register_t        = `X_REGISTER_T(CVA6Cfg, hartid_t, id_t, readregflags_t),
  localparam type x_commit_t          = `X_COMMIT_T(CVA6Cfg, hartid_t, id_t),
  localparam type x_result_t          = `X_RESULT_T(CVA6Cfg, hartid_t, id_t, writeregflags_t),
  localparam type cvxif_req_t         = `CVXIF_REQ_T(CVA6Cfg, x_compressed_req_t, x_issue_req_t, x_register_t, x_commit_t),
  localparam type cvxif_resp_t        = `CVXIF_RESP_T(CVA6Cfg, x_compressed_resp_t, x_issue_resp_t, x_result_t),
  parameter int unsigned AxiAddrWidth = ariane_axi::AddrWidth,
  parameter int unsigned AxiDataWidth = ariane_axi::DataWidth,
  parameter int unsigned AxiIdWidth   = ariane_axi::IdWidth,
  parameter type axi_ar_chan_t = ariane_axi::ar_chan_t,
  parameter type axi_aw_chan_t = ariane_axi::aw_chan_t,
  parameter type axi_w_chan_t  = ariane_axi::w_chan_t,
  parameter type noc_req_t = ariane_axi::req_t,
  parameter type noc_resp_t = ariane_axi::resp_t
) (
  input  logic                         clk_i,
  input  logic                         rst_ni,
  input  logic [CVA6Cfg.VLEN-1:0]       boot_addr_i,
  input  logic [CVA6Cfg.XLEN-1:0]       hart_id_i,
  input  logic [1:0]                   irq_i,
  input  logic                         ipi_i,
  input  logic                         time_irq_i,
  input  logic                         debug_req_i,
  output rvfi_probes_t                 rvfi_probes_o,
  output noc_req_t                     noc_req_o,
  input  noc_resp_t                    noc_resp_i
);

  logic [6973:0] rvfi_probes_flat;
  logic [448:0]  cvxif_req_unused;
  logic [177:0]  cvxif_resp_flat;
  logic [469:0]  noc_req_flat;
  logic [209:0]  noc_resp_flat;
  noc_req_t      naja_noc_req;
  noc_resp_t     naja_noc_resp;

  assign rvfi_probes_o  = rvfi_probes_t'(rvfi_probes_flat);
  assign naja_noc_req   = noc_req_t'(noc_req_flat);
  assign naja_noc_resp  = noc_resp_i;
  assign noc_req_o      = naja_noc_req;
  assign noc_resp_flat  = naja_noc_resp;
  assign cvxif_resp_flat = {
    1'b1,       // compressed_ready
    33'b0,
    1'b1,       // issue_ready
    4'b0,
    1'b1,       // register_ready
    138'b0
  };

  logic [31:0] naja_raw_aw_valid_count;
  logic [31:0] naja_raw_w_valid_count;
  logic [31:0] naja_raw_ar_valid_count;
  logic [31:0] naja_raw_nonzero_count;
  logic [31:0] naja_alu_debug_count;
  logic [31:0] naja_issue_debug_count;
  logic [31:0] naja_commit_debug_count;
  logic [31:0] naja_storebuf_debug_count;
  logic [31:0] naja_ex_debug_count;
  logic [31:0] naja_decode_debug_count;
  logic [31:0] naja_frontend_debug_count;

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_raw_noc_counts
    if (!rst_ni) begin
      naja_raw_aw_valid_count <= '0;
      naja_raw_w_valid_count <= '0;
      naja_raw_ar_valid_count <= '0;
      naja_raw_nonzero_count <= '0;
    end else begin
      if (|noc_req_flat) naja_raw_nonzero_count <= naja_raw_nonzero_count + 1;
      if (noc_req_flat[302]) naja_raw_aw_valid_count <= naja_raw_aw_valid_count + 1;
      if (noc_req_flat[164]) naja_raw_w_valid_count <= naja_raw_w_valid_count + 1;
      if (noc_req_flat[1]) naja_raw_ar_valid_count <= naja_raw_ar_valid_count + 1;
      if ($test$plusargs("naja_axi_debug") && naja_noc_req.ar_valid) begin
        $display("*** [naja_wrapper_ar] t=%0t ready=%0b addr=0x%016h id=%0d len=%0d size=%0d burst=%0d r_ready=%0b r_valid=%0b r_data=0x%016h",
                 $time, naja_noc_resp.ar_ready, naja_noc_req.ar.addr, naja_noc_req.ar.id,
                 naja_noc_req.ar.len, naja_noc_req.ar.size, naja_noc_req.ar.burst,
                 naja_noc_req.r_ready, naja_noc_resp.r_valid, naja_noc_resp.r.data);
      end
    end
  end

  final begin
    $display("*** [naja_wrapper] raw_noc nonzero=%0d aw_valid=%0d w_valid=%0d ar_valid=%0d",
             naja_raw_nonzero_count,
             naja_raw_aw_valid_count, naja_raw_w_valid_count, naja_raw_ar_valid_count);
  end

  cva6 i_cva6 (
    .clk_i         ( clk_i             ),
    .rst_ni        ( rst_ni            ),
    .boot_addr_i   ( boot_addr_i[63:0] ),
    .hart_id_i     ( hart_id_i[63:0]   ),
    .irq_i         ( irq_i             ),
    .ipi_i         ( ipi_i             ),
    .time_irq_i    ( time_irq_i        ),
    .debug_req_i   ( debug_req_i       ),
    .rvfi_probes_o ( rvfi_probes_flat  ),
    .cvxif_req_o   ( cvxif_req_unused  ),
    .cvxif_resp_i  ( cvxif_resp_flat   ),
    .noc_req_o     ( noc_req_flat      ),
    .noc_resp_i    ( noc_resp_flat     )
  );

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_frontend_debug
    if (!rst_ni) begin
      naja_frontend_debug_count <= '0;
    end else if ($test$plusargs("naja_frontend_debug") && naja_frontend_debug_count < 256 &&
                 (naja_frontend_debug_count < 32 ||
                  i_cva6.icache_dreq_cache_if[364] ||
                  i_cva6.icache_dreq_cache_if[363] ||
                  i_cva6.i_frontend.icache_valid_q ||
                  |i_cva6.i_frontend.instruction_valid ||
                  i_cva6.i_frontend.fetch_entry_valid_o ||
                  i_cva6.i_frontend.if_ready)) begin
      naja_frontend_debug_count <= naja_frontend_debug_count + 1;
      $display("*** [naja_frontend] t=%0t count=%0d rst=%0b halt_frontend=%0b flush=%0b fetch_ready=0x%0h iq_ready=%0b if_ready=%0b req=%0b kill1=%0b kill2=%0b spec=%0b dreq_vaddr=0x%016h drsp_ready=%0b drsp_valid=%0b drsp_data=0x%08h npc_rst=%0b npc_q=0x%016h npc_d=0x%016h ic_valid=%0b ic_data=0x%08h ic_data_q=0x%08h ic_vaddr=0x%016h instr=0x%016h instr_valid=0x%0h instr_addr=0x%032h fetch_instr=0x%08h replay=%0b bp_valid=%0b",
               $time,
               naja_frontend_debug_count,
               rst_ni,
               i_cva6.halt_frontend,
               i_cva6.flush_ctrl_if,
               i_cva6.fetch_ready_id_if,
               i_cva6.i_frontend.instr_queue_ready,
               i_cva6.i_frontend.if_ready,
               i_cva6.icache_dreq_if_cache[67],
               i_cva6.icache_dreq_if_cache[66],
               i_cva6.icache_dreq_if_cache[65],
               i_cva6.icache_dreq_if_cache[64],
               i_cva6.icache_dreq_if_cache[63:0],
               i_cva6.icache_dreq_cache_if[364],
               i_cva6.icache_dreq_cache_if[363],
               i_cva6.icache_dreq_cache_if[362:331],
               i_cva6.i_frontend.npc_rst_load_q,
               i_cva6.i_frontend.npc_q,
               i_cva6.i_frontend.npc_d,
               i_cva6.i_frontend.icache_valid_q,
               i_cva6.i_frontend.icache_data,
               i_cva6.i_frontend.icache_data_q,
               i_cva6.i_frontend.icache_vaddr_q,
               i_cva6.i_frontend.instr,
               i_cva6.i_frontend.instruction_valid,
               i_cva6.i_frontend.addr,
               i_cva6.i_frontend.fetch_entry_o[301:270],
               i_cva6.i_frontend.replay,
               i_cva6.i_frontend.bp_valid);
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_alu_debug
    if (!rst_ni) begin
      naja_alu_debug_count <= '0;
    end else if ($test$plusargs("naja_alu_debug") && naja_alu_debug_count < 128 &&
                 i_cva6.ex_stage_i.alu_valid_i) begin
      naja_alu_debug_count <= naja_alu_debug_count + 1;
      $display("*** [naja_alu] t=%0t count=%0d op=%0d a=0x%016h b=0x%016h shift_left=%0b shift_arith=%0b shift_amt=0x%016h shift_op_a=0x%016h shift_right=0x%017h shift_left_result=0x%016h shift_result=0x%016h result=0x%016h",
               $time,
               naja_alu_debug_count,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.fu_data_i[202:195],
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.operand_a,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.operand_b,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_left,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_arithmetic,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_amt,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_op_a,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_right_result,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_left_result,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.shift_result,
               i_cva6.ex_stage_i.alu_wrapper_i.alu_i.result_o);
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_decode_debug
    if (!rst_ni) begin
      naja_decode_debug_count <= '0;
    end else if ($test$plusargs("naja_decode_debug") && naja_decode_debug_count < 160 &&
                 i_cva6.id_stage_i.fetch_entry_valid_i &&
                 (i_cva6.id_stage_i.fetch_entry_i[365:302] >= 64'h0000000080000000 ||
                  i_cva6.id_stage_i.is_illegal_rvc ||
                  i_cva6.id_stage_i.is_illegal_deco ||
                  i_cva6.id_stage_i.decoded_instruction[370:367] == 4'd9)) begin
      naja_decode_debug_count <= naja_decode_debug_count + 1;
      $display("*** [naja_decode] t=%0t count=%0d fetch_valid=%0b fetch_ready=%0b pc=0x%016h fetch_instr=0x%08h rvc_instr=0x%08h rvc_illegal=%0b rvc_cmp=%0b macro=%0b zcmt=%0b cvxif_valid=%0b cvxif_ready=%0b cvxif_accept=%0b cvxif_i_illegal=%0b cvxif_o_illegal=%0b cvxif_o_instr=0x%08h deco_illegal=%0b deco_cmp=%0b deco_instr=0x%08h decoded_valid=%0b decoded_sbe=%0b decoded_ex=%0b decoded_pc=0x%016h decoded_fu=%0d decoded_op=%0d decoded_trans=%0d",
               $time,
               naja_decode_debug_count,
               i_cva6.id_stage_i.fetch_entry_valid_i,
               i_cva6.id_stage_i.fetch_entry_ready_o,
               i_cva6.id_stage_i.fetch_entry_i[365:302],
               i_cva6.id_stage_i.fetch_entry_i[301:270],
               i_cva6.id_stage_i.instruction_rvc,
               i_cva6.id_stage_i.is_illegal_rvc,
               i_cva6.id_stage_i.is_compressed_rvc,
               i_cva6.id_stage_i.is_macro_instr,
               i_cva6.id_stage_i.is_zcmt_instr,
               i_cva6.id_stage_i.compressed_valid_o,
               i_cva6.id_stage_i.compressed_ready_i,
               i_cva6.id_stage_i.compressed_resp_i[0],
               i_cva6.id_stage_i.is_illegal_cvxif_i,
               i_cva6.id_stage_i.is_illegal_cvxif_o,
               i_cva6.id_stage_i.instruction_cvxif_o,
               i_cva6.id_stage_i.is_illegal_deco,
               i_cva6.id_stage_i.is_compressed_deco,
               i_cva6.id_stage_i.instruction_deco,
               i_cva6.id_stage_i.decoded_instruction_valid,
               i_cva6.id_stage_i.decoded_instruction[279],
               i_cva6.id_stage_i.decoded_instruction[73],
               i_cva6.id_stage_i.decoded_instruction[437:374],
               i_cva6.id_stage_i.decoded_instruction[370:367],
               i_cva6.id_stage_i.decoded_instruction[366:359],
               i_cva6.id_stage_i.decoded_instruction[373:371]);
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_issue_debug
    if (!rst_ni) begin
      naja_issue_debug_count <= '0;
    end else if ($test$plusargs("naja_issue_debug") && naja_issue_debug_count < 640 &&
                 (i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_valid_i ||
                  !i_cva6.id_stage_i.fetch_entry_ready_o ||
                  i_cva6.ex_stage_i.alu_valid_i || |i_cva6.wt_valid_ex_id)) begin
      naja_issue_debug_count <= naja_issue_debug_count + 1;
      $display("*** [naja_issue] t=%0t count=%0d fetch_ready=%0b valid=%0b ack_o=%0b issue_ack=%0b fu_busy=%0b fus_busy=0b%013b flu_ready=%0b lsu_ready=%0b fpu_ready=%0b mult_q=%0b csr_q=%0b branch_q=%0b stall_raw=%0b stall_i=%0b stall_rs1=%0b stall_rs2=%0b stall_rs3=%0b ex_valid=%0b sbe_valid=%0b pc=0x%016h rs1=%0d rs2=%0d rd=%0d op=%0d fu=%0d trans=%0d idx_rs1=%0d idx_rs2=%0d rs1_raw=%0b rs2_raw=%0b rs1_valid=%0b rs2_valid=%0b fwd_rs1=%0b fwd_rs2=%0b fwd_valid=0x%02h rd_list=0x%010h reg_a=0x%016h rs1_res=0x%016h rs2_res=0x%016h fu_n_a=0x%016h fu_q_a=0x%016h alu_valid=%0b wt_valid=0x%02h wbdata0=0x%016h trans_id0=%0d",
               $time,
               naja_issue_debug_count,
               i_cva6.id_stage_i.fetch_entry_ready_o,
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_valid_i,
               i_cva6.issue_stage_i.i_issue_read_operands.issue_ack_o,
               i_cva6.issue_stage_i.i_issue_read_operands.issue_ack,
               i_cva6.issue_stage_i.i_issue_read_operands.fu_busy,
               i_cva6.issue_stage_i.i_issue_read_operands.fus_busy,
               i_cva6.issue_stage_i.i_issue_read_operands.flu_ready_i,
               i_cva6.issue_stage_i.i_issue_read_operands.lsu_ready_i,
               i_cva6.issue_stage_i.i_issue_read_operands.fpu_ready_i,
               i_cva6.issue_stage_i.i_issue_read_operands.mult_valid_q,
               i_cva6.issue_stage_i.i_issue_read_operands.csr_valid_q,
               i_cva6.issue_stage_i.i_issue_read_operands.branch_valid_q,
               i_cva6.issue_stage_i.i_issue_read_operands.stall_raw,
               i_cva6.issue_stage_i.i_issue_read_operands.stall_i,
               i_cva6.issue_stage_i.i_issue_read_operands.stall_rs1,
               i_cva6.issue_stage_i.i_issue_read_operands.stall_rs2,
               i_cva6.issue_stage_i.i_issue_read_operands.stall_rs3,
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[73],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[279],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[437:374],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[358:354],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[353:349],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[348:344],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[366:359],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[370:367],
               i_cva6.issue_stage_i.i_issue_read_operands.issue_instr_i[373:371],
               i_cva6.issue_stage_i.i_issue_read_operands.idx_hzd_rs1,
               i_cva6.issue_stage_i.i_issue_read_operands.idx_hzd_rs2,
               i_cva6.issue_stage_i.i_issue_read_operands.rs1_has_raw,
               i_cva6.issue_stage_i.i_issue_read_operands.rs2_has_raw,
               i_cva6.issue_stage_i.i_issue_read_operands.rs1_valid,
               i_cva6.issue_stage_i.i_issue_read_operands.rs2_valid,
               i_cva6.issue_stage_i.i_issue_read_operands.forward_rs1,
               i_cva6.issue_stage_i.i_issue_read_operands.forward_rs2,
               i_cva6.issue_stage_i.i_issue_read_operands.fwd_res_valid,
               i_cva6.issue_stage_i.i_issue_read_operands.rd_list,
               i_cva6.issue_stage_i.i_issue_read_operands.operand_a_regfile,
               i_cva6.issue_stage_i.i_issue_read_operands.rs1_res,
               i_cva6.issue_stage_i.i_issue_read_operands.rs2_res,
               i_cva6.issue_stage_i.i_issue_read_operands.fu_data_n[194:131],
               i_cva6.issue_stage_i.i_issue_read_operands.fu_data_q[194:131],
               i_cva6.ex_stage_i.alu_valid_i,
               i_cva6.wt_valid_ex_id,
               i_cva6.wbdata_ex_id[63:0],
               i_cva6.trans_id_ex_id[2:0]);
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_commit_debug
    if (!rst_ni) begin
      naja_commit_debug_count <= '0;
    end else if ($test$plusargs("naja_commit_debug") && naja_commit_debug_count < 512 &&
                 (i_cva6.commit_stage_i.commit_instr_i[279] ||
                  i_cva6.lsu_commit_commit_ex ||
                  i_cva6.store_valid_ex_id ||
                  i_cva6.load_valid_ex_id ||
                  |i_cva6.wt_valid_ex_id ||
                  !i_cva6.lsu_commit_ready_ex_commit)) begin
      naja_commit_debug_count <= naja_commit_debug_count + 1;
      $display("*** [naja_commit] t=%0t count=%0d commit_valid=%0b commit_ack_o=%0b macro_ack=%0b final_ack=%0b halt=%0b drop=%0b pc=0x%016h fu=%0d op=%0d trans=%0d ex=%0b result=0x%016h lsu_ready=%0b lsu_valid=%0b lsu_commit_ready=%0b lsu_commit=%0b lsu_trans=%0d no_st_pending=%0b stall_st_pending=%0b store_valid=%0b store_trans=%0d load_valid=%0b load_trans=%0d wt_valid=0x%02h trans_ids=0x%08h wbdata0=0x%016h wbdata1=0x%016h",
               $time,
               naja_commit_debug_count,
               i_cva6.commit_stage_i.commit_instr_i[279],
               i_cva6.commit_stage_i.commit_ack_o[0],
               i_cva6.commit_macro_ack[0],
               i_cva6.commit_ack[0],
               i_cva6.commit_stage_i.halt_i,
               i_cva6.commit_stage_i.commit_drop_i[0],
               i_cva6.commit_stage_i.commit_instr_i[437:374],
               i_cva6.commit_stage_i.commit_instr_i[370:367],
               i_cva6.commit_stage_i.commit_instr_i[366:359],
               i_cva6.commit_stage_i.commit_instr_i[373:371],
               i_cva6.commit_stage_i.commit_instr_i[73],
               i_cva6.commit_stage_i.commit_instr_i[343:280],
               i_cva6.lsu_ready_ex_id,
               i_cva6.lsu_valid_id_ex,
               i_cva6.lsu_commit_ready_ex_commit,
               i_cva6.lsu_commit_commit_ex,
               i_cva6.lsu_commit_trans_id,
               i_cva6.no_st_pending_commit,
               i_cva6.stall_st_pending_ex,
               i_cva6.store_valid_ex_id,
               i_cva6.store_trans_id_ex_id,
               i_cva6.load_valid_ex_id,
               i_cva6.load_trans_id_ex_id,
               i_cva6.wt_valid_ex_id,
               i_cva6.trans_id_ex_id,
               i_cva6.wbdata_ex_id[63:0],
               i_cva6.wbdata_ex_id[127:64]);
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_storebuf_debug
    if (!rst_ni) begin
      naja_storebuf_debug_count <= '0;
    end else if ($test$plusargs("naja_storebuf_debug") && naja_storebuf_debug_count < 512 &&
                 (i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_status_cnt_q != 0 ||
                  i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.speculative_status_cnt_q != 0 ||
                  i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_i ||
                  i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.valid_i ||
                  i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.req_port_o[22] ||
                  i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.req_port_i[130] ||
                  i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.req_port_i[22] ||
                  i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.req_port_o[130] ||
                  i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.miss_req_o ||
                  i_cva6.cva6_gen_cache_wt_i_cache_subsystem.dcache_adapter_data_req)) begin
      naja_storebuf_debug_count <= naja_storebuf_debug_count + 1;
      $display("*** [naja_storebuf] t=%0t count=%0d sb_spec_cnt=%0d sb_commit_cnt=%0d sb_rp=%0d sb_wp=%0d sb_valid_i=%0b sb_commit_i=%0b sb_ready=%0b sb_commit_ready=%0b sb_no_pending=%0b sb_req=%0b sb_gnt=%0b sb_rvalid=%0b sb_entry_valid=%0b sb_entry_wait=%0b sb_entry_addr=0x%014h sb_entry_data=0x%016h sb_entry_be=0x%02h sb_entry_size=%0d sb_entry_cbo=0x%02h top_ex_req=%0b top_to_req=%0b top_from_gnt=%0b wb_req=%0b wb_gnt=%0b wb_rdy=%0b wb_full=%0b wb_dirty=0x%02h wb_valid=0x%02h wb_bdirty=0x%016h wb_tocheck=0x%02h wb_miss_req=%0b wb_miss_ack=%0b wb_free_tx=%0b wb_tx_vld=0x%0h cache_req=%0b cache_ack=%0b cache_rtrn=%0b noc_aw=%0b noc_w=%0b",
               $time,
               naja_storebuf_debug_count,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.speculative_status_cnt_q,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_status_cnt_q,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_read_pointer_q,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_write_pointer_q,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.valid_i,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_i,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.ready_o,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_ready_o,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.no_st_pending_o,
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.req_port_o[22],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.req_port_i[130],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.req_port_i[129],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[1],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[0],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[139:84],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[83:20],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[19:12],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[11:10],
               i_cva6.ex_stage_i.lsu_i.i_store_unit.store_buffer_i.commit_queue_q_mem_rdata_0[9:2],
               i_cva6.dcache_req_ports_ex_cache[436],
               i_cva6.dcache_req_to_cache[643],
               i_cva6.dcache_req_from_cache[523],
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.req_port_i[22],
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.req_port_o[130],
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.rdy,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.full,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.dirty,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.valid,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.bdirty,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.tocheck,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.miss_req_o,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.miss_ack_i,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.free_tx_slots,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.i_wt_dcache.i_wt_dcache_wbuffer.tx_vld_o,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.dcache_adapter_data_req,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.adapter_dcache_data_ack,
               i_cva6.cva6_gen_cache_wt_i_cache_subsystem.adapter_dcache_rtrn_vld,
               i_cva6.noc_req_o[302],
               i_cva6.noc_req_o[164]);
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_ex_debug
    if (!rst_ni) begin
      naja_ex_debug_count <= '0;
    end else if ($test$plusargs("naja_ex_debug") && naja_ex_debug_count < 192 &&
                 (!i_cva6.ex_stage_i.flu_ready_o ||
                  i_cva6.ex_stage_i.csr_valid_i ||
                  i_cva6.ex_stage_i.csr_commit_i ||
                  i_cva6.commit_stage_i.commit_ack_o[0] ||
                  i_cva6.commit_stage_i.commit_csr_o ||
                  i_cva6.ex_stage_i.csr_buffer_i.csr_reg_q[0])) begin
      naja_ex_debug_count <= naja_ex_debug_count + 1;
      $display("*** [naja_ex] t=%0t count=%0d flu_ready=%0b csr_ready=%0b mult_ready=%0b csr_valid=%0b csr_commit_i=%0b csr_q_valid=%0b csr_n_valid=%0b csr_q_addr=0x%03h csr_addr_o=0x%03h flush=%0b commit_valid=%0b commit_ack=%0b commit_csr=%0b halt=%0b drop=%0b csr_ex=%0b commit_pc=0x%016h commit_fu=%0d commit_op=%0d commit_trans=%0d commit_ex=%0b commit_result=0x%016h csr_op_o=%0d",
               $time,
               naja_ex_debug_count,
               i_cva6.ex_stage_i.flu_ready_o,
               i_cva6.ex_stage_i.csr_ready,
               i_cva6.ex_stage_i.mult_ready,
               i_cva6.ex_stage_i.csr_valid_i,
               i_cva6.ex_stage_i.csr_commit_i,
               i_cva6.ex_stage_i.csr_buffer_i.csr_reg_q[0],
               i_cva6.ex_stage_i.csr_buffer_i.csr_reg_n[0],
               i_cva6.ex_stage_i.csr_buffer_i.csr_reg_q[12:1],
               i_cva6.ex_stage_i.csr_addr_o,
               i_cva6.ex_stage_i.flush_i,
               i_cva6.commit_stage_i.commit_instr_i[279],
               i_cva6.commit_stage_i.commit_ack_o[0],
               i_cva6.commit_stage_i.commit_csr_o,
               i_cva6.commit_stage_i.halt_i,
               i_cva6.commit_stage_i.commit_drop_i[0],
               i_cva6.commit_stage_i.csr_exception_i[0],
               i_cva6.commit_stage_i.commit_instr_i[437:374],
               i_cva6.commit_stage_i.commit_instr_i[370:367],
               i_cva6.commit_stage_i.commit_instr_i[366:359],
               i_cva6.commit_stage_i.commit_instr_i[373:371],
               i_cva6.commit_stage_i.commit_instr_i[73],
               i_cva6.commit_stage_i.commit_instr_i[343:280],
               i_cva6.commit_stage_i.csr_op_o);
    end
  end

endmodule
''',
        encoding="utf-8",
    )
    return path


def write_patched_testharness(repo: Path, path: Path) -> Path:
    text = (repo / "corev_apu" / "tb" / "ariane_testharness.sv").read_text(encoding="utf-8")
    sram_instance = "  sram #("
    if sram_instance not in text:
        raise SystemExit("could not patch ariane_testharness.sv SRAM instance")

    exit_assign = """  if (ariane_pkg::RVFI) begin
    assign exit_o              = (jtag_enable[0]) ? jtag_exit          : rvfi_exit;
  end else begin
    assign exit_o              = (jtag_enable[0]) ? jtag_exit          : dmi_exit;
  end
"""
    patched_exit_assign = """  logic [31:0] harness_exit;
  logic [31:0] naja_tohost_exit;

  if (ariane_pkg::RVFI) begin
    assign harness_exit        = (jtag_enable[0]) ? jtag_exit          : rvfi_exit;
  end else begin
    assign harness_exit        = (jtag_enable[0]) ? jtag_exit          : dmi_exit;
  end
  assign exit_o                = (naja_tohost_exit[0]) ? naja_tohost_exit : harness_exit;
"""
    if exit_assign not in text:
        raise SystemExit("could not patch ariane_testharness.sv exit assignment")

    axi2mem_block = """  axi2mem #(
    .AXI_ID_WIDTH   ( ariane_axi_soc::IdWidthSlave ),
    .AXI_ADDR_WIDTH ( AXI_ADDRESS_WIDTH            ),
    .AXI_DATA_WIDTH ( AXI_DATA_WIDTH               ),
    .AXI_USER_WIDTH ( AXI_USER_WIDTH               )
  ) i_axi2mem (
    .clk_i  ( clk_i        ),
    .rst_ni ( ndmreset_n   ),
    .slave  ( dram_delayed ),
    .req_o  ( req          ),
    .we_o   ( we           ),
    .addr_o ( addr         ),
    .be_o   ( be           ),
    .user_o ( wuser        ),
    .data_o ( wdata        ),
    .user_i ( ruser        ),
    .data_i ( rdata        )
  );
"""
    tohost_monitor = axi2mem_block + """
  longint unsigned naja_tohost_addr;
  logic [31:0] naja_tohost_wdata;
  logic [31:0] naja_dram_req_count;
  logic [31:0] naja_dram_write_count;
  logic [31:0] naja_rom_req_count;
  logic [31:0] naja_dram_xbar_ar_count;
  logic [31:0] naja_dram_atomic_ar_count;
  logic [31:0] naja_dram_delayed_ar_count;

  initial begin
    if (!$value$plusargs("tohost_addr=%h", naja_tohost_addr)) naja_tohost_addr = '0;
  end

  assign naja_tohost_wdata = addr[2] ? wdata[63:32] : wdata[31:0];

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_tohost_exit
    if (!rst_ni) begin
      naja_tohost_exit <= '0;
      naja_dram_req_count <= '0;
      naja_dram_write_count <= '0;
      naja_rom_req_count <= '0;
      naja_dram_xbar_ar_count <= '0;
      naja_dram_atomic_ar_count <= '0;
      naja_dram_delayed_ar_count <= '0;
    end else begin
      if (rom_req) naja_rom_req_count <= naja_rom_req_count + 1;
      if (master[ariane_soc::DRAM].ar_valid) naja_dram_xbar_ar_count <= naja_dram_xbar_ar_count + 1;
      if (dram.ar_valid) naja_dram_atomic_ar_count <= naja_dram_atomic_ar_count + 1;
      if (dram_delayed.ar_valid) naja_dram_delayed_ar_count <= naja_dram_delayed_ar_count + 1;
      if (req) begin
        naja_dram_req_count <= naja_dram_req_count + 1;
        if (we) naja_dram_write_count <= naja_dram_write_count + 1;
        if ($test$plusargs("naja_axi_debug")) begin
          $display("*** [naja_axi_mem] t=%0t req=%0b we=%0b addr=0x%016h be=0x%02h wdata=0x%016h rdata=0x%016h dram_r_valid=%0b dram_r_ready=%0b dram_r_data=0x%016h",
                   $time, req, we, addr, be, wdata, rdata,
                   dram_delayed.r_valid, dram_delayed.r_ready, dram_delayed.r_data);
        end
      end
      if (req && we && naja_tohost_addr != '0 &&
          {addr[AXI_ADDRESS_WIDTH-1:3], 3'b000} == naja_tohost_addr[AXI_ADDRESS_WIDTH-1:0]) begin
        if (naja_tohost_wdata[0]) begin
          naja_tohost_exit <= naja_tohost_wdata;
          $display("*** [naja_axi_tohost] INFO: tohost write 0x%08h at 0x%h", naja_tohost_wdata, addr);
        end
      end
      if ($test$plusargs("naja_axi_debug") && master[ariane_soc::ROM].ar_valid) begin
        $display("*** [naja_axi_rom_ar] t=%0t ready=%0b addr=0x%016h id=%0d len=%0d size=%0d r_valid=%0b r_data=0x%016h",
                 $time, master[ariane_soc::ROM].ar_ready, master[ariane_soc::ROM].ar_addr,
                 master[ariane_soc::ROM].ar_id, master[ariane_soc::ROM].ar_len,
                 master[ariane_soc::ROM].ar_size, master[ariane_soc::ROM].r_valid,
                 master[ariane_soc::ROM].r_data);
      end
      if ($test$plusargs("naja_axi_debug") && master[ariane_soc::DRAM].ar_valid) begin
        $display("*** [naja_axi_dram_ar] t=%0t ready=%0b addr=0x%016h id=%0d len=%0d size=%0d atomic_ready=%0b delayed_valid=%0b mem_req=%0b mem_rdata=0x%016h",
                 $time, master[ariane_soc::DRAM].ar_ready, master[ariane_soc::DRAM].ar_addr,
                 master[ariane_soc::DRAM].ar_id, master[ariane_soc::DRAM].ar_len,
                 master[ariane_soc::DRAM].ar_size, dram.ar_ready, dram_delayed.ar_valid, req, rdata);
      end
      if ($test$plusargs("naja_axi_debug") && dram_delayed.r_valid) begin
        $display("*** [naja_axi_dram_r] t=%0t ready=%0b id=%0d last=%0b data=0x%016h mem_rdata=0x%016h",
                 $time, dram_delayed.r_ready, dram_delayed.r_id, dram_delayed.r_last,
                 dram_delayed.r_data, rdata);
      end
    end
  end

  final begin
    $display("*** [naja_axi_dram] req=%0d write=%0d rom_req=%0d dram_xbar_ar=%0d dram_atomic_ar=%0d dram_delayed_ar=%0d",
             naja_dram_req_count, naja_dram_write_count, naja_rom_req_count,
             naja_dram_xbar_ar_count, naja_dram_atomic_ar_count, naja_dram_delayed_ar_count);
  end
"""
    if axi2mem_block not in text:
        raise SystemExit("could not patch ariane_testharness.sv AXI-to-memory bridge")

    core_axi_assign = """  `AXI_ASSIGN_FROM_REQ(slave[0], axi_ariane_req)
  `AXI_ASSIGN_TO_RESP(axi_ariane_resp, slave[0])
"""
    core_axi_monitor = core_axi_assign + """
  logic [31:0] naja_core_aw_valid_count;
  logic [31:0] naja_core_w_valid_count;
  logic [31:0] naja_core_ar_valid_count;
  logic [31:0] naja_core_reset_high_count;
  logic [31:0] naja_core_commit_count;

  always_ff @(posedge clk_i or negedge rst_ni) begin : p_naja_core_axi_counts
    if (!rst_ni) begin
      naja_core_aw_valid_count <= '0;
      naja_core_w_valid_count <= '0;
      naja_core_ar_valid_count <= '0;
      naja_core_reset_high_count <= '0;
      naja_core_commit_count <= '0;
    end else begin
      if (ndmreset_n) naja_core_reset_high_count <= naja_core_reset_high_count + 1;
      if (axi_ariane_req.aw_valid) naja_core_aw_valid_count <= naja_core_aw_valid_count + 1;
      if (axi_ariane_req.w_valid) naja_core_w_valid_count <= naja_core_w_valid_count + 1;
      if (axi_ariane_req.ar_valid) naja_core_ar_valid_count <= naja_core_ar_valid_count + 1;
      if (rvfi_instr[0].valid) naja_core_commit_count <= naja_core_commit_count + 1;
      if ($test$plusargs("naja_axi_debug") && axi_ariane_req.ar_valid) begin
        $display("*** [naja_axi_core_ar] cyc=%0d ready=%0b addr=0x%016h id=%0d len=%0d size=%0d burst=%0d r_ready=%0b r_valid=%0b r_data=0x%016h",
                 naja_core_reset_high_count, axi_ariane_resp.ar_ready, axi_ariane_req.ar.addr,
                 axi_ariane_req.ar.id, axi_ariane_req.ar.len, axi_ariane_req.ar.size,
                 axi_ariane_req.ar.burst, axi_ariane_req.r_ready, axi_ariane_resp.r_valid,
                 axi_ariane_resp.r.data);
      end
      if ($test$plusargs("naja_axi_debug") && rvfi_instr[0].valid && naja_core_commit_count < 64) begin
        $display("*** [naja_rvfi_commit] cyc=%0d count=%0d pc=0x%016h insn=0x%08h rd=%0d wdata=0x%016h trap=%0b",
                 naja_core_reset_high_count, naja_core_commit_count, rvfi_instr[0].pc_rdata,
                 rvfi_instr[0].insn, rvfi_instr[0].rd_addr, rvfi_instr[0].rd_wdata,
                 rvfi_instr[0].trap);
      end
    end
  end

  final begin
    $display("*** [naja_axi_core] ndmreset_high=%0d aw_valid=%0d w_valid=%0d ar_valid=%0d commit=%0d",
             naja_core_reset_high_count,
             naja_core_aw_valid_count, naja_core_w_valid_count, naja_core_ar_valid_count,
             naja_core_commit_count);
  end
"""
    if core_axi_assign not in text:
        raise SystemExit("could not patch ariane_testharness.sv core AXI monitor")

    text = text.replace(sram_instance, "  tb_sram #(", 1)
    text = text.replace(exit_assign, patched_exit_assign, 1)
    text = text.replace(axi2mem_block, tohost_monitor, 1)
    text = text.replace(core_axi_assign, core_axi_monitor, 1)
    path.write_text(text, encoding="utf-8")
    return path


def write_patched_ariane_tb(repo: Path, stage_dir: Path) -> Path:
    text = (repo / "corev_apu" / "tb" / "ariane_tb.cpp").read_text(encoding="utf-8")
    result_block = """  if (dtm->exit_code()) {
    fprintf(stderr, "%s *** FAILED *** (tohost = %d) after %ld cycles\\n", htif_argv[1], dtm->exit_code(), main_time);
    ret = dtm->exit_code();
  } else if (jtag->exit_code()) {
    fprintf(stderr, "%s *** FAILED *** (tohost = %d, seed %d) after %ld cycles\\n", htif_argv[1], jtag->exit_code(), random_seed, main_time);
    ret = jtag->exit_code();
  } else if (top->exit_o & 0xFFFFFFFE) {
    int exitcode = ((unsigned int) top->exit_o) >> 1;
    fprintf(stderr, "%s *** FAILED *** (tohost = %d) after %ld cycles\\n", htif_argv[1], exitcode, main_time);
    ret = exitcode;
  } else {
    fprintf(stderr, "%s *** SUCCESS *** (tohost = 0) after %ld cycles\\n", htif_argv[1], main_time);
  }
"""
    patched_result_block = """  top->final();

  if (top->exit_o & 0x1) {
    int exitcode = ((unsigned int) top->exit_o) >> 1;
    if (exitcode) {
      fprintf(stderr, "%s *** FAILED *** (tohost = %d) after %ld cycles\\n", htif_argv[1], exitcode, main_time);
      ret = exitcode;
    } else {
      fprintf(stderr, "%s *** SUCCESS *** (tohost = 0) after %ld cycles\\n", htif_argv[1], main_time);
    }
  } else if (dtm->done() && dtm->exit_code()) {
    fprintf(stderr, "%s *** FAILED *** (tohost = %d) after %ld cycles\\n", htif_argv[1], dtm->exit_code(), main_time);
    ret = dtm->exit_code();
  } else if (jtag->done() && jtag->exit_code()) {
    fprintf(stderr, "%s *** FAILED *** (tohost = %d, seed %d) after %ld cycles\\n", htif_argv[1], jtag->exit_code(), random_seed, main_time);
    ret = jtag->exit_code();
  } else {
    fprintf(stderr, "%s *** SUCCESS *** (tohost = 0) after %ld cycles\\n", htif_argv[1], main_time);
  }
"""
    if result_block not in text:
        raise SystemExit("could not patch ariane_tb.cpp result handling")
    preload_block = """  // Preload memory.
#if (VERILATOR_VERSION_INTEGER >= 5000000)
  // Verilator v5: Use rootp pointer and .data() accessor.
#define MEM top->rootp->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__i_tc_sram_wrapper__DOT__i_tc_sram__DOT__sram.m_storage
#define MEM_USER top->rootp->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__gen_mem_user__DOT__i_tc_sram_wrapper_user__DOT__i_tc_sram__DOT__sram.m_storage
#else
  // Verilator v4
#define MEM top->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__i_tc_sram_wrapper__DOT__i_tc_sram__DOT__sram
#define MEM_USER top->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__gen_mem_user__DOT__i_tc_sram_wrapper_user__DOT__i_tc_sram__DOT__sram
#endif
  long long addr;
  long long len;

  size_t mem_size = 0xFFFFFF;
  while(get_section(&addr, &len))
  {
    if (addr == 0x80000000)
        read_section_void(addr, (void *) MEM , mem_size);
    if (addr == 0x84000000)
        try {
          read_section_void(addr, (void *) MEM_USER , mem_size);
        } catch (...){
          std::cerr << "No user memory instantiated ...\\n";
        }
  }
"""
    patched_preload_block = """  // Preload memory.
#if (VERILATOR_VERSION_INTEGER >= 5000000)
  // Verilator v5: Use rootp pointer.
#define MEM top->rootp->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__i_tc_sram_wrapper__DOT__i_tc_sram__DOT__sram.m_storage
#define MEM_USER top->rootp->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__gen_mem_user__DOT__i_tc_sram_wrapper_user__DOT__i_tc_sram__DOT__sram.m_storage
#else
  // Verilator v4
#define MEM top->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__i_tc_sram_wrapper__DOT__i_tc_sram__DOT__sram
#define MEM_USER top->ariane_testharness__DOT__i_sram__DOT__gen_cut__BRA__0__KET____DOT__gen_mem_user__DOT__i_tc_sram_wrapper_user__DOT__i_tc_sram__DOT__sram
#endif
  static const unsigned long long DramBase = 0x80000000ULL;
  static const unsigned long long DramBytes = 33554432ULL * sizeof(MEM[0]);
  static const unsigned long long UserBase = 0x84000000ULL;
  long long addr;
  long long len;

  while (get_section(&addr, &len)) {
    const unsigned long long uaddr = static_cast<unsigned long long>(addr);
    const unsigned long long ulen = static_cast<unsigned long long>(len);
    if (uaddr >= DramBase && uaddr + ulen <= DramBase + DramBytes) {
      const unsigned long long word_index = (uaddr - DramBase) / sizeof(MEM[0]);
      read_section_void(addr, reinterpret_cast<unsigned char *>(&MEM[0]) + (uaddr - DramBase), ulen);
      std::cerr << std::hex << "*** [naja_preload] DRAM addr=0x" << uaddr
                << " len=0x" << ulen << " word_index=0x" << word_index
                << " word=0x" << MEM[word_index] << std::dec << "\\n";
    } else if (uaddr == UserBase) {
      try {
        read_section_void(addr, reinterpret_cast<unsigned char *>(&MEM_USER[0]), ulen);
      } catch (...) {
        std::cerr << "No user memory instantiated ...\\n";
      }
    } else {
      std::cerr << std::hex << "*** [naja_preload] skipped addr=0x" << uaddr
                << " len=0x" << ulen << std::dec << "\\n";
    }
  }
"""
    if preload_block not in text:
        raise SystemExit("could not patch ariane_tb.cpp memory preload")
    output = stage_dir / "ariane_tb_naja.cpp"
    output.write_text(
        text.replace(result_block, patched_result_block, 1).replace(preload_block, patched_preload_block, 1),
        encoding="utf-8",
    )
    return output


def write_tb_memory_models(repo: Path, stage_dir: Path) -> list[Path]:
    sram = (repo / "common" / "local" / "util" / "sram.sv").read_text(encoding="utf-8")
    sram = sram.replace("module sram #(", "module tb_sram #(", 1)
    sram = sram.replace("tc_sram_wrapper #(", "tb_tc_sram_wrapper #(")
    sram = sram.replace("endmodule : sram", "endmodule : tb_sram", 1)

    wrapper = (repo / "common" / "local" / "util" / "tc_sram_wrapper.sv").read_text(encoding="utf-8")
    wrapper = wrapper.replace("module tc_sram_wrapper #(", "module tb_tc_sram_wrapper #(", 1)
    wrapper = wrapper.replace("tc_sram #(", "tb_tc_sram #(", 1)

    tc_sram = (
        repo / "vendor" / "pulp-platform" / "tech_cells_generic" / "src" / "rtl" / "tc_sram.sv"
    ).read_text(encoding="utf-8")
    tc_sram = tc_sram.replace("module tc_sram #(", "module tb_tc_sram #(", 1)

    outputs = [
        (stage_dir / "tb_sram.sv", sram),
        (stage_dir / "tb_tc_sram_wrapper.sv", wrapper),
        (stage_dir / "tb_tc_sram.sv", tc_sram),
    ]
    for output, text in outputs:
        output.write_text(text, encoding="utf-8")
    return [output for output, _ in outputs]


def write_fesvr_dpi_bridge(repo: Path, stage_dir: Path) -> Path:
    source = repo / "verif" / "core-v-verif" / "vendor" / "riscv" / "riscv-isa-sim" / "fesvr" / "fesvr_dpi.cc"
    if not source.exists():
        raise SystemExit(f"missing core-v-verif FESVR DPI source: {source}")
    text = source.read_text(encoding="utf-8")
    for header in ("config.h", "elf.h", "htif.h", "htif_hexwriter.h", "elfloader.h", "memif.h", "byteorder.h"):
        text = text.replace(f'#include "{header}"', f"#include <fesvr/{header}>")
    text = text.replace("load_elf(filename, memif, entry);", "load_elf(filename, memif, entry, 0);")
    read_section_void = """extern "C" void read_section_void (long long address, void* buffer, uint64_t size = 0) {
    // check that the address points to a section
    assert(mems.count(address) > 0);
    // copy array
    auto it = mems.find(address);

    if (it == mems.end())
        return;

    memif->read(address, (size == 0) ? sections[address] : size , buffer);
}
"""
    patched_read_section_void = """extern "C" void read_section_void (long long address, void* buffer, uint64_t size = 0) {
    // check that the address points to a section
    assert(mems.count(address) > 0);
    auto it = mems.find(address);

    if (it == mems.end())
        return;

    uint64_t copy_size = (size == 0) ? it->second.size() : size;
    if (copy_size > it->second.size())
        copy_size = it->second.size();
    std::memcpy(buffer, it->second.data(), copy_size);
    if (size > copy_size)
        std::memset(static_cast<char*>(buffer) + copy_size, 0, size - copy_size);
}
"""
    if read_section_void not in text:
        raise SystemExit("could not patch FESVR DPI read_section_void")
    text = text.replace(read_section_void, patched_read_section_void, 1)
    output = stage_dir / "naja_fesvr_dpi.cc"
    output.write_text(text, encoding="utf-8")
    return output


def write_remote_bitbang_stub(stage_dir: Path) -> Path:
    output = stage_dir / "naja_remote_bitbang_stub.cc"
    output.write_text(
        r'''#include "remote_bitbang.h"

remote_bitbang_t::remote_bitbang_t(uint16_t) :
  err(0),
  tck(1),
  tms(1),
  tdi(1),
  trstn(1),
  tdo(0),
  quit(0),
  socket_fd(-1),
  client_fd(-1),
  recv_start(0),
  recv_end(0)
{}

void remote_bitbang_t::tick(
    unsigned char *jtag_tck,
    unsigned char *jtag_tms,
    unsigned char *jtag_tdi,
    unsigned char *jtag_trstn,
    unsigned char jtag_tdo) {
  tdo = jtag_tdo;
  *jtag_tck = tck;
  *jtag_tms = tms;
  *jtag_tdi = tdi;
  *jtag_trstn = trstn;
}

void remote_bitbang_t::accept() {}
void remote_bitbang_t::execute_command() {}
void remote_bitbang_t::reset() {}

void remote_bitbang_t::set_pins(char next_tck, char next_tms, char next_tdi) {
  tck = next_tck;
  tms = next_tms;
  tdi = next_tdi;
}
''',
        encoding="utf-8",
    )
    return output


def namespace_generated_netlist(generated: Path, stage_dir: Path, top_module: str = "cva6") -> Path:
    module_decl = re.compile(r"^\s*module\s+([A-Za-z_][A-Za-z0-9_$]*)\b")
    module_names: list[str] = []
    with generated.open(encoding="utf-8") as stream:
        for line in stream:
            match = module_decl.match(line)
            if match:
                module_names.append(match.group(1))

    if top_module not in module_names:
        raise SystemExit(f"generated CVA6 netlist does not define top module {top_module!r}: {generated}")

    rename = {
        name: f"naja_{top_module}_{name}"
        for name in module_names
        if name != top_module and not name.startswith(f"naja_{top_module}_")
    }
    if not rename:
        return generated

    output = stage_dir / f"{generated.stem}_namespaced{generated.suffix}"
    decl_or_inst = re.compile(r"^(\s*)([A-Za-z_][A-Za-z0-9_$]*)(\s+[A-Za-z_][A-Za-z0-9_$]*\s*\()")
    decl = re.compile(r"^(\s*module\s+)([A-Za-z_][A-Za-z0-9_$]*)(\b)")
    end_comment = re.compile(r"(endmodule\s*//)([A-Za-z_][A-Za-z0-9_$]*)(\s*)$")

    with generated.open(encoding="utf-8") as in_stream, output.open("w", encoding="utf-8") as out_stream:
        for line in in_stream:
            decl_match = decl.match(line)
            if decl_match and decl_match.group(2) in rename:
                line = decl.sub(
                    lambda match: f"{match.group(1)}{rename[match.group(2)]}{match.group(3)}",
                    line,
                    count=1,
                )
            else:
                inst_match = decl_or_inst.match(line)
                if inst_match and inst_match.group(2) in rename:
                    line = (
                        f"{inst_match.group(1)}{rename[inst_match.group(2)]}"
                        f"{inst_match.group(3)}{line[inst_match.end():]}"
                    )

            line = end_comment.sub(
                lambda match: f"{match.group(1)}{rename.get(match.group(2), match.group(2))}{match.group(3)}",
                line,
            )
            out_stream.write(line)

    return output


def unique(paths: list[Path]) -> list[Path]:
    seen: set[Path] = set()
    result: list[Path] = []
    for path in paths:
        resolved = path.resolve()
        if resolved not in seen:
            seen.add(resolved)
            result.append(path)
    return result


def build_source_lists(
    repo: Path,
    stage_dir: Path,
    generated: Path,
    primitives: Path,
) -> tuple[list[str], list[Path]]:
    incdirs = [
        repo / "core" / "include",
        repo / "core" / "cache_subsystem" / "hpdcache" / "rtl" / "include",
        repo / "vendor" / "pulp-platform" / "common_cells" / "include",
        repo / "vendor" / "pulp-platform" / "common_cells" / "src",
        repo / "vendor" / "pulp-platform" / "axi" / "include",
        repo / "common" / "local" / "util",
        repo / "corev_apu" / "register_interface" / "include",
        repo / "corev_apu" / "tb" / "common",
        repo / "verif" / "tb" / "core",
        repo / "corev_apu" / "instr_tracing" / "ITI" / "include",
        repo / "corev_apu" / "instr_tracing" / "rv_tracer-main" / "include",
        repo / "corev_apu" / "instr_tracing" / "rv_encapsulator-main" / "src" / "include",
    ]

    def p(rel: str) -> Path:
        return repo / rel

    def glob(rel: str) -> list[Path]:
        return sorted(repo.glob(rel))

    ariane_wrapper = write_ariane_wrapper(stage_dir / "ariane_naja_wrapper.sv")
    patched_testharness = write_patched_testharness(repo, stage_dir / "ariane_testharness_naja.sv")
    memory_models = write_tb_memory_models(repo, stage_dir)

    sources = [
        p("core/cvfpu/src/fpnew_pkg.sv"),
        p("core/include/config_pkg.sv"),
        p(f"core/include/{TARGET}_config_pkg.sv"),
        p("core/include/riscv_pkg.sv"),
        p("core/include/ariane_pkg.sv"),
        p("vendor/pulp-platform/axi/src/axi_pkg.sv"),
        p("core/include/wt_cache_pkg.sv"),
        p("core/include/std_cache_pkg.sv"),
        p("core/include/instr_tracer_pkg.sv"),
        p("core/include/build_config_pkg.sv"),
        p("core/include/aes_pkg.sv"),
        p("core/include/triggers_pkg.sv"),
        p("vendor/pulp-platform/common_cells/src/cf_math_pkg.sv"),
        p("corev_apu/tb/ariane_axi_pkg.sv"),
        p("corev_apu/tb/axi_intf.sv"),
        p("corev_apu/register_interface/src/reg_intf.sv"),
        p("corev_apu/tb/ariane_soc_pkg.sv"),
        p("corev_apu/riscv-dbg/src/dm_pkg.sv"),
        p("corev_apu/tb/ariane_axi_soc_pkg.sv"),
        p("corev_apu/instr_tracing/ITI/include/iti_pkg.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/include/te_pkg.sv"),
        p("corev_apu/instr_tracing/rv_encapsulator-main/src/include/encap_pkg.sv"),
        p("core/cache_subsystem/axi_adapter.sv"),
        generated,
        primitives,
        *memory_models,
        p("core/cva6_rvfi.sv"),
        ariane_wrapper,
        *glob("corev_apu/bootrom/*.sv"),
        *glob("corev_apu/clint/*.sv"),
        *glob("corev_apu/fpga/src/axi2apb/src/*.sv"),
        *glob("corev_apu/fpga/src/apb_timer/*.sv"),
        *glob("corev_apu/fpga/src/axi_slice/src/*.sv"),
        *glob("vendor/pulp-platform/axi_riscv_atomics/src/*.sv"),
        *glob("corev_apu/axi_mem_if/src/*.sv"),
        *[path for path in glob("corev_apu/riscv-dbg/src/*.sv") if path.name != "dm_pkg.sv"],
        p("corev_apu/rv_plic/rtl/rv_plic_target.sv"),
        p("corev_apu/rv_plic/rtl/rv_plic_gateway.sv"),
        p("corev_apu/rv_plic/rtl/plic_regmap.sv"),
        p("corev_apu/rv_plic/rtl/plic_top.sv"),
        p("corev_apu/riscv-dbg/debug_rom/debug_rom.sv"),
        p("corev_apu/register_interface/src/apb_to_reg.sv"),
        p("vendor/pulp-platform/axi/src/axi_multicut.sv"),
        p("vendor/pulp-platform/common_cells/src/rstgen_bypass.sv"),
        p("vendor/pulp-platform/common_cells/src/rstgen.sv"),
        p("vendor/pulp-platform/common_cells/src/addr_decode.sv"),
        p("vendor/pulp-platform/common_cells/src/stream_register.sv"),
        p("vendor/pulp-platform/axi/src/axi_cut.sv"),
        p("vendor/pulp-platform/axi/src/axi_join.sv"),
        p("vendor/pulp-platform/axi/src/axi_delayer.sv"),
        p("vendor/pulp-platform/axi/src/axi_to_axi_lite.sv"),
        p("vendor/pulp-platform/axi/src/axi_id_prepend.sv"),
        p("vendor/pulp-platform/axi/src/axi_atop_filter.sv"),
        p("vendor/pulp-platform/axi/src/axi_err_slv.sv"),
        p("vendor/pulp-platform/axi/src/axi_mux.sv"),
        p("vendor/pulp-platform/axi/src/axi_demux.sv"),
        p("vendor/pulp-platform/axi/src/axi_xbar.sv"),
        p("vendor/pulp-platform/common_cells/src/cdc_2phase.sv"),
        p("vendor/pulp-platform/common_cells/src/spill_register_flushable.sv"),
        p("vendor/pulp-platform/common_cells/src/spill_register.sv"),
        p("vendor/pulp-platform/common_cells/src/deprecated/fifo_v1.sv"),
        p("vendor/pulp-platform/common_cells/src/deprecated/fifo_v2.sv"),
        p("vendor/pulp-platform/common_cells/src/stream_delay.sv"),
        p("vendor/pulp-platform/common_cells/src/lfsr_16bit.sv"),
        p("vendor/pulp-platform/tech_cells_generic/src/deprecated/cluster_clk_cells.sv"),
        p("vendor/pulp-platform/tech_cells_generic/src/deprecated/pulp_clk_cells.sv"),
        p("vendor/pulp-platform/tech_cells_generic/src/rtl/tc_clk.sv"),
        patched_testharness,
        p("corev_apu/tb/ariane_peripherals.sv"),
        p("corev_apu/tb/rvfi_tracer.sv"),
        p("corev_apu/tb/common/uart.sv"),
        p("corev_apu/tb/common/SimDTM.sv"),
        p("corev_apu/tb/common/SimJTAG.sv"),
        p("corev_apu/instr_tracing/ITI/cva6_iti/iti.sv"),
        p("corev_apu/instr_tracing/ITI/cva6_iti/block_retirement.sv"),
        p("corev_apu/instr_tracing/ITI/cva6_iti/single_retirement.sv"),
        p("corev_apu/instr_tracing/ITI/cva6_iti/itype_detector.sv"),
        p("vendor/pulp-platform/common_cells/src/counter.sv"),
        p("vendor/pulp-platform/common_cells/src/sync.sv"),
        p("vendor/pulp-platform/common_cells/src/sync_wedge.sv"),
        p("vendor/pulp-platform/common_cells/src/edge_detect.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/lzc.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/te_branch_map.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/te_filter.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/te_packet_emitter.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/te_priority.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/te_reg.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/te_resync_counter.sv"),
        p("corev_apu/instr_tracing/rv_tracer-main/rtl/rv_tracer.sv"),
        p("vendor/pulp-platform/common_cells/src/fifo_v3.sv"),
        p("corev_apu/instr_tracing/DPTI/slicer_DPTI.sv"),
        p("corev_apu/instr_tracing/rv_encapsulator-main/src/rtl/encapsulator.sv"),
        p("corev_apu/tb/common/mock_uart.sv"),
    ]

    missing = [path for path in sources if not path.exists()]
    if missing:
        raise SystemExit("missing CVA6 simulation sources:\n" + "\n".join(str(path) for path in missing))
    return [str(path) for path in incdirs if path.exists()], unique(sources)


def write_flist(path: Path, incdirs: list[str], sources: list[Path]) -> Path:
    with path.open("w", encoding="utf-8") as stream:
        for incdir in incdirs:
            stream.write(f"+incdir+{incdir}\n")
        for source in sources:
            stream.write(str(source.resolve()) + "\n")
    return path


def build_verilator_model(
    repo: Path,
    artifacts: Path,
    generated: Path,
    primitives: Path,
    spike_dir: Path,
    verilator_dir: Path,
    jobs: str,
) -> Path:
    stage_dir = artifacts / "cva6_helloworld_sim"
    stage_dir.mkdir(parents=True, exist_ok=True)
    obj_dir = stage_dir / "work-ver"
    namespaced_generated = namespace_generated_netlist(generated, stage_dir)
    fesvr_dpi = write_fesvr_dpi_bridge(repo, stage_dir)
    remote_bitbang_stub = write_remote_bitbang_stub(stage_dir)
    ariane_tb = write_patched_ariane_tb(repo, stage_dir)
    flist = write_flist(
        stage_dir / "cva6_helloworld_verilator.flist",
        *build_source_lists(repo, stage_dir, namespaced_generated, primitives),
    )
    verilator_include = verilator_dir / "share" / "verilator" / "include"
    cflags = " ".join([
        f"-I{repo / 'corev_apu' / 'tb' / 'dpi'}",
        f"-I{spike_dir / 'include'}",
        f"-I{verilator_include / 'vltstd'}",
        "-std=c++17",
        "-O3",
        "-DVL_DEBUG",
    ])
    ldflags = " ".join([
        f"-L{spike_dir / 'lib'}",
        f"-Wl,-rpath,{spike_dir / 'lib'}",
        "-lfesvr",
        "-lriscv",
        "-ldisasm",
        "-lpthread",
    ])
    command = [
        "verilator",
        "--no-timing",
        str(repo / "verilator_config.vlt"),
        "-f",
        str(flist),
        "--unroll-count",
        "256",
        "-Wall",
        "-Wno-fatal",
        "-Wno-PINCONNECTEMPTY",
        "-Wno-ASSIGNDLY",
        "-Wno-DECLFILENAME",
        "-Wno-UNUSED",
        "-Wno-UNOPTFLAT",
        "-Wno-BLKANDNBLK",
        "-Wno-style",
        "-Wno-TIMESCALEMOD",
        "-Wno-WIDTH",
        "-Wno-WIDTHTRUNC",
        "-Wno-WIDTHEXPAND",
        "-Wno-ASCRANGE",
        "-Wno-SIDEEFFECT",
        "-Wno-UNSIGNED",
        "-Wno-IGNOREDRETURN",
        "-Wno-LATCH",
        "-Wno-COMBDLY",
        "--cc",
        "--vpi",
        "--top-module",
        "ariane_testharness",
        "--threads-dpi",
        "none",
        "--Mdir",
        str(obj_dir),
        "-O3",
        "--exe",
        str(ariane_tb),
        str(repo / "corev_apu" / "tb" / "dpi" / "SimDTM.cc"),
        str(repo / "corev_apu" / "tb" / "dpi" / "SimJTAG.cc"),
        str(repo / "corev_apu" / "tb" / "dpi" / "msim_helper.cc"),
        str(fesvr_dpi),
        str(remote_bitbang_stub),
        "-CFLAGS",
        cflags,
        "-LDFLAGS",
        ldflags,
    ]
    run(command, cwd=repo)
    make_command = ["make", f"-j{jobs}", "-f", "Variane_testharness.mk"]
    if shutil.which("ccache"):
        make_command.append("CXX=ccache c++")
    run(make_command, cwd=obj_dir)
    return obj_dir / "Variane_testharness"


def prepend_library_path(env: dict[str, str], variable: str, path: Path) -> None:
    existing = env.get(variable, "")
    env[variable] = str(path) + (os.pathsep + existing if existing else "")


def run_sim(
    executable: Path,
    elf: Path,
    tohost: str,
    spike_dir: Path,
    max_cycles: int,
    sim_plusargs: list[str],
) -> None:
    env = os.environ.copy()
    prepend_library_path(env, "LD_LIBRARY_PATH", spike_dir / "lib")
    prepend_library_path(env, "DYLD_LIBRARY_PATH", spike_dir / "lib")
    command = [
        str(executable),
        "--max-cycles",
        str(max_cycles),
        str(elf),
        "+debug_disable=1",
        f"+time_out={max_cycles}",
        f"+tohost_addr={tohost}",
        "+UVM_VERBOSITY=UVM_NONE",
    ]
    command.extend(sim_plusargs)
    run(command, cwd=executable.parent.parent, env=env)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--primitives", type=Path, required=True)
    parser.add_argument("--max-cycles", type=int, default=2_000_000)
    parser.add_argument(
        "--program",
        action="append",
        choices=PROGRAMS,
        default=[],
        help="Firmware program to compile and simulate. Repeat to run several programs.",
    )
    parser.add_argument("--sim-plusarg", action="append", default=[])
    parser.add_argument("--jobs", default=default_jobs())
    args = parser.parse_args()

    repo = args.repo.resolve()
    artifacts = args.artifacts.resolve()
    generated = args.generated.resolve()
    primitives = args.primitives.resolve()
    if not generated.exists():
        raise SystemExit(f"missing generated CVA6 netlist: {generated}")
    if not primitives.exists():
        raise SystemExit(f"missing Naja primitives: {primitives}")

    prefix = find_riscv_tool_prefix()
    spike_dir = find_spike_install_dir().resolve()
    verilator_dir = find_verilator_install_dir().resolve()
    executable = build_verilator_model(
        repo,
        artifacts,
        generated,
        primitives,
        spike_dir,
        verilator_dir,
        normalize_jobs(args.jobs),
    )
    programs = args.program or ["hello_world"]
    for program in programs:
        elf = compile_program(repo, artifacts, prefix, program)
        tohost = tohost_address(elf, prefix)
        print(f"[cva6-sim] running program={program}", flush=True)
        run_sim(executable, elf, tohost, spike_dir, args.max_cycles, args.sim_plusarg)
        print(f"[cva6-sim] program {program} passed", flush=True)
    print(PASS_MARKER)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
