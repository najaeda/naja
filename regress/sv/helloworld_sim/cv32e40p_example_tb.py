#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Run CV32E40P example_tb/core firmware with CV32E40P RTL or a Naja netlist."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess

HWLP_PASS_MARKER = "CV32E40P_HWLP_SIM_PASS"

PROGRAMS = {
    "hello_world": {
        "make_target": "custom/hello_world.hex",
        "firmware": "custom/hello_world.hex",
        "gcc_flags": "-march=rv32imc_zicsr -mabi=ilp32",
        "pass_marker": "CV32E40P_HELLOWORLD_SIM_PASS",
    },
    "interrupt": {
        "make_target": "interrupt/interrupt.hex",
        "firmware": "interrupt/interrupt.hex",
        "gcc_flags": "-march=rv32imc_zicsr -mabi=ilp32 -Wno-error=int-conversion",
        "pass_marker": "CV32E40P_INTERRUPT_SIM_PASS",
    },
    "hwlp": {
        "firmware": "hwlp_naja.hex",
        "gcc_flags": "-march=rv32im_zicsr_zifencei -mabi=ilp32 -Wno-error=int-conversion",
        "pass_marker": HWLP_PASS_MARKER,
    },
}

CV32E40P_HWLP_OPCODE = 0b0101011
CV32E40P_HWLP_FUNCT3 = 0b100
HWLP_REGISTER_IDS = {
    "x0": 0,
    "x1": 1,
}
GPR_IDS = {
    "zero": 0, "ra": 1, "sp": 2, "gp": 3, "tp": 4,
    "t0": 5, "t1": 6, "t2": 7, "s0": 8, "fp": 8, "s1": 9,
    "a0": 10, "a1": 11, "a2": 12, "a3": 13, "a4": 14, "a5": 15,
    "a6": 16, "a7": 17, "s2": 18, "s3": 19, "s4": 20, "s5": 21,
    "s6": 22, "s7": 23, "s8": 24, "s9": 25, "s10": 26, "s11": 27,
    "t3": 28, "t4": 29, "t5": 30, "t6": 31,
}
GPR_IDS.update({f"x{index}": index for index in range(32)})


def run(args: list[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    print("$ " + (" ".join(args) if cwd is None else f"(cd {cwd} && {' '.join(args)})"), flush=True)
    subprocess.run(args, cwd=str(cwd) if cwd else None, env=env, check=True)


def encode_hwlp_instruction(*, funct4: int, loop_id: int, rs1: int, imm12: int) -> int:
    return (
        ((imm12 & 0xfff) << 20)
        | ((rs1 & 0x1f) << 15)
        | (CV32E40P_HWLP_FUNCT3 << 12)
        | ((funct4 & 0xf) << 8)
        | ((loop_id & 0x1) << 7)
        | CV32E40P_HWLP_OPCODE
    )


def hwlp_byte_length_to_encoded(value: str) -> int:
    byte_length = int(value, 0)
    if byte_length % 4 != 0:
        raise SystemExit(f"CV32E40P HWLP byte length must be 4-byte aligned: {value}")
    encoded = byte_length // 4
    if not 0 <= encoded <= 31:
        raise SystemExit(f"CV32E40P HWLP encoded byte length out of range: {value}")
    return encoded


def hwlp_loop_id(register: str) -> int:
    try:
        return HWLP_REGISTER_IDS[register.strip()]
    except KeyError as exc:
        raise SystemExit(f"unsupported CV32E40P HWLP loop register: {register}") from exc


def gpr_id(register: str) -> int:
    try:
        return GPR_IDS[register.strip()]
    except KeyError as exc:
        raise SystemExit(f"unsupported CV32E40P HWLP source register: {register}") from exc


def hwlp_setupi_word(loop_register: str, count: str, byte_length: str) -> int:
    return encode_hwlp_instruction(
        funct4=0b0110,
        loop_id=hwlp_loop_id(loop_register),
        rs1=hwlp_byte_length_to_encoded(byte_length),
        imm12=int(count, 0),
    )


def hwlp_setup_word(loop_register: str, source_register: str, byte_length: str) -> int:
    return encode_hwlp_instruction(
        funct4=0b0111,
        loop_id=hwlp_loop_id(loop_register),
        rs1=gpr_id(source_register),
        imm12=hwlp_byte_length_to_encoded(byte_length),
    )


def hwlp_starti_word(loop_register: str, byte_length: str) -> int:
    return encode_hwlp_instruction(
        funct4=0b0000,
        loop_id=hwlp_loop_id(loop_register),
        rs1=0,
        imm12=hwlp_byte_length_to_encoded(byte_length),
    )


def hwlp_endi_word(loop_register: str, byte_length: str) -> int:
    return encode_hwlp_instruction(
        funct4=0b0010,
        loop_id=hwlp_loop_id(loop_register),
        rs1=0,
        imm12=hwlp_byte_length_to_encoded(byte_length),
    )


def hwlp_counti_word(loop_register: str, count: str) -> int:
    return encode_hwlp_instruction(
        funct4=0b0100,
        loop_id=hwlp_loop_id(loop_register),
        rs1=0,
        imm12=int(count, 0),
    )


def hwlp_count_word(loop_register: str, source_register: str) -> int:
    return encode_hwlp_instruction(
        funct4=0b0101,
        loop_id=hwlp_loop_id(loop_register),
        rs1=gpr_id(source_register),
        imm12=0,
    )


def find_riscv_tool_prefix() -> str:
    for prefix in ("riscv32-unknown-elf-", "riscv-none-elf-", "riscv64-unknown-elf-"):
        if shutil.which(prefix + "gcc") and shutil.which(prefix + "objcopy"):
            return prefix
    raise SystemExit(
        "missing RISC-V toolchain: expected riscv32-unknown-elf-{gcc,objcopy}, "
        "riscv-none-elf-{gcc,objcopy}, or riscv64-unknown-elf-{gcc,objcopy}"
    )


def capture(args: list[str]) -> str:
    return subprocess.run(
        args,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
    ).stdout.strip()


def capture_optional(args: list[str]) -> str | None:
    try:
        return capture(args)
    except subprocess.CalledProcessError:
        return None


def resolve_existing_absolute_path(path_text: str) -> Path | None:
    path = Path(path_text)
    if not path.is_absolute():
        return None
    resolved = path.resolve()
    if not resolved.exists():
        return None
    return resolved


def resolve_gcc_file(gcc: str, gcc_args: list[str], filename: str) -> Path | None:
    result = capture_optional([gcc, *gcc_args, f"-print-file-name={filename}"])
    if result is None:
        return None
    return resolve_existing_absolute_path(result)


def infer_include_dir(toolchain_root: Path, rv32_libc: Path) -> Path:
    candidates = [ancestor / "include" for ancestor in rv32_libc.parents]
    candidates += [
        toolchain_root / "riscv64-unknown-elf" / "include",
        toolchain_root / "riscv-none-elf" / "include",
        rv32_libc.parent.parent.parent / "include",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit(
        "missing RISC-V target include directory; checked: "
        + ", ".join(str(candidate) for candidate in candidates)
    )


def infer_riscv_root(env: dict[str, str], tool_prefix: str, work_dir: Path) -> None:
    if env.get("RISCV"):
        return
    gcc = shutil.which(tool_prefix + "gcc")
    if not gcc:
        return
    toolchain_root = Path(gcc).resolve().parents[1]
    if tool_prefix in ("riscv64-unknown-elf-", "riscv-none-elf-"):
        compat_root = work_dir / "riscv32-prefix-compat"
        compat_target = compat_root / "riscv32-unknown-elf"
        rv32_gcc_args = [
            "-march=rv32imc_zicsr",
            "-mabi=ilp32",
        ]
        rv32_libc = resolve_gcc_file(gcc, rv32_gcc_args, "libc.a")
        if rv32_libc is None:
            raise SystemExit(
                "missing RV32 C library for CV32E40P hello_world: "
                f"{gcc} did not resolve libc.a for -march=rv32imc_zicsr -mabi=ilp32. "
                "Install an RV32 newlib-capable bare-metal RISC-V GCC toolchain."
            )
        rv32_lib = rv32_libc.parent
        include_dir = infer_include_dir(toolchain_root, rv32_libc)
        compat_root.mkdir(parents=True, exist_ok=True)
        if compat_target.is_symlink():
            compat_target.unlink()
        compat_target.mkdir(parents=True, exist_ok=True)
        compat_include = compat_target / "include"
        compat_lib = compat_target / "lib"
        if compat_include.is_symlink():
            compat_include.unlink()
        if compat_lib.is_symlink():
            compat_lib.unlink()
        if not compat_include.exists():
            compat_include.symlink_to(include_dir, target_is_directory=True)
        if not compat_lib.exists():
            compat_lib.symlink_to(rv32_lib, target_is_directory=True)
        env["RISCV"] = str(compat_root)
    else:
        env["RISCV"] = str(toolchain_root)


def write_parameterless_tb_subsystem(source: Path, target: Path) -> Path:
    text = source.read_text(encoding="utf-8")
    parameterized_instance = """  cv32e40p_top #(
      .PULP_XPULP      (PULP_XPULP),
      .PULP_CLUSTER    (PULP_CLUSTER),
      .FPU             (FPU),
      .FPU_ADDMUL_LAT  (FPU_ADDMUL_LAT),
      .FPU_OTHERS_LAT  (FPU_OTHERS_LAT),
      .ZFINX           (ZFINX),
      .NUM_MHPMCOUNTERS(NUM_MHPMCOUNTERS)
  ) top_i ("""
    plain_instance = "  cv32e40p_top top_i ("
    if parameterized_instance not in text:
        raise SystemExit(f"could not patch cv32e40p_top instance parameters in {source}")
    debug_anchor = """
endmodule  // cv32e40p_tb_subsystem
"""
    debug_block = """
  // Core-side IRQ entry probe; signal names are shared by RTL and Naja netlist output.
  wire        naja_irq_core_all_pending = irq_software && irq_timer && irq_external && irq_fast == 16'hffff;
  wire        naja_core_irq_req         = top_i.core_i.id_stage_i.irq_req_ctrl;
  wire [4:0]  naja_core_irq_id          = top_i.core_i.id_stage_i.irq_id_ctrl;
  wire [4:0]  naja_core_fsm_cs          = top_i.core_i.id_stage_i.controller_i.ctrl_fsm_cs;
  wire [4:0]  naja_core_fsm_ns          = top_i.core_i.id_stage_i.controller_i.ctrl_fsm_ns;
  logic       naja_irq_core_entry_active;
  int unsigned naja_irq_core_entry_cycles;

  always_ff @(posedge clk_i or negedge rst_ni) begin : naja_irq_core_entry_debug
    if (!rst_ni) begin
      naja_irq_core_entry_active <= 1'b0;
      naja_irq_core_entry_cycles <= 0;
    end else if ($test$plusargs("naja_irq_entry_debug")) begin
      if (!naja_irq_core_entry_active && naja_irq_core_all_pending) begin
        naja_irq_core_entry_active <= 1'b1;
        naja_irq_core_entry_cycles <= 0;
      end
      if ((naja_irq_core_entry_active || naja_irq_core_all_pending)
          && naja_irq_core_entry_cycles < 96) begin
        $display(
            "[NAJA_IRQ_ENTRY_CORE] c=%0d t=%0t pc_if=0x%08x pc_id=0x%08x instr_addr=0x%08x instr=0x%08x instr_id=0x%08x valid=%0b dec=%0b id_ready=%0b id_valid=%0b irq_req=%0b irq_id=%0d irq_ack=%0b ack_id=%0d pc_set=%0b pc_mux=0x%0h exc_pc=0x%0h vec=%0d trap=%0d cause=0x%02x csr_cause=0x%02x csr_save=%0b%0b%0b%0b mie=%0b mie31=%0b mip=0x%08x mtvec_mode=%0d mtvec=0x%06x mepc=0x%08x fsm=%0d/%0d",
            naja_irq_core_entry_cycles, $time, top_i.core_i.pc_if, top_i.core_i.pc_id,
            instr_addr, instr_rdata, top_i.core_i.instr_rdata_id,
            top_i.core_i.instr_valid_id, top_i.core_i.is_decoding,
            top_i.core_i.id_ready, top_i.core_i.id_valid,
            naja_core_irq_req, naja_core_irq_id, irq_ack, irq_id_out,
            top_i.core_i.pc_set, top_i.core_i.pc_mux_id,
            top_i.core_i.exc_pc_mux_id, top_i.core_i.m_exc_vec_pc_mux_id,
            top_i.core_i.trap_addr_mux, top_i.core_i.exc_cause,
            top_i.core_i.csr_cause, top_i.core_i.csr_save_cause,
            top_i.core_i.csr_save_if, top_i.core_i.csr_save_id,
            top_i.core_i.csr_save_ex, top_i.core_i.m_irq_enable,
            top_i.core_i.mie_bypass[31], top_i.core_i.mip,
            top_i.core_i.mtvec_mode, top_i.core_i.mtvec, top_i.core_i.mepc,
            naja_core_fsm_cs, naja_core_fsm_ns);
        naja_irq_core_entry_cycles <= naja_irq_core_entry_cycles + 1;
      end
    end
  end

"""
    patched = text.replace(parameterized_instance, plain_instance)
    if debug_anchor not in patched:
        raise SystemExit(f"could not patch core IRQ debug probe in {source}")
    target.write_text(patched.replace(debug_anchor, debug_block + debug_anchor), encoding="utf-8")
    return target


def write_verilator_mm_ram(source: Path, target: Path) -> Path:
    text = source.read_text(encoding="utf-8")
    guarded_instance = """`ifndef VERILATOR
  cv32e40p_random_interrupt_generator random_interrupt_generator_i (
      .rst_ni          (rst_ni),
      .clk_i           (clk_i),
      .irq_i           (1'b0),
      .irq_ack_i       (irq_ack_i),
      .irq_ack_o       (),
      .irq_rnd_lines_o (irq_rnd_lines),
      .irq_mode_i      (rnd_stall_regs[10]),
      .irq_min_cycles_i(rnd_stall_regs[11]),
      .irq_max_cycles_i(rnd_stall_regs[12]),
      .irq_min_id_i    (IRQ_MIN_ID),
      .irq_max_id_i    (IRQ_MAX_ID),
      .irq_act_id_o    (),
      .irq_id_we_o     (),
      .irq_pc_id_i     (pc_core_id_i),
      .irq_pc_trig_i   (rnd_stall_regs[13]),
      .irq_lines_i     (rnd_stall_regs[14][31:0])
  );

`endif"""
    plain_instance = """  cv32e40p_random_interrupt_generator random_interrupt_generator_i (
      .rst_ni          (rst_ni),
      .clk_i           (clk_i),
      .irq_i           (1'b0),
      .irq_ack_i       (irq_ack_i),
      .irq_ack_o       (),
      .irq_rnd_lines_o (irq_rnd_lines),
      .irq_mode_i      (rnd_stall_regs[10]),
      .irq_min_cycles_i(rnd_stall_regs[11]),
      .irq_max_cycles_i(rnd_stall_regs[12]),
      .irq_min_id_i    (IRQ_MIN_ID),
      .irq_max_id_i    (IRQ_MAX_ID),
      .irq_act_id_o    (),
      .irq_id_we_o     (),
      .irq_pc_id_i     (pc_core_id_i),
      .irq_pc_trig_i   (rnd_stall_regs[13]),
      .irq_lines_i     (rnd_stall_regs[14][31:0])
    );"""
    if guarded_instance not in text:
        raise SystemExit(f"could not patch interrupt generator instance in {source}")
    debug_anchor = """  // the amo shim has a different encoding of atomics
"""
    debug_block = """  // Lightweight IRQ probe for the CV32E40P interrupt firmware debug path.
  Interrupts_tb_t naja_irq_debug_prev_lines;
  logic           naja_irq_debug_prev_ack;
  logic [4:0]     naja_irq_debug_prev_id;
  int unsigned    naja_irq_debug_events;
  logic           naja_irq_entry_active;
  int unsigned    naja_irq_entry_cycles;

  always_ff @(posedge clk_i or negedge rst_ni) begin : naja_irq_debug
    if (!rst_ni) begin
      naja_irq_debug_prev_lines <= '0;
      naja_irq_debug_prev_ack   <= 1'b0;
      naja_irq_debug_prev_id    <= '0;
      naja_irq_debug_events     <= 0;
      naja_irq_entry_active     <= 1'b0;
      naja_irq_entry_cycles     <= 0;
    end else begin
      if ($test$plusargs("naja_irq_debug")) begin
        if (naja_irq_debug_events < 128
            && (irq_rnd_lines != naja_irq_debug_prev_lines
            || irq_ack_i != naja_irq_debug_prev_ack
            || irq_id_i != naja_irq_debug_prev_id)) begin
          $display(
              "[NAJA_IRQ_DEBUG] t=%0t pc=0x%08x mode=0x%08x irq_word=0x%08x sw=%0b timer=%0b ext=%0b fast=0x%04x ack=%0b id=%0d",
              $time, pc_core_id_i, rnd_stall_regs[10], rnd_stall_regs[14],
              irq_rnd_lines.irq_software, irq_rnd_lines.irq_timer,
              irq_rnd_lines.irq_external, irq_rnd_lines.irq_fast,
              irq_ack_i, irq_id_i);
          naja_irq_debug_events <= naja_irq_debug_events + 1;
        end
        naja_irq_debug_prev_lines <= irq_rnd_lines;
        naja_irq_debug_prev_ack   <= irq_ack_i;
        naja_irq_debug_prev_id    <= irq_id_i;
      end

      if ($test$plusargs("naja_irq_entry_debug")) begin
        if (!naja_irq_entry_active
            && rnd_stall_regs[10] == 32'd4
            && rnd_stall_regs[14] == 32'hffff_ffff) begin
          naja_irq_entry_active <= 1'b1;
          naja_irq_entry_cycles <= 0;
        end
        if ((naja_irq_entry_active
            || (rnd_stall_regs[10] == 32'd4 && rnd_stall_regs[14] == 32'hffff_ffff))
            && naja_irq_entry_cycles < 96) begin
          $display(
              "[NAJA_IRQ_ENTRY] c=%0d t=%0t pc=0x%08x irq_word=0x%08x fast=0x%04x sw=%0b timer=%0b ext=%0b ack=%0b id=%0d",
              naja_irq_entry_cycles, $time, pc_core_id_i, rnd_stall_regs[14],
              irq_rnd_lines.irq_fast, irq_rnd_lines.irq_software,
              irq_rnd_lines.irq_timer, irq_rnd_lines.irq_external,
              irq_ack_i, irq_id_i);
          naja_irq_entry_cycles <= naja_irq_entry_cycles + 1;
        end
      end
    end
  end

"""
    patched = text.replace(guarded_instance, plain_instance)
    if debug_anchor not in patched:
        raise SystemExit(f"could not patch IRQ debug probe in {source}")
    target.write_text(patched.replace(debug_anchor, debug_block + debug_anchor), encoding="utf-8")
    return target


def write_verilator_interrupt_generator(target: Path) -> Path:
    target.write_text("""module cv32e40p_random_interrupt_generator (
    input  logic        rst_ni,
    input  logic        clk_i,
    input  logic        irq_i,
    input  logic        irq_ack_i,
    output logic [18:0] irq_rnd_lines_o,
    output logic        irq_ack_o,
    input  logic [31:0] irq_mode_i,
    input  logic [31:0] irq_min_cycles_i,
    input  logic [31:0] irq_max_cycles_i,
    input  logic [31:0] irq_min_id_i,
    input  logic [31:0] irq_max_id_i,
    output logic [31:0] irq_act_id_o,
    output logic        irq_id_we_o,
    input  logic [31:0] irq_pc_id_i,
    input  logic [31:0] irq_pc_trig_i,
    input  logic [31:0] irq_lines_i
);
  localparam logic [31:0] STANDARD = 32'd1;
  localparam logic [31:0] RANDOM = 32'd2;
  localparam logic [31:0] SOFTWARE_DEFINED = 32'd4;

  logic [31:0] irq_mode_q;
  logic [18:0] irq_lines_q;
  logic [31:0] cycle_count_q;
  logic [31:0] random_irq_id_q;

  function automatic logic [18:0] map_irq_word(input logic [31:0] irq_word);
    map_irq_word = {irq_word[3], irq_word[7], irq_word[11], irq_word[31:16]};
  endfunction

  function automatic logic [18:0] map_irq_id(input logic [31:0] irq_id);
    map_irq_id = '0;
    unique case (irq_id)
      32'd3: map_irq_id[18] = 1'b1;
      32'd7: map_irq_id[17] = 1'b1;
      32'd11: map_irq_id[16] = 1'b1;
      default: begin
        if (irq_id >= 32'd16 && irq_id <= 32'd31) begin
          map_irq_id[irq_id - 32'd16] = 1'b1;
        end
      end
    endcase
  endfunction

  function automatic logic [31:0] bounded_irq_id(
      input logic [31:0] irq_id,
      input logic [31:0] min_id,
      input logic [31:0] max_id
  );
    if (irq_id < min_id || irq_id > max_id) begin
      bounded_irq_id = min_id;
    end else begin
      bounded_irq_id = irq_id;
    end
  endfunction

  function automatic logic [31:0] next_irq_id(
      input logic [31:0] irq_id,
      input logic [31:0] min_id,
      input logic [31:0] max_id
  );
    if (irq_id < min_id || irq_id >= max_id) begin
      next_irq_id = min_id;
    end else begin
      next_irq_id = irq_id + 32'd1;
    end
  endfunction

  assign irq_ack_o = irq_ack_i;
  assign irq_rnd_lines_o = irq_lines_q;
  assign irq_act_id_o = random_irq_id_q;
  assign irq_id_we_o = irq_ack_i;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      irq_mode_q <= 32'd0;
      irq_lines_q <= '0;
      cycle_count_q <= 32'd0;
      random_irq_id_q <= 32'd0;
    end else begin
      irq_mode_q <= irq_mode_i;

      unique case (irq_mode_i)
        SOFTWARE_DEFINED: begin
          irq_lines_q <= map_irq_word(irq_lines_i);
          cycle_count_q <= 32'd0;
        end

        RANDOM: begin
          if (irq_mode_q != RANDOM) begin
            irq_lines_q <= '0;
            cycle_count_q <= 32'd0;
            random_irq_id_q <= bounded_irq_id(irq_min_id_i, irq_min_id_i, irq_max_id_i);
          end else if (irq_ack_i) begin
            irq_lines_q <= '0;
            cycle_count_q <= 32'd0;
          end else if (cycle_count_q >= irq_min_cycles_i) begin
            irq_lines_q <= map_irq_id(random_irq_id_q);
            cycle_count_q <= 32'd0;
            random_irq_id_q <= next_irq_id(random_irq_id_q, irq_min_id_i, irq_max_id_i);
          end else if (irq_lines_q != '0) begin
            cycle_count_q <= 32'd0;
          end else begin
            cycle_count_q <= cycle_count_q + 32'd1;
          end
        end

        STANDARD: begin
          irq_lines_q <= '0;
          cycle_count_q <= 32'd0;
        end

        default: begin
          irq_lines_q <= '0;
          cycle_count_q <= 32'd0;
        end
      endcase
    end
  end
endmodule
""", encoding="utf-8")
    return target


def normalize_manifest_token(token: str, *, rtl_dir: Path) -> str:
    return token.replace("${DESIGN_RTL_DIR}", str(rtl_dir))


def read_cv32e40p_manifest(repo_dir: Path) -> tuple[list[str], list[Path]]:
    rtl_dir = repo_dir / "rtl"
    manifest = repo_dir / "cv32e40p_manifest.flist"
    include_dirs: list[str] = []
    sources: list[Path] = []
    for raw_line in manifest.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("//"):
            continue
        token = normalize_manifest_token(line, rtl_dir=rtl_dir)
        if token.startswith("+incdir+"):
            include_dirs.append(token)
        else:
            sources.append(Path(token).resolve())
    return include_dirs, sources


def patch_cv32e40p_hwlp_header(header_path: Path) -> None:
    text = header_path.read_text(encoding="utf-8")

    replacements = [
        (
            r"lp\.setupi\s+(x[01]),\s*([0-9]+),\s*([0-9]+)",
            lambda match: (
                f".word 0x{hwlp_setupi_word(match[1], match[2], match[3]):08x}"
            ),
        ),
        (
            r"lp\.setup\s+(x[01]),\s*([a-z0-9]+),\s*([0-9]+)",
            lambda match: (
                f".word 0x{hwlp_setup_word(match[1], match[2], match[3]):08x}"
            ),
        ),
        (
            r"lp\.starti\s+(x[01]),\s*([0-9]+)",
            lambda match: (
                f".word 0x{hwlp_starti_word(match[1], match[2]):08x}"
            ),
        ),
        (
            r"lp\.endi\s+(x[01]),\s*([0-9]+)",
            lambda match: (
                f".word 0x{hwlp_endi_word(match[1], match[2]):08x}"
            ),
        ),
        (
            r"lp\.counti\s+(x[01]),\s*([0-9]+)",
            lambda match: (
                f".word 0x{hwlp_counti_word(match[1], match[2]):08x}"
            ),
        ),
        (
            r"lp\.count\s+(x[01]),\s*([a-z0-9]+)",
            lambda match: (
                f".word 0x{hwlp_count_word(match[1], match[2]):08x}"
            ),
        ),
    ]

    patched = text
    for pattern, replacement in replacements:
        patched = re.sub(pattern, replacement, patched)
    if re.search(r"lp\.(setupi|setup|starti|endi|counti|count)\s+x[01]", patched):
        raise SystemExit(f"could not patch all CV32E40P HWLP mnemonics in {header_path}")
    if patched != text:
        header_path.write_text(patched, encoding="utf-8")


def build_firmware(
    *,
    program_name: str,
    program: dict[str, str],
    tb_dir: Path,
    work_dir: Path,
    tool_prefix: str,
    env: dict[str, str],
) -> Path:
    if program_name == "hello_world":
        run([
            "make",
            str(program["make_target"]),
            f"RISCV_EXE_PREFIX={tool_prefix}",
            "CUSTOM_GCC_FLAGS=" + str(program["gcc_flags"]),
        ], cwd=tb_dir, env=env)
        return tb_dir / str(program["firmware"])

    if program_name == "hwlp":
        patch_cv32e40p_hwlp_header(tb_dir / "hwlp_test" / "hwlp.h")
        elf = work_dir / "hwlp_naja.elf"
        hex_path = work_dir / str(program["firmware"])
        riscv_root = Path(env["RISCV"])
        run([
            tool_prefix + "gcc",
            "-o",
            str(elf),
            "-w",
            "-O0",
            "-g",
            "-nostdlib",
            *str(program["gcc_flags"]).split(),
            "-T",
            str(tb_dir / "custom" / "link.ld"),
            "-static",
            str(tb_dir / "custom" / "crt0.S"),
            str(tb_dir / "hwlp_test" / "hwlp_test.c"),
            str(tb_dir / "mem_stall" / "mem_stall.c"),
            str(tb_dir / "custom" / "syscalls.c"),
            str(tb_dir / "custom" / "vectors.S"),
            "-I",
            str(riscv_root / "riscv32-unknown-elf" / "include"),
            "-I",
            str(tb_dir / "mem_stall"),
            "-I",
            str(tb_dir / "hwlp_test"),
            "-L",
            str(riscv_root / "riscv32-unknown-elf" / "lib"),
            "-lc",
            "-lm",
            "-lgcc",
        ], cwd=tb_dir, env=env)
        run([tool_prefix + "objcopy", "-O", "verilog", str(elf), str(hex_path)], cwd=tb_dir, env=env)
        return hex_path

    if program_name != "interrupt":
        raise SystemExit(f"unsupported CV32E40P program: {program_name}")

    elf = work_dir / "interrupt_naja.elf"
    hex_path = work_dir / "interrupt_naja.hex"
    riscv_root = Path(env["RISCV"])
    run([
        tool_prefix + "gcc",
        "-march=rv32imc",
        "-o",
        str(elf),
        "-w",
        "-Os",
        "-g",
        "-nostdlib",
        *str(program["gcc_flags"]).split(),
        "-T",
        str(tb_dir / "custom" / "link.ld"),
        "-static",
        str(tb_dir / "custom" / "crt0.S"),
        str(tb_dir / "interrupt" / "interrupt.c"),
        str(tb_dir / "mem_stall" / "mem_stall.c"),
        str(tb_dir / "custom" / "syscalls.c"),
        str(tb_dir / "interrupt" / "vectors.S"),
        "-I",
        str(riscv_root / "riscv32-unknown-elf" / "include"),
        "-I",
        str(tb_dir / "mem_stall"),
        "-I",
        str(tb_dir / "interrupt"),
        "-L",
        str(riscv_root / "riscv32-unknown-elf" / "lib"),
        "-lc",
        "-lm",
        "-lgcc",
    ], cwd=tb_dir, env=env)
    run([tool_prefix + "objcopy", "-O", "verilog", str(elf), str(hex_path)], cwd=tb_dir, env=env)
    return hex_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--primitives", type=Path, required=True)
    parser.add_argument(
        "--core-source",
        choices=("generated", "rtl"),
        default="generated",
        help="Use the Naja-generated cv32e40p_top netlist or upstream RTL.",
    )
    parser.add_argument("--program", choices=sorted(PROGRAMS), default="hello_world")
    parser.add_argument("--max-cycles", default="2000000")
    parser.add_argument(
        "--sim-plusarg",
        action="append",
        default=[],
        help="Extra Verilator runtime plusarg, for example +naja_irq_debug.",
    )
    args = parser.parse_args()
    program = PROGRAMS[args.program]

    repo_dir = args.repo.resolve()
    artifacts_dir = args.artifacts.resolve()
    generated_path = args.generated.resolve()
    primitives_path = args.primitives.resolve()
    tb_dir = repo_dir / "example_tb" / "core"
    rtl_include_dir = repo_dir / "rtl" / "include"
    stage_dir = f"{args.program}_sim" if args.core_source == "generated" else f"{args.program}_sim_rtl"
    work_dir = artifacts_dir / stage_dir / "cv32e40p_example_tb"
    obj_dir = work_dir / "obj"
    work_dir.mkdir(parents=True, exist_ok=True)
    patched_tb_subsystem = write_parameterless_tb_subsystem(
        tb_dir / "cv32e40p_tb_subsystem.sv",
        work_dir / "cv32e40p_tb_subsystem_naja.sv",
    )
    patched_mm_ram = tb_dir / "mm_ram.sv"
    patched_interrupt_generator = None
    if args.program == "interrupt":
        patched_mm_ram = write_verilator_mm_ram(
            tb_dir / "mm_ram.sv",
            work_dir / "mm_ram_naja.sv",
        )
        patched_interrupt_generator = write_verilator_interrupt_generator(
            work_dir / "cv32e40p_random_interrupt_generator_naja.sv",
        )

    env = os.environ.copy()
    tool_prefix = find_riscv_tool_prefix()
    infer_riscv_root(env, tool_prefix, work_dir)
    firmware = build_firmware(
        program_name=args.program,
        program=program,
        tb_dir=tb_dir,
        work_dir=work_dir,
        tool_prefix=tool_prefix,
        env=env,
    )
    if not firmware.exists():
        raise SystemExit(f"missing CV32E40P {args.program} firmware: {firmware}")

    package_sources = [
        rtl_include_dir / "cv32e40p_apu_core_pkg.sv",
        rtl_include_dir / "cv32e40p_fpu_pkg.sv",
        rtl_include_dir / "cv32e40p_pkg.sv",
    ]
    tb_sources = [
        tb_dir / "include" / "perturbation_pkg.sv",
        tb_dir / "riscv_gnt_stall.sv",
        tb_dir / "riscv_rvalid_stall.sv",
        tb_dir / "dp_ram.sv",
        patched_mm_ram,
        tb_dir / "amo_shim.sv",
        patched_tb_subsystem,
        tb_dir / "tb_top.sv",
    ]
    if patched_interrupt_generator is not None:
        tb_sources.insert(5, patched_interrupt_generator)
    core_include_dirs: list[str] = []
    if args.core_source == "rtl":
        core_include_dirs, core_sources = read_cv32e40p_manifest(repo_dir)
    else:
        core_sources = [generated_path, primitives_path]

    command = [
        "verilator",
        "--binary",
        "--timing",
        "--sv",
        "--top-module",
        "tb_top",
        "-Mdir",
        str(obj_dir),
        "+incdir+" + str(tb_dir / "include"),
        "+incdir+" + str(rtl_include_dir),
        *core_include_dirs,
        "-Wno-PINMISSING",
        "-Wno-TIMESCALEMOD",
        "-Wno-WIDTH",
        "-Wno-REALCVT",
        "-Wno-CASEINCOMPLETE",
        "-Wno-COMBDLY",
        "-Wno-UNOPTFLAT",
        *(str(source) for source in ([] if args.core_source == "rtl" else package_sources)),
        *(str(source) for source in core_sources),
        *(str(source) for source in tb_sources),
    ]
    run(command, cwd=repo_dir)

    executable = obj_dir / "Vtb_top"
    run([
        str(executable),
        f"+firmware={firmware}",
        f"+maxcycles={args.max_cycles}",
        *args.sim_plusarg,
    ], cwd=work_dir)
    print(program["pass_marker"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
