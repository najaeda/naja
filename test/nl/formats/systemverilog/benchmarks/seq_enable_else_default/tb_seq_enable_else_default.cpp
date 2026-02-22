// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_enable_else_default.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_enable_else_default;

  dut->eval();
  dut->final();
  delete dut;

  std::printf("seq_enable_else_default: smoke test passed\n");
  return 0;
}
