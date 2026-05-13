// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_rhs_direct_match.h"
#include "verilated.h"

#include <cstdio>

static void tick(Vseq_rhs_direct_match* dut) {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_rhs_direct_match;

  int errors = 0;

  dut->clk = 0;
  dut->reset = 1;
  dut->din = 0;
  tick(dut);

  dut->reset = 0;
  const unsigned patterns[] = {0x12u, 0xA5u, 0x00u, 0xFFu, 0x3Cu};
  for (auto value : patterns) {
    dut->din = value;
    tick(dut);
    if ((dut->out & 0xFFu) != value) {
      std::printf("FAIL: expected out=0x%02X got 0x%02X\n",
                  value,
                  static_cast<unsigned>(dut->out & 0xFFu));
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("seq_rhs_direct_match: %d errors\n", errors);
    return 1;
  }
  std::printf("seq_rhs_direct_match: all tests passed\n");
  return 0;
}
