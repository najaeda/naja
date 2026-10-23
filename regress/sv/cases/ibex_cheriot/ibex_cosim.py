#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Co-simulate CHERIoT Ibex RTL against its Naja netlist in several configurations.

For each configuration, ibex_top is loaded twice with najaeda (the two Verilog
dumps must be identical), then the original RTL and the Naja dump are built as
two Verilator models and driven in lockstep with the same random stimulus.
Every top-level output is compared after both clock edges. On a mismatch the
failing seed is re-run with VCD tracing and the first diverging internal
signals are reported.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time


REPO_ROOT = Path(__file__).resolve().parents[4]
CASE_DIR = Path(__file__).resolve().parent
BASE_FLIST = CASE_DIR / "ibex_top.f"
DEFAULT_NAJAEDA_PATH = REPO_ROOT / "build" / "test" / "najaeda"
PASS_MARKER = "IBEX_COSIM_PASS"
TOP = "ibex_top"

# Parameter overrides per configuration: name -> (slang value, Verilator value).
# Verilator needs numeric values for enum parameters.
OPENTITAN = {
    "BaseIsa": ("ibex_pkg::BaseIsaRV32IorCHERIoT", "1"),
    "RV32E": ("0", "0"),
    "RV32M": ("ibex_pkg::RV32MSingleCycle", "3"),
    "RV32B": ("ibex_pkg::RV32BFull", "2"),
    "RV32ZC": ("ibex_pkg::RV32ZcaZcbZcmp", "3"),
    "RegFile": ("ibex_pkg::RegFileFF", "0"),
    "BranchTargetALU": ("1", "1"),
    "WritebackStage": ("1", "1"),
    "ICache": ("1", "1"),
    "ICacheECC": ("1", "1"),
    "ICacheScramble": ("1", "1"),
    "BranchPredictor": ("0", "0"),
    "DbgTriggerEn": ("1", "1"),
    "SecureIbex": ("1", "1"),
    "PMPEnable": ("1", "1"),
    "PMPGranularity": ("0", "0"),
    "PMPNumRegions": ("16", "16"),
    "MHPMCounterNum": ("10", "10"),
    "MHPMCounterWidth": ("32", "32"),
}
CONFIGS: dict[str, dict[str, tuple[str, str]]] = {
    "default": {},
    "pmp": {"PMPEnable": ("1", "1"), "PMPNumRegions": ("16", "16")},
    "icache": {"ICache": ("1", "1"), "ICacheECC": ("1", "1"), "ICacheScramble": ("1", "1")},
    "cheriot": {"BaseIsa": ("ibex_pkg::BaseIsaRV32IorCHERIoT", "1")},
    "rv32b": {"RV32B": ("ibex_pkg::RV32BFull", "2")},
    "opentitan": OPENTITAN,
}

VERILATOR_FLAGS = [
    "--cc", "--build", "--top-module", TOP,
    "--x-assign", "0", "--x-initial", "0",
    "-Wno-fatal", "-Wno-lint", "-Wno-style", "-Wno-UNOPTFLAT", "-Wno-MULTIDRIVEN",
]

# Harness stimulus: inputs held at fixed values, and inputs that toggle rarely so
# the core makes progress. Everything else is randomized each cycle.
FIXED_INPUTS = {
    "test_en_i": "0",
    "scan_rst_ni": "1",
    "fetch_enable_i": "0x5",     # IbexMuBiOn
    "cheriot_enable_i": "0x5",   # IbexMuBiOn
    "boot_addr_i": "0x00100000",
    "hart_id_i": "0",
}
RARE_INPUTS = {"irq_nm_i", "debug_req_i", "irq_external_i", "irq_timer_i",
               "irq_software_i", "irq_fast_i", "instr_err_i", "data_err_i",
               "trvk_revbm_err_i"}

LOAD_SCRIPT = r"""
import json, sys
from najaeda import netlist
flist, out_dir, diagnostics = sys.argv[1:4]
config = netlist.SystemVerilogConfig(flist=flist, top="ibex_top",
                                     diagnostics_report_path=diagnostics)
top = netlist.load_system_verilog([], config)
ports = [[t.get_name(), str(t.get_direction()), t.get_width()] for t in top.get_terms()]
with open(f"{out_dir}/ports.json", "w") as stream:
    json.dump(ports, stream, indent=1)
top.dump_gate_level_verilog(f"{out_dir}/ibex_top_naja.v")
"""


class CosimError(RuntimeError):
    pass


def log(message: str) -> None:
    print(message, flush=True)


def run(args: list[str], *, cwd: Path, log_path: Path, env: dict[str, str] | None = None) -> None:
    with log_path.open("w", encoding="utf-8") as stream:
        stream.write("$ " + " ".join(args) + "\n")
        stream.flush()
        result = subprocess.run(args, cwd=str(cwd), env=env, stdout=stream,
                                stderr=subprocess.STDOUT, check=False)
    if result.returncode != 0:
        tail = "".join(log_path.read_text(encoding="utf-8", errors="replace")
                       .splitlines(keepends=True)[-30:])
        raise CosimError(f"command failed ({result.returncode}), see {log_path}:\n{tail}")


def flist_text(config: dict[str, tuple[str, str]], *, verilator: bool) -> str:
    lines = [f"-f {BASE_FLIST}"]
    for name, (slang_value, verilator_value) in config.items():
        lines.append(f"-G{name}={verilator_value}" if verilator else f"-G {name}={slang_value}")
    return "\n".join(lines) + "\n"


def normalized_dump(path: Path) -> str:
    # The dump header holds a timestamp and the naja version.
    return "".join(line for line in path.read_text(encoding="utf-8").splitlines(keepends=True)
                   if not line.startswith("//"))


def generate_harness(ports: list[list], *, max_mismatches: int = 50) -> str:
    lines: list[str] = []
    emit = lines.append
    emit('#include <cstdio>\n#include <cstdlib>\n#include <cstdint>\n#include <random>')
    emit('#include "Vrtl.h"\n#include "Vnaja.h"\n#include "verilated.h"')
    emit('#ifdef TRACE\n#include "verilated_vcd_c.h"\n#endif\n')
    emit('double sc_time_stamp() { return 0; }')
    emit('static std::mt19937_64 rng;')
    emit('static uint64_t rnd(int w) { uint64_t v = rng(); '
         'return w >= 64 ? v : (v & ((1ULL << w) - 1)); }')
    emit('static bool rare() { return (rng() % 200) == 0; }\n')

    emit('static void drive(Vrtl* a, Vnaja* b) {')
    for name, direction, width in ports:
        if "INPUT" not in direction or name in ("clk_i", "rst_ni"):
            continue
        words = (width + 31) // 32
        if name in FIXED_INPUTS:
            value = FIXED_INPUTS[name]
            if width > 64:
                for i in range(words):
                    emit(f'  a->{name}[{i}] = b->{name}[{i}] = {value};')
            else:
                emit(f'  a->{name} = b->{name} = {value};')
        elif width > 64:
            for i in range(words):
                emit(f'  {{ uint32_t v = rnd(32); a->{name}[{i}] = v; b->{name}[{i}] = v; }}')
        elif name in RARE_INPUTS:
            emit(f'  {{ uint64_t v = rare() ? rnd({width}) : 0; a->{name} = v; b->{name} = v; }}')
        else:
            emit(f'  {{ uint64_t v = rnd({width}); a->{name} = v; b->{name} = v; }}')
    emit('}\n')

    emit('static int compare(Vrtl* a, Vnaja* b, uint64_t cyc, const char* edge) {')
    emit('  int bad = 0;')
    for name, direction, width in ports:
        if "OUTPUT" not in direction:
            continue
        if width > 64:
            for i in range((width + 31) // 32):
                emit(f'  if (a->{name}[{i}] != b->{name}[{i}]) {{ bad++; '
                     f'printf("MISMATCH cyc=%llu %s {name}[word {i}] rtl=%08x naja=%08x\\n", '
                     f'(unsigned long long)cyc, edge, a->{name}[{i}], b->{name}[{i}]); }}')
        else:
            emit(f'  if (a->{name} != b->{name}) {{ bad++; '
                 f'printf("MISMATCH cyc=%llu %s {name} rtl=%llx naja=%llx\\n", '
                 f'(unsigned long long)cyc, edge, (unsigned long long)a->{name}, '
                 f'(unsigned long long)b->{name}); }}')
    emit('  return bad;\n}\n')

    emit('''int main(int argc, char** argv) {
  uint64_t cycles = argc > 1 ? strtoull(argv[1], nullptr, 0) : 20000;
  uint64_t seed = argc > 2 ? strtoull(argv[2], nullptr, 0) : 1;
  rng.seed(seed);
  // Separate contexts keep the two models (and their traces) independent.
  VerilatedContext* ca = new VerilatedContext; VerilatedContext* cb = new VerilatedContext;
#ifdef TRACE
  ca->traceEverOn(true); cb->traceEverOn(true);
#endif
  Vrtl* a = new Vrtl{ca}; Vnaja* b = new Vnaja{cb};
#ifdef TRACE
  VerilatedVcdC* ta = new VerilatedVcdC; VerilatedVcdC* tb = new VerilatedVcdC;
  a->trace(ta, 99); b->trace(tb, 99); ta->open("rtl.vcd"); tb->open("naja.vcd");
  uint64_t t = 0;
#define DUMP() do { ta->dump(t); tb->dump(t); t += 5; } while (0)
#else
#define DUMP() do {} while (0)
#endif
  uint64_t mismatches = 0, instr_reqs = 0, data_reqs = 0;
  a->clk_i = b->clk_i = 0; a->rst_ni = b->rst_ni = 0;
  drive(a, b); a->eval(); b->eval();
  for (uint64_t c = 0; c < cycles; c++) {
    a->rst_ni = b->rst_ni = (c >= 5);
    // Change inputs while the clock is low, then raise it.
    drive(a, b);
    a->clk_i = b->clk_i = 0; a->eval(); b->eval(); DUMP();
    mismatches += compare(a, b, c, "negedge");
    a->clk_i = b->clk_i = 1; a->eval(); b->eval(); DUMP();
    mismatches += compare(a, b, c, "posedge");
    if (a->instr_req_o) instr_reqs++;
    if (a->data_req_o) data_reqs++;
    if (mismatches > MAX_MISMATCHES) {
      printf("Too many mismatches, stopping at cycle %llu\\n", (unsigned long long)c);
      break;
    }
  }
  printf("RESULT seed=%llu cycles=%llu mismatches=%llu instr_req_cycles=%llu data_req_cycles=%llu\\n",
         (unsigned long long)seed, (unsigned long long)cycles, (unsigned long long)mismatches,
         (unsigned long long)instr_reqs, (unsigned long long)data_reqs);
#ifdef TRACE
  ta->close(); tb->close();
#endif
  a->final(); b->final(); delete a; delete b;
  return mismatches ? 1 : 0;
}'''.replace("MAX_MISMATCHES", str(max_mismatches)))
    return "\n".join(lines) + "\n"


# --- VCD comparison (used only to explain a failure) -------------------------

def parse_vcd(path: Path) -> tuple[dict[str, str], dict[str, list[tuple[int, str]]]]:
    scope: list[str] = []
    names: dict[str, str] = {}
    changes: dict[str, list[tuple[int, str]]] = {}
    time_now = 0
    in_definitions = True
    with path.open(encoding="utf-8", errors="replace") as stream:
        for line in stream:
            if in_definitions:
                tokens = line.split()
                if not tokens:
                    continue
                if tokens[0] == "$scope":
                    scope.append(tokens[2])
                elif tokens[0] == "$upscope":
                    scope.pop()
                elif tokens[0] == "$var":
                    names.setdefault(normalize_signal(scope[1:] + [tokens[4]]), tokens[3])
                elif tokens[0] == "$enddefinitions":
                    in_definitions = False
                continue
            head = line[:1]
            if head == "#":
                time_now = int(line[1:])
            elif head in "01xz":
                changes.setdefault(line[1:].strip(), []).append((time_now, head))
            elif head == "b":
                value, code = line[1:].split()
                changes.setdefault(code, []).append((time_now, value.lstrip("0") or "0"))
    return names, changes


def normalize_signal(parts: list[str]) -> str:
    # Naja flattens generate scopes into instance names such as
    # ibex_top_u_ibex_core_if_stage_i_..., so both sides become one
    # underscore-joined path starting at the deepest ibex_top_ prefix.
    parts = [re.sub(r"[\[\]().]", "_", part).strip("_") for part in parts]
    start = 0
    for index, part in enumerate(parts):
        if part.startswith("ibex_top_"):
            start = index
    return re.sub(r"_+", "_", "_".join(parts[start:]))


def value_at(history: list[tuple[int, str]], when: int) -> str | None:
    value = None
    for change_time, change_value in history:
        if change_time > when:
            break
        value = change_value
    return value


def first_divergences(rtl_vcd: Path, naja_vcd: Path, limit: int = 20) -> list[str]:
    rtl_names, rtl_changes = parse_vcd(rtl_vcd)
    naja_names, naja_changes = parse_vcd(naja_vcd)
    times = sorted({change_time for history in rtl_changes.values() for change_time, _ in history})
    found = []
    for name in sorted(set(rtl_names) & set(naja_names)):
        rtl_history = rtl_changes.get(rtl_names[name], [])
        naja_history = naja_changes.get(naja_names[name], [])
        for when in times:
            rtl_value, naja_value = value_at(rtl_history, when), value_at(naja_history, when)
            if rtl_value != naja_value:
                found.append((when, name, rtl_value, naja_value))
                break
    found.sort()
    return [f"t={when} (cycle ~{when // 10}) {name}: rtl={rtl_value} naja={naja_value}"
            for when, name, rtl_value, naja_value in found[:limit]]


# --- build and run ------------------------------------------------------------

class Tools:
    def __init__(self) -> None:
        verilator = shutil.which("verilator")
        if verilator is None:
            raise CosimError("verilator not found on PATH")
        self.verilator = verilator
        self.version = subprocess.run([verilator, "--version"], capture_output=True,
                                      text=True, check=True).stdout.strip()
        self.root = Path(subprocess.run([verilator, "--getenv", "VERILATOR_ROOT"],
                                        capture_output=True, text=True,
                                        check=True).stdout.strip())
        self.cxx = os.environ.get("CXX", "c++")


def build_model(tools: Tools, *, prefix: str, sources: list[str], build_dir: Path,
                cwd: Path, jobs: int, trace: bool) -> None:
    flags = VERILATOR_FLAGS + ["-j", str(jobs), "--prefix", prefix, "-Mdir", str(build_dir)]
    flags += ["-O1", "--trace", "--trace-structs"] if trace else ["-O3"]
    build_dir.mkdir(parents=True, exist_ok=True)
    run([tools.verilator, *flags, *sources], cwd=cwd, log_path=build_dir / "verilator.log")


def link_harness(tools: Tools, *, harness: Path, rtl_dir: Path, naja_dir: Path,
                 output: Path, trace: bool) -> None:
    include = tools.root / "include"
    args = [tools.cxx, "-std=c++20", "-O1" if trace else "-O2",
            f"-I{rtl_dir}", f"-I{naja_dir}", f"-I{include}", f"-I{include / 'vltstd'}"]
    if trace:
        args += ["-DTRACE", str(include / "verilated_vcd_c.cpp")]
    args += [str(harness), str(rtl_dir / "Vrtl__ALL.a"), str(naja_dir / "Vnaja__ALL.a"),
             str(rtl_dir / "libverilated.a"), "-pthread", "-o", str(output)]
    if sys.platform.startswith("linux"):
        args.append("-latomic")
    run(args, cwd=output.parent, log_path=output.with_suffix(".link.log"))


def rtl_cache_key(tools: Tools, repo: Path, verilator_flist: str) -> str:
    commit = subprocess.run(["git", "-C", str(repo), "rev-parse", "HEAD"], capture_output=True,
                            text=True, check=False).stdout.strip()
    digest = hashlib.sha256()
    for part in (commit, tools.version, verilator_flist, BASE_FLIST.read_text(encoding="utf-8"),
                 " ".join(VERILATOR_FLAGS)):
        digest.update(part.encode("utf-8"))
        digest.update(b"\0")
    return digest.hexdigest()[:16]


def run_cosim(binary: Path, cycles: int, seed: int) -> dict[str, object]:
    result = subprocess.run([str(binary), str(cycles), str(seed)], cwd=binary.parent,
                            capture_output=True, text=True, check=False)
    summary = re.search(r"RESULT seed=\d+ cycles=\d+ mismatches=(\d+) "
                        r"instr_req_cycles=(\d+) data_req_cycles=(\d+)", result.stdout)
    if summary is None:
        raise CosimError(f"{binary} produced no RESULT line (exit {result.returncode}):\n"
                         f"{result.stdout[-2000:]}{result.stderr[-2000:]}")
    mismatch_lines = [line for line in result.stdout.splitlines() if line.startswith("MISMATCH")]
    first_cycle = None
    if mismatch_lines:
        first_cycle = int(re.search(r"cyc=(\d+)", mismatch_lines[0]).group(1))
    return {
        "seed": seed,
        "mismatches": int(summary.group(1)),
        "instr_req_cycles": int(summary.group(2)),
        "data_req_cycles": int(summary.group(3)),
        "first_mismatch_cycle": first_cycle,
        "first_mismatches": mismatch_lines[:10],
    }


def explain_failure(tools: Tools, *, repo: Path, work: Path, sources_rtl: list[str],
                    sources_naja: list[str], harness: Path, seed: int, cycle: int,
                    jobs: int) -> list[str]:
    debug = work / "debug"
    shutil.rmtree(debug, ignore_errors=True)
    debug.mkdir(parents=True)
    build_model(tools, prefix="Vrtl", sources=sources_rtl, build_dir=debug / "rtl",
                cwd=repo, jobs=jobs, trace=True)
    build_model(tools, prefix="Vnaja", sources=sources_naja, build_dir=debug / "naja",
                cwd=repo, jobs=jobs, trace=True)
    binary = debug / "cosim"
    link_harness(tools, harness=harness, rtl_dir=debug / "rtl", naja_dir=debug / "naja",
                 output=binary, trace=True)
    subprocess.run([str(binary), str(cycle + 3), str(seed)], cwd=debug, capture_output=True,
                   check=False)
    return first_divergences(debug / "rtl.vcd", debug / "naja.vcd")


def run_config(name: str, *, tools: Tools, repo: Path, artifacts: Path, najaeda_path: Path,
               rtl_cache: Path, cycles: int, seeds: list[int], jobs: int,
               debug_on_failure: bool) -> dict[str, object]:
    config = CONFIGS[name]
    work = artifacts / name
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    started = time.monotonic()
    result: dict[str, object] = {"config": name, "status": "failed"}

    naja_flist = work / "naja.f"
    naja_flist.write_text(flist_text(config, verilator=False), encoding="utf-8")
    verilator_flist_content = flist_text(config, verilator=True)
    verilator_flist = work / "verilator.f"
    verilator_flist.write_text(verilator_flist_content, encoding="utf-8")

    # 1. Load twice in fresh processes: the dumps must be identical.
    env = os.environ.copy()
    env["PYTHONPATH"] = f"{najaeda_path}{os.pathsep}{env.get('PYTHONPATH', '')}"
    dumps = []
    for index in (1, 2):
        out_dir = work / f"load{index}"
        out_dir.mkdir()
        load_started = time.monotonic()
        run([sys.executable, "-c", LOAD_SCRIPT, str(naja_flist), str(out_dir),
             str(out_dir / "diagnostics.log")], cwd=repo, log_path=out_dir / "load.log", env=env)
        result[f"load{index}_seconds"] = round(time.monotonic() - load_started, 2)
        dumps.append(out_dir / "ibex_top_naja.v")
    result["deterministic"] = normalized_dump(dumps[0]) == normalized_dump(dumps[1])
    if not result["deterministic"]:
        log(f"[{name}] najaeda produced different netlists on two loads of the same sources")

    netlist_dir = work / "load1"
    ports = json.loads((netlist_dir / "ports.json").read_text(encoding="utf-8"))
    harness = work / "harness.cpp"
    harness.write_text(generate_harness(ports), encoding="utf-8")
    sources_rtl = ["-f", str(verilator_flist)]
    sources_naja = [str(netlist_dir / "ibex_top_naja.v")]
    if (netlist_dir / "naja_primitives.v").exists():
        sources_naja.insert(0, str(netlist_dir / "naja_primitives.v"))

    # 2. Build the RTL model (cached: it only depends on Ibex and Verilator) and
    #    the netlist model in parallel.
    build_started = time.monotonic()
    rtl_dir = rtl_cache / f"{name}-{rtl_cache_key(tools, repo, verilator_flist_content)}"
    result["rtl_cache_hit"] = (rtl_dir / "Vrtl__ALL.a").exists()
    with ThreadPoolExecutor(max_workers=2) as pool:
        futures = []
        if not result["rtl_cache_hit"]:
            shutil.rmtree(rtl_dir, ignore_errors=True)
            futures.append(pool.submit(build_model, tools, prefix="Vrtl", sources=sources_rtl,
                                       build_dir=rtl_dir, cwd=repo, jobs=jobs, trace=False))
        futures.append(pool.submit(build_model, tools, prefix="Vnaja", sources=sources_naja,
                                   build_dir=work / "naja", cwd=repo, jobs=jobs, trace=False))
        for future in futures:
            future.result()
    binary = work / "cosim"
    link_harness(tools, harness=harness, rtl_dir=rtl_dir, naja_dir=work / "naja",
                 output=binary, trace=False)
    result["build_seconds"] = round(time.monotonic() - build_started, 2)

    # 3. Run every seed.
    runs = [run_cosim(binary, cycles, seed) for seed in seeds]
    result["runs"] = runs
    failed = [entry for entry in runs if entry["mismatches"]]
    if failed and debug_on_failure:
        worst = failed[0]
        try:
            result["first_divergences"] = explain_failure(
                tools, repo=repo, work=work, sources_rtl=sources_rtl, sources_naja=sources_naja,
                harness=harness, seed=int(worst["seed"]), cycle=int(worst["first_mismatch_cycle"]),
                jobs=jobs)
        except CosimError as exc:
            result["first_divergences"] = [f"trace build failed: {exc}"]
    result["status"] = "passed" if not failed and result["deterministic"] else "failed"
    result["seconds"] = round(time.monotonic() - started, 2)
    return result


def report(results: list[dict[str, object]]) -> None:
    log("")
    log(f"{'config':<10} {'status':<7} {'determ.':<8} {'mismatches per seed':<22} "
        f"{'data req cycles':<18} time")
    for entry in results:
        runs = entry.get("runs", [])
        mismatches = "/".join(str(run_entry["mismatches"]) for run_entry in runs) or "-"
        data = "/".join(str(run_entry["data_req_cycles"]) for run_entry in runs) or "-"
        determinism = {True: "yes", False: "NO"}.get(entry.get("deterministic"), "-")
        log(f"{entry['config']:<10} {entry['status']:<7} {determinism:<8} {mismatches:<22} "
            f"{data:<18} {entry.get('seconds', '-')}s")
        if entry.get("error"):
            log(f"  error: {entry['error']}")
        for run_entry in runs:
            for line in run_entry["first_mismatches"][:5]:
                log(f"  seed {run_entry['seed']}: {line}")
        for line in entry.get("first_divergences", []):
            log(f"  first diverging signal: {line}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", type=Path, required=True, help="CHERIoT Ibex checkout")
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--najaeda-pythonpath", type=Path, default=DEFAULT_NAJAEDA_PATH)
    parser.add_argument("--config", action="append", choices=sorted(CONFIGS),
                        help="configuration to run (repeatable, default: all)")
    # The environment overrides let a scheduled CI run go deeper with the same
    # manifest command.
    parser.add_argument("--cycles", type=int,
                        default=int(os.environ.get("IBEX_COSIM_CYCLES", "20000")))
    parser.add_argument("--seeds", type=int, nargs="+",
                        default=[int(seed) for seed in
                                 os.environ.get("IBEX_COSIM_SEEDS", "1 2 3").split()])
    parser.add_argument("--rtl-cache", type=Path,
                        help="directory for cached RTL Verilator models "
                             "(default: <artifacts>/rtl-cache)")
    parser.add_argument("--jobs", type=int, default=2,
                        help="configurations processed in parallel")
    parser.add_argument("--no-debug-on-failure", action="store_true",
                        help="do not rebuild with tracing to locate the first divergence")
    args = parser.parse_args()

    repo = args.repo.resolve()
    artifacts = (args.artifacts / "ibex-cosim").resolve()
    artifacts.mkdir(parents=True, exist_ok=True)
    rtl_cache = (args.rtl_cache or artifacts / "rtl-cache").resolve()
    rtl_cache.mkdir(parents=True, exist_ok=True)
    najaeda_path = args.najaeda_pythonpath.resolve()
    if not najaeda_path.exists():
        raise SystemExit(f"missing built najaeda package path: {najaeda_path}")
    tools = Tools()
    names = args.config or list(CONFIGS)
    parallel = max(1, min(args.jobs, len(names)))
    build_jobs = max(1, (os.cpu_count() or 2) // parallel)
    log(f"ibex cosim: {len(names)} configs, seeds {args.seeds}, {args.cycles} cycles, "
        f"{tools.version}")

    def one(name: str) -> dict[str, object]:
        try:
            entry = run_config(name, tools=tools, repo=repo, artifacts=artifacts,
                               najaeda_path=najaeda_path, rtl_cache=rtl_cache,
                               cycles=args.cycles, seeds=args.seeds, jobs=build_jobs,
                               debug_on_failure=not args.no_debug_on_failure)
        except CosimError as exc:
            entry = {"config": name, "status": "failed", "error": str(exc)}
        log(f"[{name}] {entry['status']}")
        return entry

    with ThreadPoolExecutor(max_workers=parallel) as pool:
        results = list(pool.map(one, names))
    (artifacts / "summary.json").write_text(json.dumps(results, indent=2) + "\n",
                                            encoding="utf-8")
    report(results)
    if all(entry["status"] == "passed" for entry in results):
        log(f"{PASS_MARKER} {len(results)} configs x {len(args.seeds)} seeds x {args.cycles} cycles")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
