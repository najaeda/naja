// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vtop.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto top = new Vtop;

  int errors = 0;
  // Exhaustively check y = a & b for all input combinations
  for (int a = 0; a <= 1; ++a) {
    for (int b = 0; b <= 1; ++b) {
      top->a = a;
      top->b = b;
      top->eval();
      int expected = a & b;
      if (top->y != expected) {
        printf("FAIL: a=%d b=%d expected y=%d got y=%d\n",
               a, b, expected, (int)top->y);
        ++errors;
      }
    }
  }

  top->final();
  delete top;

  if (errors) {
    printf("simple: %d errors\n", errors);
    return 1;
  }
  printf("simple: all tests passed\n");
  return 0;
}
