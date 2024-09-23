// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaPerf.h"
#include <fstream>
#include <sstream>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace naja {

NajaPerf* NajaPerf::singleton_ = nullptr;
const long NajaPerf::UnknownMemoryUsage = std::numeric_limits<long>::max();

NajaPerf::MemoryUsage NajaPerf::getMemoryUsage() {
#ifdef __linux__
  std::ifstream file("/proc/self/status");
  std::string line;
  long vmRSS = -1;
  long vmPeak = -1;

  bool foundVmRSS = false;
  bool foundVmPeak = false;
  while (std::getline(file, line)) {
    if (line.substr(0, 6) == "VmRSS:") {
      std::istringstream iss(line);
      std::string key;
      long value;
      std::string unit;
      iss >> key >> value >> unit;
      vmRSS = value; // Memory usage in kilobytes
      foundVmRSS = true;
    }
    if (line.substr(0, 7) == "VmPeak:") {
      std::istringstream iss(line);
      std::string key;
      long value;
      std::string unit;
      iss >> key >> value >> unit;
      vmPeak = value; // Memory usage in kilobytes
      foundVmPeak = true;
    }
    if (foundVmRSS and foundVmPeak) {
      break;
    }
  }
  return std::make_pair(vmRSS, vmPeak);
#elif defined(__APPLE__)
  // Get current task (process) information
  mach_task_basic_info info;
  mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;

  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
    return std::make_pair(NajaPerf::UnknownMemoryUsage, NajaPerf::UnknownMemoryUsage); // Return error values
  }

  // Resident set size (physical memory used by the process)
  long vmRSS = info.resident_size / 1024; // in kilobytes

  // Peak resident set size (using getrusage)
  struct rusage r_usage;
  if (getrusage(RUSAGE_SELF, &r_usage) != 0) {
    return std::make_pair(vmRSS, NajaPerf::UnknownMemoryUsage); // Error fetching peak memory usage
  }
  
  long vmPeak = r_usage.ru_maxrss / 1024; // in kilobytes

  return std::make_pair(vmRSS, vmPeak);
#endif
  return std::make_pair(NajaPerf::UnknownMemoryUsage, NajaPerf::UnknownMemoryUsage);
}

} // namespace naja