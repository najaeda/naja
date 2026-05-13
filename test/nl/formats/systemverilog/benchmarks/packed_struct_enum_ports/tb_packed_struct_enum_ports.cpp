// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vpacked_struct_enum_ports_top.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vpacked_struct_enum_ports_top;

  int errors = 0;
  for (int data = 0; data < 32; ++data) {
    for (int state = 0; state < 4; ++state) {
      dut->in_s = data;
      dut->st_i = state;
      dut->eval();
      if ((dut->out_s & 0x1F) != data) {
        std::printf("FAIL: in_s=%d expected out_s=%d got out_s=%d\n",
                    data, data, static_cast<int>(dut->out_s & 0x1F));
        ++errors;
      }
      if ((dut->st_o & 0x3) != state) {
        std::printf("FAIL: st_i=%d expected st_o=%d got st_o=%d\n",
                    state, state, static_cast<int>(dut->st_o & 0x3));
        ++errors;
      }
    }
  }

  dut->final();
  delete dut;

  if (errors) {
    std::printf("packed_struct_enum_ports: %d errors\n", errors);
    return 1;
  }
  std::printf("packed_struct_enum_ports: all tests passed\n");
  return 0;
}
