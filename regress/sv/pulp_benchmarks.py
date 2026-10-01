#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Load/dump the pinned, prepared PULP EDA benchmark release in isolated processes."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
VERSION = "0.1.0"
PACKAGE = f"pulp-benchmarks-v{VERSION}"
URL = f"https://github.com/pulp-platform/eda-benchmarks/releases/download/v{VERSION}/{PACKAGE}.tar.gz"
SHA256 = "a27eaca9359cd2c0b646892325355e102ae563bd8f772c77f18697ff8c4ad603"
# Keep this small enough for pull requests; the complete survey is opt-in.
SMOKE = ("cv32e40p.default", "cvfpu.fp32_fma", "ibex.minimal", "spi_host.default")


def unpack(archive, destination):
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise ValueError(f"SHA-256 mismatch for {archive}; expected {SHA256}")
    with tarfile.open(archive) as source:
        source.extractall(destination, filter="data")
    return destination / PACKAGE


def targets(package):
    pairs = re.findall(r"^([\w]+\.[\w]+)\.TOP := (\w+)\s*$",
                       (package / "targets.mk").read_text(), re.MULTILINE)
    if not pairs or len(dict(pairs)) != len(pairs):
        raise ValueError("Missing or duplicate benchmark tops in targets.mk")
    return dict(pairs)


def validate_diagnostics(diagnostics):
    if ("summary.status=success" not in diagnostics
            or 'severity="error"' in diagnostics
            or 'severity="fatal"' in diagnostics
            or 'stage="naja_unsupported_' in diagnostics):
        raise RuntimeError("Incomplete or unsupported lowering; see diagnostics.log")


def worker(top_name, artifacts, dump_netlist):
    from najaeda import netlist

    start = time.monotonic()
    top = netlist.load_system_verilog([], config=netlist.SystemVerilogConfig(
        top=top_name, flist="filelist.f",
        diagnostics_report_path=str(artifacts / "diagnostics.log"),
        blackbox_unknown_modules=False,
    ))
    loaded = time.monotonic()
    if top.get_name() != top_name or not top.count_terms():
        raise RuntimeError("Missing or unexpected top-level interface")
    validate_diagnostics((artifacts / "diagnostics.log").read_text())
    stats = dict(top=top.get_name(), top_terms=top.count_terms(),
                 top_nets=top.count_nets(), top_instances=top.count_child_instances(),
                 load_seconds=loaded - start)
    if dump_netlist:
        top.dump_verilog(str(artifacts / "netlist.v"))
        stats["dump_seconds"] = time.monotonic() - loaded
    (artifacts / "design-stats.json").write_text(json.dumps(stats, indent=2) + "\n")
    netlist.reset()


def run_target(package, name, top, artifacts, najaeda_path, timeout, lint_runner=None,
               dump_netlist=True):
    artifacts.mkdir(parents=True, exist_ok=True)
    # Never let stale success products from an earlier run satisfy this run.
    for filename in ("design-stats.json", "netlist.v", "diagnostics.log", "lint.log", "summary.json"):
        (artifacts / filename).unlink(missing_ok=True)
    design, variant = name.split(".")
    source_dir = package / design / "variants" / variant
    source_list = (source_dir / "filelist.f").read_text()
    (artifacts / "filelist.f").write_text(source_list)
    command = [sys.executable, str(Path(__file__).resolve()), "--worker", top,
               "--output", str(artifacts)]
    if not dump_netlist:
        command.append("--elaboration-only")
    env = dict(os.environ, PYTHONPATH=str(najaeda_path))
    record = dict(target=name, top=top, command=command, cwd=str(source_dir),
                  source_count=sum(line.strip().endswith((".v", ".sv"))
                                   for line in source_list.splitlines()))
    start = time.monotonic()
    with (artifacts / "load-dump.log").open("w") as log:
        try:
            result = subprocess.run(command, cwd=source_dir, env=env, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=timeout)
            record["returncode"] = result.returncode
            record["status"] = "passed" if result.returncode == 0 else "failed"
        except subprocess.TimeoutExpired:
            record["status"] = "timeout"
    record["seconds"] = round(time.monotonic() - start, 3)
    if record["status"] == "passed":
        stats = artifacts / "design-stats.json"
        dump = artifacts / "netlist.v"
        if not stats.is_file() or (dump_netlist and (not dump.is_file() or dump.stat().st_size == 0)):
            record["status"] = "failed"
            record["error"] = "Missing load/dump products"
        else:
            record["design_stats"] = json.loads(stats.read_text())
            if dump_netlist:
                record["dump_bytes"] = dump.stat().st_size
    if record["status"] == "passed" and lint_runner and dump_netlist:
        primitives = ROOT / "test/nl/formats/systemverilog/benchmarks/najaeda_primitives.v"
        flags = ["--lint-only", "--sv", "--top-module", top, "-Wno-ASCRANGE"]
        if lint_runner == "docker":
            cidfile = artifacts / "lint-container.cid"
            cidfile.unlink(missing_ok=True)
            command = ["docker", "run", "--rm", "--cidfile", str(cidfile), "-v", f"{artifacts}:/work:ro",
                       "-v", f"{primitives}:/primitives.v:ro", "--entrypoint", "verilator",
                       "verilator/verilator:v5.046", *flags, "/work/netlist.v", "/primitives.v"]
        else:
            command = ["verilator", *flags, str(artifacts / "netlist.v"), str(primitives)]
        record["lint_command"] = command
        start = time.monotonic()
        with (artifacts / "lint.log").open("w") as log:
            try:
                result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                        timeout=timeout)
                record["lint_returncode"] = result.returncode
                if result.returncode:
                    record["status"] = "lint_failed"
            except subprocess.TimeoutExpired:
                record["status"] = "lint_timeout"
            except OSError as error:
                record["status"] = "lint_failed"
                record["error"] = str(error)
            finally:
                if lint_runner == "docker" and cidfile.exists():
                    try:
                        subprocess.run(["docker", "rm", "-f", cidfile.read_text().strip()],
                                       stdout=log, stderr=subprocess.STDOUT, timeout=30)
                    except (OSError, subprocess.TimeoutExpired) as error:
                        record["status"] = "lint_failed"
                        record["error"] = f"Container cleanup failed: {error}"
        record["lint_seconds"] = round(time.monotonic() - start, 3)
    (artifacts / "summary.json").write_text(json.dumps(record, indent=2) + "\n")
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, help="Use a local release archive (checksum still enforced)")
    parser.add_argument("--output", type=Path, default=ROOT / "build/pulp-benchmarks")
    parser.add_argument("--najaeda-path", type=Path, default=ROOT / "build/test/najaeda")
    parser.add_argument("--case", action="append", help="Release target, repeatable; defaults to smoke selection")
    parser.add_argument("--all", action="store_true", help="Survey every release target; failures remain failures")
    parser.add_argument("--timeout", type=int, default=120, help="Per-target wall-clock limit in seconds")
    parser.add_argument("--lint-runner", choices=("local", "docker"),
                        help="Also lint each complete dump with Naja primitive models")
    parser.add_argument("--elaboration-only", action="store_true",
                        help="Load and validate the design without structural Verilog dumping")
    parser.add_argument("--worker", help=argparse.SUPPRESS)
    args = parser.parse_args()
    args.output = args.output.resolve()
    if args.worker:
        worker(args.worker, args.output, not args.elaboration_only)
        return 0
    if args.timeout <= 0 or (args.all and args.case) or (args.elaboration_only and args.lint_runner):
        parser.error("--timeout must be positive; --all and --case are mutually exclusive")
    args.output.mkdir(parents=True, exist_ok=True)
    archive = args.archive or args.output / f"{PACKAGE}.tar.gz"
    if not archive.exists() and args.archive is None:
        with urllib.request.urlopen(URL, timeout=60) as response:
            archive.write_bytes(response.read())
    with tempfile.TemporaryDirectory(prefix="pulp-rtl-", dir=args.output) as tmp:
        package = unpack(archive, Path(tmp))
        available = targets(package)
        selected = list(available) if args.all else list(dict.fromkeys(args.case or SMOKE))
        unknown = set(selected) - available.keys()
        if unknown:
            parser.error(f"Unknown cases: {sorted(unknown)}; available: {list(available)}")
        summary = dict(release=VERSION, url=URL, sha256=SHA256, results=[])
        for name in selected:
            result = run_target(package, name, available[name], args.output / name,
                                args.najaeda_path.resolve(), args.timeout, args.lint_runner,
                                not args.elaboration_only)
            summary["results"].append(result)
            (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
            print(f"{name}: {result['status']} ({result['seconds']:.2f}s)", flush=True)
    return int(any(result["status"] != "passed" for result in summary["results"]))


if __name__ == "__main__":
    sys.exit(main())
