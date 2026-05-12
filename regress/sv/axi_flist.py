#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Generate flists for pulp-platform/axi using Bender.

Requires bender to be installed (https://github.com/pulp-platform/bender).
On macOS: brew install bender   OR   cargo install bender

Produces two artefacts:
  --output          RTL flist for Naja's SystemVerilog loader
  --output/../axi_sim_sources.f  VIP sources + include dirs for Verilator sim
"""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def bender(args_list: list[str], *, cwd: Path) -> str:
    result = subprocess.run(
        ["bender"] + args_list,
        check=True,
        cwd=str(cwd),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return result.stdout


def bender_path(package: str, *, cwd: Path) -> Path:
    return Path(bender(["path", package], cwd=cwd).strip())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo",   type=Path, required=True,
                        help="Path to axi repo checkout")
    parser.add_argument("--output", type=Path, required=True,
                        help="Path for the generated RTL flist")
    args = parser.parse_args()

    args.output.parent.mkdir(parents=True, exist_ok=True)

    # ------------------------------------------------------------------ #
    # 1. Resolve all dependencies                                          #
    # ------------------------------------------------------------------ #
    print("bender update …", flush=True)
    subprocess.run(["bender", "update"], check=True, cwd=str(args.repo))

    # ------------------------------------------------------------------ #
    # 2. Resolve package paths (needed by both flists)                    #
    # ------------------------------------------------------------------ #
    print("Resolving package paths via bender …", flush=True)
    cc_path = bender_path("common_cells",        cwd=args.repo)
    cv_path = bender_path("common_verification", cwd=args.repo)

    # ------------------------------------------------------------------ #
    # 3. RTL flist for Naja's SV loader                                   #
    #                                                                      #
    # `bender script flist` lists source files but may omit +incdir+      #
    # entries for packages that use `include "pkg/file.svh"` style        #
    # headers.  Append the known include dirs explicitly.                 #
    # ------------------------------------------------------------------ #
    print("bender script flist (rtl) …", flush=True)
    rtl_content = bender(["script", "flist"], cwd=args.repo)
    # Append include dirs if bender didn't emit them.
    for incdir in (f"+incdir+{args.repo}/include", f"+incdir+{cc_path}/include"):
        if incdir not in rtl_content:
            rtl_content += incdir + "\n"
    # Append our elaboration wrapper (not part of the upstream axi repo).
    axi_cut_top = Path(__file__).parent / "axi_cut_top.sv"
    rtl_content += str(axi_cut_top) + "\n"
    args.output.write_text(rtl_content, encoding="utf-8")
    print(f"Wrote RTL flist: {args.output}", flush=True)

    # ------------------------------------------------------------------ #
    # 4. Sim sources file for Verilator (VIP files + include dirs)        #
    #                                                                      #
    # Uses `bender path` to get exact checkout locations so the file is   #
    # always consistent with the pinned Bender.lock versions.             #
    # ------------------------------------------------------------------ #

    sim_lines: list[str] = [
        # Include dirs needed by axi/typedef.svh and axi/assign.svh macros
        f"+incdir+{args.repo}/include",
        f"+incdir+{cc_path}/include",
        # axi_pkg.sv must be compiled: typedef.svh macros reference axi_pkg:: types
        str(args.repo / "src/axi_pkg.sv"),
    ]

    sim_flist = args.output.parent / "axi_sim_sources.f"
    sim_flist.write_text("\n".join(sim_lines) + "\n", encoding="utf-8")
    print(f"Wrote sim sources: {sim_flist}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
