// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
#include "Vstreaming_lhs_top.h"
#include "verilated.h"
#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  Vstreaming_lhs_top dut;
  for (unsigned value = 0; value < (1u << 20); ++value) {
    dut.value = value;
    dut.enable = value & 1;
    dut.eval();
    const unsigned reverse = ((value & 3) << 8) | (((value >> 2) & 15) << 4) | ((value >> 6) & 15);
    const unsigned nested = ((reverse & 3) << 8) | (((reverse >> 2) & 3) << 6) |
      (((reverse >> 4) & 3) << 4) | (((reverse >> 6) & 3) << 2) | ((reverse >> 8) & 3);
    const unsigned selected = 0x301 | ((value & 3) << 6) | (((value >> 2) & 3) << 4) |
      (((value >> 4) & 3) << 2);
    const unsigned multi = ((value & 4095) << 4) | ((value >> 12) & 15);
    const unsigned temp = (value ^ 0x1234) & 65535;
    const unsigned rhs = ((temp & 255) << 8) | (temp >> 8);
    if (dut.stream_rhs_o != rhs || dut.forward_o != ((value & 1) ? value >> 10 : 0) || dut.reverse_o != reverse ||
        dut.nested_o != nested || dut.selected_o != selected || dut.swap_o != (value & 255) ||
        dut.array_o != ((value & 255) * 257) || dut.multi_o != multi) {
      std::printf("FAIL streaming value=%x fwd=%x rev=%x nested=%x selected=%x swap=%x array=%x multi=%x\n",
        value, dut.forward_o, dut.reverse_o, dut.nested_o, dut.selected_o, dut.swap_o, dut.array_o, dut.multi_o);
      return 1;
    }
  }
  std::puts("streaming_lhs: 1048576 vectors passed");
  dut.final();
}
