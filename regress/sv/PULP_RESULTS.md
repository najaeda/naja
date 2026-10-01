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

Logs are under `build/pulp-lowering-fixes/`. The initial survey table above is
retained as the baseline; neither design has been promoted to the passing CI
tier.

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
