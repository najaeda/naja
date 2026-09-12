# Naja bundled pyslang adapter

This directory contains Naja's adaptation layer around the Python bindings from
the pinned `thirdparty/slang` checkout. It is kept outside Naja's first-party
netlist bindings because parts of the extension entry point and numeric shim
are derived from upstream pyslang sources and retain their MIT license.

The upstream binding implementation is still compiled directly from
`thirdparty/slang/bindings/python`. The files here add the Naja-specific module
entry point, private slang runtime identity, pybind11 isolation, generated-stub
normalization, and packaging rules required to publish it as
`najaeda.pyslang`.

`src/nl/python/naja_wrapping/NajaPyslangAPI.h` intentionally remains with
Naja's Python API headers. It defines Naja's versioned cross-module capsule
contract rather than implementing or modifying upstream pyslang.
