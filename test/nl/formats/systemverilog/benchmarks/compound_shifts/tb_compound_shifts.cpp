// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "Vcompound_shifts_top.h"
#include "verilated.h"
#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  Vcompound_shifts_top dut;
  for (unsigned value = 0; value < 256; ++value) {
    for (unsigned amount = 0; amount < 1024; ++amount) {
      dut.value = value;
      dut.amount = amount;
      dut.eval();
      const unsigned left = amount < 8 ? (value << amount) & 255 : 0;
      const unsigned right = amount < 8 ? value >> amount : 0;
      const unsigned arithmetic = amount < 8
        ? ((value >> amount) | ((value & 128) ? (255 << (8 - amount)) : 0)) & 255
        : ((value & 128) ? 255 : 0);
      const unsigned replay = amount < 8 ? ((value << 1) & 255) >> amount : 0;
      const unsigned field = amount < 4 ? (value << amount) & 15 : 0;
      if (dut.left_o != left || dut.arithmetic_left_o != left ||
          dut.right_o != right || dut.unsigned_arithmetic_o != right ||
          dut.signed_right_o != arithmetic || dut.replay_o != replay ||
          dut.giant_o != ((value & 1) ? 0 : left) || dut.field_o != field || dut.constant_o != (value >> 2)) {
        std::printf("FAIL compound shifts value=%u amount=%u left=%u right=%u signed=%u replay=%u field=%u\n",
          value, amount, dut.left_o, dut.right_o, dut.signed_right_o, dut.replay_o, dut.field_o);
        return 1;
      }
    }
  }
  std::puts("compound_shifts: 262144 vectors passed");
  dut.final();
}
