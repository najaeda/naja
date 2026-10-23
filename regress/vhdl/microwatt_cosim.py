#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Run unmodified Microwatt VUnit benches against RTL and Naja exports."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

REVISION = "f341bed306a9a7281bf65200df8cc1f0a002617d"
BASE_SOURCES = ["decode_types", "utils", "common", "helpers", "ppc_fx_insns", "insn_helpers"]
CASES = {
    "rotator": ("rotator", [
        ("rs", 64, "in"), ("ra", 64, "in"), ("shift", 7, "in"),
        ("insn", 32, "in"), ("is_32bit", 1, "in"), ("right_shift", 1, "in"),
        ("arith", 1, "in"), ("clear_left", 1, "in"), ("clear_right", 1, "in"),
        ("sign_ext_rs", 1, "in"), ("result", 64, "out"), ("carry_out", 1, "out"),
    ]),
    "countbits": ("bit_counter", [
        ("clk", 1, "in"), ("rs", 64, "in"), ("stall", 1, "in"),
        ("count_right", 1, "in"), ("do_popcnt", 1, "in"), ("is_32bit", 1, "in"),
        ("datalen", 4, "in"), ("result", 64, "out"),
    ]),
    "multiply": ("multiply", [
        ("clk", 1, "in"), ("m_in", 259, "in"), ("m_out", 130, "out"),
    ]),
    "divider": ("divider", [
        ("clk", 1, "in"), ("rst", 1, "in"), ("d_in", 135, "in"), ("d_out", 66, "out"),
    ]),
}
# Record fields are packed in declaration order, first field at the MSB,
# matching Naja's VHDL port representation. Keep the original VHDL types at
# the bench interface; only the bridge transports their packed bits.
RECORD_FIELDS = {
    "m_in": [("valid", 1), ("data1", 64), ("data2", 64), ("addend", 128),
             ("is_signed", 1), ("subtract", 1)],
    "m_out": [("valid", 1), ("result", 128), ("overflow", 1)],
    "d_in": [("valid", 1), ("flush", 1), ("dividend", 64), ("divisor", 64),
             ("is_signed", 1), ("is_32bit", 1), ("is_extended", 1),
             ("is_modulus", 1), ("neg_result", 1)],
    "d_out": [("valid", 1), ("write_reg_data", 64), ("overflow", 1)],
}
# The upstream multiplier bench explicitly instantiates a four-stage pipeline.
GENERICS = {"multiply": {"PIPELINE_DEPTH": 4}}


def run(command, cwd, log, timeout=300, env=None):
    with log.open("w") as output:
        result = subprocess.run([str(x) for x in command], cwd=cwd, env=env,
                                stdout=output, stderr=subprocess.STDOUT, timeout=timeout)
    if result.returncode:
        raise RuntimeError(f"Command failed ({result.returncode}): {command}\n"
                           f"{log.read_text()[-6000:]}\nFull log: {log}")


def entity_interface(top, original):
    match = re.search(rf"(?is)^.*?end\s+(?:entity\s+)?{top}\s*;", original)
    if not match:
        raise ValueError(f"Cannot locate entity {top}")
    return match[0]


def wrapper(top, ports, original, record_fields=None, generic_checks=None, vector_ports=()):
    """Keep the upstream entity interface; replace only its architecture."""
    records = RECORD_FIELDS if record_fields is None else record_fields
    checks = GENERICS.get(top, {}) if generic_checks is None else generic_checks
    fields = []
    for name, width, direction in ports:
        if name in records:
            members = records[name]
            if sum(w for _, w in members) != width:
                raise ValueError(f"Invalid record layout: {name}")
            fields.extend((name + "." + field, w, direction) for field, w in members)
        elif width == 1 and name in vector_ports:
            fields.append((name + "(0)", width, direction))
        else:
            fields.append((name, width, direction))
    ports = fields
    inputs = [p for p in ports if p[2] == "in"]
    outputs = [p for p in ports if p[2] == "out"]
    n_in = sum(p[1] for p in inputs)
    n_out = sum(p[1] for p in outputs)
    declarations = f"""
architecture naja_cosim of {top} is
    procedure put_bit(index : integer; value : integer);
    attribute foreign of put_bit : procedure is "VHPIDIRECT naja_put_bit";
    procedure put_bit(index : integer; value : integer) is
    begin assert false report "unbound naja_put_bit" severity failure; end;
    procedure evaluate;
    attribute foreign of evaluate : procedure is "VHPIDIRECT naja_eval";
    procedure evaluate is
    begin assert false report "unbound naja_eval" severity failure; end;
    impure function get_bit(index : integer) return integer;
    attribute foreign of get_bit : function is "VHPIDIRECT naja_get_bit";
    impure function get_bit(index : integer) return integer is
    begin assert false report "unbound naja_get_bit" severity failure; return 0; end;
begin
{''.join(f'    assert {name} = {value} report "Naja export generic mismatch" severity failure;' for name, value in checks.items())}
    bridge : process(all)
        variable packed_inputs : std_ulogic_vector({n_in - 1} downto 0);
        variable packed_outputs : std_ulogic_vector({n_out - 1} downto 0);
    begin
"""
    body = ["        packed_inputs := " + " & ".join(p[0] for p in inputs) + ";"]
    # Verilator uses binary values. Keep an explicit count of metavalues rather
    # than claiming this tests VHDL's nine-state semantics. Some upstream tests
    # leave unused inputs uninitialized (e.g. rotator's sign_ext_rs).
    body += ["        for i in packed_inputs'range loop",
             "            put_bit(i, std_ulogic'pos(packed_inputs(i)));",
             "        end loop;", "        evaluate;",
             "        for i in packed_outputs'range loop",
             "            packed_outputs(i) := std_ulogic'val(get_bit(i));",
             "        end loop;"]
    cursor = n_out
    for name, width, _ in outputs:
        cursor -= width
        selection = str(cursor) if width == 1 else f"{cursor + width - 1} downto {cursor}"
        body.append(f"        {name} <= packed_outputs({selection});")
    return entity_interface(top, original) + declarations + "\n".join(body) + "\n    end process;\nend;\n"


def configured_dut(top, ports, original, generics=None):
    """Select the bench's generics without changing the upstream DUT source."""
    interface = entity_interface(top, original)
    values = GENERICS[top] if generics is None else generics
    declaration = re.search(r"(?i)\bgeneric\s*\(", interface)
    if declaration:
        # Skip the complete generic clause, including nested aggregate defaults.
        port = re.search(r"(?im)^\s*port\s*\(", interface)
        if not port or port.start() < declaration.start():
            raise ValueError(f"Cannot locate ports of {top}")
        interface = interface[:declaration.start()] + interface[port.start():]
    interface = re.sub(rf"\b{top}\b", "naja_configured", interface)
    for name, value in values.items():
        interface = re.sub(rf"\b{re.escape(name)}\b", str(value), interface, flags=re.I)
    generics = ", ".join(f"{key} => {value}" for key, value in values.items())
    connections = ", ".join(f"{name} => {name}" for name, _, _ in ports)
    return (interface + "\narchitecture rtl of naja_configured is begin\n"
            f"    dut : entity work.{top} generic map ({generics}) port map ({connections});\nend;\n")


def adapter(top, ports):
    sizes = {direction: sum(w for _, w, d in ports if d == direction) for direction in ("in", "out")}
    lines = [f"module naja_bridge(input [{sizes['in'] - 1}:0] inputs,",
             f"                   output [{sizes['out'] - 1}:0] outputs);", f"  {top} dut ("]
    cursors = dict(sizes)
    connections = []
    for name, width, direction in ports:
        cursors[direction] -= width
        low = cursors[direction]
        selection = str(low) if width == 1 else f"{low + width - 1}:{low}"
        bus = "inputs" if direction == "in" else "outputs"
        connections.append(f"    .{name}({bus}[{selection}])")
    return "\n".join(lines) + "\n" + ",\n".join(connections) + "\n  );\nendmodule\n"


def cpp_bridge(n_in, n_out):
    def access(bus, size):
        if size > 64:
            return f"model.{bus}[index / 32]", "index % 32", "1U"
        return f"model.{bus}", "index", "1ULL"
    in_word, in_shift, in_one = access("inputs", n_in)
    out_word, out_shift, out_one = access("outputs", n_out)
    return f"""#include "Vnaja_bridge.h"
#include "verilated.h"
#include <cstdlib>
#include <cstdio>
static VerilatedContext context;
static Vnaja_bridge model{{&context}};
static unsigned long long evaluations = 0;
static unsigned long long metavalues = 0;
double sc_time_stamp() {{ return 0; }}
extern "C" void naja_put_bit(int index, int value) {{
    if (index < 0 || index >= {n_in} || value < 0 || value > 8) std::abort();
    if (value != 2 && value != 3 && value != 6 && value != 7) ++metavalues;
    const auto mask = {in_one} << ({in_shift});
    {in_word} = ({in_word} & ~mask) | (value == 3 || value == 7 ? mask : 0);
}}
extern "C" void naja_eval() {{ model.eval(); ++evaluations; }}
extern "C" int naja_get_bit(int index) {{
    if (index < 0 || index >= {n_out}) std::abort();
    return ({out_word} & ({out_one} << ({out_shift}))) ? 3 : 2;
}}
struct Completion {{ ~Completion() {{
    model.final();
    std::printf("NAJA_COSIM_EVALUATIONS=%llu\\n", evaluations);
    std::printf("NAJA_COSIM_METAVALUES=%llu\\n", metavalues);
}} }};
static Completion completion;
"""


def vunit_run(source, case, work, backend, link_flags):
    # Compile the original bench and its packages, replacing only the DUT.
    # No copied/modified upstream testbench is needed.
    code = f"""import os
os.environ['VUNIT_SIMULATOR'] = 'ghdl'
from vunit import VUnit
vu = VUnit.from_argv(compile_builtins=False)
vu.add_vhdl_builtins()
vu.add_osvvm()
lib = vu.add_library('lib')
"""
    for name in BASE_SOURCES:
        code += f"lib.add_source_file({str(source / (name + '.vhdl'))!r})\n"
    dut = source / (case + ".vhdl") if backend == "reference" else work / "dut_wrapper.vhdl"
    code += f"lib.add_source_file({str(dut)!r})\n"
    code += f"lib.add_source_file({str(source / (case + '_tb.vhdl'))!r})\n"
    code += "vu.set_sim_option('disable_ieee_warnings', True)\n"
    code += f"vu.set_sim_option('ghdl.elab_flags', {link_flags!r})\n"
    code += "vu.set_sim_option('ghdl.sim_flags', ['--stop-time=10ms'])\nvu.main()\n"
    script = work / f"run_{backend}.py"
    script.write_text(code)
    return [sys.executable, script, "--output-path", work / backend,
            "--xunit-xml", work / f"{backend}.xml", "--no-color"]


def execute_case(args, case, env):
    import naja
    top_name, ports = CASES[case]
    work = args.output / case
    work.mkdir(parents=True, exist_ok=True)
    print(f"{case}: running upstream reference bench", flush=True)
    run(vunit_run(args.source, case, work, "reference", []), work,
        work / "reference.log", env=env)
    print(f"{case}: elaborating and exporting with Naja", flush=True)
    naja.NLUniverse.create()
    try:
        db = naja.NLDB.create(naja.NLUniverse.get())
        sources = [str(args.source / (s + ".vhdl")) for s in BASE_SOURCES + [case]]
        export_top = top_name
        if top_name in GENERICS:
            configured = work / "configured.vhdl"
            configured.write_text(configured_dut(top_name, ports, (args.source / (case + ".vhdl")).read_text()))
            sources.append(str(configured))
            export_top = "naja_configured"
        top = db.loadVHDL(sources, top=export_top,
                          diagnostics_report_path=str(work / "diagnostics.log"))
        actual = [(t.getName(), t.getWidth(), "in" if t.getDirection() == naja.SNLTerm.Direction.Input else "out")
                  for t in top.getTerms()]
        if actual != ports:
            raise RuntimeError(f"DUT interface changed: {actual!r}")
        export = work / "verilog"
        export.mkdir(exist_ok=True)
        top.dumpGateLevelVerilog(str(export), f"{top_name}.v")
    finally:
        naja.NLUniverse.get().destroy()
    (work / "dut_wrapper.vhdl").write_text(wrapper(top_name, ports, (args.source / (case + ".vhdl")).read_text()))
    (work / "adapter.sv").write_text(adapter(export_top, ports))
    (work / "bridge.cpp").write_text(cpp_bridge(
        sum(w for _, w, d in ports if d == "in"), sum(w for _, w, d in ports if d == "out")))
    obj = work / "obj_dir"
    run([args.verilator, "--cc", "--build", "--top-module", "naja_bridge", "--Mdir", obj,
         "--prefix", "Vnaja_bridge", "-Wno-fatal", "-CFLAGS", "-fPIC",
         work / "adapter.sv", *sorted(export.glob("*.v")), work / "bridge.cpp"],
        work, work / "verilator.log", env=env)
    # Link the static Verilated model and bridge into each GHDL simulation.
    # Verilator's generated makefile builds the supplied bridge.cpp for us.
    root_text = subprocess.check_output([args.verilator, "--getenv", "VERILATOR_ROOT"], text=True).strip()
    include = Path(root_text) / "include"
    objects = [obj / "bridge.o", obj / "Vnaja_bridge__ALL.a"]
    for runtime in ("verilated", "verilated_threads"):
        target = obj / (runtime + ".o")
        run([args.cxx, "-std=c++17", "-fPIC", "-pthread", "-I" + str(include),
             "-I" + str(include / "vltstd"), "-c", include / (runtime + ".cpp"), "-o", target],
            work, work / (runtime + ".log"), env=env)
        objects.append(target)
    flags = ["-Wl," + str(p) for p in objects]
    flags += ["-Wl,-lc++" if sys.platform == "darwin" else "-Wl,-lstdc++", "-Wl,-lpthread"]
    print(f"{case}: running original bench against Naja/Verilator", flush=True)
    run(vunit_run(args.source, case, work, "naja", flags), work, work / "naja.log", env=env)
    # A timeout/empty test selection must never count as success.
    reports = {}
    for backend in ("reference", "naja"):
        root = ET.parse(work / f"{backend}.xml").getroot()
        tests = root.findall(".//testcase")
        if not tests or any(any(t.find(tag) is not None for tag in ("failure", "error", "skipped")) for t in tests):
            raise RuntimeError(f"Incomplete or failed {backend} test report")
        reports[backend] = sorted((t.get("classname"), t.get("name")) for t in tests)
    if reports["reference"] != reports["naja"]:
        raise RuntimeError("Reference and Naja ran different tests")
    markers = list((work / "naja" / "test_output").glob("*/output.txt"))
    counts = [int(x) for p in markers for x in re.findall(r"NAJA_COSIM_EVALUATIONS=(\d+)", p.read_text())]
    if len(counts) != len(reports["naja"]) or not all(counts):
        raise RuntimeError("Not every bench exercised the Verilated DUT")
    metas = [int(x) for p in markers for x in re.findall(r"NAJA_COSIM_METAVALUES=(\d+)", p.read_text())]
    result = {"tests": len(reports["naja"]), "evaluations": sum(counts),
              "metavalue_input_bits_mapped_to_zero": sum(metas),
              "generics": GENERICS.get(top_name, {}), "status": "passed"}
    print(f"{case}: PASS {result}", flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True, help="Microwatt checkout (unmodified)")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cases", nargs="+", choices=CASES, default=list(CASES))
    parser.add_argument("--ghdl", default="ghdl")
    parser.add_argument("--verilator", default="verilator")
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    args.source = args.source.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, VUNIT_SIMULATOR="ghdl")
    for tool in ("ghdl", "verilator", "cxx"):
        resolved = shutil.which(getattr(args, tool))
        if not resolved:
            parser.error(f"Missing tool: {getattr(args, tool)}")
        setattr(args, tool, resolved)
    env["VUNIT_GHDL_PATH"] = str(Path(args.ghdl).parent)
    env["GHDL"] = Path(args.ghdl).name
    import naja
    sources = sorted(set(BASE_SOURCES + args.cases + [c + "_tb" for c in args.cases]))
    tool_versions = {tool: subprocess.check_output([getattr(args, tool), "--version"], text=True).splitlines()[0]
                     for tool in ("ghdl", "verilator", "cxx")}
    report = {"recommended_revision": REVISION, "naja_version": naja.getVersion(),
              "naja_git_hash": naja.getGitHash(),
              "tools": tool_versions,
              "source_sha256": {s + ".vhdl": hashlib.sha256((args.source / (s + ".vhdl")).read_bytes()).hexdigest()
                                for s in sources}, "cases": {}}
    report_path = args.output / "report.json"
    try:
        for case in args.cases:
            report["cases"][case] = execute_case(args, case, env)
    except Exception as error:
        report["error"] = str(error)
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
