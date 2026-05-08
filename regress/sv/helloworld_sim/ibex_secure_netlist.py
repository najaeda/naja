#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Generate parameterized Ibex Naja netlists for simple-system diagnostics."""

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
PMP_PASS_MARKER = "IBEX_PMP_NETLIST_PASS"

VARIANTS = {
    "secure": {
        "params": ["SecureIbex=1"],
        "default_output": DEFAULT_OUTPUT_NAME,
        "flist": "ibex_secure.flist",
        "diagnostics": "ibex_secure_diagnostics.log",
        "pass_marker": SECURE_PASS_MARKER,
        "required_text": [
            "module ibex_dummy_instr",
            "module ibex_lockstep",
        ],
        "forbidden_text": [
            "gen_no_dummy_instr",
            "dummy_instr_id_o = 1'b0",
        ],
    },
    "pmp": {
        "params": ["PMPEnable=1"],
        "default_output": "ibex_pmp_naja.v",
        "flist": "ibex_pmp.flist",
        "diagnostics": "ibex_pmp_diagnostics.log",
        "pass_marker": PMP_PASS_MARKER,
        "required_text": [
            "module ibex_pmp",
        ],
        "forbidden_text": [],
    },
}

sys.path.insert(0, str(REGRESS_SV_ROOT))
import sv_regress  # noqa: E402


def write_if_changed(path: Path, content: str) -> None:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def variant_flist_content(vc_path: Path, params: list[str]) -> str:
    parameter_lines = "".join(f"-G{param}\n" for param in params)
    return parameter_lines + sv_regress.normalize_fusesoc_vc(vc_path)


def secure_flist_content(vc_path: Path) -> str:
    return variant_flist_content(vc_path, VARIANTS["secure"]["params"])


def netlist_is_current(output_path: Path, dependencies: list[Path]) -> bool:
    if not output_path.exists():
        return False
    output_mtime = output_path.stat().st_mtime
    return all(not dependency.exists() or dependency.stat().st_mtime <= output_mtime
               for dependency in dependencies)


def check_variant_netlist(output_path: Path, variant: dict[str, object]) -> None:
    text = output_path.read_text(encoding="utf-8", errors="replace")
    for forbidden in variant["forbidden_text"]:
        if forbidden in text:
            raise SystemExit(
                f"{output_path} does not look like the requested Ibex elaboration: "
                f"found forbidden text {forbidden!r}"
            )
    for required in variant["required_text"]:
        if required not in text:
            raise SystemExit(
                f"{output_path} does not look like the requested Ibex elaboration: "
                f"missing required text {required!r}"
            )


def check_secure_netlist(output_path: Path) -> None:
    try:
        check_variant_netlist(output_path, VARIANTS["secure"])
    except SystemExit as exc:
        raise SystemExit(
            f"{output_path} does not look like a SecureIbex elaboration: {exc}"
        ) from exc


def generate_variant_netlist(
    *,
    repo_dir: Path,
    artifacts_dir: Path,
    najaeda_path: Path,
    output_path: Path,
    variant_name: str,
) -> None:
    if not najaeda_path.exists():
        raise SystemExit(
            f"missing built najaeda package path: {najaeda_path}; "
            "build target 'naja' and prepare build/test/najaeda first"
        )
    variant = VARIANTS[variant_name]
    vc_path = next((repo_dir / "build" / "fusesoc").glob("**/lint-verilator/*.vc"), None)
    if vc_path is None:
        raise SystemExit(
            f"missing Ibex fusesoc lint-verilator command file under {repo_dir / 'build' / 'fusesoc'}"
        )

    flist_path = artifacts_dir / str(variant["flist"])
    diagnostics_path = artifacts_dir / str(variant["diagnostics"])
    write_if_changed(flist_path, variant_flist_content(vc_path, variant["params"]))

    constructor_source = (
        REPO_ROOT / "src" / "nl" / "formats" / "systemverilog" / "frontend" /
        "SNLSVConstructor.cpp"
    )
    dependencies = [flist_path, constructor_source, Path(__file__)]
    if netlist_is_current(output_path, dependencies):
        check_variant_netlist(output_path, variant)
        print(f"{variant['pass_marker']} {output_path}")
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

    check_variant_netlist(output_path, variant)
    print(f"{variant['pass_marker']} {output_path}")


def generate_secure_netlist(
    *,
    repo_dir: Path,
    artifacts_dir: Path,
    najaeda_path: Path,
    output_path: Path,
) -> None:
    generate_variant_netlist(
        repo_dir=repo_dir,
        artifacts_dir=artifacts_dir,
        najaeda_path=najaeda_path,
        output_path=output_path,
        variant_name="secure",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--najaeda-pythonpath", type=Path, default=DEFAULT_NAJAEDA_PATH)
    parser.add_argument("--variant", choices=sorted(VARIANTS), default="secure")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    artifacts_dir = args.artifacts.resolve()
    variant = VARIANTS[args.variant]
    output_path = (
        args.output.resolve() if args.output else artifacts_dir / str(variant["default_output"])
    )
    generate_variant_netlist(
        repo_dir=args.repo.resolve(),
        artifacts_dir=artifacts_dir,
        najaeda_path=args.najaeda_pythonpath.resolve(),
        output_path=output_path,
        variant_name=args.variant,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
