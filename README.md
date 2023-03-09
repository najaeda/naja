# Naja

![build](https://github.com/xtofalex/naja/actions/workflows/build.yml/badge.svg)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/b224740790e24c80a381a6eede28cad8)](https://app.codacy.com/gh/xtofalex/naja?utm_source=github.com&utm_medium=referral&utm_content=xtofalex/naja&utm_campaign=Badge_Grade_Settings)
[![codecov](https://codecov.io/gh/xtofalex/naja/branch/main/graph/badge.svg?token=59ZKZ74HFP)](https://codecov.io/gh/xtofalex/naja)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
***
## Introduction
Naja is an EDA (Electronic Design Automation) project aiming at offering open source data structures and APIs for the development of post logic synthesis EDA algorithms such as: netlist simplification (constant and dead logic propagation), logic replication, netlist partitioning, ASIC and FPGA place and route, …

In most EDA flows, data exchange is done by using standard netlist formats (Verilog, LEF/DEF, EDIF, …) which were not designed to represent data structures content with high fidelity. To address this problem, Naja relies on [Cap'n Proto](https://github.com/capnproto/capnproto) open source interchange format.

Naja also emphasizes EDA applications parallelization (targeting in particular cloud computing) by providing a robust object identification mechanism allowing to partition and merge data across the network.

Naja contains mainly the SNL (Structured Netlist) API.
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
2. cmake: at least 3.22 version.
4. Python3: for building the SNL Python3 interface used to load primitive cells. 

Optional dependencies:
1. Doxygen: for the documentation

Embedded dependencies, through git sub modules:
1. naja-verilog (https://github.com/xtofalex/naja-verilog): for verilog parsing.
2. google test (https://github.com/google/googletest) for unit testing.

On Ubuntu:
```bash
sudo apt-get install python3-dev
sudo apt-get install libcapnp-dev
sudo apt-get install pkg-config
sudo apt-get install bison
sudo apt-get install flex
sudo apt-get install doxygen
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

## Inputs/Outputs
### SNL Interchange Format
SNL relies on [Cap'n Proto](https://github.com/capnproto/capnproto) for data serialization and streaming. Schema files and C++ implementation can be found [here](https://github.com/xtofalex/naja/tree/main/src/snl/snl/serialization/capnp).

### Verilog
For Verilog parsing, Naja relies on naja-verilog submodule (https://github.com/xtofalex/naja-verilog).

A Verilog dumper is part of SNL inside SNL API.

## Snippets
### c++
https://github.com/xtofalex/naja/blob/main/src/snl/snippets/app/src/SNLSnippet.cpp
### Python
https://github.com/xtofalex/naja/blob/main/src/snl/snippets/python/snl_snippet.py
### Application snippet
An application snippet can be found at:

https://github.com/xtofalex/naja/blob/main/src/snl/snippets/app

This "app" directory and its contents can be copied to start a new application.
