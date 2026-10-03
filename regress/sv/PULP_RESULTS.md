<!--
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# PULP EDA benchmark frontend survey

Measured on 2026-10-01 with Naja `927b9ca2` (0.7.27), rebuilt from source
using the existing macOS arm64 **Debug** CMake build and Python 3.14. The input is the
[prepared v0.1.0 release](https://github.com/pulp-platform/eda-benchmarks/releases/tag/v0.1.0),
SHA-256 `a27eaca9359cd2c0b646892325355e102ae563bd8f772c77f18697ff8c4ad603`.
This checksum also matches GitHub's published asset digest.

Every variant uses its supplied top, filelist, macros, wrappers, and source
stubs. No RTL fixes, best-effort hierarchy, or unknown-module blackboxing were
used. The initial survey applied a 90-second wall-clock limit per load/dump.
Timings are local observations, not Linux Release CI performance guarantees; some
follow-up tests ran concurrently with the survey.

## Recommended PR tier

| Target | Input sources | Load/dump | Complete-dump lint |
| --- | ---: | ---: | ---: |
| `cv32e40p.default` | 28 | ~7–9 s | ~4 s, pass |
| `cvfpu.fp32_fma` | 8 | ~1.5 s | ~1 s, pass |
| `ibex.minimal` | 45 | ~4–5 s | ~2 s, pass |
| `spi_host.default` | 25 | ~1 s | <1 s, pass |

These are the runner's default cases and the new `PULP SV Regress` workflow's
PR/push selection. FP32 FMA and SPI host add coverage beyond the existing
Ibex/CV32E40P external regressions. Those CPU cases also exercise this release's
specific source pins, configuration, and wrappers.

Local lint used Verilator 5.052 and the complete Naja dump plus
`najaeda_primitives.v`, with only `ASCRANGE` suppressed, matching the existing
external-regression convention. CI uses the existing pinned Verilator 5.046
Docker image. The Docker daemon was unavailable locally, so that image and the
Ubuntu GitHub job have not been executed here. No simulation or equivalence
check was performed; successful load/dump/lint does not prove functional
correctness.

`ibex.full` also loads and dumps in ~20–22 seconds but fails strict lint with
two `UNOPTFLAT` warnings involving the secure/lockstep shadow core's ALU. It is
available as an explicit case, but not promoted to the green tier. These
warnings need investigation before either suppression or promotion.

## Complete initial survey

All 23 variants were attempted: **5 load/dump passes, 7 failures, 11 timeouts**.

| Target | Load/dump outcome | Seconds | Observation |
| --- | --- | ---: | --- |
| `ara.default` | fail | 1.94 | Slang rejects implicit enum conversions in `lane_sequencer.sv`. |
| `cheshire.default` | fail | 2.68 | Slang rejects an enum conversion and use of `dtmcs_q` before declaration. |
| `cv32e40p.default` | pass | 6.52 | Also passes complete-dump lint; PR tier. |
| `cva6.default` | timeout | 90.08 | No unsupported error emitted before timeout; inconclusive. |
| `cvfpu.fp32_fma` | pass | 1.39 | Also passes complete-dump lint; PR tier. |
| `cvfpu.full` | timeout | 90.03 | No unsupported error emitted before timeout; inconclusive. |
| `ibex.minimal` | pass | 4.27 | Also passes complete-dump lint; PR tier. |
| `ibex.full` | pass | 20.15 | Load/dump passes; lint fails two `UNOPTFLAT` warnings. |
| `ita.default` | timeout | 90.02 | Before timeout: unsupported streaming assignment LHS, `ita_inp1_mux.sv:16`. |
| `mempool.minpool` | timeout | 90.12 | Before timeout: unsupported `always_latch` pattern, `register_file_1r_1w.sv:146`. |
| `mempool.mempool` | timeout | 90.09 | Before timeout: same unsupported latch pattern as minpool. |
| `mempool.terapool` | timeout | 90.10 | No unsupported error emitted before timeout; inconclusive. |
| `nvdla.top` | fail | 0.77 | Slang rejects inconsistent/missing timescales across modules. |
| `opope.default` | fail | 17.02 | Unsupported continuous-assignment LHS in `hci_core_fifo.sv:278`. |
| `pulp_c910.default` | fail | 4.04 | Slang reports expected-expression errors in `ct_lsu_sq.v:3860–3866`. |
| `redmule.default` | fail | 2.85 | Slang rejects implicit conversions in `redmule_streamer.sv:245,395`. |
| `serial_link.default` | fail | 2.02 | Unsupported register/interface aggregate types in `slink`/`slink_reg`, with cascading connection errors. |
| `snitch_cluster.minimal` | timeout | 90.04 | No unsupported error emitted before timeout; inconclusive. |
| `snitch_cluster.default` | timeout | 90.04 | No unsupported error emitted before timeout; inconclusive. |
| `snitch_cluster.default_latch` | timeout | 90.04 | Before timeout: unsupported latch pattern, `snitch_regfile_latch.sv:76`. |
| `snitch_cluster.occamy` | timeout | 90.05 | No unsupported error emitted before timeout; inconclusive. |
| `spatz.default` | timeout | 90.06 | Before timeout: unsupported compound shift assignment, `spatz_controller.sv:123`. |
| `spi_host.default` | pass | 0.97 | Also passes complete-dump lint; PR tier. |

The initial local logs and summary are under `build/pulp-survey/`; the final
strict smoke/lint artifacts are under `build/pulp-benchmarks/`. Full-Ibex lint
artifacts are under `build/pulp-additional-passing/ibex.full/`. These generated
files are intentionally untracked.

Useful follow-up work is streaming assignment LHS lowering (ITA), latch
statement patterns (MemPool/Snitch), compound shifts (Spatz), and aggregate/LHS
lowering (Serial Link/O-POPE). Re-run the inconclusive cases in a Release build
with a larger budget before deciding whether they belong in a scheduled tier.

## Follow-up: compound shifts and streaming LHS

The working-tree frontend changes add combinational compound shifts and
fixed-size streaming assignment LHS lowering. They preserve signed right
shifts, full-width shift counts, blocking-assignment replay, nested streams,
non-divisible slice widths, fixed arrays, multiple targets, and MSB-aligned
consumption of wider streaming sources. Dynamic/with-clause/overlapping targets
and sequential streaming assignments remain explicitly rejected.

The generated `compound_shifts` benchmark passes 262,144 vectors, including a
70-bit shift count. Original RTL passes the same vectors. The generated
`streaming_lhs` benchmark passes 1,048,576 vectors. Verilator 5.052 disagrees on
partial streaming-target slices and swapping target variables when simulating
the original streaming RTL. A separate C++ constructor test forces Slang
constant evaluation through localparams and confirms the independent expected
partial-slice, nested-stream, wider-source, and swap values. The streaming CI
simulation therefore checks the Naja dump against these golden values, not
Verilator's streaming RTL behavior.

Re-running the unmodified release with a 180-second limit:

- **ITA:** the streaming-LHS diagnostic is gone. Loading finishes with only
  unsupported latch patterns in `ita_register_file_1w_multi_port_read.sv:161`
  (two elaborations) and `ita_register_file_1w_multi_port_read_we.sv:171`.
- **Spatz:** the compound-shift diagnostic is gone; the full cluster still
  times out after 180 seconds in the local Debug build. This is not a full
  Spatz pass.

## Follow-up: independent always_latch writes

The latch lowering now handles independent guarded writes in statement lists
and statically bounded nested loops. It preserves per-bit retention while an
enable is low, source-order priority for repeated writes, static array and
part-select targets, and both blocking and nonblocking scheduling when used
consistently. Dynamic targets, same-block read-after-write dependencies,
function calls, timed assignments, and mixed scheduling are rejected.

The focused `latch_loops` fixture passes 16,385 state transitions in both the
original RTL and the generated Naja netlist. The complete frontend suite passes
1,140 tests. The ITA register-file modules, MemPool register file, and Snitch
register file now each load, dump, and pass Verilator lint, so these cases do
not require a new NLDB0 primitive.

The MemPool full design now elaborates successfully with zero Naja unsupported
diagnostics. The `axi_dw_downsizer.sv` `resp_precedence` function-call RHS
cases and the `ctrl_registers.sv` loop-step lowering are handled by the
frontend. Structural dumping of the full MemPool netlist exceeded 300 seconds
locally, so CI runs this design as an elaboration-only frontend regression;
full dumping remains a separate performance investigation.

The exhaustive case path accepts a no-`default` function case only when all
values of a small fixed-width domain are covered by constant labels. Other
incomplete cases remain unsupported. This keeps the lowering conservative while
supporting the AXI response precedence function.

Logs are under `build/pulp-lowering-fixes/`. The initial survey table above is
retained as the baseline; neither design has been promoted to the passing CI
tier.

## Follow-up: expanded elaboration tier (2026-10-03)

Rebuilt Naja `20d67a27` in the existing macOS arm64 Release build, using
Python 3.14 and the same checksum-verified v0.1.0 archive. The workflow now
adds `ita.default`, `cvfpu.full`, and `cva6.default` alongside `mempool.minpool` in the strict
elaboration-only tier, with a 240-second limit per case:

| Target | Elaboration outcome | Wall-clock seconds |
| --- | --- | ---: |
| `mempool.minpool` | pass | 24.66 |
| `ita.default` | pass | 52.00 |
| `cvfpu.full` | pass | 11.03 |
| `cva6.default` | pass | 27.98 |

Each passing case has a successful diagnostics summary, no error/fatal or
Naja unsupported diagnostics, the expected top, and a nonempty top interface.
These measurements include process startup and cleanup; other local work was
running, so they are not isolated performance measurements. This tier does
not establish structural dump, lint, simulation, or equivalence success.

`snitch_cluster.minimal` completes in Release but fails strict elaboration
after about 50 seconds on register aggregate types and sequential lowering
in `snitch_cluster_peripheral`. `spatz.default` fails after about 75 seconds
on an unresolved function-call RHS in `spatz_vfu.sv:1522` (`always_comb`).
O-POPE and Serial Link also remain failures
when rechecked with the refreshed Debug build. They are not promoted.

Release reports are under `build/pulp-expansion-release/`; the additional
failure reports are under `build/pulp-expansion-extra/`. CI writes the
elaboration tier into `build/pulp-benchmarks/elaboration/`, so its aggregate
summary no longer overwrites the complete-dump tier's summary.

## ITA upstream functional validation (2026-10-03)

The experimental `ita_simulation.py` runner reuses upstream ITA revision
`ba96519becce195d64e85eb9a5302e8a1d5487e7`, matching the benchmark release,
and its golden-vector generator and seven-phase testbench. Local validation
used Verilator 5.052, Naja `20d67a27` Release, Python 3.14, NumPy 2.5.3, and
ONNX 1.23.1. Vectors use seed 0, dimensions S/E/P/F=64, one head, bias,
identity activation, and randomized stalls (Verilator seed 1).

The original RTL and Naja netlist each completed all **2,048 output
transactions**, with **identical phase, output data, and timestamps**. This
is finite simulation evidence of preserved behavior, not formal equivalence.
Both also reported the **same six mismatches against upstream golden values**.
The first occurs in phase 3 at 4622.6 ns: actual
`80131f1db08af3ec80807f753c7f80dd`, expected
`80131f1db08af3ec80807f753c7f80ef`. The cause of this shared baseline mismatch
has not been established; the runner returns failure even when RTL and
netlist traces agree. It is not promoted to a green CI gate.

The synthesis SRAM stub is replaced by the upstream behavioral model on the
RTL side; the same model is attached to the generated netlist's explicit
SRAM blackbox. Its shape is checked (256 words, 416 bits, two ports, one-cycle
latency). The generated logic is otherwise unchanged. The testbench adapter
works around Verilator's packed-array `$fscanf` C++ generation issue using
scalar temporaries and checks successful reads. Golden comparisons remain
intact, with complete transaction counts required.

Flat optimized Verilation hit an internal compiler fault; flat `-O0`
exceeded a 300-second limit. Hierarchical Verilation of the four large
datapaths succeeded, and the generated simulation completed in about
2.6 seconds. This is now the runner's default compile strategy.

Reports and logs are under `build/ita-simulation/`, including
`functional-comparison.json`, `original-complete-run.log`, and
`generated-complete-run.log`. See the README for the repeatable command.

## Reproduction and interpretation

See [README.md](README.md#pulp-prepared-eda-benchmarks) for the commands and
workflow dispatch options. A complete survey intentionally returns nonzero
when any variant fails or times out. There are no expected-failure masks.

A timeout alone is inconclusive. Some designs emit explicit unsupported
construct diagnostics before their timeout; those observations are useful
frontend work items even though the entire load did not finish. Strict Slang
compilation errors are separate from Naja lowering failures: upstream's Yosys
flow enables permissive compatibility options that this runner does not apply.

The local code knowledge graph was refreshed incrementally for code. Its full
semantic refresh needs an unavailable LLM API key; the code refresh also
reported existing duplicate-node collisions in `SNLDesignModeling`.
