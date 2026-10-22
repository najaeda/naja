<!---
SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
SPDX-License-Identifier: Apache-2.0
-->

# Deterministic netlist construction and dumping

Determinism is a correctness requirement. Given the same ordered source paths,
contents, loader options, Naja build and initial universe, separate processes
must create the same objects in the same order, with the same names, IDs and
connectivity. A process's addresses, allocator layout and hash seed must not
change the result. Transformations that create or rename objects have the same
obligation.

Verilog and primitive sidecar dumps must match byte for byte except for the date
line of their banner. DOT files and every NajaIF file, including the manifest,
must match without normalization. Snapshot reload must preserve these properties.
Elapsed-time diagnostics and logs are outside this artifact contract.

## Automated checks

`test_najaeda_determinism.py` is discovered by the ordinary Python suite and is
also exposed as a dedicated CTest and Bazel target. It uses independently written,
synthetic fixtures; no external design or checkout is needed:

- SV branch replay with several assigned variables, repeated automatic function
  calls, nested conditions, loops, generate blocks, guarded registers and multiple
  memories with independent writers;
- structural Verilog with hierarchy, buses, slices and concatenations;
- VHDL with hierarchy, generated instances, logic lowering and inferred memory;
- all canonical DB0 primitive families through the native serialization audit,
  including shared models and reversed instance/model creation order.

For each fixture, at least five fresh processes use different Python hash seeds
and keep differently sized heap allocations alive during native elaboration.
Each process captures live object IDs, names, terms and connections **before**
calling a dumper. It exports Verilog, DOT and NajaIF, destroys the universe,
reloads the snapshot and repeats the capture and exports. An additional fresh reader process loads each snapshot without prior elaboration.
Checks compare writer processes, fresh readers and the snapshot round trip,
including the DB0 interface payload. Failures identify the artifact and hashes,
with a bounded text diff where appropriate.

Run locally after building the Python extension:

```sh
ctest --test-dir build -L determinism --no-tests=error --output-on-failure
bazel test //test/najaeda:najaeda_determinism_tests --test_output=errors
```

Linux and macOS source-build CI explicitly runs the dedicated check for both
CMake and Bazel. The Linux najaeda workflow also runs weekly with 30 processes
per fixture. To reproduce that stress check locally:

```sh
NAJA_DETERMINISM_RUNS=30 ctest --test-dir build -L determinism --no-tests=error --output-on-failure
bazel test //test/najaeda:najaeda_determinism_tests --test_env=NAJA_DETERMINISM_RUNS=30 --nocache_test_results
```

The tests compare repeated executions of each build; they do not pin generated
names to one historical revision. Intentional changes to lowering can change
creation order once, but must remain reproducible. A finite fixture suite cannot
prove determinism for all designs: review and focused regression additions remain
necessary. Repository branch protection should require the source-build CI jobs;
workflow files alone cannot enforce a repository's merge policy.

## Review and extension rules

For every new or changed traversal, check whether it creates, deletes, merges,
connects or renames objects, assigns IDs, advances a naming counter, chooses a
representative, or emits artifacts. This includes speculative inference, cache
population, diagnostics that select the first failing object, and snapshot load.

- Hash tables and pointer-keyed containers are fine for lookup and membership.
  Never use their traversal order for observable decisions. Copy keys to an
  explicitly ordered sequence, or maintain insertion order alongside the index.
- Order frontend symbols by source/declaration order, including enclosing scopes
  and generate indices. Names alone cannot distinguish shadowed or unnamed
  declarations. The SV frontend's `symbolsInDeclarationOrder` helper is the
  reference implementation.
- Order SNL objects by their full stable NLID, or by a documented source/insertion
  order. Comparing pointer values, including as a final sorting tie breaker, is
  forbidden. A pointer-keyed map with a comparator that dereferences stable IDs
  is different from a map that compares addresses.
- A sort key must distinguish all relevant objects. If equivalent keys are
  intentional, preserve a known deterministic input order with a stable sort.
- Keep serialization in stable object order. Sorting a dump cannot repair IDs or
  names assigned nondeterministically during construction.
- When a feature introduces a new creation path or naming cache, extend a fixture
  that exercises it. For a reported failure, write a minimal synthetic fixture,
  demonstrate that it fails before the fix, and retain it in this suite.
- Do not suppress a failure by ignoring additional artifact fields, stripping
  more comments, weakening the live-state comparison, or replacing repeated-run
  comparisons with a one-time golden regeneration. Investigate the creation path.

Broad bans on `unordered_map`, pointer keys or `std::sort` would flag safe lookup
code while missing indirect order dependencies. This policy therefore combines
observable regression tests with explicit review of creation and emission paths.
