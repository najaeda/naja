#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Generate the combined SV flist for ariane_testharness (CVA6 full SoC simulation).

Uses the CVA6 repo's own util/flist_flattener.py to expand core/Flist.cva6,
then appends the testharness-specific sources and include dirs derived from
the Makefile's verilate_command.

Usage:
  python3 cva6_sim_flist.py --repo <cva6_repo> --output <flist_path>

Required environment (passed automatically by sv_regress.py via cases.yml env):
  CVA6_REPO_DIR   absolute path to the CVA6 repo checkout
  TARGET_CFG      e.g. cv64a6_imafdc_sv39
  HPDCACHE_DIR    e.g. $CVA6_REPO_DIR/core/cache_subsystem/hpdcache
"""

from __future__ import annotations

import argparse
import glob
import os
import subprocess
from pathlib import Path


def flatten_flist(repo: Path, flist: Path) -> list[str]:
    """Run util/flist_flattener.py and return one path per entry.

    flist_flattener.py defaults to space-separated output; pass --print_newline
    to get one entry per line so we can split correctly.
    """
    env = os.environ.copy()
    env.setdefault("CVA6_REPO_DIR", str(repo))
    env.setdefault("HPDCACHE_DIR", str(repo / "core/cache_subsystem/hpdcache"))
    env.setdefault("TARGET_CFG", "cv64a6_imafdc_sv39")
    result = subprocess.run(
        ["python3", str(repo / "util/flist_flattener.py"), "--print_newline"],
        stdin=flist.open("r", encoding="utf-8"),
        capture_output=True,
        text=True,
        env=env,
        check=True,
    )
    return [ln.strip() for ln in result.stdout.splitlines() if ln.strip()]


def glob_sv(repo: Path, pattern: str) -> list[str]:
    return sorted(str(p) for p in Path(repo).glob(pattern) if p.suffix == ".sv")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo",   type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo

    seen: set[str] = set()
    lines: list[str] = []

    def add(path: str) -> None:
        if path not in seen:
            seen.add(path)
            lines.append(path)

    # ------------------------------------------------------------------ #
    # 1. Include directories (prepended so Naja sees them first)          #
    # ------------------------------------------------------------------ #
    incdir_candidates = [
        f"+incdir+{repo}/core/include",
        f"+incdir+{repo}/core/cache_subsystem/hpdcache/rtl/include",
        f"+incdir+{repo}/vendor/pulp-platform/common_cells/include",
        f"+incdir+{repo}/vendor/pulp-platform/axi/include",
        f"+incdir+{repo}/corev_apu/register_interface/include",
        f"+incdir+{repo}/corev_apu/tb/common",
        f"+incdir+{repo}/verif/tb/core",
        # corev_apu/axi_node no longer exists in this repo version — skip silently
        f"+incdir+{repo}/corev_apu/axi_node",
        f"+incdir+{repo}/corev_apu/instr_tracing/ITI/include",
    ]
    for d in incdir_candidates:
        # Only emit +incdir+ entries for directories that actually exist.
        dir_path = Path(d.replace("+incdir+", "", 1))
        if dir_path.exists():
            lines.append(d)
        else:
            print(f"  skipping missing incdir: {d}", flush=True)

    # Use custom (lightweight) UVM macros instead of the full UVM library
    lines.append("+define+VERILATOR")


    # ------------------------------------------------------------------ #
    # 2. Core RTL — expanded from core/Flist.cva6                        #
    # ------------------------------------------------------------------ #
    # Files/directories excluded from the flattened core flist.
    core_flist_excludes = (
        # fpga-support: FPGA-specific RAM primitives, not needed for simulation;
        # some also lack a trailing newline which Slang rejects.
        "/fpga-support/",
        # CVXIF driver interface files have port dimensions that use
        # CVA6Cfg.NrRgprPorts / CVA6Cfg.NrIssuePorts.  In the
        # cv64a6_imafdc_sv39 configuration CVXIF is disabled, making those
        # values 0 (0/0 = X).  Slang emits a hard error for unknown-bit
        # port dimensions, so exclude these files.
        "cvxif_issue_register_commit_if_driver.sv",
        "cvxif_compressed_if_driver.sv",
    )

    print("Expanding core/Flist.cva6 via flist_flattener.py …", flush=True)
    for entry in flatten_flist(repo, repo / "core/Flist.cva6"):
        if any(excl in entry for excl in core_flist_excludes):
            continue
        add(entry)

    # ------------------------------------------------------------------ #
    # 3. ariane_pkg files — SoC interface packages                       #
    # ------------------------------------------------------------------ #
    ariane_pkg = [
        "corev_apu/tb/ariane_axi_pkg.sv",
        "corev_apu/tb/axi_intf.sv",
        "corev_apu/register_interface/src/reg_intf.sv",
        "corev_apu/tb/ariane_soc_pkg.sv",
        "corev_apu/riscv-dbg/src/dm_pkg.sv",
        "corev_apu/tb/ariane_axi_soc_pkg.sv",
    ]
    for rel in ariane_pkg:
        p = str(repo / rel)
        if Path(p).exists():
            add(p)
        else:
            print(f"  WARNING: ariane_pkg file not found: {p}", flush=True)

    # ------------------------------------------------------------------ #
    # 4. SoC sources — src variable from Makefile (non-wildcard)         #
    # ------------------------------------------------------------------ #
    # ariane.sv is the synthesizable DUT top.  It wraps the CVA6 core and
    # exposes an AXI interface — it does NOT include any testbench components.
    # The simulation testbench (ariane_testharness) stays unmodified and wraps
    # ariane during the simulation step.
    src_explicit = [
        "corev_apu/src/ariane.sv",
        "corev_apu/rv_plic/rtl/rv_plic_target.sv",
        "corev_apu/rv_plic/rtl/rv_plic_gateway.sv",
        "corev_apu/rv_plic/rtl/plic_regmap.sv",
        "corev_apu/rv_plic/rtl/plic_top.sv",
        "corev_apu/riscv-dbg/debug_rom/debug_rom.sv",
        "corev_apu/register_interface/src/apb_to_reg.sv",
        "vendor/pulp-platform/axi/src/axi_multicut.sv",
        "vendor/pulp-platform/common_cells/src/rstgen_bypass.sv",
        "vendor/pulp-platform/common_cells/src/rstgen.sv",
        "vendor/pulp-platform/common_cells/src/addr_decode.sv",
        "vendor/pulp-platform/common_cells/src/stream_register.sv",
        "vendor/pulp-platform/axi/src/axi_cut.sv",
        "vendor/pulp-platform/axi/src/axi_join.sv",
        "vendor/pulp-platform/axi/src/axi_delayer.sv",
        "vendor/pulp-platform/axi/src/axi_to_axi_lite.sv",
        "vendor/pulp-platform/axi/src/axi_id_prepend.sv",
        "vendor/pulp-platform/axi/src/axi_atop_filter.sv",
        "vendor/pulp-platform/axi/src/axi_err_slv.sv",
        "vendor/pulp-platform/axi/src/axi_mux.sv",
        "vendor/pulp-platform/axi/src/axi_demux.sv",
        "vendor/pulp-platform/axi/src/axi_xbar.sv",
        "vendor/pulp-platform/common_cells/src/cdc_2phase.sv",
        "vendor/pulp-platform/common_cells/src/spill_register_flushable.sv",
        "vendor/pulp-platform/common_cells/src/spill_register.sv",
        "vendor/pulp-platform/common_cells/src/deprecated/fifo_v1.sv",
        "vendor/pulp-platform/common_cells/src/deprecated/fifo_v2.sv",
        "vendor/pulp-platform/common_cells/src/stream_delay.sv",
        "vendor/pulp-platform/common_cells/src/lfsr_16bit.sv",
        "vendor/pulp-platform/tech_cells_generic/src/deprecated/cluster_clk_cells.sv",
        "vendor/pulp-platform/tech_cells_generic/src/deprecated/pulp_clk_cells.sv",
        "vendor/pulp-platform/tech_cells_generic/src/rtl/tc_clk.sv",
        # Instruction tracing infrastructure (packages used by rvfi_tracer in
        # the testbench — only needed when compiling ariane_testharness, but
        # ariane.sv itself imports te_pkg so it must be included here).
        "corev_apu/instr_tracing/ITI/include/iti_pkg.sv",
        "corev_apu/instr_tracing/rv_tracer-main/include/te_pkg.sv",
        "corev_apu/instr_tracing/rv_encapsulator-main/src/include/encap_pkg.sv",
        # NOTE: ariane_testharness.sv, rvfi_tracer.sv, SimDTM.sv, SimJTAG.sv
        # and ariane_peripherals.sv are the VERIFICATION ENVIRONMENT —
        # they are NOT part of the synthesizable DUT.  They stay unmodified
        # and wrap the Naja-generated ariane netlist during the simulation step.
        # Do NOT add them here.
        # Instruction tracing modules
        "corev_apu/instr_tracing/ITI/cva6_iti/iti.sv",
        "corev_apu/instr_tracing/ITI/cva6_iti/block_retirement.sv",
        "corev_apu/instr_tracing/ITI/cva6_iti/single_retirement.sv",
        "corev_apu/instr_tracing/ITI/cva6_iti/itype_detector.sv",
        "vendor/pulp-platform/common_cells/src/counter.sv",
        "vendor/pulp-platform/common_cells/src/sync.sv",
        "vendor/pulp-platform/common_cells/src/sync_wedge.sv",
        "vendor/pulp-platform/common_cells/src/edge_detect.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/lzc.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/te_branch_map.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/te_filter.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/te_packet_emitter.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/te_priority.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/te_reg.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/te_resync_counter.sv",
        "corev_apu/instr_tracing/rv_tracer-main/rtl/rv_tracer.sv",
        "vendor/pulp-platform/common_cells/src/fifo_v3.sv",
        "corev_apu/instr_tracing/DPTI/slicer_DPTI.sv",
        "corev_apu/instr_tracing/rv_encapsulator-main/src/rtl/encapsulator.sv",
    ]
    # Files excluded by the Makefile filter
    excluded_patterns = {"fpu_wrap.sv", "_config_pkg.sv"}

    for rel in src_explicit:
        if any(excl in Path(rel).name for excl in excluded_patterns):
            continue
        p = str(repo / rel)
        if Path(p).exists():
            add(p)

    # Wildcard patterns from $(src)
    src_wildcards = [
        "corev_apu/bootrom/*.sv",
        "corev_apu/clint/*.sv",
        "corev_apu/fpga/src/axi2apb/src/*.sv",
        "corev_apu/fpga/src/apb_timer/*.sv",
        "corev_apu/fpga/src/axi_slice/src/*.sv",
        "vendor/pulp-platform/axi_riscv_atomics/src/*.sv",
        "corev_apu/axi_mem_if/src/*.sv",
        "corev_apu/riscv-dbg/src/*.sv",
    ]
    for pattern in src_wildcards:
        for p in glob_sv(repo, pattern):
            name = Path(p).name
            if any(excl in name for excl in excluded_patterns):
                continue
            add(p)

    # mock_uart — added explicitly in Makefile after src
    mock_uart = str(repo / "corev_apu/tb/common/mock_uart.sv")
    if Path(mock_uart).exists():
        add(mock_uart)

    # ------------------------------------------------------------------ #
    # 5. Write output                                                      #
    # ------------------------------------------------------------------ #
    args.output.parent.mkdir(parents=True, exist_ok=True)
    sv_count = sum(1 for l in lines if l.endswith(".sv") or l.endswith(".v"))
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(
        f"Wrote flist: {args.output}  ({sv_count} SV/V files, "
        f"{len(lines) - sv_count} directives)",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
