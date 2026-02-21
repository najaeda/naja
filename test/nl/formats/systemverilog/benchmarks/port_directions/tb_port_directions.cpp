// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vport_directions_top.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vport_directions_top;

  dut->eval();
  dut->final();
  delete dut;

  std::printf("port_directions: smoke test passed\n");
  return 0;
}
