// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_rhs_unresolved.h"
#include "verilated.h"

#include <cstdio>

static void tick(Vseq_rhs_unresolved* dut) {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_rhs_unresolved;

  dut->clk = 0;
  dut->reset = 1;
  tick(dut);
  dut->reset = 0;
  for (int i = 0; i < 8; ++i) {
    tick(dut);
  }

  std::printf("seq_rhs_unresolved: smoke test passed (out=%d)\n", static_cast<int>(dut->out));
  dut->final();
  delete dut;
  return 0;
}
