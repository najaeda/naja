// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_concurrent_assertion_ignored.h"
#include "verilated.h"

#include <cstdio>

static void tick(Vseq_concurrent_assertion_ignored* dut) {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_concurrent_assertion_ignored;

  int errors = 0;

  dut->clk = 0;
  dut->reset = 1;
  dut->d = 0;
  dut->eval();

  tick(dut);
  if (dut->q != 0) {
    std::printf("FAIL: expected q=0 during reset, got %d\n", static_cast<int>(dut->q));
    ++errors;
  }

  dut->reset = 0;
  dut->d = 1;
  tick(dut);
  if (dut->q != 1) {
    std::printf("FAIL: expected q=1 after first data tick, got %d\n", static_cast<int>(dut->q));
    ++errors;
  }

  dut->d = 0;
  tick(dut);
  if (dut->q != 0) {
    std::printf("FAIL: expected q=0 after second data tick, got %d\n", static_cast<int>(dut->q));
    ++errors;
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("seq_concurrent_assertion_ignored: %d errors\n", errors);
    return 1;
  }

  std::printf("seq_concurrent_assertion_ignored: all tests passed\n");
  return 0;
}
