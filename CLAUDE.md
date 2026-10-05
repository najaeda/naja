# Naja — Agent Guide

Guidance for AI coding agents (Claude Code reads this as `CLAUDE.md`; Codex reads it as `AGENTS.md` via symlink) working in this repository.

## Codebase questions: use the knowledge graph *if it's there*

This is a large codebase (~1500 files). A graphify knowledge graph makes orienting much faster — but it is **optional and machine-local** (`graphify-out/` is gitignored, so a fresh clone won't have it).

**First, check availability.** The graph is usable only if **both** are true: the `/graphify` skill is installed *and* `graphify-out/graph.json` exists in the project root.

- **If the graph is available** — use it first, before grepping or reading files top-to-bottom:
  - Natural-language question: `/graphify query "how does the SystemVerilog constructor lower always_ff blocks?"`
  - Trace between two concepts: `/graphify path "SNLSVConstructor" "DNL"`
  - Explain one node: `/graphify explain "SNLSVConstructor"`
  - Browse: [`graphify-out/GRAPH_REPORT.md`](graphify-out/GRAPH_REPORT.md) — its "Community Hubs" section is a navigable TOC (*SNLSVConstructor Core Implementation*, *Sequential Assignment Lowering*, *najaeda Instance Query API*, *NL Library Management*, …).

  Graph first for *where* and *how it connects*; then Read/Grep the candidate files for the *exact lines*.

- **If `graphify-out/` is absent or the skill isn't installed** — just use the normal tools (Grep, Glob, Read) and the source layout below. **Do NOT run a full `/graphify` build to answer an ordinary question** — that's a slow, expensive 1500-file extraction and is not worth it for a lookup. Building the graph is opt-in, and only when the user explicitly asks for it.

**Keeping it fresh (only if you already have a graph):** after a non-trivial change, `/graphify . --update` re-extracts just the changed files so future queries stay accurate.

## Local research inputs

Machine-local papers and implementation notes may be present under
`internal/`, which is gitignored. Before implementing a feature, check
`internal/design/` for a relevant design note and `internal/papers/README.md`
for its research sources.

Treat the design note as the implementation contract. A paper is supporting
evidence, not a requirement to implement every idea it contains. When adding a
paper, catalog it and create or update a design note with exact sections,
Naja-specific mappings, non-goals, verification criteria, and unresolved
questions. Do not commit or redistribute papers unless their license permits
it.

## What Naja is

Open-source EDA framework for hardware design loading and transformation — from Verilog and SystemVerilog RTL elaboration through structural netlist analysis, optimization, and editing. Usable from C++ and Python (`najaeda`). See [README.md](README.md) for the public overview.

Two complementary C++ APIs:
- **SNL** (Structured Netlist) — full read/write netlist representation.
- **DNL** (Dissolved Netlist) — fast, read-only flattened view for parallel analysis.

## Source layout

- `src/nl/netlist/` — core netlist model: `snl/`, `pnl/`, `core/`, `decorators/`, `serialization/` (Cap'n Proto interchange), `visual/`.
- `src/nl/formats/` — frontends/backends: `systemverilog/` (slang-based), `verilog/`, `liberty/`, `lefdef/`.
- `src/nl/python/` — Python bindings for the `najaeda` package.
- `src/dnl/` — Dissolved Netlist (flattened, read-only, parallel).
- `src/najaeda/` — Python package (`najaeda/`, `examples/`, `benchmarks/`).
- `src/apps/naja_edit/` — `naja_edit` CLI (optimize/translate netlists).
- `src/app_snippet/` — copy-to-start template for a new C++ tool.
- `src/{bne,core,metrics,optimization}/` — supporting libraries (logic opt: DLE, constant propagation).
- `primitives/` — primitive/standard-cell libraries. `test/` mirrors `src/`. `tutorials/` — the six Colab notebooks.

## NajaIF snapshot compatibility — current status

NajaIF snapshots carry a small `snl.mf` manifest. The immediate objective is
to prevent a snapshot written by a different Naja build from being silently
deserialized into a truncated or otherwise incorrect netlist.

- The manifest writes `V <major> <minor> <revision>` (the legacy format/schema
  revision) and `P <naja-version> <git-hash>` (the producer identity).
- `SNLCapnP::load()` reads this manifest before either Cap'n Proto payload.
  It retains the strict `V` check and, for now, also requires an exact match
  of both producer values with `naja::NAJA_VERSION` and
  `naja::NAJA_GIT_HASH`. A mismatch, or a legacy manifest without `P`, throws
  `SNLDumpException`; callers must regenerate the snapshot.
- `naja.snapshot_manifest(path)` reads only `snl.mf` and returns
  `schema_version`, `producer_version`, and `producer_git_hash`, without
  creating an `NLUniverse` or loading payloads.
- This exact-build producer gate is deliberately temporary and conservative.
  The remaining design work is to define and maintain a schema version owned
  by `thirdparty/naja-if`, then use that version as the durable compatibility
  contract so compatible Naja builds can exchange snapshots.

Relevant implementation: `SNLCapnP.cpp`, `SNLDumpManifest.cpp`, and
`PyNLDB.cpp`; focused tests live under `test/nl/snl/serialization/capnp/` and
`test/nl/python/naja_wrapping/test_nldb.py`.

## Build & test

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$NAJA_INSTALL
make && make test && make install
```

- Build dirs already present: `build/`, `build-coverage/`, `build-coverage-svconstructor/`. Prefer building in an existing one to reuse the CMake cache.
- Tests are CTest-driven; run `ctest` (or `make test`) from the build dir. Test sources live under `test/` mirroring `src/`.
- Python usage after install needs `export PYTHONPATH=$PYTHONPATH:$NAJA_INSTALL/lib/python`.
- Build deps (macOS): `brew install cmake capnp tbb bison flex boost` and put flex/bison on `PATH`.

## Build systems: CMake (primary) + Bazel (validated smoke test)

CMake is naja's primary build system — it's what CI, packaging (wheels,
Docker images), and most workflows use, and it's the one to reach for by
default. Bazel (`MODULE.bazel`, `BUILD.bazel` throughout the tree) is
kept in parallel as a validated smoke test only, via `ubuntu-bazel.yml`/
`macos-bazel.yml` (`bazel build //... && bazel test //...`, no
submodules, no host packages — every dependency is a `bazel_dep`). It
is **not** CI's primary gate and doesn't need to track every workflow's
behavior (sanitizer suppressions, coverage flags, etc.) — just prove the
Bazel side keeps compiling and passing tests.

**Keep submodule pins and Bazel pins in sync.** CMake pins shared
upstream dependencies via git submodules (`.gitmodules`, `thirdparty/*`);
Bazel takes the *same* dependencies as `bazel_dep`s, and those not on
BCR are pinned to commits by their registry entries
(`modules/<name>/<version>/source.json` in the registries `.bazelrc`
lists). Nothing forces these to move together —
bumping one without the other silently makes the two build systems test
different upstream code. When you bump a submodule commit (or vice
versa), submit the matching BCR version and point `MODULE.bazel` at it
in the same change:

- `slang` (Bazel module `sv-lang`), `naja-verilog`: must be an
  **exact** commit match.
- `naja-if`: Bazel tracks a separate `bazel-support` branch (native
  Bazel BUILD files added on top) rather than the branch CMake tracks —
  so an exact match isn't meaningful. Instead, the submodule's pinned
  commit must be an **ancestor of (or equal to)** the `bazel-support`
  pin, i.e. `bazel-support` must never fall behind main.
- `googletest`: deliberately excluded — CMake pins an old submodule dev
  commit, Bazel takes a BCR release. Different dependency-sourcing
  mechanisms entirely; not meant to track in lockstep.

This is enforced automatically: `ci/check_submodule_bazel_sync.py`
(run by `.github/workflows/dependency-sync-check.yml` on every push/PR)
checks exactly this and fails CI if a pin has drifted. Run it locally
after bumping any of these dependencies: `python3
ci/check_submodule_bazel_sync.py`.

## Bazel: a BCR-ready module

naja's Bazel build is meant to be published to the Bazel Central
Registry (BCR) and consumed by other modules (kepler-formal does) with a
plain `bazel_dep`. Keep it that way. Invariants:

- `MODULE.bazel` contains only `bazel_dep`s. No `http_archive`,
  `git_override`, `archive_override`, module extensions or repository
  rules of our own; overrides are ignored for non-root modules, so they
  would only hide breakage that consumers then hit.
- Nothing runs cmake/make inside Bazel (no `rules_foreign_cc`), and
  nothing is found on the host (`PATH`, pkg-config, `python3-config`,
  system headers). Tools and libraries come from Bazel modules: bison
  and flex rules from BCR `bison`/`flex`, Python from `rules_python`.
- Load every rule (`@rules_cc//cc:cc_library.bzl`,
  `@rules_shell//shell:sh_test.bzl`, …); Bazel 9 has no native ones.
- Fix a dependency in its own module (registry entry or upstream), never
  by patching its BUILD files from this repository, and never by asking
  consumers to patch naja.

**Where dependencies come from.** Everything comes from BCR. A module
version that is not on BCR yet comes from its open
bazel-central-registry pull request: `.bazelrc` lists the PR's commit
(`https://raw.githubusercontent.com/<fork>/bazel-central-registry/<sha>/`)
ahead of BCR, and Bazel takes each `name@version` from the first
registry that has it. Drop the line once the PR merges. Unreleased
commits use `<release>-<YYYYMMDD>-<commit>` versions. Never change a
version's contents once something depends on it; add a new version.
When a PR needs a fix, push a new commit (don't force-push, so pinned
commits stay reachable) and move the pin.

**Publishing to BCR**: one module per bazel-central-registry pull
request, each based on BCR main, so each goes in on its own. naja itself is
published from a release tag by the publish-to-bcr app, using the
templates in `.bcr/` (maintainers: xtofalex, nanocoh), once its
dependencies are on BCR. BCR rejects symlinks in entries, so
`overlay/MODULE.bazel` is a copy.

**Known traps** (each already fixed; don't reintroduce):

- hermetic toolchains (BCR `llvm`, used by kepler-formal) pass libc
  headers as early `-isystem` flags. Libraries whose own headers must
  shadow libc's (gnulib in bison) need `-I`, not `includes = [...]`;
  hence the registry's `bison 3.8.2.bcr.10`. Building only with the host
  toolchain hides this class of bug, and also hides headers leaking in
  from `/usr/include` (slang's `boost/regex.hpp` did).
- `cc_shared_library` (`naja_runtime`) drops linker inputs its graph
  aspect cannot see: rules must advertise `CcInfo` and own their linker
  inputs. That's why `src/nl/python/pyloader/python_libs.bzl` re-owns
  libpython instead of using `current_py_cc_libs` directly. Libraries
  linked into `naja_runtime` that a binary also uses (TBB, zlib) must be
  in its `exports_filter`, or the binary links a second copy.
- `NAJA_GIT_HASH` comes from the module version
  (`src/core/naja_version.bzl`): "unknown" when naja is the root module.

**Before merging Bazel changes**, besides `bazel test //...` here, build
naja as a dependency with a hermetic toolchain: kepler-formal's tests
(`bazel test //... --override_module=naja=<path to this checkout>`) are
the reference consumer.

## Conventions

- Whenever the `najaeda` version is incremented, update the pinned `najaeda` version in all Colab tutorials under `tutorials/notebooks/` and the local installation command in `tutorials/README.md` in the same change.
- Every new file, including test fixtures and helper scripts, must have both copyright and licensing information. For Naja-authored files, add `SPDX-FileCopyrightText: <year> The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>` and `SPDX-License-Identifier: Apache-2.0` using the file format's comment syntax. Preserve third-party attribution and licenses. For files that cannot contain comments, add coverage in `.reuse/dep5`. Before finishing a change that adds files or changes licensing metadata, run `reuse lint` and fix any missing copyright or licensing information introduced by the change.
- Match the surrounding code's style, naming, and comment density — the SNL layer uses `NL*`/`SNL*` prefixes; follow the local idiom.
- The SystemVerilog frontend is built on **slang**; sequential lowering and always-block handling live in `SNLSVConstructor` and the "Sequential Assignment Lowering" community — query the graph before touching them.
- Post-elaboration netlists must never silently encode an unsupported construct incorrectly. If faithful lowering is not available, reject the construct or emit a clear diagnostic rather than dropping a value, leaving a net undriven, or otherwise producing a plausible but wrong netlist.
- Prefer general frontend support over fixes tailored to one reproducer: identify the shared language semantics and cover representative variations in syntax, type/shape, and downstream use where practical.
- New DB0 primitives (flops, etc.) follow a canonical ID scheme resolved on capnp load; don't invent ad-hoc primitive IDs.
- `*.py~`, `*.txt~`, `build*/`, and `graphify-out/.venv*` are local artifacts — don't edit or commit them.

## VHDL and SystemVerilog loading alignment

Keep VHDL and SystemVerilog loading behavior and APIs parallel wherever practical.
Use the existing SystemVerilog loader as the reference for API names, defaults,
configuration options, diagnostics, warning deduplication and suppression, report
file routing, and error handling. Apply this consistently across the C++ loaders,
raw Python bindings, and high-level `najaeda.netlist` APIs. Prefer matching an
existing SV convention over introducing a VHDL-specific one. Differences should
reflect language requirements or clearly documented implementation limits; update
the relevant documentation and focused tests when introducing such differences.

HDL loading uses `NLLibrary` as the destination and logical-library identity.
Both Python VHDL and SV loaders accept `library="DESIGN"`; basic library names
match case-insensitively, extended names exactly, and ambiguous root names are
errors. Lookup stays within root libraries of the same `NLDB`, without recursive
or cross-database fallback. In VHDL, `work` always denotes the library owning the
referenced source unit, including imported package contexts. Retained sources,
packages, model caches, and elaborated designs must keep that ownership. `std`
and `ieee` use explicit built-in providers for the supported language subset.

## Primitive timing-model alignment

`SNLDesignModeling.h` is the canonical C++ primitive timing-model API. Timing
metadata can be populated by the Liberty frontend, by direct C++ construction
of NLDB0 primitives, and by the Python primitive libraries under
`src/najaeda/najaeda/primitives/`. Keep these three paths aligned: when adding
or changing timing arcs, timing parameters, term roles, active levels, or
related queries, verify that the raw `najaeda.naja` bindings expose the feature
needed to express the same model from Python and update the Python primitive
loaders where applicable. Add or update focused tests for both the raw bindings
and the affected Python primitive libraries so that equivalent decorations do
not silently drift apart.

## najaeda Python API documentation

When changing either Python API level exposed by the `najaeda` package, update the package documentation in `src/najaeda/najaeda/docs/source/` in the same change. This includes both the high-level `najaeda.netlist` API and the raw compiled `najaeda.naja` / `naja.so` API.

This applies to:
- the high-level API in `src/najaeda/najaeda/netlist.py`;
- Python helper modules such as `instance_visitor.py`, `net_visitor.py`, `stats.py`, and `pandas_stats.py`;
- raw Python bindings under `src/nl/python/naja_wrapping/`, including exported module functions, exception types, classes, constructors, enum-like values, and public methods;
- changes in underlying SNL/NL C++ APIs when they alter what the raw Python bindings expose or how raw Python users should call them.

Documentation expectations:
- High-level API changes should update `api.rst`, the relevant class guide page, or the user guide pages (`concepts.rst`, `quickstart.rst`, `loading.rst`, `editing.rst`).
- Raw `najaeda.naja` / `naja.so` API changes should update `raw_api.rst`, including the expert reference table when public raw classes, module functions, exceptions, enum-like values, constructors, or methods change.
- If a raw binding change makes a new native feature usable from Python, document when experts should use the raw API directly and whether a high-level `najaeda.netlist` wrapper should also be added.
- Workflow or example changes should update `examples.rst.in` or the relevant guide page.

Before finishing a documentation-impacting change, run a Sphinx build when the local environment supports it:

```bash
sphinx-build -b html src/najaeda/najaeda/docs/source /tmp/najaeda-docs-check
```

If `sphinx_rtd_theme` is not installed locally, use the built-in theme override:

```bash
sphinx-build -b html -D html_theme=alabaster src/najaeda/najaeda/docs/source /tmp/najaeda-docs-check
```

## Git

- Default/PR base branch: `main`. Don't commit or push unless asked; if on `main`, branch first.
