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
    ("implicit_width_ports/implicit_width_ports.sv",
     "implicit_width_ports/implicit_width_ports_naja.v"),
    ("port_directions/port_directions.sv", "port_directions/port_directions_naja.v"),
    ("non_ansi_ports/non_ansi_ports.sv", "non_ansi_ports/non_ansi_ports_naja.v"),
    ("model_reuse/model_reuse.sv", "model_reuse/model_reuse_naja.v"),
    ("unary_not/unary_not.sv", "unary_not/unary_not_naja.v"),
    ("unary_not_binary_supported/unary_not_binary_supported.sv",
     "unary_not_binary_supported/unary_not_binary_supported_naja.v"),
    ("seq_add_out_plus_one/seq_add_out_plus_one.sv", "seq_add_out_plus_one/seq_add_out_plus_one_naja.v"),
    ("seq_add_one_plus_out/seq_add_one_plus_out.sv", "seq_add_one_plus_out/seq_add_one_plus_out_naja.v"),
    ("seq_enable_else_default/seq_enable_else_default.sv",
     "seq_enable_else_default/seq_enable_else_default_naja.v"),
    ("seq_enable_bus1_supported/seq_enable_bus1_supported.sv",
     "seq_enable_bus1_supported/seq_enable_bus1_supported_naja.v"),
    ("seq_preincrement_supported/seq_preincrement_supported.sv",
     "seq_preincrement_supported/seq_preincrement_supported_naja.v"),
    ("seq_reset_only_supported/seq_reset_only_supported.sv",
     "seq_reset_only_supported/seq_reset_only_supported_naja.v"),
    ("seq_list_single_wrapper_supported/seq_list_single_wrapper_supported.sv",
     "seq_list_single_wrapper_supported/seq_list_single_wrapper_supported_naja.v"),
    ("seq_nested_begin_wrapper_supported/seq_nested_begin_wrapper_supported.sv",
     "seq_nested_begin_wrapper_supported/seq_nested_begin_wrapper_supported_naja.v"),
    ("seq_rhs_direct_match/seq_rhs_direct_match.sv", "seq_rhs_direct_match/seq_rhs_direct_match_naja.v"),
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
