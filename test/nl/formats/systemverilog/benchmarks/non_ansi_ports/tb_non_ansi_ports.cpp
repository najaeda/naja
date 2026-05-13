// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vnon_ansi_ports_top.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vnon_ansi_ports_top;

  dut->eval();
  dut->final();
  delete dut;

  std::printf("non_ansi_ports: smoke test passed\n");
  return 0;
}
