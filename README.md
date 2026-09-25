<div align="center">
<img width="150" alt="Naja Logo" src="./docs/images/Naja-Logo.png"><h1>Naja</h1>
</div>

[![PyPI](https://img.shields.io/pypi/v/najaeda)](https://pypi.org/project/najaeda)
[![Open Chapter 1](https://colab.research.google.com/assets/colab-badge.svg)](
https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/01_getting_started.ipynb)
[![Join Matrix Chat →](https://img.shields.io/badge/Matrix-Join%20Chat-success?logo=matrix)](https://matrix.to/#/#naja:fossi-chat.org)
![Ubuntu Build](https://github.com/najaeda/naja/actions/workflows/ubuntu-build.yml/badge.svg)
![MacOS Build](https://github.com/najaeda/naja/actions/workflows/macos-build.yml/badge.svg)
[![codecov](https://codecov.io/gh/najaeda/naja/branch/main/graph/badge.svg?token=59ZKZ74HFP)](https://codecov.io/gh/najaeda/naja)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![REUSE status](https://api.reuse.software/badge/github.com/najaeda/naja)](https://api.reuse.software/info/github.com/najaeda/naja)

## What is Naja?

Naja is an open source EDA framework for loading, elaborating, analyzing, optimizing, and transforming hardware designs from RTL SystemVerilog through structural netlists. It is usable from Python or C++.

- **SV/Verilog frontend** — parse Verilog and elaborate SystemVerilog RTL into a browsable design model
- **Netlist analysis** — hierarchy, connectivity, equipotentials
- **Logic optimization** — dead logic elimination, constant propagation
- **ECO transformations** — direct netlist editing
- **Serialization** — SNL interchange format (Cap'n Proto) and Verilog output

### Architecture

The diagram below shows how formats, frontends, APIs, and companion tools
integrate around Naja's C++ netlist engine:

- **Design inputs** — SystemVerilog is parsed and elaborated through
  [`slang`](https://github.com/MikePopoloski/slang); gate-level Verilog can be
  loaded and emitted; and Liberty plus the
  [Python primitive libraries](./src/najaeda/najaeda/primitives/) provide cell
  and primitive models.
- **Core and interchange** — Naja represents hierarchy, buses, bit-level nets
  and terms, connectivity, and primitive functional models. The
  [`naja-if`](https://github.com/najaeda/naja-if) Cap'n Proto format provides a
  logical-view interchange path.
- **APIs and tools** — [`najaeda`](https://pypi.org/project/najaeda/) exposes
  the engine through Python. Companion projects build on Naja directly or
  through that API: [`kepler-formal`](https://github.com/keplertech/kepler-formal),
  [`naja-schematic`](https://github.com/najaeda/naja-schematic), and
  [`naja-scope`](https://github.com/najaeda/naja-scope).

```mermaid
---
config:
  layout: dagre
  theme: base
  themeVariables:
    fontFamily: ''
    fontSize: 14px
    primaryTextColor: '#172033'
    lineColor: '#64748b'
    clusterBkg: '#f8fafc'
    clusterBorder: '#cbd5e1'
---
flowchart LR
    sv["`**SystemVerilog**`"] ==> slang(["`**slang**<br>SystemVerilog Frontend`"])
    verilog["`**gate-level verilog**`"] ==> naja-verilog["`**naja-verilog**<br>gate verilog Parser`"]
    naja-verilog ==> naja["`**naja C++ · netlist engine**<br><br>hierarchy · buses<br>bit-level nets &amp; terms · connectivity<br>Primitive Functional Models`"]
    najaif@{ label: "**naja-if**<br>Logical View Interchange Format<br>Cap'n Proto" } <==> naja
    slang =="`**Elaboration**`"==> naja
    naja <==> najaeda["`**najaeda**<br>Python API`"]
    naja ==> kf("`**kepler-formal**<br>Formal Comparison`") & ns("`**naja-schematic**<br>Schematic Viewer`")
    najaeda ==> scope("`**naja-scope**<br>najaeda MCP server`")
    kf ==> kfm("`**kepler-formal-mcp**<br>kepler-formal MCP server`")
    lib["`**Liberty**`"] ==> naja
    pythonlibs["`**Python libraries**<br>naja representation`"] ==> naja

    sv@{ shape: disk}
    verilog@{ shape: disk}
    naja@{ shape: rounded}
    najaif@{ shape: disk}
    lib@{ shape: disk}
    pythonlibs@{ shape: disk}
     sv:::input
     slang:::frontend
     naja-verilog:::frontend
     verilog:::input
     naja:::core
     najaif:::input
     najaeda:::api
     kf:::tool
     kfm:::tool
     ns:::tool
     scope:::tool
     lib:::input
     pythonlibs:::input
    classDef input fill:#eef2ff,stroke:#6366f1,color:#1e1b4b,stroke-width:2px
    classDef frontend fill:#fff7ed,stroke:#f97316,color:#431407,stroke-width:2px
    classDef core fill:#fef2f2,stroke:#ef4444,color:#450a0a,stroke-width:3px
    classDef api fill:#f0fdfa,stroke:#14b8a6,color:#042f2e,stroke-width:2px
    classDef tool fill:#f5f3ff,stroke:#8b5cf6,color:#2e1065,stroke-width:2px
    click slang "https://github.com/MikePopoloski/slang"
    click naja-verilog "https://github.com/najaeda/naja-verilog"
    click naja "https://github.com/najaeda/naja"
    click najaif "https://github.com/najaeda/naja-if"
    click najaeda "https://pypi.org/project/najaeda/"
    click kf "https://github.com/keplertech/kepler-formal"
    click kfm "https://github.com/keplertech/kepler-formal-mcp"
    click ns "https://github.com/najaeda/naja-schematic"
    click scope "https://github.com/najaeda/naja-scope"

```

## Get Started

The best entry point is the [`najaeda`](https://pypi.org/project/najaeda/) Python package:

```bash
pip install najaeda
```

Requires Python 3.10 or later. Pre-built wheels are published for:

- Linux x86_64 and AArch64 (`manylinux_2_28`; glibc 2.28 or later)
- macOS Apple Silicon (arm64), macOS 11 or later
- Windows x86_64

Full documentation: [najaeda.readthedocs.io](https://najaeda.readthedocs.io/en/latest/)

For AI-assisted design exploration, [`naja-scope`](https://github.com/najaeda/naja-scope)
is a najaeda-based MCP server that gives MCP-compatible assistants a precise,
structured view of elaborated SystemVerilog designs. Instead of pasting large
RTL files into chat, agents can ask targeted questions — what drives a signal,
what is inside a module, where a net comes from — and get small, exact answers
with file-and-line references.

### Tutorials

Six hands-on notebooks — open any of them in Colab with no local install needed:

| # | Topic | Colab |
| --- | --- | --- |
| 1 | Getting started — load Verilog, navigate hierarchy, visualize | [![Open in Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/01_getting_started.ipynb) |
| 2 | Liberty primitives — load a synthesised design with standard cells | [![Open in Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/02_liberty_primitives_design.ipynb) |
| 3 | Editing a netlist — rename, disconnect, reconnect, delete | [![Open in Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/03_editing_a_netlist.ipynb) |
| 4 | SystemVerilog elaboration — load and browse an elaborated SV design | [![Open in Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/04_systemverilog_elaborated_netlist.ipynb) |
| 5 | ibex RISC-V core — explore a real-world SV core, collect stats | [![Open in Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/05_ibex_riscv_core.ipynb) |
| 6 | Fanout analysis — compute fanout for every net, trace drivers, export to pandas | [![Open in Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/najaeda/naja/blob/main/tutorials/notebooks/06_fanout_analysis.ipynb) |

## `naja_edit` — Netlist CLI

`naja_edit` is a command-line tool for optimizing and translating netlists.

:tv: Presented at [ORConf 2024](https://www.youtube.com/watch?v=JpwZGCuWekU).

```bash
# Translate Verilog → SNL
naja_edit -f verilog -t snl -i input.v -o output.snl

# Parse SystemVerilog with explicit top
naja_edit -f systemverilog -t verilog -i input.sv -o output.v --sv_top top

# Dead logic elimination
naja_edit -f snl -t snl -i input.snl -o output.snl -a dle

# Chain optimizations with Python scripts
naja_edit -f snl -t snl -i input.snl -o output.snl -a dle -e pre.py -z post.py
```

Available optimizations (`-a`): `all` (DLE + constant propagation + primitives), `dle`.

Python script examples: [src/apps/naja_edit/examples](https://github.com/najaeda/naja/blob/main/src/apps/naja_edit/examples)  
Regression suite: [naja-regress](https://github.com/najaeda/naja-regress)

## Building from Source

CMake is Naja's primary build, test, and install workflow. Bazel is maintained
as a build-and-test smoke path; it does not replace the CMake install and
packaging workflows.

### CMake

#### Dependencies

**Ubuntu:**

```bash
sudo apt-get update
sudo apt-get install build-essential cmake git libboost-dev python3-dev \
  capnproto libcapnp-dev libtbb-dev pkg-config bison flex
```

**macOS (Homebrew):**

```bash
brew install cmake capnp tbb bison flex boost
export PATH="/opt/homebrew/opt/flex/bin:/opt/homebrew/opt/bison/bin:$PATH"
```

**Nix:**

```bash
nix-shell -p cmake gnumake boost python3 capnproto bison flex pkg-config tbb_2021_8
```

#### Build, test, and install

```bash
git clone --recurse-submodules https://github.com/najaeda/naja.git
cd naja
export NAJA_INSTALL="$PWD/install"
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$NAJA_INSTALL"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build

# Add the installed Python package to your environment.
export PYTHONPATH="${PYTHONPATH:+$PYTHONPATH:}$NAJA_INSTALL/lib/python"
```

### Bazel smoke build

The Bazel build covers the repository's build and test targets on Ubuntu and
macOS. The version is pinned in [`.bazelversion`](./.bazelversion); using
[Bazelisk](https://github.com/bazelbuild/bazelisk) as the `bazel` command
automatically selects it.

Bazel fetches its own pinned copies of the shared source dependencies, so a
Bazel-only checkout does not need Git submodules. It still uses system
toolchains and libraries for parts of the build.

**Ubuntu:**

```bash
sudo apt-get update
sudo apt-get install build-essential cmake libboost-dev libfl-dev libtbb-dev \
  bison flex m4 pkg-config python3-dev git
```

**macOS (Homebrew):**

Install the Xcode Command Line Tools, then:

```bash
brew install cmake capnp tbb bison flex boost fmt tomlplusplus pkg-config
export PATH="$(brew --prefix flex)/bin:$(brew --prefix bison)/bin:$PATH"
```

Build and test from the repository root:

```bash
git clone https://github.com/najaeda/naja.git
cd naja
bazel build //... --jobs=auto
bazel test //... --test_output=errors --jobs=auto
```

These are the same smoke commands used by
[`ubuntu-bazel.yml`](./.github/workflows/ubuntu-bazel.yml) and
[`macos-bazel.yml`](./.github/workflows/macos-bazel.yml). There is no Bazel
install target; use the CMake workflow above when you need an installed
library, Python package, or packaged artifact.

When changing a shared dependency pin, keep the Git submodule and
[`MODULE.bazel`](./MODULE.bazel) entries synchronized, then run:

```bash
python3 ci/check_submodule_bazel_sync.py
```

## C++ API

Naja exposes two complementary APIs:

- **SNL** (Structured Netlist) — full read/write netlist representation
- **DNL** (Dissolved Netlist) — fast, read-only flattened view for parallel analysis

Extended documentation: [naja.readthedocs.io](https://naja.readthedocs.io/en/latest/)  
C++ snippet: [NLUniverseSnippet.cpp](https://github.com/najaeda/naja/blob/main/src/app_snippet/src/NLUniverseSnippet.cpp)  
App template (copy to start a new tool): [src/app_snippet](https://github.com/najaeda/naja/blob/main/src/app_snippet)

## Community

- Chat: [Matrix #naja:fossi-chat.org](https://matrix.to/#/#naja:fossi-chat.org)
- Bugs / features: [GitHub Issues](https://github.com/najaeda/naja/issues)
- Contact: [contact@keplertech.io](mailto:contact@keplertech.io)

:star: If you find Naja useful, starring the repo helps spread the word.

## Acknowledgement

[<img src="https://nlnet.nl/logo/banner.png" width=100>](https://nlnet.nl/project/Naja)
[<img src="https://nlnet.nl/image/logos/NGI0Entrust_tag.svg" width=100>](https://nlnet.nl/project/Naja)

Supported by [NLNet](https://nlnet.nl/project/Naja) through the [NGI0 Entrust](https://nlnet.nl/entrust) Fund.
