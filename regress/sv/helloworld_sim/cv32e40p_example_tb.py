#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Run CV32E40P example_tb/core firmware with a Naja-generated cv32e40p_top."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess

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
}


def run(args: list[str], *, cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    print("$ " + (" ".join(args) if cwd is None else f"(cd {cwd} && {' '.join(args)})"), flush=True)
    subprocess.run(args, cwd=str(cwd) if cwd else None, env=env, check=True)


def find_riscv_tool_prefix() -> str:
    for prefix in ("riscv32-unknown-elf-", "riscv64-unknown-elf-"):
        if shutil.which(prefix + "gcc") and shutil.which(prefix + "objcopy"):
            return prefix
    raise SystemExit(
        "missing RISC-V toolchain: expected riscv32-unknown-elf-{gcc,objcopy} "
        "or riscv64-unknown-elf-{gcc,objcopy}"
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


def infer_riscv_root(env: dict[str, str], tool_prefix: str, work_dir: Path) -> None:
    if env.get("RISCV"):
        return
    gcc = shutil.which(tool_prefix + "gcc")
    if not gcc:
        return
    toolchain_root = Path(gcc).resolve().parents[1]
    if tool_prefix == "riscv64-unknown-elf-":
        compat_root = work_dir / "riscv32-prefix-compat"
        compat_target = compat_root / "riscv32-unknown-elf"
        rv32_libc = Path(capture([
            gcc,
            "-march=rv32imc_zicsr",
            "-mabi=ilp32",
            "-print-file-name=libc.a",
        ])).resolve()
        rv32_lib = rv32_libc.parent
        target_root = toolchain_root / "riscv64-unknown-elf"
        include_dir = target_root / "include"
        if not rv32_libc.exists():
            raise SystemExit(f"missing RV32 multilib libc for CV32E40P hello_world: {rv32_libc}")
        if not include_dir.exists():
            raise SystemExit(f"missing RISC-V target include directory: {include_dir}")
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


def write_generated_netlist_tb_subsystem(source: Path, target: Path) -> Path:
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
    target.write_text(text.replace(parameterized_instance, plain_instance), encoding="utf-8")
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
    target.write_text(text.replace(guarded_instance, plain_instance), encoding="utf-8")
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
  logic [3:0] random_fast_id_q;

  function automatic logic [18:0] map_irq_word(input logic [31:0] irq_word);
    map_irq_word = {irq_word[3], irq_word[7], irq_word[11], irq_word[31:16]};
  endfunction

  assign irq_ack_o = irq_ack_i;
  assign irq_rnd_lines_o = irq_lines_q;
  assign irq_act_id_o = {28'd0, random_fast_id_q} + 32'd16;
  assign irq_id_we_o = 1'b0;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      irq_mode_q <= 32'd0;
      irq_lines_q <= '0;
      cycle_count_q <= 32'd0;
      random_fast_id_q <= 4'd0;
    end else begin
      irq_mode_q <= irq_mode_i;

      unique case (irq_mode_i)
        SOFTWARE_DEFINED: begin
          irq_lines_q <= map_irq_word(irq_lines_i);
          cycle_count_q <= 32'd0;
        end

        RANDOM: begin
          if (irq_mode_q != RANDOM || irq_lines_q != '0) begin
            irq_lines_q <= '0;
            cycle_count_q <= 32'd0;
          end else if (cycle_count_q >= irq_min_cycles_i) begin
            irq_lines_q <= {3'b0, (16'h1 << random_fast_id_q)};
            cycle_count_q <= 32'd0;
            random_fast_id_q <= random_fast_id_q + 4'd1;
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
    parser.add_argument("--program", choices=sorted(PROGRAMS), default="hello_world")
    parser.add_argument("--max-cycles", default="2000000")
    args = parser.parse_args()
    program = PROGRAMS[args.program]

    repo_dir = args.repo.resolve()
    artifacts_dir = args.artifacts.resolve()
    generated_path = args.generated.resolve()
    primitives_path = args.primitives.resolve()
    tb_dir = repo_dir / "example_tb" / "core"
    rtl_include_dir = repo_dir / "rtl" / "include"
    work_dir = artifacts_dir / f"{args.program}_sim" / "cv32e40p_example_tb"
    obj_dir = work_dir / "obj"
    work_dir.mkdir(parents=True, exist_ok=True)
    patched_tb_subsystem = write_generated_netlist_tb_subsystem(
        tb_dir / "cv32e40p_tb_subsystem.sv",
        work_dir / "cv32e40p_tb_subsystem_naja.sv",
    )
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
        patched_interrupt_generator,
        tb_dir / "amo_shim.sv",
        patched_tb_subsystem,
        tb_dir / "tb_top.sv",
    ]
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
        "-Wno-PINMISSING",
        "-Wno-TIMESCALEMOD",
        "-Wno-WIDTH",
        "-Wno-REALCVT",
        *(str(source) for source in package_sources),
        str(generated_path),
        str(primitives_path),
        *(str(source) for source in tb_sources),
    ]
    run(command, cwd=repo_dir)

    executable = obj_dir / "Vtb_top"
    run([str(executable), f"+firmware={firmware}", f"+maxcycles={args.max_cycles}"], cwd=work_dir)
    print(program["pass_marker"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
