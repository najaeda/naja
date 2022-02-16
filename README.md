# Naja
***
![build](https://github.com/xtofalex/naja/actions/workflows/build.yml/badge.svg)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/b224740790e24c80a381a6eede28cad8)](https://app.codacy.com/gh/xtofalex/naja?utm_source=github.com&utm_medium=referral&utm_content=xtofalex/naja&utm_campaign=Badge_Grade_Settings)
[![codecov](https://codecov.io/gh/xtofalex/naja/branch/main/graph/badge.svg?token=59ZKZ74HFP)](https://codecov.io/gh/xtofalex/naja)
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

For convenience, google test (https://github.com/google/googletest) is provided through git submodule.

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
Documentation will be installed in $NAJA_INSTALL/doc directory. Starting file to open in browser is: $NAJA_INSTALL/doc/html/index.html.
## Snippets
### c++
https://github.com/xtofalex/naja/blob/main/src/snl/snippet/app/src/SNLSnippet.cpp
### Python
https://github.com/xtofalex/naja/blob/main/src/snl/snippet/python/snl_snippet.py
### Application snippet
An application snippet can be found at:

https://github.com/xtofalex/naja/blob/main/src/snl/snippet/app

This "app" directory and its contents can be copied to start a new application.


