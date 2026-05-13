// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_reset_all_zero_literal_wide_supported.h"
#include "verilated.h"

#include <cstdio>

static void tick(Vseq_reset_all_zero_literal_wide_supported* dut) {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_reset_all_zero_literal_wide_supported;

  int errors = 0;

  dut->clk = 0;
  dut->reset = 1;
  dut->d[0] = 0xCAFEBABE;
  dut->d[1] = 0xDEADBEEF;
  dut->d[2] = 0x89ABCDEF;
  dut->d[3] = 0x01234567;
  tick(dut);

  for (int i = 0; i < 4; ++i) {
    if (dut->q[i] != 0) {
      std::printf("FAIL: expected q='0 during reset, got q[%d]=0x%08x\n", i, dut->q[i]);
      ++errors;
    }
  }

  dut->reset = 0;
  dut->d[0] = 0x01234567;
  dut->d[1] = 0x89ABCDEF;
  dut->d[2] = 0xFEDCBA98;
  dut->d[3] = 0x76543210;
  tick(dut);

  for (int i = 0; i < 4; ++i) {
    if (dut->q[i] != dut->d[i]) {
      std::printf("FAIL: expected q[%d]=d[%d]=0x%08x got 0x%08x\n", i, i, dut->d[i], dut->q[i]);
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("seq_reset_all_zero_literal_wide_supported: %d errors\n", errors);
    return 1;
  }

  std::printf("seq_reset_all_zero_literal_wide_supported: all tests passed\n");
  return 0;
}
