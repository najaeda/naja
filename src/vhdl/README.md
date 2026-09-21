# Independent VHDL frontend

This directory is the future standalone repository root. It contains the
VHDL-2008 semantic probes and a Python-standard-library runner, plus an initial
handwritten C++20 lexer and recursive-descent parser. The parser handles entity
ports, internal signal declarations, concurrent and conditional signal
assignments, and multiple scheduled writes in a restricted event-guarded process. The initial analyzer binds architectures to entities and
resolves names in assignment values, conditions, and clock guards. Type analysis and
elaboration are not implemented yet. Nothing
here imports, links or discovers Naja/SNL, or requires the parent build.

Naja currently has a deliberately narrow integration proof in
`src/nl/formats/vhdl`: scalar `bit` ports and one concurrent conditional
assignment are lowered to the shared SNL mux primitive. One clocked process
with internal signals and distinct scheduled targets uses the shared DFF builder. The proof rejects
nine-valued `std_logic` ports and every unsupported shape before creating a
design. It does not provide general VHDL type analysis or elaboration.

The frontend uses handwritten lexing and will use recursive descent parsing.
Its language model and VHDL-specific elaboration services will live here. The
Naja adapter and shared SV/VHDL hardware construction will remain outside this
directory. In-tree hosting is temporary; extraction is a required milestone.

## Build the standalone C++ slice

```sh
cmake -S . -B /tmp/naja-vhdl-build -DCMAKE_BUILD_TYPE=Release \
  -DVHDL_GTEST_SOURCE_DIR=/path/to/googletest
cmake --build /tmp/naja-vhdl-build
ctest --test-dir /tmp/naja-vhdl-build --output-on-failure
```

Tests use GoogleTest. Provide an installed CMake package for GTest, or point
`VHDL_GTEST_SOURCE_DIR` at a GoogleTest checkout when configuring. The extracted
repository will pin GoogleTest as its own test-only dependency (the current Naja
checkout pins commit `063de7e9578f82b369302001269680b4b1553359`). The exported
CMake target is `vhdl::frontend`. Tests cover basic identifier case
handling, extended identifiers, literal/apostrophe distinction, compound symbols,
source locations, port groups and ranges, expression precedence, and malformed
input recovery. They do not imply broad language-conformance coverage.

## Run the reference probes

Use an installed GHDL **6.0.0**, then run from this directory:

```sh
python3 tests/semantic/run.py --ghdl /path/to/ghdl --report /tmp/vhdl-results.json
python3 tests/semantic/run_nvc.py --nvc /path/to/nvc --report /tmp/vhdl-nvc-results.json
python3 -m unittest discover -s tests/semantic -p test_runner.py
```

`tests/semantic/reference.json` records the exact tested distribution and source
package hashes. The runner checks the compiler version; it does not assume a
distribution layout or enforce those package hashes on other installations.
The asset hash pins the archive used for the baseline, not every GHDL 6.0.0 build.

The runner enforces the version, uses VHDL-2008 mode, and creates a fresh work
directory for every case. It records commands, exit statuses, diagnostics,
source fingerprints and compiler identity in the JSON report. Per-command
wall-clock limits supplement simulation time/delta limits. Successful cases
must reach their own completion marker; rejected cases must fail at an expected
stage with the expected diagnostic. A crash or timeout never counts as a pass.
Use repeated `--case NAME` arguments to select probes.

`tests/semantic/manifest.json` is the case inventory. The 35 probes exercise
identifiers/literals, overload resolution, array bounds/direction, numeric and
aggregate semantics, packages, hierarchy/binding, process scheduling, and
expected errors. `analysis_only` marks behavior outside the initial binary
hardware profile, such as resolved multi-driver logic and weak clock levels.
Testbench assertions, delays and termination are reference-test infrastructure,
not a proposed synthesizable language subset. `rtl` labels an intended semantic
area, not implemented frontend support.

All cases are locally authored under Apache-2.0. Standard packages come from the
selected reference installation; they are not copied into this tree. These tests
establish a reference baseline, not standards conformance or Naja VHDL support.
A second independent runtime comparison is recorded in
`tests/semantic/reference_nvc_1.23.0.json`. It covers the same 35 probes; the
report records three diagnostic or detection-stage differences from the GHDL
baseline, so those should not be mistaken for frontend behavior. The NVC runner
pins version 1.23.0 and records its version-specific diagnostic and stage
overrides in `tests/semantic/nvc_expectations.json`.

To check independence, copy this directory alone to a temporary location and run
the same commands there. The standalone CMake library and exported target can also be built, installed
and consumed without any Naja/SNL dependency.

## Scalar clocked scheduling proof

The Phase 1 adapter accepts one event-guarded process over scalar `bit`
ports and internal scalar `bit` signals, including grouped declarations:

```vhdl
entity pipeline is
  port (clk, d : in bit; q : out bit);
end;
architecture rtl of pipeline is
  signal stage : bit;
begin
  process(clk) begin
    if clk'event and clk = '1' then
      stage <= d;
      q <= stage;
    end if;
  end process;
end;
```

The sensitivity, event and level names must resolve to the same input port.
Each assignment targets a distinct output port or internal signal; its RHS is
an input port or internal signal name (parentheses are accepted). Every internal
signal must have exactly one assignment in this process. Basic names are case
insensitive. Output-port reads remain unsupported. Multiple processes and mixing
concurrent assignments with the process are rejected, even for disjoint drivers.

The standalone parser retains declaration groups, ordered assignments and source
spans; the analyzer resolves every internal name and diagnoses duplicates and
missing declarations without importing Naja/SNL. The adapter validates all types,
names, modes and supported driver patterns before creating any design. It creates
all signal nets first, then calls `SNLRTLPrimitives::createDFF()` for each write.
Every RHS connects to the current signal net, never an earlier assignment's RHS.
Consequently `q` captures the previous `stage` value in either source order.
Repeated writes to one target are rejected rather than implementing VHDL's
last-write scheduling rule incorrectly.

`PipelineConnectivityAndCycles` verifies both canonical DFF models, shared clock,
and data/output connectivity, reversed writes, case variations, parentheses and
multiple internal signals. A small test evaluator samples all canonical DFF data
nets together. `VHDLPipelineReference` compares its output against NVC for both
source orders using the same `pipeline.vhd` fixture and eight input cycles; the
reference also checks stability on data changes and falling edges. CMake enables
this comparison when NVC and Python are available. From the Naja repository root, run it explicitly with:

```sh
python3 test/nl/formats/vhdl/compare_pipeline.py --nvc /path/to/nvc \
  --adapter build-vhdl-feasibility/test/nl/formats/vhdl/snlVHDLConstructorTests
```

This proof promises clocked behavior after the pipeline fills, not power-up
state equivalence. VHDL `bit` defaults to `'0'`; the canonical hardware DFF has
no initialization guarantee. The evaluator begins with unknown register values
and the reference comparison starts after two rising edges. Explicit initializers
(including `'0'`) are rejected. No initialization metadata is silently discarded.

Unsupported forms include reset/enable or other nested control flow, falling-edge
registers, variables, repeated targets, multiple drivers, undriven internal
signals, timing/waveforms (`after`, `transport`, `reject`, `wait`), vector and
nine-valued types, expressions beyond names, and function calls (including
`rising_edge`). Parser errors or adapter diagnostics reject these before design
publication. This remains a narrow scheduling proof: general type analysis,
variables, hierarchy, vectors, source-rich adapter diagnostics, and full Phase 1
coverage remain future work.

Validation (2026-09-21): 29 focused CMake tests passed in
`build-vhdl-feasibility`, including the NVC 1.23.0 reference comparison for both
write orders and the existing SV mux/register checks. All 18 standalone tests
passed from a temporary copy, followed by installation and a separate client
linked only to the exported `vhdl::frontend` target. The local LLVM build used
`-DCMAKE_CXX_SCAN_FOR_MODULES=OFF` to avoid a stale dependency-scanner path.
