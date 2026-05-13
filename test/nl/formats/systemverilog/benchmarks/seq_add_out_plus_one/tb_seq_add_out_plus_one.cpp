// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_add_out_plus_one.h"
#include "verilated.h"

#include <cstdio>

static void tick(Vseq_add_out_plus_one* dut) {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_add_out_plus_one;

  int errors = 0;

  dut->clk = 0;
  dut->reset = 1;
  tick(dut);
  if (dut->out != 0) {
    std::printf("FAIL: expected out=0 after reset, got %d\n", static_cast<int>(dut->out));
    ++errors;
  }

  dut->reset = 0;
  for (int expected = 1; expected <= 8; ++expected) {
    tick(dut);
    if (dut->out != expected) {
      std::printf("FAIL: expected out=%d got %d\n", expected, static_cast<int>(dut->out));
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("seq_add_out_plus_one: %d errors\n", errors);
    return 1;
  }
  std::printf("seq_add_out_plus_one: all tests passed\n");
  return 0;
}
