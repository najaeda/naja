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

## What Naja is

Open-source EDA framework for gate-level netlists — SystemVerilog/Verilog parsing through analysis, optimization, and transformation. Usable from C++ and Python (`najaeda`). See [README.md](README.md) for the public overview.

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

## Conventions

- Match the surrounding code's style, naming, and comment density — the SNL layer uses `NL*`/`SNL*` prefixes; follow the local idiom.
- The SystemVerilog frontend is built on **slang**; sequential lowering and always-block handling live in `SNLSVConstructor` and the "Sequential Assignment Lowering" community — query the graph before touching them.
- New DB0 primitives (flops, etc.) follow a canonical ID scheme resolved on capnp load; don't invent ad-hoc primitive IDs.
- `*.py~`, `*.txt~`, `build*/`, and `graphify-out/.venv*` are local artifacts — don't edit or commit them.

## Git

- Default/PR base branch: `main`. Don't commit or push unless asked; if on `main`, branch first.
