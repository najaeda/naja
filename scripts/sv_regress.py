#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""External SystemVerilog regression runner.

The runner checks out pinned external RTL repositories, generates Verilog with
Naja's SystemVerilog frontend and VRL dumper, then lints the generated netlist
with Dockerized Verilator.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
from typing import Any

try:
    import yaml
except ImportError as exc:  # pragma: no cover - exercised by user environment
    raise SystemExit("Missing dependency: install PyYAML to use scripts/sv_regress.py") from exc


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPO_ROOT / "test" / "regress" / "sv" / "cases.yml"
DEFAULT_WORK_DIR = REPO_ROOT / "build" / "sv-regress"
DEFAULT_NAJAEDA_PATH = REPO_ROOT / "build" / "test" / "najaeda"
PRIMITIVES_PATH = REPO_ROOT / "test" / "nl" / "formats" / "systemverilog" / \
    "benchmarks" / "najaeda_primitives.v"


class RegressError(RuntimeError):
    pass


def load_manifest(path: Path) -> list[dict[str, Any]]:
    with path.open("r", encoding="utf-8") as stream:
        data = yaml.safe_load(stream)
    if not isinstance(data, dict) or not isinstance(data.get("cases"), list):
        raise RegressError(f"Invalid manifest, expected top-level 'cases' list: {path}")

    cases: list[dict[str, Any]] = []
    seen: set[str] = set()
    required = {"name", "repo", "commit", "top", "flist", "output", "verilator"}
    for raw_case in data["cases"]:
        if not isinstance(raw_case, dict):
            raise RegressError("Invalid manifest case entry, expected mapping")
        missing = sorted(required.difference(raw_case))
        if missing:
            raise RegressError(f"Case is missing required keys {missing}: {raw_case}")
        name = raw_case["name"]
        if not isinstance(name, str) or not name:
            raise RegressError(f"Invalid case name: {name!r}")
        if name in seen:
            raise RegressError(f"Duplicate case name: {name}")
        seen.add(name)
        cases.append(raw_case)
    return cases


def select_cases(cases: list[dict[str, Any]], case_name: str) -> list[dict[str, Any]]:
    if case_name == "all":
        return cases
    selected = [case for case in cases if case["name"] == case_name]
    if not selected:
        valid = ", ".join(case["name"] for case in cases)
        raise RegressError(f"Unknown case '{case_name}'. Valid cases: {valid}")
    return selected


def print_command(args: list[str], cwd: Path | None) -> None:
    prefix = f"(cd {cwd} && " if cwd else ""
    suffix = ")" if cwd else ""
    print(f"$ {prefix}{' '.join(args)}{suffix}", flush=True)


def run_command(
    args: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    timeout: int | None = None,
    log_path: Path | None = None,
) -> None:
    print_command(args, cwd)
    log_file = None
    try:
        if log_path:
            log_path.parent.mkdir(parents=True, exist_ok=True)
            log_file = log_path.open("w", encoding="utf-8")
        process = subprocess.Popen(
            args,
            cwd=str(cwd) if cwd else None,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        assert process.stdout is not None
        start = time.monotonic()
        for line in process.stdout:
            sys.stdout.write(line)
            if log_file:
                log_file.write(line)
            if timeout and time.monotonic() - start > timeout:
                process.kill()
                raise RegressError(f"Command timed out after {timeout}s: {' '.join(args)}")
        return_code = process.wait()
        if return_code != 0:
            raise RegressError(f"Command failed with exit code {return_code}: {' '.join(args)}")
    finally:
        if log_file:
            log_file.close()


def capture_command(args: list[str], *, cwd: Path | None = None) -> str:
    print_command(args, cwd)
    completed = subprocess.run(
        args,
        cwd=str(cwd) if cwd else None,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return completed.stdout.strip()


def format_env_value(value: str, *, repo_dir: Path, case_dir: Path, artifacts_dir: Path) -> str:
    return value.format(
        repo=repo_dir,
        case=case_dir,
        artifacts=artifacts_dir,
        work=case_dir.parent,
        naja=REPO_ROOT,
    )


def case_env(
    case: dict[str, Any],
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> dict[str, str]:
    env = os.environ.copy()
    for key, value in case.get("env", {}).items():
        env[key] = format_env_value(
            str(value),
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
        )
    env["GIT_TERMINAL_PROMPT"] = "0"
    return env


def ensure_checkout(case: dict[str, Any], repo_dir: Path, log_dir: Path) -> str:
    repo_url = case["repo"]
    commit = case["commit"]
    if repo_dir.exists():
        if not (repo_dir / ".git").exists():
            raise RegressError(f"Existing checkout is not a git repository: {repo_dir}")
    else:
        repo_dir.parent.mkdir(parents=True, exist_ok=True)
        run_command(["git", "clone", repo_url, str(repo_dir)], log_path=log_dir / "git-clone.log")

    run_command(["git", "remote", "set-url", "origin", repo_url], cwd=repo_dir)
    run_command(
        ["git", "fetch", "origin", commit],
        cwd=repo_dir,
        log_path=log_dir / "git-fetch.log",
    )
    run_command(
        ["git", "checkout", "--detach", commit],
        cwd=repo_dir,
        log_path=log_dir / "git-checkout.log",
    )
    run_command(["git", "submodule", "sync", "--recursive"], cwd=repo_dir)
    run_command(
        ["git", "submodule", "update", "--init", "--recursive"],
        cwd=repo_dir,
        log_path=log_dir / "git-submodule.log",
    )
    return capture_command(["git", "rev-parse", "HEAD"], cwd=repo_dir)


GENERATOR = r'''
import argparse
import logging
from najaeda import netlist

parser = argparse.ArgumentParser()
parser.add_argument("--top", required=True)
parser.add_argument("--flist", required=True)
parser.add_argument("--output", required=True)
parser.add_argument("--diagnostics", required=True)
parser.add_argument("--rtl-infos-as-attributes", action="store_true")
parser.add_argument("--assigns-as-instances", action="store_true")
args = parser.parse_args()

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s", force=True)
netlist.reset()
svconfig = netlist.SystemVerilogConfig(
    top=args.top,
    flist=args.flist,
    diagnostics_report_path=args.diagnostics,
)
top = netlist.load_system_verilog([], config=svconfig)
dump_config = netlist.VerilogDumpConfig()
dump_config.dumpRTLInfosAsAttributes = args.rtl_infos_as_attributes
dump_config.dumpAssignsAsInstances = args.assigns_as_instances
top.dump_verilog(args.output, config=dump_config)
netlist.reset()
'''


def generate_verilog(
    case: dict[str, Any],
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
    log_dir: Path,
    najaeda_path: Path,
) -> Path:
    if not najaeda_path.exists():
        raise RegressError(
            f"Missing built najaeda package path: {najaeda_path}. "
            "Build target 'naja' and prepare build/test/najaeda first."
        )
    helper_path = case_dir / "generate_naja_verilog.py"
    helper_path.write_text(GENERATOR, encoding="utf-8")

    output_path = artifacts_dir / case["output"]
    diagnostics_path = artifacts_dir / "diagnostics.log"
    flist_path = materialize_flist(case, repo_dir, case_dir, artifacts_dir)

    env = case_env(case, repo_dir, case_dir, artifacts_dir)
    env["PYTHONPATH"] = f"{najaeda_path}{os.pathsep}{env.get('PYTHONPATH', '')}"

    dump = case.get("dump", {})
    args = [
        sys.executable,
        str(helper_path),
        "--top",
        case["top"],
        "--flist",
        str(flist_path),
        "--output",
        str(output_path),
        "--diagnostics",
        str(diagnostics_path),
    ]
    if dump.get("rtl_infos_as_attributes", False):
        args.append("--rtl-infos-as-attributes")
    if dump.get("assigns_as_instances", False):
        args.append("--assigns-as-instances")

    timeout = int(case.get("generate_timeout_seconds", 7200))
    run_command(args, cwd=repo_dir, env=env, timeout=timeout, log_path=log_dir / "generate.log")
    if not output_path.exists():
        raise RegressError(f"Generation did not create expected output: {output_path}")
    return output_path


def materialize_flist(
    case: dict[str, Any],
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> Path:
    flist_path = repo_dir / case["flist"]
    if not flist_path.exists():
        raise RegressError(f"Missing flist for case {case['name']}: {flist_path}")

    flist_append = case.get("flist_append", [])
    if not flist_append:
        return flist_path
    if not isinstance(flist_append, list):
        raise RegressError(f"Invalid flist_append for case {case['name']}: expected list")

    generated_flist = artifacts_dir / f"{case['name']}.flist"
    generated_flist.parent.mkdir(parents=True, exist_ok=True)
    content = flist_path.read_text(encoding="utf-8")
    with generated_flist.open("w", encoding="utf-8") as stream:
        stream.write(content)
        if content and not content.endswith("\n"):
            stream.write("\n")
        stream.write("\n# Appended by scripts/sv_regress.py\n")
        for entry in flist_append:
            stream.write(format_env_value(
                str(entry),
                repo_dir=repo_dir,
                case_dir=case_dir,
                artifacts_dir=artifacts_dir,
            ))
            stream.write("\n")
    return generated_flist


def run_verilator(
    case: dict[str, Any],
    *,
    case_dir: Path,
    artifacts_dir: Path,
    generated_path: Path,
    log_dir: Path,
) -> None:
    primitives_path = artifacts_dir / PRIMITIVES_PATH.name
    shutil.copy2(PRIMITIVES_PATH, primitives_path)

    verilator = case["verilator"]
    image = verilator.get("image", "verilator/verilator:v5.046")
    suppressions = verilator.get("suppressions", [])
    flags = ["--lint-only", "--sv", "--top-module", case["top"]]
    flags.extend(f"-Wno-{warning}" for warning in suppressions)
    flags.extend(verilator.get("extra_args", []))

    generated_in_container = f"/work/artifacts/{generated_path.name}"
    primitives_in_container = f"/work/artifacts/{primitives_path.name}"
    command = [
        "docker",
        "run",
        "--rm",
        "-v",
        f"{case_dir}:/work",
        "--entrypoint",
        "verilator",
        image,
        *flags,
        generated_in_container,
        primitives_in_container,
    ]
    (artifacts_dir / "verilator-command.json").write_text(
        json.dumps(command, indent=2) + "\n",
        encoding="utf-8",
    )
    timeout = int(verilator.get("timeout_seconds", 7200))
    run_command(command, cwd=case_dir, timeout=timeout, log_path=log_dir / "verilator.log")


def run_case(case: dict[str, Any], work_dir: Path, najaeda_path: Path) -> dict[str, Any]:
    case_name = case["name"]
    case_dir = work_dir / case_name
    repo_dir = case_dir / "repo"
    artifacts_dir = case_dir / "artifacts"
    log_dir = artifacts_dir / "logs"
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    log_dir.mkdir(parents=True, exist_ok=True)

    started = time.time()
    summary: dict[str, Any] = {
        "case": case_name,
        "repo": case["repo"],
        "requested_commit": case["commit"],
        "status": "failed",
        "artifacts": str(artifacts_dir),
    }
    try:
        actual_commit = ensure_checkout(case, repo_dir, log_dir)
        summary["actual_commit"] = actual_commit
        generated_path = generate_verilog(
            case,
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
            log_dir=log_dir,
            najaeda_path=najaeda_path,
        )
        summary["generated_verilog"] = str(generated_path)
        run_verilator(
            case,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
            generated_path=generated_path,
            log_dir=log_dir,
        )
        summary["status"] = "passed"
        return summary
    except Exception as exc:
        summary["error"] = str(exc)
        raise
    finally:
        summary["elapsed_seconds"] = round(time.time() - started, 3)
        (artifacts_dir / "summary.json").write_text(
            json.dumps(summary, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )


def command_list(args: argparse.Namespace) -> int:
    cases = load_manifest(args.manifest)
    for case in cases:
        suppressions = ", ".join(case.get("verilator", {}).get("suppressions", []))
        print(f"{case['name']}: {case['repo']} @ {case['commit']} top={case['top']}")
        print(f"  flist={case['flist']} output={case['output']}")
        print(f"  suppressions={suppressions}")
    return 0


def command_clean(args: argparse.Namespace) -> int:
    if args.work_dir.exists():
        shutil.rmtree(args.work_dir)
        print(f"Removed {args.work_dir}")
    return 0


def command_run(args: argparse.Namespace) -> int:
    cases = select_cases(load_manifest(args.manifest), args.case)
    args.work_dir.mkdir(parents=True, exist_ok=True)

    summaries: list[dict[str, Any]] = []
    failures: list[str] = []
    for case in cases:
        print(f"=== SV regress case: {case['name']} ===", flush=True)
        try:
            summaries.append(run_case(case, args.work_dir, args.najaeda_pythonpath))
        except Exception as exc:
            failures.append(f"{case['name']}: {exc}")
            case_summary_path = args.work_dir / case["name"] / "artifacts" / "summary.json"
            if case_summary_path.exists():
                with case_summary_path.open("r", encoding="utf-8") as stream:
                    summaries.append(json.load(stream))
            else:
                summaries.append({
                    "case": case["name"],
                    "status": "failed",
                    "error": str(exc),
                })
            if not args.keep_going:
                break

    summary_path = args.work_dir / "summary.json"
    summary_path.write_text(
        json.dumps(summaries, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(f"Wrote summary: {summary_path}")

    if failures:
        for failure in failures:
            print(f"FAILED: {failure}", file=sys.stderr)
        return 1
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--work-dir", type=Path, default=DEFAULT_WORK_DIR)
    parser.add_argument("--najaeda-pythonpath", type=Path, default=DEFAULT_NAJAEDA_PATH)
    subparsers = parser.add_subparsers(dest="command", required=True)

    list_parser = subparsers.add_parser("list", help="List configured SV regress cases")
    list_parser.set_defaults(func=command_list)

    run_parser = subparsers.add_parser("run", help="Run SV regress cases")
    run_parser.add_argument("--case", default="all", help="Case name or 'all'")
    run_parser.add_argument("--keep-going", action="store_true", help="Continue after a case fails")
    run_parser.set_defaults(func=command_run)

    clean_parser = subparsers.add_parser("clean", help="Remove the SV regress work directory")
    clean_parser.set_defaults(func=command_clean)
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        return args.func(args)
    except RegressError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
