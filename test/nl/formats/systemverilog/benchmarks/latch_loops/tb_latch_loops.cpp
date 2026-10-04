// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "Vlatch_loops_top.h"
#include "verilated.h"
#include <cstdint>
#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  Vlatch_loops_top dut;
  uint32_t words = 0;
  unsigned priority = 0, partial = 0, branch = 0;
  auto check = [&](unsigned en, unsigned data, unsigned other, unsigned overrideEn) {
    dut.en = en;
    dut.d = data;
    dut.other = other;
    dut.override_en = overrideEn;
    for (unsigned i = 0; i < 4; ++i) {
      if (en & (1u << i)) words = (words & ~(255u << (8*i))) | (data << (8*i));
    }
    if (en & 1) priority = data;
    if (overrideEn) priority = other;
    if (en & 2) partial = (partial & 240) | (data & 15);
    if (en & 4) partial = (partial & 15) | (data & 240);
    if (en & 8) partial = (partial & 195) | ((overrideEn ? other & 15 : other >> 4) << 2);
    if (en & 2) branch = (branch & 240) | (data & 15);
    else if (en & 4) branch = (branch & 15) | (other & 240);
    dut.eval();
    if (dut.array_o != words || dut.nested_o != words || dut.priority_o != priority ||
        dut.partial_o != partial || dut.zero_o != 0 ||
        dut.nb_priority_o != priority || dut.branch_o != branch) {
      std::printf("FAIL en=%u data=%u other=%u override=%u array=%x/%x nested=%x priority=%u/%u partial=%u/%u\n",
        en, data, other, overrideEn, dut.array_o, words, dut.nested_o,
        dut.priority_o, priority, dut.partial_o, partial);
      return false;
    }
    return true;
  };
  // Open every latch first; subsequent checks do not assume initial state.
  dut.en = 4;
  dut.d = 0;
  dut.other = 0;
  dut.override_en = 0;
  dut.eval();
  if (!check(15, 0, 0, 1)) return 1;
  for (unsigned data = 0; data < 256; ++data) {
    for (unsigned en = 0; en < 16; ++en) {
      for (unsigned overrideEn = 0; overrideEn < 2; ++overrideEn) {
        unsigned other = (data * 73 + en * 11) & 255;
        if (!check(en, data, other, overrideEn)) return 1;
        if (!check(0, data ^ 255, other ^ 255, 0)) return 1;
      }
    }
  }
  std::puts("latch_loops: 16385 state transitions passed");
  dut.final();
}
