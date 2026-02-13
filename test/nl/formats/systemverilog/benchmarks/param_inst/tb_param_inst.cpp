// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vleaf.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vleaf;

  int errors = 0;
  // Test: y should equal a for several values (leaf is a passthrough)
  // Verilator elaborates leaf with default W=1, so a and y are 1-bit
  for (int val = 0; val <= 1; ++val) {
    dut->a = val;
    dut->eval();
    if (dut->y != val) {
      printf("FAIL: a=%d expected y=%d got y=%d\n",
             val, val, (int)dut->y);
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    printf("param_inst: %d errors\n", errors);
    return 1;
  }
  printf("param_inst: all tests passed\n");
  return 0;
}
