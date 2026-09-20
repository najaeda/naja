# Independent VHDL frontend

This directory is the future standalone repository root. It currently contains
the first feasibility artifact: executable VHDL-2008 semantic probes and a
Python-standard-library runner. There is no native VHDL parser or analyzer yet.
Nothing here imports, links or discovers Naja/SNL, or requires the parent build.

The planned C++ frontend will use handwritten lexing and recursive descent.
Its language model and VHDL-specific elaboration services will live here. The
Naja adapter and shared SV/VHDL hardware construction will remain outside this
directory. In-tree hosting is temporary; extraction is a required milestone.

## Run the reference probes

Use an installed GHDL **6.0.0**, then run from this directory:

```sh
python3 tests/semantic/run.py --ghdl /path/to/ghdl --report /tmp/vhdl-results.json
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
A second independent reference and a real RTL corpus remain necessary.

To check independence, copy this directory alone to a temporary location and run
the same commands there. A future standalone CMake library/exported consumer
check will complement this executable test boundary when native code is added.
