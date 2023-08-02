# Naja

![build](https://github.com/xtofalex/naja/actions/workflows/build.yml/badge.svg)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/b224740790e24c80a381a6eede28cad8)](https://app.codacy.com/gh/xtofalex/naja?utm_source=github.com&utm_medium=referral&utm_content=xtofalex/naja&utm_campaign=Badge_Grade_Settings)
[![codecov](https://codecov.io/gh/xtofalex/naja/branch/main/graph/badge.svg?token=59ZKZ74HFP)](https://codecov.io/gh/xtofalex/naja)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
***
## Introduction
Naja is an Electronic Design Automation (EDA) project that provides open source data structures and APIs for the development of post logic synthesis EDA algorithms such as: netlist simplification (constant and dead logic propagation), logic replication, netlist partitioning, ASIC and FPGA place and route, …

Naja contains two main components SNL (Structured Netlist) API (located in this repo) and [naja-verilog](https://github.com/xtofalex/naja-verilog), a data structure independent structural verilog parser.

In most EDA flows, data exchange is done by using standard netlist formats (Verilog, LEF/DEF, EDIF, …) which were not designed to represent data structures content with high fidelity. To address this problem, SNL relies on [Cap'n Proto](https://github.com/capnproto/capnproto) open source interchange format.

SNL also emphasizes EDA applications parallelization (targeting in particular cloud computing) by providing a robust object identification mechanism allowing to partition and merge data across the network.

SNL is summarized in below's image.
![SNL](./docs/images/Naja-SNL.png)

### Acknowledgement
[<img src="https://nlnet.nl/logo/banner.png" width=100>](https://nlnet.nl/project/Naja)
[<img src="https://nlnet.nl/image/logos/NGI0Entrust_tag.svg" width=100>](https://nlnet.nl/project/Naja)

This project is supported and funded by NLNet through the [NGI0 Entrust](https://nlnet.nl/entrust) Fund.

## Compilation
### Getting sources
```bash
# First clone the repository and go inside it
git clone https://github.com/xtofalex/naja.git
cd naja
git submodule init
git submodule update
```
### Dependencies
Mandatory dependencies:
1. Boost
2. [cmake](https://cmake.org): at least 3.22 version.
3. Python3: for building the SNL Python3 interface. This interface is used to load primitive cells (associated to Verilog parsing)
and their associated characteristics (for instance: ressource count, timing characteristics, ...).    

Optional dependencies:
1. [Doxygen](https://www.doxygen.nl): for the documentation generation.

Embedded dependencies, through git sub modules:
1. [naja-verilog](https://github.com/xtofalex/naja-verilog): for verilog parsing.
2. [google test](https://github.com/google/googletest) for unit testing.

On Ubuntu:
```bash
sudo apt-get install python3-dev
sudo apt-get install capnproto
sudo apt-get install libcapnp-dev
sudo apt-get install pkg-config
sudo apt-get install bison
sudo apt-get install flex
sudo apt-get install doxygen
```
Using [nix-shell](https://nixos.wiki/wiki/Development_environment_with_nix-shell):
```bash
nix-shell -p cmake boost python3 doxygen capnproto bison flex pkg-config
```
### Building and Installing
```bash
#First define an env variable that points to the directory where you want naja to be installed:
export NAJA_INSTALL=<path_to_installation_dir>
# Create a build dir and go inside it
mkdir build
cd build
cmake <path_to_naja_sources_dir> -DCMAKE_INSTALL_PREFIX=$NAJA_INSTALL
#For instance: cmake ~/srcs/naja -DCMAKE_INSTALL_PREFIX=$NAJA_INSTALL
make
make test
make install
```
### Building and Installing Documentation
```bash
#make sure that doxygen was available when launching the cmake command
cd build
make docs
make install
```
Documentation will be installed in $NAJA_INSTALL/doc directory. Starting file to open in browser is: $NAJA_INSTALL/doc/html/index.html.

## Use 
### Environment
After building and installing, start by setting up a runtime environment.
```bash
export NAJA_INSTALL=<path_to_installation_dir>
#For Naja python interface and in particular primitives loading
export PYTHONPATH=$PYTHONPATH:$NAJA_INSTALL/lib/python
```
### Inputs/Outputs
#### SNL Interchange Format
SNL relies on [Cap'n Proto](https://github.com/capnproto/capnproto) for data serialization and streaming. Schema files and C++ implementation can be found [here](https://github.com/xtofalex/naja/tree/main/src/snl/snl/serialization/capnp).

#### Verilog
For Verilog parsing, Naja relies on naja-verilog submodule (https://github.com/xtofalex/naja-verilog).
Leaf primitives are loaded through the Python primitive loader: [SNLPrimitivesLoader](https://github.com/xtofalex/naja/blob/main/src/snl/python/primitives/SNLPrimitivesLoader.h).
A application snippet can be found [here](https://github.com/xtofalex/naja/blob/main/src/snl/snippets/app/src/SNLVRLSnippet.cpp) and examples of
primitive libraries described using the Python interface can be found in the
[primitives](https://github.com/xtofalex/naja/blob/main/primitives) directory.

A Verilog dumper is part of SNL inside SNL API.

## Snippets
### c++
This [snippet](https://github.com/xtofalex/naja/blob/main/src/snl/snippets/app/src/SNLSnippet.cpp) shows various SNL API netlist construction, manipulation and browsing examples.
### Python
This [snippet](https://github.com/xtofalex/naja/blob/main/src/snl/snippets/python/snl_snippet.py) shows an equivalent example using Python interface.
### Application snippet
An application snippet can be found [here](https://github.com/xtofalex/naja/blob/main/src/snl/snippets/app).

This "app" directory and its contents can be copied to start a new application.
