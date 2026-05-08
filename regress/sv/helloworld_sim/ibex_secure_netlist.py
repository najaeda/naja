#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Generate a SecureIbex=1 Naja netlist for Ibex simple-system diagnostics."""

from __future__ import annotations

import argparse
import logging
from pathlib import Path
import sys


REPO_ROOT = Path(__file__).resolve().parents[3]
REGRESS_SV_ROOT = REPO_ROOT / "regress" / "sv"
DEFAULT_NAJAEDA_PATH = REPO_ROOT / "build" / "test" / "najaeda"
DEFAULT_OUTPUT_NAME = "ibex_secure_naja.v"
SECURE_PASS_MARKER = "IBEX_SECURE_NETLIST_PASS"

sys.path.insert(0, str(REGRESS_SV_ROOT))
import sv_regress  # noqa: E402


def write_if_changed(path: Path, content: str) -> None:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def secure_flist_content(vc_path: Path) -> str:
    return "-GSecureIbex=1\n" + sv_regress.normalize_fusesoc_vc(vc_path)


def secure_netlist_is_current(output_path: Path, dependencies: list[Path]) -> bool:
    if not output_path.exists():
        return False
    output_mtime = output_path.stat().st_mtime
    return all(not dependency.exists() or dependency.stat().st_mtime <= output_mtime
               for dependency in dependencies)


def check_secure_netlist(output_path: Path) -> None:
    text = output_path.read_text(encoding="utf-8", errors="replace")
    if "gen_no_dummy_instr" in text or "dummy_instr_id_o = 1'b0" in text:
        raise SystemExit(
            f"{output_path} does not look like a SecureIbex elaboration: "
            "dummy instruction insertion is still tied off"
        )
    if "module ibex_dummy_instr" not in text:
        raise SystemExit(
            f"{output_path} does not contain the SecureIbex dummy instruction module"
        )
    if "module ibex_lockstep" not in text:
        raise SystemExit(f"{output_path} does not contain the SecureIbex lockstep module")


def generate_secure_netlist(
    *,
    repo_dir: Path,
    artifacts_dir: Path,
    najaeda_path: Path,
    output_path: Path,
) -> None:
    if not najaeda_path.exists():
        raise SystemExit(
            f"missing built najaeda package path: {najaeda_path}; "
            "build target 'naja' and prepare build/test/najaeda first"
        )
    vc_path = next((repo_dir / "build" / "fusesoc").glob("**/lint-verilator/*.vc"), None)
    if vc_path is None:
        raise SystemExit(
            f"missing Ibex fusesoc lint-verilator command file under {repo_dir / 'build' / 'fusesoc'}"
        )

    flist_path = artifacts_dir / "ibex_secure.flist"
    diagnostics_path = artifacts_dir / "ibex_secure_diagnostics.log"
    write_if_changed(flist_path, secure_flist_content(vc_path))

    constructor_source = (
        REPO_ROOT / "src" / "nl" / "formats" / "systemverilog" / "frontend" /
        "SNLSVConstructor.cpp"
    )
    dependencies = [flist_path, constructor_source, Path(__file__)]
    if secure_netlist_is_current(output_path, dependencies):
        check_secure_netlist(output_path)
        print(f"{SECURE_PASS_MARKER} {output_path}")
        return

    sys.path.insert(0, str(najaeda_path))
    from najaeda import netlist  # pylint: disable=import-outside-toplevel

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        force=True,
    )
    netlist.reset()
    svconfig = netlist.SystemVerilogConfig(
        top="ibex_top",
        flist=str(flist_path),
        diagnostics_report_path=str(diagnostics_path),
    )
    top = netlist.load_system_verilog([], config=svconfig)
    dump_config = netlist.VerilogDumpConfig()
    dump_config.dumpRTLInfosAsAttributes = True
    dump_config.dumpAssignsAsInstances = False
    top.dump_verilog(str(output_path), config=dump_config)
    netlist.reset()

    check_secure_netlist(output_path)
    print(f"{SECURE_PASS_MARKER} {output_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--najaeda-pythonpath", type=Path, default=DEFAULT_NAJAEDA_PATH)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    artifacts_dir = args.artifacts.resolve()
    output_path = (
        args.output.resolve() if args.output else artifacts_dir / DEFAULT_OUTPUT_NAME
    )
    generate_secure_netlist(
        repo_dir=args.repo.resolve(),
        artifacts_dir=artifacts_dir,
        najaeda_path=args.najaeda_pythonpath.resolve(),
        output_path=output_path,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
