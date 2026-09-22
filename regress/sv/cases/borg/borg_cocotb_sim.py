#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Run pinned Borg pin-level cocotb tests against ONLY Naja's structural dump."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from sv_regress import RegressError, run_command

TESTS = {
    "core": ("test", "test_start"),
    "math": ("user_peripherals.borg.test", "test_borg_shader_math_batch"),
}
PASS_MARKER = "BORG_COCOTB_SIM_PASS"


def build_command(repo: Path, artifacts: Path, generated: Path,
                  primitives: Path) -> list[str]:
    sources = [generated.resolve(), primitives.resolve(), (repo / "test/soc/tb.v").resolve()]
    if len(set(sources)) != 3:
        raise RegressError("Borg simulation requires three distinct sources")
    if sources[0].is_relative_to(repo.resolve()):
        raise RegressError("Borg simulation generated file must not come from original checkout")
    for source in sources:
        if not source.is_file():
            raise RegressError(f"Missing Borg simulation source: {source}")
    return ["iverilog", "-g2012", "-s", "tb", "-DSIM", "-f",
            str(artifacts / "borg-cocotb-sim" / "precision.f"), "-o",
            str(artifacts / "borg-cocotb-sim" / "sim.vvp"), *map(str, sources)]


def check_results(path: Path, module: str, test: str) -> dict[str, str]:
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise RegressError(f"Missing/invalid Borg cocotb results: {path}") from exc
    cases = list(root.iter("testcase"))
    if not cases or any(list(root.iter(tag)) for tag in ("failure", "error")):
        raise RegressError(f"Borg cocotb results report failure, error, or no tests: {path}")
    selected = [case for case in cases if case.get("classname") == module and case.get("name") == test]
    if len(selected) != 1 or selected[0].find("skipped") is not None:
        raise RegressError(f"Borg cocotb did not pass selected test {module}.{test}: {path}")
    # Cocotb 2 writes filtered-out tests as skipped testcase entries too.
    # Only these unselected entries may skip; never the required selected test.
    excluded = [case for case in cases if case is not selected[0]]
    if any(case.get("classname") != module or case.find("skipped") is None for case in excluded):
        raise RegressError(f"Borg cocotb ran unexpected tests instead of exactly {module}.{test}: {path}")
    for suite in root.iter():
        for field in ("failures", "errors"):
            if field in suite.attrib and suite.attrib[field] not in {"0", "0.0"}:
                raise RegressError(f"Borg cocotb results report {field}: {path}")
        if "skipped" in suite.attrib:
            try:
                skipped = int(suite.attrib["skipped"])
            except ValueError as exc:
                raise RegressError(f"Invalid Borg skipped-test count: {path}") from exc
            if skipped != len(list(suite.iter("skipped"))):
                raise RegressError(f"Borg cocotb results report unexplained skipped tests: {path}")
    return {"module": module, "test": test, "status": "passed", "results": str(path)}


def simulate(repo: Path, artifacts: Path, generated: Path, primitives: Path) -> None:
    repo, artifacts = repo.resolve(), artifacts.resolve()
    sim_dir = artifacts / "borg-cocotb-sim"
    sim_dir.mkdir(parents=True, exist_ok=True)
    # Naja's dump has no timescale. Upstream tb.v is 1ns/100ps, while its
    # Python helpers request 2ps. Give the dumped modules 1ps precision,
    # restoring a global simulator resolution that can represent that delay.
    (sim_dir / "precision.f").write_text("+timescale+1ns/1ps\n", encoding="utf-8")
    (artifacts / "borg-cocotb-sim-results.json").unlink(missing_ok=True)
    log_dir = artifacts / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    command = build_command(repo, artifacts, generated, primitives)
    (artifacts / "borg-cocotb-sim-build-command.json").write_text(
        json.dumps({"argv": command, "cwd": str(sim_dir)}, indent=2) + "\n", encoding="utf-8")
    run_command(command, cwd=sim_dir, timeout=1800,
                log_path=log_dir / "borg-cocotb-build.log")

    def config(*args: str) -> str:
        return subprocess.check_output(["cocotb-config", *args], text=True).strip()

    env = dict(os.environ)
    # Do not inherit a stale test selector, PDK mode, or another Python ABI.
    for key in ("COCOTB_TESTCASE", "TESTCASE", "MODULE", "GATES", "GPI_EXTRA"):
        env.pop(key, None)
    env.update({
        "COCOTB_TOPLEVEL": "tb", "TOPLEVEL_LANG": "verilog", "CLOCK_MHZ": "4",
        "PYGPI_PYTHON_BIN": config("--python-bin"),
        "LIBPYTHON_LOC": config("--libpython"),
        "PYTHONPATH": os.pathsep.join(map(str, [repo / "test/soc",
            artifacts / "rtl-work/out/hardware/borg/rdl", repo])),
        "COCOTB_RANDOM_SEED": "1",
    })
    # Preserve upstream LUT staging (current ASIC LUTs are combinational).
    for name in ("rcp_lut.hex", "frsq_lut.hex", "srgb_lut.hex"):
        shutil.copy2(repo / "hardware/borg/src" / name, sim_dir / name)
    run = ["vvp", "-M", config("--lib-dir"), "-m", config("--lib-name", "vpi", "icarus"),
           str(sim_dir / "sim.vvp"), "-none"]
    results = []
    for tier, (module, test) in TESTS.items():
        results_file = sim_dir / f"{tier}-results.xml"
        # Never allow an earlier successful run to satisfy this run.
        results_file.unlink(missing_ok=True)
        env.update({"COCOTB_TEST_MODULES": module,
                    "COCOTB_TEST_FILTER": f"^{re.escape(module)}\\.{re.escape(test)}$",
                    "COCOTB_RESULTS_FILE": str(results_file)})
        (artifacts / f"borg-cocotb-sim-{tier}-run-command.json").write_text(
            json.dumps({"argv": run, "cwd": str(sim_dir), "env": {
                key: value for key, value in env.items() if key.startswith("COCOTB_") or
                key in {"CLOCK_MHZ", "PYTHONPATH", "PYGPI_PYTHON_BIN", "LIBPYTHON_LOC"}
            }}, indent=2) + "\n", encoding="utf-8")
        run_command(run, cwd=sim_dir, env=env, timeout=1800,
                    log_path=log_dir / f"borg-cocotb-{tier}.log")
        results.append(check_results(results_file, module, test))
    (artifacts / "borg-cocotb-sim-results.json").write_text(
        json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(PASS_MARKER, flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repo", "artifacts", "generated", "primitives"):
        parser.add_argument(f"--{name}", type=Path, required=True)
    args = parser.parse_args()
    try:
        simulate(args.repo, args.artifacts, args.generated, args.primitives)
    except (OSError, subprocess.CalledProcessError, RegressError) as exc:
        parser.exit(1, f"Borg cocotb simulation failed: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
