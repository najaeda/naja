// SPDX-FileCopyrightText: 2026 The Naja authors
// SPDX-License-Identifier: Apache-2.0

#include "Vseq_timing_event_list_negedge_reset_supported.h"
#include "verilated.h"

#include <cstdio>

int main(int argc, char** argv) {
  Verilated::commandArgs(argc, argv);
  auto dut = new Vseq_timing_event_list_negedge_reset_supported;

  dut->eval();
  dut->final();
  delete dut;

  std::printf("seq_timing_event_list_negedge_reset_supported: smoke test passed\n");
  return 0;
}
