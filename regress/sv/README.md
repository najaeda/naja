# SystemVerilog External Regress

This regress checks out pinned external RTL repositories, generates a Naja
Verilog netlist, then runs selected Verilator stages on that generated netlist.

## Prerequisites

- Build Naja so `build/test/najaeda` exists.
- Install Python dependencies, including `PyYAML`.
- Install `fusesoc` for the Ibex setup flow.
- Install Docker for the default Verilator runs, or install local `verilator`
  when using `--lint-runner local`.

GitHub CI installs the Python dependencies and uses Dockerized Verilator.

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

## Local Simulation Tier

The `local_sim` stage is opt-in. It is intended for richer local Verilator
flows that can reuse more of the upstream verification environment.

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --case cv32e40p \
  --stage lint \
  --stage github_sim \
  --stage local_sim
```

Missing optional local simulation tools are reported as skipped. To make missing
tools fail the command:

```sh
python3 regress/sv/sv_regress.py run \
  --case ibex \
  --stage local_sim \
  --require-local-sim-tools
```

The local simulation log is:

```text
build/sv-regress/<case>/artifacts/logs/local-sim.log
```

## Artifacts

Per case, artifacts are written under:

```text
build/sv-regress/<case>/artifacts/
```

Useful files include:

```text
summary.json
logs/generate.log
logs/verilator-lint.log
logs/github-sim.log
logs/local-sim.log
verilator-lint-command.json
github_sim-build-command.json
github_sim-run-command.json
local-sim-command-<n>.json
```

The legacy `verilator-command.json` is still written for compatibility and
contains the lint command.

## Clean

Remove the SV regress work directory:

```sh
python3 regress/sv/sv_regress.py clean
```
