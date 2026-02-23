// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vbinary_ops_supported_top.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vbinary_ops_supported_top;

  int errors = 0;
  auto check = [&](const char* signal, int a, int b, int got, int expected) {
    if (got != expected) {
      std::printf("FAIL: %s a=%d b=%d expected=%d got=%d\n", signal, a, b, expected, got);
      ++errors;
    }
  };

  for (int s = 0; s < 16; ++s) {
    dut->s = s;
    for (int a = 0; a <= 1; ++a) {
      for (int b = 0; b <= 1; ++b) {
        dut->a = a;
        dut->b = b;
        dut->eval();

        check("y_and", a, b, dut->y_and, a & b);
        check("y_land", a, b, dut->y_land, a && b);
        check("y_or", a, b, dut->y_or, a | b);
        check("y_lor", a, b, dut->y_lor, a || b);
        check("y_xor", a, b, dut->y_xor, a ^ b);
        check("y_xnor", a, b, dut->y_xnor, (a == b) ? 1 : 0);
        if ((dut->y_shr & 0xF) != ((s >> 1) & 0xF)) {
          std::printf("FAIL: y_shr s=0x%x expected=0x%x got=0x%x\n",
                      s, (s >> 1) & 0xF, static_cast<int>(dut->y_shr & 0xF));
          ++errors;
        }
      }
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("binary_ops_supported: %d errors\n", errors);
    return 1;
  }
  std::printf("binary_ops_supported: all tests passed\n");
  return 0;
}
