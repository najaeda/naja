#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

"""Artifact-local equivalent of pinned Borg's make generate_verilog.

Keep TTMain's two elaborations and upstream memory-initialization post-step.
Invoke RDL Python explicitly instead of requiring the upstream Nix shell shim.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from sv_regress import RegressError, run_command


def asic_sources(work: Path) -> list[Path]:
    source_list = work / "out/hardware/borg/verilog/asic_files.txt"
    sources = []
    for line in source_list.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        source = (work / "src" / line.strip()).resolve()
        if not source.is_relative_to(work.resolve() / "out/hardware/borg/verilog"):
            raise RegressError(f"Unexpected Borg ASIC source: {line}")
        if source.suffix not in {".v", ".sv"} or not source.is_file():
            raise RegressError(f"Missing/invalid Borg ASIC source: {source}")
        if source not in sources:
            sources.append(source)
    if not sources:
        raise RegressError("Borg asic_files.txt contains no RTL sources")
    return sources


def recorded_run(args: list[str], *, work: Path, artifacts: Path,
                 env: dict[str, str], name: str) -> None:
    (artifacts / f"{name}-command.json").write_text(
        json.dumps({"argv": args, "cwd": str(work), "env": {
            key: env[key] for key in ("CLOCK_MHZ", "PYTHONPATH") if key in env
        }}, indent=2) + "\n", encoding="utf-8")
    run_command(args, cwd=work, env=env, timeout=7200,
                log_path=artifacts / "logs" / f"{name}.log")


def generate(repo: Path, artifacts: Path) -> Path:
    repo, artifacts = repo.resolve(), artifacts.resolve()
    artifacts.mkdir(parents=True, exist_ok=True)
    work = artifacts / "rtl-work"
    commit = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip()
    marker = work / ".naja-source-commit"
    if marker.exists() and marker.read_text().strip() != commit:
        raise RegressError(f"Borg build copy belongs to another commit: {work}")
    if not (repo / "PeakRDL-chisel/src/peakrdl_chisel").is_dir():
        raise RegressError("Initialize Borg's pinned PeakRDL-chisel submodule first")

    def ignore(directory: str, names: list[str]) -> set[str]:
        excluded = {".git", "out", "__pycache__"}
        if Path(directory) == repo:
            excluded.update({"mesa", "Vulkan-Tools", "tt"})
        return set(names).intersection(excluded)

    shutil.copytree(repo, work, ignore=ignore, dirs_exist_ok=True)
    marker.write_text(commit + "\n", encoding="utf-8")
    python = os.environ.get("BORG_PYTHON", sys.executable)
    mill = os.environ.get("BORG_MILL", "mill")
    env = dict(os.environ)
    env["CLOCK_MHZ"] = "4"
    env["PYTHONPATH"] = str(work / "PeakRDL-chisel/src")
    for key in ("HUTT_TRACE", "HUTT_TIMER_TRACE", "BORG_TRACE"):
        env.pop(key, None)
    version_command = [mill, "--no-server", "--version"]
    (artifacts / "borg-mill-version-command.json").write_text(json.dumps({
        "argv": version_command, "cwd": str(work)}, indent=2) + "\n", encoding="utf-8")
    version_result = subprocess.run(version_command, cwd=work, env=env, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    version = version_result.stdout
    (artifacts / "borg-mill-version.log").write_text(version, encoding="utf-8")
    if version_result.returncode:
        raise RegressError(f"Mill version check failed: {version.strip()}")
    if not re.search(r"\bMill Build Tool version 1\.1\.2\b", version):
        raise RegressError(f"Borg requires Mill 1.1.2; got: {version.strip()}")
    commands = [
        ("borg-rdl-validate", [python, "hardware/rdl/validate_rdl.py"]),
        ("borg-rdl-generate", [python, "hardware/rdl/generate.py",
                               "hardware/borg/src/generated", "out/hardware/borg/rdl"]),
        ("borg-rtl-generate", [mill, "--no-server", "-j", "1",
                               "asic.tt.runMain", "asic.tt.TTMain"]),
        ("borg-bram-init", [python, "scripts/init_bram_zero.py",
                            "out/hardware/borg/verilog"]),
    ]
    for name, command in commands:
        recorded_run(command, work=work, artifacts=artifacts, env=env, name=name)
    sources = asic_sources(work)
    # Same translate_on annotation cleanup as upstream, without GNU sed.
    for source in sources:
        text = source.read_text(encoding="utf-8")
        source.write_text(re.sub(r"// synthesis translate_on\t[^\n]*",
                                 "// synthesis translate_on", text), encoding="utf-8")
    flist = artifacts / "borg.flist"
    flist.write_text("+define+SIM\n+define+ENABLE_INITIAL_MEM_\n" +
                     f"+incdir+{work / 'out/hardware/borg/verilog'}\n" +
                     "".join(f"{source}\n" for source in sources), encoding="utf-8")
    (artifacts / "borg-source-selection.json").write_text(json.dumps({
        "commit": commit, "emitter": "asic.tt.TTMain", "clock_mhz": 4,
        "config": "BorgConfig.Asic", "source_list": str(work /
            "out/hardware/borg/verilog/asic_files.txt"),
        "sources": [str(source) for source in sources],
    }, indent=2) + "\n", encoding="utf-8")
    return flist


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    try:
        print(generate(args.repo, args.artifacts))
    except (OSError, subprocess.CalledProcessError, RegressError) as exc:
        parser.exit(1, f"Borg RTL generation failed (required, never skipped): {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
