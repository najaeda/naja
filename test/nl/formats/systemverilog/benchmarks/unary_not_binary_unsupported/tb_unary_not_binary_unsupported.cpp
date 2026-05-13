// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vunary_not_binary_unsupported_top.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vunary_not_binary_unsupported_top;

  for (int a = 0; a <= 1; ++a) {
    for (int b = 0; b <= 1; ++b) {
      dut->a = a;
      dut->b = b;
      dut->eval();
      std::printf("a=%d b=%d y=%d\n", a, b, static_cast<int>(dut->y));
    }
  }

  dut->final();
  delete dut;
  std::printf("unary_not_binary_unsupported: smoke test passed\n");
  return 0;
}
