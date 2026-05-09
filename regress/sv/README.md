<!---
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# SystemVerilog External Regress

This regress checks out pinned external RTL repositories, generates a Naja
Verilog netlist, then runs selected Verilator stages on that generated netlist.

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
load/dump checks for the large BlackParrot and CVA6 designs. The `External SV
Simulation` workflow runs the checked-in Ibex and CV32E40P smoke simulations
plus the Ibex helloworld simulation, SecureIbex diagnostics, the CV32E40P
helloworld simulation, and an upstream CV32E40P interrupt simulation.

## List Cases

```sh
python3 regress/sv/sv_regress.py list
```

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
`regress/sv/helloworld_sim/cv32e40p_example_tb.py` to print the interrupt mode,
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
