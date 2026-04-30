#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""External SystemVerilog regression runner.

The runner checks out pinned external RTL repositories, generates Verilog with
Naja's SystemVerilog frontend and VRL dumper, then runs selected Dockerized
Verilator lint and simulation stages on the generated netlist.
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
import time
from typing import Any

try:
    import yaml
except ImportError as exc:  # pragma: no cover - exercised by user environment
    raise SystemExit("Missing dependency: install PyYAML to use regress/sv/sv_regress.py") from exc


REGRESS_SV_ROOT = Path(__file__).resolve().parent
REPO_ROOT = REGRESS_SV_ROOT.parents[1]
DEFAULT_MANIFEST = REGRESS_SV_ROOT / "cases.yml"
DEFAULT_WORK_DIR = REPO_ROOT / "build" / "sv-regress"
DEFAULT_NAJAEDA_PATH = REPO_ROOT / "build" / "test" / "najaeda"
PRIMITIVES_PATH = REPO_ROOT / "test" / "nl" / "formats" / "systemverilog" / \
    "benchmarks" / "najaeda_primitives.v"
DEFAULT_STAGES = ["lint", "github_sim"]
VALID_STAGES = {"lint", "github_sim", "local_sim"}
VALID_LINT_RUNNERS = {"docker", "local"}
VALID_SIM_RUNNERS = {"docker", "local"}


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


def select_requested_cases(
    cases: list[dict[str, Any]],
    case_names: list[str] | None,
) -> list[dict[str, Any]]:
    if not case_names:
        return cases
    if "all" in case_names:
        return cases

    selected: list[dict[str, Any]] = []
    seen: set[str] = set()
    for case_name in case_names:
        if case_name in seen:
            continue
        seen.add(case_name)
        selected.extend(select_cases(cases, case_name))
    return selected


def select_stages(stage_names: list[str] | None) -> list[str]:
    requested = stage_names or DEFAULT_STAGES
    selected: list[str] = []
    seen: set[str] = set()
    for stage in requested:
        if stage not in VALID_STAGES:
            valid = ", ".join(sorted(VALID_STAGES))
            raise RegressError(f"Unknown stage '{stage}'. Valid stages: {valid}")
        if stage in seen:
            continue
        seen.add(stage)
        selected.append(stage)
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
    append_log: bool = False,
) -> None:
    print_command(args, cwd)
    log_file = None
    try:
        if log_path:
            log_path.parent.mkdir(parents=True, exist_ok=True)
            log_file = log_path.open("a" if append_log else "w", encoding="utf-8")
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


def format_command(
    command: list[Any],
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> list[str]:
    if not command:
        raise RegressError("Invalid empty setup command")
    return [
        format_env_value(
            str(arg),
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
        )
        for arg in command
    ]


def run_setup_commands(
    case: dict[str, Any],
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
    log_dir: Path,
) -> None:
    commands = case.get("setup_commands", [])
    if not commands:
        return
    if not isinstance(commands, list):
        raise RegressError(f"Invalid setup_commands for case {case['name']}: expected list")

    env = case_env(case, repo_dir, case_dir, artifacts_dir)
    timeout = int(case.get("setup_timeout_seconds", 7200))
    for index, command in enumerate(commands):
        if not isinstance(command, list):
            raise RegressError(
                f"Invalid setup_commands[{index}] for case {case['name']}: expected argument list"
            )
        formatted_command = format_command(
            command,
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
        )
        run_command(
            formatted_command,
            cwd=repo_dir,
            env=env,
            timeout=timeout,
            log_path=log_dir / f"setup-{index}.log",
        )


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
    run_setup_commands(
        case,
        repo_dir=repo_dir,
        case_dir=case_dir,
        artifacts_dir=artifacts_dir,
        log_dir=log_dir,
    )
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


def resolve_case_path(
    value: str,
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> Path:
    formatted = format_env_value(
        value,
        repo_dir=repo_dir,
        case_dir=case_dir,
        artifacts_dir=artifacts_dir,
    )
    path = Path(formatted)
    return path if path.is_absolute() else repo_dir / path


def resolve_glob(
    pattern: str,
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> list[Path]:
    formatted = format_env_value(
        pattern,
        repo_dir=repo_dir,
        case_dir=case_dir,
        artifacts_dir=artifacts_dir,
    )
    path = Path(formatted)
    if path.is_absolute():
        return sorted(path.parent.glob(path.name))
    return sorted(repo_dir.glob(formatted))


def resolve_flist_path(
    case: dict[str, Any],
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> Path:
    flist_path = resolve_case_path(
        str(case["flist"]),
        repo_dir=repo_dir,
        case_dir=case_dir,
        artifacts_dir=artifacts_dir,
    )
    if flist_path.exists():
        return flist_path

    flist_glob = case.get("flist_glob")
    if flist_glob:
        matches = resolve_glob(
            str(flist_glob),
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
        )
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1:
            raise RegressError(
                f"Flist glob for case {case['name']} matched multiple files: {flist_glob}"
            )

    raise RegressError(f"Missing flist for case {case['name']}: {flist_path}")


def absolutize_from_base(path_text: str, base_dir: Path) -> str:
    path = Path(path_text)
    return str(path if path.is_absolute() else base_dir / path)


def normalize_fusesoc_vc(flist_path: Path) -> str:
    lines: list[str] = []
    base_dir = flist_path.parent
    for raw_line in flist_path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("+incdir+"):
            include_path = absolutize_from_base(line[len("+incdir+"):], base_dir)
            lines.append("+incdir+" + include_path)
        elif line.startswith("+define+"):
            lines.append(line)
        elif line.startswith("-D") and len(line) > 2:
            lines.append("+define+" + line[2:])
        elif line.endswith((".sv", ".v", ".svh", ".vh")):
            lines.append(absolutize_from_base(line, base_dir))
    return "\n".join(lines) + ("\n" if lines else "")


def write_flist(
    case: dict[str, Any],
    *,
    artifacts_dir: Path,
    content: str,
    repo_dir: Path,
    case_dir: Path,
) -> Path:
    flist_append = case.get("flist_append", [])
    if not isinstance(flist_append, list):
        raise RegressError(f"Invalid flist_append for case {case['name']}: expected list")

    generated_flist = artifacts_dir / f"{case['name']}.flist"
    generated_flist.parent.mkdir(parents=True, exist_ok=True)
    with generated_flist.open("w", encoding="utf-8") as stream:
        stream.write(content)
        if content and not content.endswith("\n"):
            stream.write("\n")
        if flist_append:
            stream.write("\n# Appended by regress/sv/sv_regress.py\n")
            for entry in flist_append:
                stream.write(format_env_value(
                    str(entry),
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                ))
                stream.write("\n")
    return generated_flist


def materialize_flist(
    case: dict[str, Any],
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
) -> Path:
    flist_path = resolve_flist_path(case, repo_dir, case_dir, artifacts_dir)

    flist_format = case.get("flist_format", "plain")
    flist_append = case.get("flist_append", [])
    if flist_format == "plain" and not flist_append:
        return flist_path

    if flist_format == "plain":
        content = flist_path.read_text(encoding="utf-8")
    elif flist_format == "fusesoc_vc":
        content = normalize_fusesoc_vc(flist_path)
    else:
        raise RegressError(f"Unsupported flist_format for case {case['name']}: {flist_format}")

    return write_flist(
        case,
        artifacts_dir=artifacts_dir,
        content=content,
        repo_dir=repo_dir,
        case_dir=case_dir,
    )


def run_verilator(
    case: dict[str, Any],
    *,
    case_dir: Path,
    artifacts_dir: Path,
    generated_path: Path,
    log_dir: Path,
) -> None:
    run_lint(
        case,
        case_dir=case_dir,
        artifacts_dir=artifacts_dir,
        generated_path=generated_path,
        log_dir=log_dir,
    )


def run_lint(
    case: dict[str, Any],
    *,
    case_dir: Path,
    artifacts_dir: Path,
    generated_path: Path,
    log_dir: Path,
    lint_runner: str = "docker",
) -> dict[str, Any]:
    if lint_runner not in VALID_LINT_RUNNERS:
        valid = ", ".join(sorted(VALID_LINT_RUNNERS))
        raise RegressError(f"Unknown lint runner '{lint_runner}'. Valid runners: {valid}")

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
    if lint_runner == "docker":
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
    else:
        command = [
            "verilator",
            *flags,
            str(generated_path),
            str(primitives_path),
        ]
    (artifacts_dir / "verilator-lint-command.json").write_text(
        json.dumps(command, indent=2) + "\n",
        encoding="utf-8",
    )
    # Keep the historical artifact name for scripts that still inspect it.
    (artifacts_dir / "verilator-command.json").write_text(
        json.dumps(command, indent=2) + "\n",
        encoding="utf-8",
    )
    timeout = int(verilator.get("timeout_seconds", 7200))
    run_command(command, cwd=case_dir, timeout=timeout, log_path=log_dir / "verilator-lint.log")
    return {"status": "passed", "log": str(log_dir / "verilator-lint.log")}


def configured_stage(case: dict[str, Any], stage: str) -> dict[str, Any]:
    config = case.get(stage)
    if not isinstance(config, dict):
        raise RegressError(f"Case {case['name']} does not define stage '{stage}'")
    return config


def docker_mount_path(case_dir: Path, path: Path) -> str:
    try:
        relative = path.relative_to(case_dir)
    except ValueError as exc:
        raise RegressError(f"Path is outside case directory and cannot be mounted: {path}") from exc
    return "/work/" + str(relative).replace(os.sep, "/")


def copy_stage_sources(
    config: dict[str, Any],
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
    stage: str,
) -> list[Path]:
    source_values = config.get("sources", [])
    if not isinstance(source_values, list) or not source_values:
        raise RegressError(f"Stage {stage} requires a non-empty sources list")

    stage_dir = artifacts_dir / stage
    stage_dir.mkdir(parents=True, exist_ok=True)
    copied: list[Path] = []
    for source_value in source_values:
        source_path = resolve_case_path(
            str(source_value),
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
        )
        if not source_path.exists():
            raise RegressError(f"Missing {stage} source for case: {source_path}")
        target_path = stage_dir / source_path.name
        shutil.copy2(source_path, target_path)
        copied.append(target_path)
    return copied


def verilator_stage_flags(case: dict[str, Any], config: dict[str, Any]) -> list[str]:
    suppressions = list(case.get("verilator", {}).get("suppressions", []))
    suppressions.extend(config.get("suppressions", []))
    flags = ["--binary", "--timing", "--sv", "--top-module", config["top_module"]]
    flags.extend(f"-Wno-{warning}" for warning in suppressions)
    flags.extend(config.get("extra_args", []))
    return flags


def run_verilator_binary_stage(
    case: dict[str, Any],
    *,
    stage: str,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
    generated_path: Path,
    log_dir: Path,
) -> dict[str, Any]:
    config = configured_stage(case, stage)
    primitives_path = artifacts_dir / PRIMITIVES_PATH.name
    shutil.copy2(PRIMITIVES_PATH, primitives_path)
    sources = copy_stage_sources(
        config,
        repo_dir=repo_dir,
        case_dir=case_dir,
        artifacts_dir=artifacts_dir,
        stage=stage,
    )

    image = config.get("image", case.get("verilator", {}).get("image", "verilator/verilator:v5.046"))
    obj_dir = artifacts_dir / f"{stage}-obj"
    top_module = config["top_module"]
    runner = str(config.get("runner", "docker"))
    if runner not in VALID_SIM_RUNNERS:
        valid = ", ".join(sorted(VALID_SIM_RUNNERS))
        raise RegressError(f"Unknown {stage} runner '{runner}'. Valid runners: {valid}")

    if runner == "docker":
        build_command = [
            "docker",
            "run",
            "--rm",
            "-v",
            f"{case_dir}:/work",
            "--entrypoint",
            "verilator",
            image,
            *verilator_stage_flags(case, config),
            "-Mdir",
            docker_mount_path(case_dir, obj_dir),
            docker_mount_path(case_dir, generated_path),
            docker_mount_path(case_dir, primitives_path),
            *(docker_mount_path(case_dir, source) for source in sources),
        ]
    else:
        build_command = [
            "verilator",
            *verilator_stage_flags(case, config),
            "-Mdir",
            str(obj_dir),
            str(generated_path),
            str(primitives_path),
            *(str(source) for source in sources),
        ]
    run_command_path = artifacts_dir / f"{stage}-run-command.json"
    build_command_path = artifacts_dir / f"{stage}-build-command.json"
    build_command_path.write_text(json.dumps(build_command, indent=2) + "\n", encoding="utf-8")

    executable = obj_dir / f"V{top_module}"
    if runner == "docker":
        sim_command = [
            "docker",
            "run",
            "--rm",
            "-v",
            f"{case_dir}:/work",
            "--entrypoint",
            docker_mount_path(case_dir, executable),
            image,
        ]
    else:
        sim_command = [str(executable)]
    run_command_path.write_text(json.dumps(sim_command, indent=2) + "\n", encoding="utf-8")

    log_path = log_dir / f"{stage.replace('_', '-')}.log"
    timeout = int(config.get("timeout_seconds", case.get("verilator", {}).get("timeout_seconds", 7200)))
    run_command(build_command, cwd=case_dir, timeout=timeout, log_path=log_path)
    run_command(sim_command, cwd=case_dir, timeout=timeout, log_path=log_path, append_log=True)

    pass_regex = config.get("pass_regex")
    if pass_regex:
        log_text = log_path.read_text(encoding="utf-8", errors="replace")
        if not re.search(str(pass_regex), log_text):
            raise RegressError(f"{stage} did not match pass_regex {pass_regex!r}")
    return {"status": "passed", "log": str(log_path)}


def run_local_sim(
    case: dict[str, Any],
    *,
    repo_dir: Path,
    case_dir: Path,
    artifacts_dir: Path,
    generated_path: Path,
    log_dir: Path,
    require_tools: bool,
) -> dict[str, Any]:
    config = configured_stage(case, "local_sim")
    log_path = log_dir / "local-sim.log"
    configured_tool_checks = config.get("tool_checks", [])
    if not isinstance(configured_tool_checks, list):
        raise RegressError(f"Invalid local_sim.tool_checks for case {case['name']}: expected list")
    tool_checks = list(configured_tool_checks)
    if "top_module" in config or "sources" in config:
        tool_checks.append("verilator")

    missing: list[str] = []
    for tool in tool_checks:
        if isinstance(tool, list):
            alternatives = [str(alternative) for alternative in tool]
            if not any(shutil.which(alternative) for alternative in alternatives):
                missing.append("one of {" + ", ".join(alternatives) + "}")
        elif shutil.which(str(tool)) is None:
            missing.append(str(tool))
    if missing:
        reason = f"missing optional local simulation tools: {', '.join(missing)}"
        log_path.write_text(f"SKIPPED: {reason}\n", encoding="utf-8")
        if require_tools:
            raise RegressError(reason)
        return {"status": "skipped", "reason": reason, "log": str(log_path)}

    if "top_module" in config or "sources" in config:
        return run_verilator_binary_stage(
            case,
            stage="local_sim",
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
            generated_path=generated_path,
            log_dir=log_dir,
        )

    commands = config.get("commands", [])
    if not isinstance(commands, list):
        raise RegressError(f"Invalid local_sim.commands for case {case['name']}: expected list")
    if not commands:
        reason = config.get("skip_reason", "local simulation commands are not configured")
        log_path.write_text(f"SKIPPED: {reason}\n", encoding="utf-8")
        if require_tools:
            raise RegressError(str(reason))
        return {"status": "skipped", "reason": str(reason), "log": str(log_path)}

    env = case_env(case, repo_dir, case_dir, artifacts_dir)
    timeout = int(config.get("timeout_seconds", 7200))
    for index, command in enumerate(commands):
        if not isinstance(command, list):
            raise RegressError(
                f"Invalid local_sim.commands[{index}] for case {case['name']}: expected argument list"
            )
        formatted_command = format_command(
            command,
            repo_dir=repo_dir,
            case_dir=case_dir,
            artifacts_dir=artifacts_dir,
        )
        (artifacts_dir / f"local-sim-command-{index}.json").write_text(
            json.dumps(formatted_command, indent=2) + "\n",
            encoding="utf-8",
        )
        run_command(
            formatted_command,
            cwd=repo_dir,
            env=env,
            timeout=timeout,
            log_path=log_path,
            append_log=index > 0,
        )

    pass_regex = config.get("pass_regex")
    if pass_regex:
        log_text = log_path.read_text(encoding="utf-8", errors="replace")
        if not re.search(str(pass_regex), log_text):
            raise RegressError(f"local_sim did not match pass_regex {pass_regex!r}")
    return {"status": "passed", "log": str(log_path)}


def run_case(
    case: dict[str, Any],
    work_dir: Path,
    najaeda_path: Path,
    stages: list[str],
    *,
    lint_runner: str = "docker",
    require_local_sim_tools: bool = False,
) -> dict[str, Any]:
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
        "stages": {},
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
        for stage in stages:
            print(f"--- stage: {stage} ---", flush=True)
            if stage == "lint":
                result = run_lint(
                    case,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated_path,
                    log_dir=log_dir,
                    lint_runner=lint_runner,
                )
            elif stage == "github_sim":
                result = run_verilator_binary_stage(
                    case,
                    stage=stage,
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated_path,
                    log_dir=log_dir,
                )
            elif stage == "local_sim":
                result = run_local_sim(
                    case,
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated_path,
                    log_dir=log_dir,
                    require_tools=require_local_sim_tools,
                )
            else:  # pragma: no cover - guarded by select_stages.
                raise RegressError(f"Unhandled stage: {stage}")
            summary["stages"][stage] = result
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
    cases = select_requested_cases(load_manifest(args.manifest), args.case)
    stages = select_stages(args.stage)
    args.work_dir.mkdir(parents=True, exist_ok=True)

    summaries: list[dict[str, Any]] = []
    failures: list[str] = []
    for case in cases:
        print(f"=== SV regress case: {case['name']} stages={','.join(stages)} ===", flush=True)
        try:
            summaries.append(
                run_case(
                    case,
                    args.work_dir,
                    args.najaeda_pythonpath,
                    stages,
                    lint_runner=args.lint_runner,
                    require_local_sim_tools=args.require_local_sim_tools,
                )
            )
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
    run_parser.add_argument(
        "--case",
        action="append",
        help="Case name or 'all'. Repeat to run multiple named cases.",
    )
    run_parser.add_argument(
        "--stage",
        action="append",
        choices=sorted(VALID_STAGES),
        help="Stage to run. Repeat to run multiple stages. Defaults to lint and github_sim.",
    )
    run_parser.add_argument(
        "--lint-runner",
        choices=sorted(VALID_LINT_RUNNERS),
        default="docker",
        help="Run the lint stage with Dockerized Verilator or the local verilator binary.",
    )
    run_parser.add_argument(
        "--require-local-sim-tools",
        action="store_true",
        help="Fail local_sim when optional firmware/toolchain dependencies are missing.",
    )
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
