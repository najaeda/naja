// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vunsupported_generic_type.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vunsupported_generic_type;

  int errors = 0;
  for (int cycle = 0; cycle < 8; ++cycle) {
    dut->eval();
    if (dut->y != 0) {
      std::printf("FAIL: cycle=%d expected y=0 got y=%d\n", cycle, static_cast<int>(dut->y));
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("unsupported_generic_type: %d errors\n", errors);
    return 1;
  }
  std::printf("unsupported_generic_type: all tests passed\n");
  return 0;
}
