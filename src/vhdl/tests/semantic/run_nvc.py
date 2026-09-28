#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Run the VHDL-2008 semantic probes against the pinned NVC reference."""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "manifest.json"
EXPECTATIONS = ROOT / "nvc_expectations.json"


def invoke(command, cwd, timeout):
    try:
        result = subprocess.run(
            command, cwd=cwd, capture_output=True, text=True, timeout=timeout,
            check=False,
        )
        return {"command": command, "returncode": result.returncode,
                "output": result.stdout + result.stderr, "timeout": False}
    except subprocess.TimeoutExpired:
        return {"command": command, "returncode": None,
                "output": "wall-clock timeout", "timeout": True}


def run_probe(nvc, probe, work, timeout, overrides):
    work.mkdir()
    library = probe.get("library", "work")
    options = ["--std=2008", f"--work={library}"]
    commands = [
        ("analyze", [nvc, *options, "-a",
                     *[str(ROOT / source) for source in probe["sources"]]]),
    ]
    if probe.get("profile") != "analysis_only":
        commands.extend([
            ("elaborate", [nvc, *options, "-e", probe["top"]]),
            ("run", [nvc, *options, "-r", probe["top"],
                     "--exit-severity=error", "--stop-time=1us",
                     "--stop-delta=1000"]),
        ])

    expected = probe.get("failure")
    expectation = overrides.get(probe["id"], expected)
    steps = []
    for stage, command in commands:
        result = invoke(command, work, timeout)
        result["stage"] = stage
        steps.append(result)
        if result["timeout"] or result["returncode"] is None or result["returncode"] < 0:
            return {"id": probe["id"], "passed": False, "steps": steps}
        if result["returncode"]:
            matched = bool(expected and result["returncode"] == 1 and
                           stage in expectation["stages"] and
                           re.search(expectation["diagnostic"], result["output"], re.I))
            return {"id": probe["id"], "passed": matched, "steps": steps}

    passed = bool(not expected and (
        probe.get("profile") == "analysis_only" or
        f"PROBE_PASS:{probe['id']}" in steps[-1]["output"]))
    return {"id": probe["id"], "passed": passed, "steps": steps}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nvc", default="nvc")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--case", action="append", default=[])
    parser.add_argument("--timeout", type=float, default=30)
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")

    manifest = json.loads(MANIFEST.read_text())
    expectations = json.loads(EXPECTATIONS.read_text())
    probes = manifest["probes"]
    known = {probe["id"] for probe in probes}
    if len(known) != len(probes):
        parser.error("duplicate probe identifiers")
    if set(args.case) - known:
        parser.error(f"unknown cases: {sorted(set(args.case) - known)}")
    if args.case:
        probes = [probe for probe in probes if probe["id"] in args.case]

    executable = shutil.which(args.nvc)
    if not executable:
        parser.error(f"NVC executable not found: {args.nvc}")
    executable = str(Path(executable).resolve())
    version = invoke([executable, "--version"], ROOT, args.timeout)
    if version["returncode"] != 0 or not re.search(
            expectations["version_pattern"], version["output"], re.M):
        parser.error(f"expected {expectations['reference']}; got {version['output']}")

    source_hashes = {
        source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest()
        for probe in probes for source in probe["sources"]
    }
    results = []
    with tempfile.TemporaryDirectory(prefix="vhdl-nvc-") as directory:
        for probe in probes:
            result = run_probe(executable, probe,
                               Path(directory) / probe["id"], args.timeout,
                               expectations.get("overrides", {}))
            results.append(result)
            print(f"{'PASS' if result['passed'] else 'FAIL'} {probe['id']}", flush=True)
            if not result["passed"]:
                print(result["steps"][-1]["output"])

    report = {
        "schema": 1,
        "reference": version["output"],
        "executable": executable,
        "standard": "2008",
        "manifest_sha256": hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),
        "expectations_sha256": hashlib.sha256(EXPECTATIONS.read_bytes()).hexdigest(),
        "source_sha256": source_hashes,
        "results": results,
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    passed = sum(result["passed"] for result in results)
    print(f"{passed}/{len(results)} passed; report: {args.report}")
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
