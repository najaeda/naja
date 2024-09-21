// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaPerf.h"
#include <fstream>
#include <sstream>

namespace naja {

NajaPerf* NajaPerf::singleton_ = nullptr;

long NajaPerf::getMemoryUsage() {
#ifdef __linux__
  std::ifstream file("/proc/self/status");
  std::string line;
  long memoryUsage = -1;

  while (std::getline(file, line)) {
    if (line.substr(0, 6) == "VmRSS:") {
      std::istringstream iss(line);
      std::string key;
      long value;
      std::string unit;
      iss >> key >> value >> unit;
      memoryUsage = value; // Memory usage in kilobytes
      break;
    }
  }
  return memoryUsage;
#endif
  return NajaPerf::UnknownMemoryUsage;
}

} // namespace naja