# Independent VHDL frontend

This directory is the future standalone repository root. It contains the
VHDL-2008 semantic probes and a Python-standard-library runner, plus an initial
handwritten C++20 lexer and recursive-descent parser. The parser handles entity
ports, simple architecture bodies and concurrent assignments. The initial
analyzer binds architectures to entities and resolves names in those concurrent
assignments. Type analysis and elaboration are not implemented yet. Nothing
here imports, links or discovers Naja/SNL, or requires the parent build.

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
the same commands there. A future standalone CMake library/exported consumer
check will complement this executable test boundary as native code grows.
