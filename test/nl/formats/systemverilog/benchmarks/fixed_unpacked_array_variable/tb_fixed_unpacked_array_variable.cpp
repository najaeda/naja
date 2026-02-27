// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vfixed_unpacked_array_variable_top.h"
#include "verilated.h"

#include <cstdint>
#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vfixed_unpacked_array_variable_top;

  int errors = 0;
  for (uint32_t i = 0; i < 128; ++i) {
    const uint32_t in0 = 0x13579BDFu ^ (i * 0x1020304u);
    const uint32_t in1 = 0x2468ACE0u ^ (i * 0x11223344u);
    dut->in0 = in0;
    dut->in1 = in1;
    dut->eval();
    if (dut->y != in0) {
      std::printf("FAIL: in0=0x%08x in1=0x%08x expected y=0x%08x got y=0x%08x\n",
                  in0, in1, in0, static_cast<uint32_t>(dut->y));
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("fixed_unpacked_array_variable: %d errors\n", errors);
    return 1;
  }
  std::printf("fixed_unpacked_array_variable: all tests passed\n");
  return 0;
}
