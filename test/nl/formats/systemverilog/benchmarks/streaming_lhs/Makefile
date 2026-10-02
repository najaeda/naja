# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

TOP := streaming_lhs_top
# Verilator disagrees with Slang on partial LHS slices and target swaps.
# Simulate the Naja dump against the independently checked golden values.
# Generate with ../generate_najaeda_verilog.py first.
SV := streaming_lhs_naja.v ../najaeda_primitives.v
TB := tb_streaming_lhs.cpp

include ../Makefile.verilator
