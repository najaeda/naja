# naja
***
![build](https://github.com/xtofalex/naja/actions/workflows/build.yml/badge.svg)
***
## Introduction
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
3. cmake: at least 3.19 version

Optional dependencies:
1. Python3: for building the Python3 interface
2. Doxygen: for the documentation
### Building
```bash
# Create a build dir and go inside it
mkdir build
cd build
cmake <path_to_naja_sources_dir> --prefix <path_to_installation_dir>
#For instance: cmake ~/srcs/naja ~/install
make
make test
make install
```
## Snippets
### c++
https://github.com/xtofalex/naja/blob/main/src/snippet/cpp/SNLSnippet.cpp
### python
https://github.com/xtofalex/naja/blob/main/src/snippet/python/snl_snippet.py
## Main objects
### Object identification
