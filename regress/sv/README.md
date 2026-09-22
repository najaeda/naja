<!---
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# SystemVerilog External Regress

This regress checks out pinned external RTL repositories, loads them into Naja,
and runs structural, Verilog generation, lint, or simulation checks.

## Layout

```text
regress/sv/
├── sv_regress.py       # stable runner entry point
├── cases.yml           # pinned repositories and stage configuration
├── cases/              # per-design adapters, requirements, and testbenches
│   ├── axi/
│   ├── borg/
│   ├── cv32e40p/
│   ├── cva6/
│   ├── ibex/
│   └── zcore/
├── common/             # shared analysis helpers
└── tests/              # runner and adapter unit tests
```

Run the focused tests with `python3 -m pytest regress/sv/tests/`.
The CLI and manifest paths are unchanged; stage names such as `helloworld_sim`
remain unchanged too.

## Prerequisites

- Build Naja so `build/test/najaeda` exists.
- Install Python dependencies, including `PyYAML`.
- Install `fusesoc` for the Ibex setup flow.
- Install Docker for the default lint and GitHub smoke-simulation runs.
- Install local `verilator` when using `--lint-runner local` or `helloworld_sim`.
- Install a RISC-V firmware toolchain for `helloworld_sim`. The scripts prefer the
  `riscv32-unknown-elf-` prefix and also accept `riscv-none-elf-` and
  `riscv64-unknown-elf-`. `RISCV` is inferred from `PATH` when unset for
  CV32E40P. CV32E40P's upstream testbench links its firmware against newlib; the
  GitHub workflow installs xPack GNU RISC-V Embedded GCC for that reason.

GitHub CI installs the Python dependencies and uses Dockerized Verilator. The
`External SV Regress` workflow runs lint for the small external designs and
load/dump checks for the large BlackParrot and CVA6 designs, plus Borg's
load/dump, lint, and pin-level cocotb regression. The `External SV
Simulation` workflow runs the checked-in Ibex and CV32E40P smoke simulations
plus the Ibex helloworld simulation, SecureIbex diagnostics, the CV32E40P
helloworld simulation, and an upstream CV32E40P interrupt simulation.

## List Cases

```sh
python3 regress/sv/sv_regress.py list
```

## Borg ASIC Semantic Round-Trip

The `borg` case pins gonsolo/Borg to
`fe915b3763768c7f49be18cc541ad6c96e9de5d8` and targets
`tt_um_gonsolo_borg`. The flow is Chisel-emitted ASIC SystemVerilog → Naja
load/elaboration → structural Verilog dump → upstream pin-level cocotb tests.
No generated Borg RTL is vendored.

Prerequisites in addition to a built Naja/PyYAML environment:

- Mill **1.1.2** (`BORG_MILL` may name an explicit executable). Its launcher
  resolves its versioned JVM/Scala dependencies; the pinned Borg build uses
  Scala 2.13.18 and Chisel 7.15.0, including Chisel's versioned firtool resolver.
  The first run needs network access to Maven Central and the JVM provider.
- A Python environment with `systemrdl-compiler==1.32.2`,
  `peakrdl-cheader==1.1.0`, and Jinja2. Set `BORG_PYTHON` to its Python executable
  if it differs from the Naja runner's interpreter.
- Icarus Verilog (`iverilog`, `vvp`), `cocotb-config` from **cocotb 2.0.1**,
  `riscv-model==0.6.6`, and NumPy. Python 3.13 is supported by this cocotb
  release; Python 3.14 is not. `cocotb-config` must belong to the simulation
  Python environment. The adapter asks it for the Python binary and libpython,
  keeping that ABI separate from Naja's compiled Python bindings.

For example, create a separate environment for RDL/cocotb if Naja is built
against Python 3.14:

```sh
python3.13 -m venv /tmp/naja-borg-tools
/tmp/naja-borg-tools/bin/pip install \
  -r regress/sv/cases/borg/requirements.txt
export BORG_PYTHON=/tmp/naja-borg-tools/bin/python
export PATH="$PATH:/tmp/naja-borg-tools/bin"
```

If Mill is not installed, download the exact version's launcher to a file,
inspect it, then make it executable (no unpinned download or `curl | sh`):

```sh
curl -L --fail \
  https://repo.maven.apache.org/maven2/com/lihaoyi/mill-dist/1.1.2/mill-dist-1.1.2-mill.sh \
  -o /tmp/naja-borg-mill-1.1.2
chmod +x /tmp/naja-borg-mill-1.1.2
export BORG_MILL=/tmp/naja-borg-mill-1.1.2
```

This is Mill's [documented launcher installation flow](https://mill-build.org/mill/cli/installation-ide.html),
with the version fixed for this regression. The generator verifies the actual
Mill version and rejects a mismatch. Nix, PDK libraries, a firmware compiler,
and GNU sed are not required.

Run:

```sh
python3 -m pytest regress/sv/tests/test_sv_regress.py
python3 regress/sv/sv_regress.py list
python3 regress/sv/sv_regress.py run --case borg --stage load_dump
python3 regress/sv/sv_regress.py run --case borg --stage lint --lint-runner local
python3 regress/sv/sv_regress.py run \
  --case borg --stage borg_cocotb_sim --require-firmware-sim-tools
```

Keep `python3` on PATH in Naja's built Python environment. Only `BORG_PYTHON`
and `cocotb-config` need to point to the separate tools environment. As with
other configured simulation stages, missing optional simulation executables
skip unless `--require-firmware-sim-tools` is supplied. **RTL generation is
required and never silently skips**; missing Mill/RDL/firtool dependencies fail
even without that option.

`cases/borg/borg_generate.py` builds in `artifacts/rtl-work`, leaving the pinned checkout
unmodified. Only the pinned `PeakRDL-chisel` submodule is initialized (not Mesa,
Vulkan-Tools, or tapeout tools). The adapter invokes upstream RDL validation
and generation, then `CLOCK_MHZ=4 mill --no-server -j 1 asic.tt.runMain
asic.tt.TTMain` and upstream `init_bram_zero.py`. This preserves the semantics
of `make generate_verilog`, including its zero-initialized cache memory and
translate-on cleanup, without the Nix-specific Python shim or GNU sed.
Debug-trace environment switches are disabled for reproducibility.

Source selection reads **`asic_files.txt`**, resolving its paths relative to
upstream `src/`. TTMain forms that union from each firtool `filelist.f`; the
last `filelist.f` alone is insufficient. Both elaborations retain
`BorgConfig.Asic` and the same 4 MHz clock, preventing configuration-dependent
module overwrites. No broad RTL glob is used to reconstruct the list. SIM and
memory-initializer defines are enabled at Naja loading.

`cases/borg/borg_cocotb_sim.py` compiles exactly:

1. `artifacts/borg_naja.v`;
2. Naja's `test/nl/formats/systemverilog/benchmarks/najaeda_primitives.v`;
3. upstream `test/soc/tb.v`.

It imports the upstream Python tests/helpers and the freshly RDL-generated
`borg_mmio.py`, and reuses upstream test vectors. It runs exactly the core
pin-level smoke test `test.test_start` and peripheral ADD/MUL/FMA math test
`user_peripherals.borg.test.test_borg_shader_math_batch`, in separate simulator
processes. Firmware boot, Scala/Chisel unit tests, PDK simulation, FPGA/GDS,
Arcilator, and the framebuffer harness are not included. Chisel
`EphemeralSimulator` tests elaborate typed Chisel modules; they cannot replace
their DUT with this external Verilog netlist.

All generated design/build/simulation products are under
`build/sv-regress/borg/artifacts/`:

- `borg.flist`, `borg-source-selection.json`, and `rtl-work/.../asic_files.txt`:
  audit the exact ASIC sources and configuration;
- `borg-*-command.json`: exact RDL/Mill/post-step commands and working dirs;
- `borg_naja.v`, `design-stats.json`, `diagnostics.log`, and `summary.json`;
- `borg-cocotb-sim-build-command.json`: actual three-source Icarus command;
- `borg-cocotb-sim-{core,math}-run-command.json`: exact vvp commands and
  simulation environment;
- `borg-cocotb-sim/{core,math}-results.xml` and
  `borg-cocotb-sim-results.json`: explicit selected-test results;
- stable `logs/borg-cocotb-{build,core,math}.log` and combined
  `logs/borg-cocotb-sim.log`.

The pass indication is **`BORG_COCOTB_SIM_PASS`**, emitted only after fresh XML
reports exactly the expected passing test in both runs. Simulator exit alone,
missing/empty XML, selected-test skips, failures, errors, and stale results
cannot pass. Cocotb's explicitly filtered-out tests may appear as skipped XML
entries; these do not count as executed tests. The artifact-local `precision.f`
sets Icarus's [default timescale](https://steveicarus.github.io/iverilog/usage/command_files.html)
to 1ns/1ps for the structural dump and primitives, so upstream helpers' 2 ps
delays can be represented without modifying the 1ns/100ps upstream testbench.
Waveform dumping is disabled with vvp's `-none` argument to bound artifact size.

This is a **Naja structural netlist**, with portable behavioral models of
Naja's inferred operators, flops, and memories—not a technology-mapped PDK
gate-level netlist. The adapter never sets `GATES=yes`/`GL_TEST`, invokes the
upstream simulation Makefile, or compiles original Chisel-emitted RTL alongside
the dump. PDK cell models and power-pin glue are deliberately excluded.

Local macOS verification passed all three stages with 37 selected ASIC input
sources and no Naja unsupported diagnostics. Cold Scala/Chisel generation took
about 69 seconds; warm generation about 9 seconds, Naja load/dump about 13
seconds, local lint 21–27 seconds (about 2 GB allocated), and the complete
cocotb adapter about 110 seconds (including compile/startup). Both tests passed:
core smoke about 39 seconds and peripheral math about 47 seconds.

The central `External SV Regress` workflow (`.github/workflows/sv-regress.yml`)
runs all three Borg stages on pull requests and pushes to `main`, and supports
manual dispatch. It installs Icarus, the pinned Mill 1.1.2 launcher, and
`cases/borg/requirements.txt` in a separate Python 3.13 environment. Only Mill
and `cocotb-config` are added to PATH, preserving the runner's Python 3.12
interpreter and Naja bindings. The job uses `--require-firmware-sim-tools`, so
missing simulation tools fail rather than skip. Borg's logs and test results
are included in the workflow's always-uploaded `sv-regress-artifacts` artifact.

For a second tier, BorgSimTop plus the existing Verilator `cts-uart` replay and
`compare_ppm.py` golden-image comparison is a practical candidate. At this pin,
`BorgSimMain` emits the four-lane `BorgConfig.Simt` design into a separate
directory and collects `sim_files.txt`; it must not share the ASIC build's
module files. The C++ wrapper loads/reads SDRAM through public `dbg_*` top-level
ports, rather than relying on internal signal names. That tier would need a
Naja-only Verilator source adapter, a firmware toolchain/build, support for the
simulation SDRAM model, and a runtime/memory budget for up to 100 million
cycles. Upstream's runner compares the captured render with the pinned golden
using `--max-diff 1 --max-fail-pixels 2`. It is deliberately not implemented here.

## Default Run

The default `run` stages are `lint` and `github_sim`.

```sh
python3 regress/sv/sv_regress.py run --case ibex --case cv32e40p
```

This matches the lightweight CI intent: Verilator lint plus checked-in smoke
simulation testbenches.

## Large Design Load/Dump

BlackParrot and CVA6 are too large for GitHub-hosted simulation jobs, so CI
only checks that Naja can load, elaborate, and dump them back to Verilog:

```sh
python3 regress/sv/sv_regress.py run \
  --case black_parrot \
  --case cva6 \
  --stage load_dump
```

The generated Verilog and `load-dump.log` are written under each case artifact
directory.

## Z-Core Full Dump Verification

The Z-Core case follows the pinned upstream `tb/Makefile` source-list and
simulation model. It elaborates `z_core_top`, dumps the complete reachable
design, lints that complete dump, and runs a top-level reset/execution smoke
simulation against the same dump:

```sh
python3 regress/sv/sv_regress.py run \
  --case zcore \
  --stage load_dump \
  --stage lint \
  --stage github_sim
```

The simulation expects `ZCORE_TOP_SMOKE_PASS` after reset and 200 full-SoC
clock cycles, and rejects unknown UART or GPIO outputs.

## Logic-Cone Signatures

The `logic_cones` stage builds manifest-selected cones while the pinned external
design is loaded, before dumping Verilog. It checks stable structural counts:
nodes, edges, leaves, roots, registers, ports, blackboxes, and internal nodes.

```sh
python3 regress/sv/sv_regress.py run \
  --case cva6 \
  --stage logic_cones
```

Probe roots and expected counts live under `logic_cones` in `cases.yml`. Actual
results are always written to
`build/sv-regress/<case>/artifacts/logic-cones.json`, including on a signature
mismatch, so an intentional update can be reviewed rather than guessed.

## Ibex Smoke Simulation

Run lint and the Ibex smoke simulation:

```sh
python3 regress/sv/sv_regress.py run --case ibex --stage lint --stage github_sim
```

Run only the Ibex smoke simulation stage:

```sh
python3 regress/sv/sv_regress.py run --case ibex --stage github_sim
```

The expected pass marker is:

```text
IBEX_SMOKE_PASS
```

The main smoke log is:

```text
build/sv-regress/ibex/artifacts/logs/github-sim.log
```

## CV32E40P Smoke Simulation

Run lint and the CV32E40P smoke simulation:

```sh
python3 regress/sv/sv_regress.py run --case cv32e40p --stage lint --stage github_sim
```

Run only the CV32E40P smoke simulation stage:

```sh
python3 regress/sv/sv_regress.py run --case cv32e40p --stage github_sim
```

The expected pass marker is:

```text
CV32E40P_SMOKE_PASS
```

The main smoke log is:

```text
build/sv-regress/cv32e40p/artifacts/logs/github-sim.log
```

## Local Verilator Lint

Docker remains the default lint runner. To use the local `verilator` binary for
lint:

```sh
python3 regress/sv/sv_regress.py run --case ibex --stage lint --lint-runner local
```

For both smoke cases:

```sh
python3 regress/sv/sv_regress.py run --case ibex --case cv32e40p --stage lint --lint-runner local
```

The lint log is:

```text
build/sv-regress/<case>/artifacts/logs/verilator-lint.log
```

## Helloworld Simulation Tier

The `helloworld_sim` stage is opt-in and runs real firmware in upstream-style
testbench environments:

- Ibex uses upstream `examples/simple_system`, replaces the original
  `ibex_top` with the Naja-generated `ibex_top`, builds `hello_test.vmem`, and
  expects `IBEX_HELLOWORLD_SIM_PASS`.
- The `ibex_dit_sim` and `ibex_dummy_instr_sim` stages reuse the same upstream
  simple-system environment to run the self-checking `dit_test` and
  `dummy_instr_test` programs. They generate a separate `ibex_secure_naja.v`
  with `-GSecureIbex=1`, then compile the upstream simple-system wrapper with
  matching SecureIbex memory-integrity wiring.
- The `ibex_pmp_sim` stage generates a separate `ibex_pmp_naja.v` with
  `-GPMPEnable=1`, then runs upstream `pmp_smoke_test` and checks that the
  protected store raises the expected store access fault.
- The `ibex_secure_dit_sim` and `ibex_secure_dummy_instr_sim` stages generate a
  separate `ibex_secure_naja.v` with `-GSecureIbex=1`, then compile the upstream
  simple-system wrapper with matching SecureIbex memory-integrity wiring. These
  are compatibility aliases for the secure configuration.
- CV32E40P uses upstream `example_tb/core`, replaces the original
  `cv32e40p_top` with the Naja-generated `cv32e40p_top`, builds
  `custom/hello_world.hex`, compiles the upstream Verilator testbench around
  the generated netlist, and expects `CV32E40P_HELLOWORLD_SIM_PASS`.
- The `cv32e40p_hwlp_sim` stage generates a separate `cv32e40p_pulp_naja.v`
  with `-GCOREV_PULP=1`, patches the upstream `hwlp_test` hardware-loop
  mnemonics into raw instruction words for stock RISC-V GNU toolchains, and
  expects `CV32E40P_HWLP_SIM_PASS`.
- CVA6 uses the upstream Verilator `ariane_testharness` and FESVR loader from
  the CVA6 repository, wraps the generated elaborated `cva6` netlist with a
  small `ariane` compatibility module, runs upstream `hello_world.c`, and
  expects `CVA6_HELLOWORLD_SIM_PASS`.

Case-specific stages must be launched per case:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage lint \
  --stage github_sim \
  --stage helloworld_sim \
  --stage ibex_pmp_sim \
  --stage ibex_dit_sim \
  --stage ibex_dummy_instr_sim

python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage lint \
  --stage github_sim \
  --stage helloworld_sim \
  --stage interrupt_sim \
  --stage cv32e40p_hwlp_sim

python3 regress/sv/sv_regress.py run \
  --case cva6_testharness \
  --stage lint \
  --stage helloworld_sim \
  --lint-runner local \
  --require-firmware-sim-tools
```

Run only helloworld simulation:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --case cv32e40p \
  --stage helloworld_sim
```

Missing optional firmware simulation tools are reported as skipped. To make missing
tools fail the command:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage helloworld_sim \
  --require-firmware-sim-tools
```

Run the extended Ibex simple-system diagnostics:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage ibex_pmp_sim \
  --stage ibex_dit_sim \
  --stage ibex_dummy_instr_sim \
  --require-firmware-sim-tools
```

Run the CV32E40P hardware-loop diagnostic:

```sh
python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage cv32e40p_hwlp_sim \
  --require-firmware-sim-tools
```

The helloworld simulation log is:

```text
build/sv-regress/<case>/artifacts/logs/helloworld-sim.log
```

## CV32E40P Extended Simulation Tier

The `interrupt_sim` stage runs the CV32E40P upstream `example_tb/core`
interrupt program with the same generated `cv32e40p_top` netlist and expects
`CV32E40P_INTERRUPT_SIM_PASS` when the generated netlist handles the interrupt
sequence correctly:

```sh
python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage interrupt_sim \
  --require-firmware-sim-tools
```

The `cv32e40p_hwlp_sim` stage runs the upstream `hwlp_test` program against a
`COREV_PULP=1` generated netlist:

```sh
python3 regress/sv/sv_regress.py run \
  --case cv32e40p \
  --stage cv32e40p_hwlp_sim \
  --require-firmware-sim-tools
```

The extended CV32E40P simulation logs are:

```text
build/sv-regress/cv32e40p/artifacts/logs/interrupt-sim.log
build/sv-regress/cv32e40p/artifacts/logs/cv32e40p-hwlp-sim.log
```

For local diagnosis, pass `--sim-plusarg +naja_irq_debug` to
`regress/sv/cases/cv32e40p/cv32e40p_example_tb.py` to print the interrupt mode,
pending word, visible IRQ lines, acknowledge ID, and core PC. Use
`--sim-plusarg +naja_irq_entry_debug` for a focused 96-cycle trace once Test 2
raises all IRQs. The same helper also accepts `--core-source rtl`, which builds
the upstream CV32E40P RTL from `cv32e40p_manifest.flist` into a separate
`interrupt_sim_rtl` work directory for reference comparison.

The core-v-verif UVM environment remains outside the GitHub open-tool tier
because it needs a full SystemVerilog/UVM commercial simulator stack. Its
Verilator-compatible CV32E40P core-testbench path overlaps with the upstream
CV32E40P core testbench used here, so the CI tier focuses on expanding those
firmware simulations.

## Artifacts

Per case, artifacts are written under:

```text
build/sv-regress/<case>/artifacts/
```

Useful files include:

```text
summary.json
logs/generate.log
logs/load-dump.log
logs/verilator-lint.log
logs/github-sim.log
logs/helloworld-sim.log
logs/ibex-dit-sim.log
logs/ibex-dummy-instr-sim.log
logs/interrupt-sim.log
logs/cv32e40p-hwlp-sim.log
ibex_secure.flist
ibex_secure_diagnostics.log
ibex_pmp.flist
ibex_pmp_diagnostics.log
cv32e40p_pulp.flist
cv32e40p_pulp_diagnostics.log
verilator-lint-command.json
github_sim-build-command.json
github_sim-run-command.json
helloworld-sim-command-<n>.json
ibex-dit-sim-command-<n>.json
ibex-dummy-instr-sim-command-<n>.json
interrupt-sim-command-<n>.json
cv32e40p-hwlp-sim-command-<n>.json
```

The legacy `verilator-command.json` is still written for compatibility and
contains the lint command.

## Clean

Remove the SV regress work directory:

```sh
python3 regress/sv/sv_regress.py clean
```
