<div align="center">
<img width="150" alt="Naja Logo" src="./docs/images/Naja-Logo.png"><h1>Naja</h1>
</div>

[![PyPI](https://img.shields.io/pypi/v/najaeda)](https://pypi.org/project/najaeda)
[![Open Chapter 1](https://colab.research.google.com/assets/colab-badge.svg)](
https://colab.research.google.com/github/najaeda/najaeda-tutorials/blob/main/notebooks/01_getting_started.ipynb)
[![Join Matrix Chat →](https://img.shields.io/badge/Matrix-Join%20Chat-success?logo=matrix)](https://matrix.to/#/#naja:fossi-chat.org)
![Ubuntu Build](https://github.com/najaeda/naja/actions/workflows/ubuntu-build.yml/badge.svg)
![MacOS Build](https://github.com/najaeda/naja/actions/workflows/macos-build.yml/badge.svg)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/b224740790e24c80a381a6eede28cad8)](https://app.codacy.com/gh/najaeda/naja?utm_source=github.com&utm_medium=referral&utm_content=najaeda/naja&utm_campaign=Badge_Grade_Settings)
[![codecov](https://codecov.io/gh/najaeda/naja/branch/main/graph/badge.svg?token=59ZKZ74HFP)](https://codecov.io/gh/najaeda/naja)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![REUSE status](https://api.reuse.software/badge/github.com/najaeda/naja)](https://api.reuse.software/info/github.com/najaeda/naja)

## What is Naja?

Naja is an open source EDA framework for working with gate-level netlists — from SystemVerilog parsing through analysis, optimization, and transformation. It is usable from Python or C++.

- **SV/Verilog frontend** — parse and elaborate gate-level designs
- **Netlist analysis** — hierarchy, connectivity, equipotentials
- **Logic optimization** — dead logic elimination, constant propagation
- **ECO transformations** — direct netlist editing
- **Serialization** — SNL interchange format (Cap'n Proto) and Verilog output

![Naja Architecture](./docs/images/Naja-Architecture.png)

## Get Started

The best entry point is the [`najaeda`](https://pypi.org/project/najaeda/) Python package:

```bash
pip install najaeda
```

Full documentation: [najaeda.readthedocs.io](https://najaeda.readthedocs.io/en/latest/)

Try it interactively in the browser with the Colab badge above — no install needed.

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

### Dependencies

**Ubuntu:**

```bash
sudo apt-get install g++ libboost-dev python3-dev capnproto libcapnp-dev libtbb-dev pkg-config bison flex
```

**macOS (Homebrew):**

```bash
brew install cmake capnp tbb bison flex boost
export PATH="/opt/homebrew/opt/flex/bin:/opt/homebrew/opt/bison/bin:$PATH"
```

**Nix:**

```bash
nix-shell -p cmake boost python3 capnproto bison flex pkg-config tbb_2021_8
```

### Build

```bash
git clone --recurse-submodules https://github.com/najaeda/naja.git
export NAJA_INSTALL=<install-dir>
mkdir build && cd build
cmake ../naja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$NAJA_INSTALL
make && make test && make install
# Add to your environment after install:
export PYTHONPATH=$PYTHONPATH:$NAJA_INSTALL/lib/python
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
