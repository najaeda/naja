// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vup_counter.h"
#include <cstdio>
#include <cstdlib>

static void tick(Vup_counter* dut) {
  dut->clk = 0;
  dut->eval();
  dut->clk = 1;
  dut->eval();
}

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vup_counter;

  int errors = 0;

  // Reset sequence
  dut->clk = 0;
  dut->reset = 1;
  dut->enable = 0;
  tick(dut);
  tick(dut);

  if (dut->out != 0) {
    printf("FAIL: after reset, expected out=0 got out=%d\n", (int)dut->out);
    ++errors;
  }

  // Release reset, enable counting
  dut->reset = 0;
  dut->enable = 1;

  uint8_t expected = 0;
  for (int i = 0; i < 20; ++i) {
    tick(dut);
    ++expected;
    if (dut->out != expected) {
      printf("FAIL: cycle %d, expected out=%d got out=%d\n",
             i, (int)expected, (int)dut->out);
      ++errors;
    }
  }

  // Disable enable, counter should hold
  dut->enable = 0;
  uint8_t held = expected;
  for (int i = 0; i < 5; ++i) {
    tick(dut);
    if (dut->out != held) {
      printf("FAIL: hold cycle %d, expected out=%d got out=%d\n",
             i, (int)held, (int)dut->out);
      ++errors;
    }
  }

  // Re-enable, should continue from held value
  dut->enable = 1;
  for (int i = 0; i < 5; ++i) {
    tick(dut);
    ++expected;
    if (dut->out != expected) {
      printf("FAIL: resume cycle %d, expected out=%d got out=%d\n",
             i, (int)expected, (int)dut->out);
      ++errors;
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    printf("up_counter: %d errors\n", errors);
    return 1;
  }
  printf("up_counter: all tests passed\n");
  return 0;
}
