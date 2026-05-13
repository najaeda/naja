// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vbyte_ports_top.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vbyte_ports_top;

  int errors = 0;
  for (int value = 0; value < 256; ++value) {
    dut->a = value;
    dut->eval();
    if ((dut->y & 0xFF) != value) {
      std::printf("FAIL: a=%d expected y=%d got y=%d\n", value, value, static_cast<int>(dut->y));
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("byte_ports: %d errors\n", errors);
    return 1;
  }
  std::printf("byte_ports: all tests passed\n");
  return 0;
}
