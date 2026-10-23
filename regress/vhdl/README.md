<!--
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# Microwatt VHDL co-simulation

`microwatt_cosim.py` runs Microwatt's original VUnit/OSVVM testbenches twice:
first against the original VHDL DUT in GHDL, then against Naja's exported
Verilog in Verilator. The stimulus, random seeds, behavioral reference
functions, assertions, clock generation and checking times remain in the
unmodified upstream VHDL bench. A generated VHDL DUT architecture calls the
Verilated model through GHDL's VHPIDIRECT interface. Both runs must pass the
same nonempty test set, and every co-simulation run must evaluate the exported
DUT. Process timeouts, stopped/incomplete tests and missing bridge counters
are failures. No NajaIF snapshot is used.

The default cases are:

| Case | Tests | Upstream checks |
| --- | ---: | --- |
| `rotator` | 13 | Rotates, shifts, masks, sign extension and carry |
| `countbits` | 3 | Leading/trailing zero counts and clocked latency |
| `multiply` | 8 | Interface latency and signed/unsigned low/high/word products |
| `divider` | 13 | Interface latency, signed/unsigned and extended division, remainders |

The countbits bench covers zero counting; it does not exercise every function
implemented by the DUT. The multiplier uses `PIPELINE_DEPTH=4`, matching the
upstream bench's explicit generic map. A generated configuration wrapper selects
that value during Naja elaboration; the simulation wrapper rejects a different
generic value. Multiplier/divider record ports are packed in field declaration
order at the bridge, retaining the original record interface in the VHDL bench.

These unit cases do not need the temporary dcache assignment-order patch. The
separate full-core smoke below currently does.

## Run locally

Use a Python interpreter matching the ABI of the built Naja extension. Install
`vunit_hdl==4.7.1` in that environment, plus Verilator, a C++ compiler, make,
and GHDL with its LLVM or GCC backend (not mcode). For example:

```sh
git clone https://github.com/antonblanchard/microwatt.git build/microwatt
git -C build/microwatt checkout f341bed306a9a7281bf65200df8cc1f0a002617d
cmake --build build --target naja --parallel 6
python3 -m pip install vunit_hdl==4.7.1
PYTHONPATH="$PWD/build/src/nl/python/naja_wrapping" \
  python3 regress/vhdl/microwatt_cosim.py \
    --source build/microwatt \
    --output build/microwatt-results \
    --ghdl ghdl-llvm
```

Use `--cases multiply divider` to select cases. An absolute path
to GHDL is also accepted. Verilator builds use four parallel jobs and a
600-second timeout by default; adjust these with `--jobs` and
`--build-timeout` (seconds). Generated C++ is split with
`--output-split 20000` to keep compilation units manageable. Simulation
commands retain their 300-second timeout.
Keep checkouts and generated files outside tracked
source directories. The runner records hashes of its input files, Naja's
producer identity, test counts and model evaluation counts in `report.json`.
The `reference.xml`/`naja.xml` reports and per-stage logs remain under each case
output directory. CI runs these cases in `.github/workflows/vhdl-regress.yml`
against the pinned revision above.

## Full-core console smoke

`microwatt_core_smoke.py` runs the upstream `core_tb` and SoC, including its
Wishbone interconnect, simulated RAM and UART, in GHDL. It first runs the original
core as a reference, then replaces only the core with Naja's exported Verilog.
The defaults cover all 16 upstream noninteractive console programs, containing
137 reported checks. Both executions must terminate and reproduce each program's
checked-in `.console_out` byte for byte. Reaching the simulation time limit is a
failure, even if partial console output matches.

Use `--build-timeout` to set the GHDL and Verilator build timeout in seconds
(default: 600). CI allows 1,800 seconds for the full-core build. Each program
retains its 300-second simulation timeout.

| Programs | Reported checks |
| --- | ---: |
| `test_branch_alias`, `test_misc`, `test_prefix` | 13 |
| `test_illegal`, `test_modes`, `test_privileged`, `test_sc`, `test_trace` | 30 |
| `test_reservation`, `test_mmu` | 24 |
| `test_fpu`, `test_hash` | 31 |
| `test_spr_read`, `test_decrementer`, `test_pmu`, `test_xics` | 39 |

Both cores use hardware mode (`SIM=false`) with the default FPU enabled and
the synthesis `nonrandom` provider. The surrounding SoC remains in simulation
mode. The core's `SIM=true` configuration includes non-synthesizable diagnostic
processes; it is not the DUT elaborated by Naja. A small GHDL wrapper selects
hardware mode for the reference and detects `terminated_out` for both backends,
allowing 100 ns before stopping. The bridge itself uses the same protocol as
the unit cases.

```sh
PYTHONPATH="$PWD/build/src/nl/python/naja_wrapping" \
  python3 -B regress/vhdl/microwatt_core_smoke.py \
    --source build/microwatt \
    --output build/microwatt-core-results \
    --preview-dcache-fix --ghdl ghdl-llvm --jobs 4
```

The preview flag authorizes moving the known `nc` assignment before its read
in an **output-directory source copy**, for both backends. The upstream checkout
is unchanged. Without the flag, the runner rejects the affected source. The
generated `dcache-preview.patch` and source hashes make the correction reviewable.
The correction becomes unnecessary when the upstream source already has the
assignment in the correct order.

Use `--programs test_branch_alias` for the smallest smoke test, or select any
subset of the programs above. `--stop-time` sets the per-program simulated time
budget; all defaults pass within the default 2 ms. `report.json` records the
configuration, binary and expected-console hashes, and results; build logs and
each program's console/simulation logs remain in the output directory. CI runs
all 16 programs in this explicitly named preview stage alongside the
unmodified-source unit tests.

This is a functional CPU-in-SoC smoke test, not comprehensive ISA validation,
cycle-by-cycle equivalence, or elaboration of the full SoC by Naja. The RAM and
peripheral models remain VHDL simulated by GHDL.

## Interpretation and limits

This checks the upstream bench's functional assertions against the exported
netlist; it is not exhaustive equivalence or timing simulation. GHDL owns time
and calls the zero-delay Verilated model whenever DUT inputs change. Outputs
return to VHDL through ordinary signal assignments, before the bench's next
sampling time.

Verilator uses binary values. The bridge preserves 0/1 and weak L/H levels,
and explicitly maps U/X/Z/W/- inputs to zero, counting those mappings in the
report. For example, several rotator tests leave the unused `sign_ext_rs`
control uninitialized. The original VHDL reference still runs without this
conversion and must pass every assertion. Passing the co-simulation validates
these tested binary results and latencies; it does not establish nine-state
VHDL semantics, unknown propagation or runtime `is_X` behavior. Initial unknown
register values in Verilator are likewise outside this check.

Microwatt sources remain external and retain their upstream CC-BY-4.0 license.
The runner does not modify them. Upstream references:

- [VUnit setup](https://github.com/antonblanchard/microwatt/blob/f341bed306a9a7281bf65200df8cc1f0a002617d/run.py)
- [Rotator bench](https://github.com/antonblanchard/microwatt/blob/f341bed306a9a7281bf65200df8cc1f0a002617d/rotator_tb.vhdl)
- [Countbits bench](https://github.com/antonblanchard/microwatt/blob/f341bed306a9a7281bf65200df8cc1f0a002617d/countbits_tb.vhdl)
- [Multiplier bench](https://github.com/antonblanchard/microwatt/blob/f341bed306a9a7281bf65200df8cc1f0a002617d/multiply_tb.vhdl)
- [Divider bench](https://github.com/antonblanchard/microwatt/blob/f341bed306a9a7281bf65200df8cc1f0a002617d/divider_tb.vhdl)
- [GHDL foreign interface](https://ghdl.github.io/ghdl-cosim/vhpidirect/declarations.html)
