#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from najaeda import netlist


BENCHMARKS = (
    ("simple/simple.sv", "simple/simple_naja.v"),
    ("up_counter/up_counter.sv", "up_counter/up_counter_naja.v"),
    ("param_inst/param_inst.sv", "param_inst/param_inst_naja.v"),
    ("binary_ops_supported/binary_ops_supported.sv", "binary_ops_supported/binary_ops_supported_naja.v"),
    ("byte_ports/byte_ports.sv", "byte_ports/byte_ports_naja.v"),
)


def generate_one(benchmarks_dir: Path, sv_rel_path: str, generated_rel_path: str) -> None:
    sv_path = benchmarks_dir / sv_rel_path
    generated_path = benchmarks_dir / generated_rel_path
    if not sv_path.exists():
        raise FileNotFoundError(f"Missing benchmark input file: {sv_path}")

    netlist.reset()
    top = netlist.load_systemverilog(str(sv_path))
    top.dump_verilog(str(generated_path))
    netlist.reset()

    if not generated_path.exists():
        raise RuntimeError(f"Failed to generate Verilog file: {generated_path}")
    print(f"Generated: {generated_path}")


def main() -> None:
    benchmarks_dir = Path(__file__).resolve().parent
    for sv_rel_path, generated_rel_path in BENCHMARKS:
        generate_one(benchmarks_dir, sv_rel_path, generated_rel_path)
    print("SystemVerilog roundtrip generation completed")


if __name__ == "__main__":
    main()
