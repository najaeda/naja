# Naja
***
![build](https://github.com/xtofalex/naja/actions/workflows/build.yml/badge.svg)
***
## Introduction
Naja is an umbrella repository containing mainly (for the moment) the SNL netlist data structure.

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
2. libxml2 
3. cmake: at least 3.22 version

Optional dependencies:
1. Python3: for building the Python3 interface
2. Doxygen: for the documentation

Embedded dependencies:

For convenience, the following dependencies are provided through git submodules:
1. google test https://github.com/google/googletest
2. range-v3 https://github.com/ericniebler/range-v3: Naja relies on range-v3 for everything related to browsing of the data structures. range-v3 is not only embedded in the sources but also part of the installation tree to avoid versions crash.

### Building
```bash
#First define an env variable that points to the directory where you want naja to be installed:
export NAJA_INSTALL=<path_to_installation_dir>
# Create a build dir and go inside it
mkdir build
cd build
cmake <path_to_naja_sources_dir> --prefix $NAJA_INSTALL
#For instance: cmake ~/srcs/naja --prefix $NAJA_INSTALL
make
make test
make install
```
### Building documentation
```bash
#make sure that doxygen was available when launching the cmake command
cd build
make docs
make install
```
## Snippets
### c++
https://github.com/xtofalex/naja/blob/main/src/snippet/app/src/SNLSnippet.cpp
### python
https://github.com/xtofalex/naja/blob/main/src/snippet/python/snl_snippet.py
### Application snippet
An application snippet can be found at:

https://github.com/xtofalex/naja/blob/main/src/snippet/app

This "app" directory and its contents can be copied to start a new application.


