// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_preincrement_supported.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_preincrement_supported;

  dut->eval();
  dut->final();
  delete dut;

  std::printf("seq_preincrement_supported: smoke test passed\n");
  return 0;
}
