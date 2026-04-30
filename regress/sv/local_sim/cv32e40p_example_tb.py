#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Run CV32E40P example_tb/core hello_world with a Naja-generated cv32e40p_top."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess


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
        compat_root.mkdir(parents=True, exist_ok=True)
        if not compat_target.exists():
            compat_target.symlink_to(toolchain_root / "riscv64-unknown-elf", target_is_directory=True)
        env["RISCV"] = str(compat_root)
    else:
        env["RISCV"] = str(toolchain_root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--primitives", type=Path, required=True)
    parser.add_argument("--max-cycles", default="200000")
    args = parser.parse_args()

    repo_dir = args.repo.resolve()
    artifacts_dir = args.artifacts.resolve()
    generated_path = args.generated.resolve()
    primitives_path = args.primitives.resolve()
    tb_dir = repo_dir / "example_tb" / "core"
    work_dir = artifacts_dir / "local_sim" / "cv32e40p_example_tb"
    obj_dir = work_dir / "obj"
    work_dir.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    tool_prefix = find_riscv_tool_prefix()
    infer_riscv_root(env, tool_prefix, work_dir)
    run([
        "make",
        "custom/hello_world.hex",
        f"RISCV_EXE_PREFIX={tool_prefix}",
        "CUSTOM_GCC_FLAGS=-march=rv32imc_zicsr -mabi=ilp32",
    ], cwd=tb_dir, env=env)
    firmware = tb_dir / "custom" / "hello_world.hex"
    if not firmware.exists():
        raise SystemExit(f"missing CV32E40P hello_world firmware: {firmware}")

    tb_sources = [
        tb_dir / "include" / "perturbation_pkg.sv",
        tb_dir / "riscv_gnt_stall.sv",
        tb_dir / "riscv_rvalid_stall.sv",
        tb_dir / "dp_ram.sv",
        tb_dir / "mm_ram.sv",
        tb_dir / "cv32e40p_random_interrupt_generator.sv",
        tb_dir / "amo_shim.sv",
        tb_dir / "cv32e40p_tb_subsystem.sv",
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
        "-Wno-PINMISSING",
        "-Wno-TIMESCALEMOD",
        "-Wno-WIDTH",
        str(generated_path),
        str(primitives_path),
        *(str(source) for source in tb_sources),
    ]
    run(command, cwd=repo_dir)

    executable = obj_dir / "Vtb_top"
    run([str(executable), f"+firmware={firmware}", f"+maxcycles={args.max_cycles}"], cwd=work_dir)
    print("CV32E40P_LOCAL_SIM_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
