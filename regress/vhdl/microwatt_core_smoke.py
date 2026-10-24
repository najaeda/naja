#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Run Microwatt console programs with a Naja core inside the GHDL SoC."""

import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

import microwatt_cosim as bridge

CORE = """decode_types common wishbone_types fetch1 utils plrufn cache_ram icache
predecode decode1 helpers insn_helpers control decode2 register_file cr_file
crhelpers ppc_fx_insns rotator logical countbits multiply multiply-32s divider
execute1 loadstore1 mmu dcache writeback core_debug core fpu pmu bitsort nonrandom""".split()
SOC = """wishbone_arbiter wishbone_bram_wrapper sync_fifo wishbone_debug_master xics
syscon gpio soc spi_rxtx spi_flash_ctrl git sim_console sim_pp_uart sim_bram_helpers
sim_bram sim_jtag_socket sim_jtag dmi_dtm_xilinx sim_16550_uart core_tb""".split()
FOREIGN = ["sim_vhpi_c", "sim_bram_helpers_c", "sim_console_c", "sim_jtag_socket_c"]
DEFAULT_PROGRAMS = """test_branch_alias test_decrementer test_fpu test_hash test_illegal
 test_misc test_mmu test_modes test_pmu test_prefix test_privileged test_reservation
 test_sc test_spr_read test_trace test_xics""".split()
PORTS = [
    ("clk", 1, "in"), ("rst", 1, "in"), ("alt_reset", 1, "in"), ("tb_ctrl", 3, "in"),
    ("wishbone_insn_in", 66, "in"), ("wishbone_insn_out", 104, "out"),
    ("wishbone_data_in", 66, "in"), ("wishbone_data_out", 104, "out"),
    ("wb_snoop_in", 104, "in"), ("dmi_addr", 4, "in"), ("dmi_din", 64, "in"),
    ("dmi_dout", 64, "out"), ("dmi_req", 1, "in"), ("dmi_wr", 1, "in"),
    ("dmi_ack", 1, "out"), ("ext_irq", 1, "in"), ("msg_in", 1, "in"),
    ("msg_out", 1, "out"), ("run_out", 1, "out"), ("terminated_out", 1, "out"),
]
MASTER = [("adr", 29), ("dat", 64), ("sel", 8), ("cyc", 1), ("stb", 1), ("we", 1)]
SLAVE = [("dat", 64), ("ack", 1), ("stall", 1)]
FIELDS = {"tb_ctrl": [("reset", 1), ("rd_prot", 1), ("freeze", 1)],
          "wishbone_insn_in": SLAVE, "wishbone_data_in": SLAVE,
          "wishbone_insn_out": MASTER, "wishbone_data_out": MASTER, "wb_snoop_in": MASTER}
# Values passed by core_tb/soc, including SoC's alternate reset address.
VALUES = {"CPU_INDEX": 0, "NCPUS": 1, "DISABLE_FLATTEN": "false", "EX1_BYPASS": "true",
          "HAS_FPU": "true", "HAS_BTC": "true", "ALT_RESET_ADDRESS": 'x"ffffffffff000000"',
          "LOG_LENGTH": 512, "ICACHE_NUM_LINES": 64, "ICACHE_NUM_WAYS": 2,
          "ICACHE_TLB_SIZE": 64, "DCACHE_NUM_LINES": 64, "DCACHE_NUM_WAYS": 2,
          "DCACHE_TLB_SET_SIZE": 64, "DCACHE_TLB_NUM_WAYS": 2}
COMPLETION = """    completion : process
    begin
        wait until terminated_out = '1';
        wait for 100 ns;
        report "CORE_PROGRAM_DONE";
        std.env.stop;
        wait;
    end process;
"""


def prepare(args):
    source = args.output / "source"
    source.mkdir(exist_ok=True)
    for pattern in ("*.vhdl", "*.vhdl.in", "*.c", "*.h", "LICENSE"):
        for path in args.source.glob(pattern):
            shutil.copy2(path, source / path.name)
    (source / "sim-unisim").mkdir(exist_ok=True)
    for path in (args.source / "sim-unisim").glob("*.vhdl"):
        shutil.copy2(path, source / "sim-unisim" / path.name)
    dcache = source / "dcache.vhdl"
    original = dcache.read_text()
    lines = original.splitlines(keepends=True)
    reads = [i for i, line in enumerate(lines) if line.strip() == "req_nc <= nc;"]
    writes = [i for i, line in enumerate(lines) if line.strip() == "nc := r0.req.nc or perm_attr.nocache;"]
    if len(reads) != 1 or len(writes) != 1:
        raise RuntimeError("Unrecognized dcache source; inspect the assignment-order fix before proceeding")
    patched = writes[0] > reads[0]
    if patched:
        if not args.preview_dcache_fix:
            raise RuntimeError("Upstream dcache reads nc before assignment; use --preview-dcache-fix for the temporary corrected-source experiment")
        lines.insert(reads[0], lines.pop(writes[0]))
        dcache.write_text("".join(lines))
    (args.output / "dcache-preview.patch").write_text("".join(difflib.unified_diff(
        original.splitlines(keepends=True), dcache.read_text().splitlines(keepends=True),
        fromfile="a/dcache.vhdl", tofile="b/dcache.vhdl")))
    # Avoid the version script accidentally reading Naja's enclosing Git tree.
    revision = "00000000000000"
    if (args.source / ".git").exists():
        revision = subprocess.check_output(["git", "-C", str(args.source), "rev-parse", "HEAD"], text=True).strip()
    (source / "git.vhdl").write_text((source / "git.vhdl.in").read_text().replace(
        "@hash@", revision[:14]).replace("@dirty@", "1" if patched else "0"))
    return source, patched


def build_soc(args, source, backend, objects):
    work = args.output / backend
    work.mkdir(exist_ok=True)
    unisim = work / "unisim"
    unisim.mkdir(exist_ok=True)
    def run(command, stage):
        bridge.run(command, work, work / (stage + ".log"), timeout=args.build_timeout)
    run([args.ghdl, "-i", "--std=08", "--work=unisim", "--workdir=" + str(unisim),
         *[source / "sim-unisim" / n for n in ("BSCANE2.vhdl", "BUFG.vhdl", "unisim_vcomponents.vhdl")]], "unisim")
    flags = ["--std=08", "-P" + str(unisim)]
    linked = list(objects)
    for name in FOREIGN:
        target = work / (name + ".o")
        run([args.cc, "-O2", "-c", source / (name + ".c"), "-o", target], name)
        linked.insert(0, "-Wl," + str(target))
    core = [source / (n + ".vhdl") for n in CORE]
    if backend == "reference":
        # Analyze the original core first, then add an architecture that selects
        # its synthesizable mode explicitly. Inner binding names 'behave', so
        # the wrapper cannot recursively instantiate itself.
        run([args.ghdl, "-i", *flags, *core], "core-import")
        run([args.ghdl, "-m", *flags, "core"], "core-build")
        proxy = "library ieee; use ieee.std_logic_1164.all;\narchitecture hardware_reference of core is begin\n"
        proxy += "dut : entity work.core(behave) generic map(SIM=>false, " + ", ".join(
            n + "=>" + n for n in VALUES) + ") port map(" + ", ".join(n + "=>" + n for n, _, _ in PORTS) + ");\n"
        proxy += COMPLETION + "end;\n"
        (work / "hardware_reference.vhdl").write_text(proxy)
        run([args.ghdl, "-a", *flags, work / "hardware_reference.vhdl"], "core-wrapper")
        core = []
    else:
        core[CORE.index("core")] = work / "core_wrapper.vhdl"
    run([args.ghdl, "-i", *flags, *core, *[source / (n + ".vhdl") for n in SOC]], "soc-import")
    run([args.ghdl, "-m", *flags, *linked, "core_tb"], "soc-build")


def export_core(args, source):
    import naja
    work = args.output / "naja"
    work.mkdir(exist_ok=True)
    original = (source / "core.vhdl").read_text()
    configured = work / "configured.vhdl"
    configured.write_text(bridge.configured_dut("core", PORTS, original, {**VALUES, "SIM": "false"}))
    wrapper = bridge.wrapper("core", PORTS, original, FIELDS, VALUES, ("msg_out",))
    (work / "core_wrapper.vhdl").write_text(wrapper.replace("    bridge : process(all)", COMPLETION + "    bridge : process(all)"))
    export = work / "verilog"
    export.mkdir(exist_ok=True)
    naja.NLUniverse.create()
    try:
        db = naja.NLDB.create(naja.NLUniverse.get())
        top = db.loadVHDL([str(source / (n + ".vhdl")) for n in CORE] + [str(configured)],
                          top="naja_configured", diagnostics_report_path=str(work / "diagnostics.log"))
        if [(t.getName(), t.getWidth()) for t in top.getTerms()] != [(n, w) for n, w, _ in PORTS]:
            raise RuntimeError("Core interface changed")
        top.dumpGateLevelVerilog(str(export), "core.v")
    finally:
        naja.NLUniverse.get().destroy()
    (work / "adapter.sv").write_text(bridge.adapter("naja_configured", PORTS))
    (work / "bridge.cpp").write_text(bridge.cpp_bridge(
        sum(w for _, w, d in PORTS if d == "in"), sum(w for _, w, d in PORTS if d == "out")))
    obj = work / "obj_dir"
    bridge.run([args.verilator, "--cc", "--build", "-j", str(args.jobs), "--output-split", "20000",
                "--top-module", "naja_bridge", "--prefix", "Vnaja_bridge", "--Mdir", obj,
                "-Wno-fatal", "-CFLAGS", "-fPIC", work / "adapter.sv",
                *sorted(export.glob("*.v")), work / "bridge.cpp"], work, work / "verilator.log", timeout=args.build_timeout)
    include = Path(subprocess.check_output([args.verilator, "--getenv", "VERILATOR_ROOT"], text=True).strip()) / "include"
    objects = [obj / "bridge.o", obj / "Vnaja_bridge__ALL.a"]
    for name in ("verilated", "verilated_threads"):
        target = obj / (name + ".o")
        bridge.run([args.cxx, "-std=c++17", "-fPIC", "-pthread", "-I" + str(include), "-I" + str(include / "vltstd"),
                    "-c", include / (name + ".cpp"), "-o", target], work, work / (name + ".log"))
        objects.append(target)
    return ["-Wl," + str(p) for p in objects] + ["-Wl,-lc++" if sys.platform == "darwin" else "-Wl,-lstdc++", "-Wl,-lpthread"]


def simulate(args, backend, program):
    work = args.output / backend / "runs" / program
    work.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.source / "tests" / (program + ".bin"), work / "main_ram.bin")
    with (work / "simulation.log").open("wb") as log, (work / "console.out").open("wb") as console:
        # The simulated UART treats stdin EOF as fatal. Keep an empty pipe open
        # so polling reports no input until the noninteractive program finishes.
        with subprocess.Popen([str(args.output / backend / "core_tb"), "--stop-time=" + args.stop_time],
                              cwd=work, stdin=subprocess.PIPE, stdout=log, stderr=console) as result:
            try:
                result.wait(timeout=300)
            except BaseException:
                result.kill()
                result.wait()
                raise
    text = (work / "simulation.log").read_text()
    if result.returncode or "CORE_PROGRAM_DONE" not in text:
        raise RuntimeError(f"{backend}/{program}: core did not complete successfully; see {work / 'simulation.log'}")
    actual = (work / "console.out").read_bytes().replace(b"Failed to bind debug socket !\r\n", b"")
    expected = (args.source / "tests" / (program + ".console_out")).read_bytes()
    if not expected or actual != expected:
        raise RuntimeError(f"{backend}/{program}: console mismatch: {actual!r} != {expected!r}")
    counts = re.findall(r"NAJA_COSIM_EVALUATIONS=(\d+)", text)
    metavalues = re.findall(r"NAJA_COSIM_METAVALUES=(\d+)", text)
    if backend == "naja" and (len(counts) != 1 or int(counts[0]) == 0):
        raise RuntimeError("Core completion did not exercise the Verilated model")
    return {"status": "passed", "console_sha256": hashlib.sha256(actual).hexdigest(),
            "evaluations": int(counts[0]) if counts else None,
            "metavalue_input_bits_mapped_to_zero": int(metavalues[0]) if metavalues else None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--programs", nargs="+", default=DEFAULT_PROGRAMS,
                        help="Upstream console test basenames (default: all 16 validated programs)")
    parser.add_argument("--preview-dcache-fix", action="store_true")
    parser.add_argument("--stop-time", default="2ms")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--build-timeout", type=int, default=600,
                        help="GHDL and Verilator build timeout in seconds (default: 600)")
    for tool, default in (("ghdl", "ghdl"), ("verilator", "verilator"), ("cc", "cc"), ("cxx", "c++")):
        parser.add_argument("--" + tool, default=default)
    args = parser.parse_args()
    args.source = args.source.resolve()
    args.output = args.output.resolve()
    if args.source == args.output or args.source in args.output.parents or args.output in args.source.parents:
        parser.error("Keep the output outside the upstream source directory")
    args.output.mkdir(parents=True, exist_ok=True)
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    if args.build_timeout < 1:
        parser.error("--build-timeout must be positive")
    for tool in ("ghdl", "verilator", "cc", "cxx"):
        path = shutil.which(getattr(args, tool))
        if not path:
            parser.error(f"Missing tool: {getattr(args, tool)}")
        setattr(args, tool, path)
    for program in args.programs:
        if not re.fullmatch(r"[A-Za-z0-9_]+", program):
            parser.error("Program names must be simple upstream test basenames")
    import naja
    report = {"naja_version": naja.getVersion(), "naja_git_hash": naja.getGitHash(),
              "core_generics": {**VALUES, "SIM": "false"}, "soc_sim": True,
              "random_provider": "nonrandom", "programs": {}}
    try:
        source, patched = prepare(args)
        report["preview_dcache_fix_applied"] = patched
        report["source_sha256"] = {str(p.relative_to(source)): hashlib.sha256(p.read_bytes()).hexdigest()
                                   for p in sorted(source.rglob("*")) if p.is_file()}
        print("Building GHDL hardware-core reference", flush=True)
        build_soc(args, source, "reference", [])
        for program in args.programs:
            report["programs"][program] = {
                "binary_sha256": hashlib.sha256((args.source / "tests" / (program + ".bin")).read_bytes()).hexdigest(),
                "expected_console_sha256": hashlib.sha256((args.source / "tests" / (program + ".console_out")).read_bytes()).hexdigest(),
                "reference": simulate(args, "reference", program)}
            print(f"{program}: reference PASS", flush=True)
        print("Elaborating, exporting and building Naja hardware core", flush=True)
        objects = export_core(args, source)
        build_soc(args, source, "naja", objects)
        for program in args.programs:
            report["programs"][program]["naja"] = simulate(args, "naja", program)
            print(f"{program}: Naja PASS", flush=True)
    except Exception as error:
        report["error"] = str(error)
        raise
    finally:
        (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
