// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaPerf.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace naja {

NajaPerf* NajaPerf::singleton_ = nullptr;
const long NajaPerf::UnknownMemoryUsage = std::numeric_limits<long>::max();

NajaPerf::Scope::Scope(const std::string& phase): phase_(phase) {
  auto perf = NajaPerf::get();
  if (perf != nullptr) {
    perf->start(this);
    started_ = true;
  }
}

NajaPerf::Scope::~Scope() {
  if (!started_) {
    return;
  }
  auto perf = NajaPerf::get();
  if (perf != nullptr) {
    perf->end(this);
  }
}

NajaPerf* NajaPerf::create(const std::filesystem::path& logPath, const std::string& topName) {
  if (singleton_ == nullptr) {
    singleton_ = new NajaPerf(logPath, topName);
    registerDestructor(); // Register atexit cleanup
    //start top scope
    new Scope(topName);
  }
  return singleton_;
}

NajaPerf* NajaPerf::get() {
  return singleton_;
}

NajaPerf::~NajaPerf() {
  while (!scopeStack_.empty()) {
    auto scope = scopeStack_.top();
    delete scope;
  }
  writeSummary();
}

const NajaPerf::ScopeStack& NajaPerf::getStack() {
  return scopeStack_;
}

NajaPerf::NajaPerf(const std::filesystem::path& logPath, const std::string& topName):
  startClock_(std::chrono::steady_clock::now()) {
  os_.open(logPath);
  NajaUtils::createBanner(os_, "Naja Performance Report", "#");
  os_ << std::endl;
}

void NajaPerf::registerDestructor() {
  std::atexit(&NajaPerf::destroy);
}

void NajaPerf::destroy() {
  delete singleton_;
  singleton_ = nullptr;
}

void NajaPerf::start(Scope* scope) {
  scope->startMemoryUsage_ = NajaPerf::getMemoryUsage();
  scope->startClock_ = std::chrono::steady_clock::now();
  auto indent = scopeStack_.size();
  scopeStack_.push(scope);
  updatePeak(scope->startMemoryUsage_);
  auto tag = formatPhaseTag(scope->phase_, indent, false);
  updatePhaseWidth(tag);
  os_ << std::left << std::setw(static_cast<int>(phaseWidth_)) << tag;
  os_ << std::right;
  auto elapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(scope->startClock_ - startClock_);
  os_ << " total:";
  writeSeconds(elapsed);
  writeMemoryField(" rss:", scope->startMemoryUsage_.first);
  writeMemoryField(" peak:", scope->startMemoryUsage_.second);
  os_ << std::endl;
}

void NajaPerf::end(Scope* scope) {
  if (scopeStack_.empty()) {
    throw NajaException("Scope stack is empty"); //LCOV_EXCL_LINE
  }
  if (scopeStack_.top() != scope) {
    throw NajaException("Scope stack is corrupted"); //LCOV_EXCL_LINE
  }
  scopeStack_.pop();
  auto indent = scopeStack_.size();
  auto endMemoryUsage = NajaPerf::getMemoryUsage();
  updatePeak(endMemoryUsage);
  auto endClock = std::chrono::steady_clock::now();
  auto tag = formatPhaseTag(scope->phase_, indent, true);
  updatePhaseWidth(tag);
  os_ << std::left << std::setw(static_cast<int>(phaseWidth_)) << tag;
  os_ << std::right;
  auto totalElapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(endClock - startClock_);
  auto scopeElapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(endClock - scope->startClock_);
  os_ << " total:";
  writeSeconds(totalElapsed);
  os_ << " scope:";
  writeSeconds(scopeElapsed);
  writeMemoryField(" rss:", endMemoryUsage.first);
  writeMemoryField(" peak:", endMemoryUsage.second);
  writeDeltaMemoryFields(scope->startMemoryUsage_, endMemoryUsage);
  os_ << std::endl;
}

bool NajaPerf::memoryKnown(long value) {
  return value != NajaPerf::UnknownMemoryUsage;
}

double NajaPerf::toMegabytes(long kbytes) {
  return static_cast<double>(kbytes) / 1024.0;
}

double NajaPerf::toSeconds(const std::chrono::milliseconds& duration) {
  return static_cast<double>(duration.count()) / 1000.0;
}

void NajaPerf::updatePeak(const MemoryUsage& usage) {
  if (memoryKnown(usage.first)) {
    if (peakMemoryUsage_.first == NajaPerf::UnknownMemoryUsage
      || usage.first > peakMemoryUsage_.first) {
      peakMemoryUsage_.first = usage.first;
    }
  }
  if (memoryKnown(usage.second)) {
    if (peakMemoryUsage_.second == NajaPerf::UnknownMemoryUsage
      || usage.second > peakMemoryUsage_.second) {
      peakMemoryUsage_.second = usage.second;
    }
  }
}

void NajaPerf::writeSeconds(const std::chrono::milliseconds& duration) {
  os_ << std::setw(kTimeWidth) << std::fixed << std::setprecision(3)
      << toSeconds(duration) << "s";
}

void NajaPerf::writeMemoryField(const char* label, long value) {
  os_ << label;
  if (memoryKnown(value)) {
    os_ << std::setw(kMemWidth) << std::fixed << std::setprecision(2)
        << toMegabytes(value) << "Mb";
  } else {
    os_ << std::setw(kMemWidth) << "n/a" << "  "; // LCOV_EXCL_LINE
  }
}

void NajaPerf::writeDeltaMemoryFields(const MemoryUsage& start, const MemoryUsage& end) {
  if (memoryKnown(start.first) && memoryKnown(end.first)) {
    auto delta = toMegabytes(end.first - start.first);
    os_ << " drss:";
    writeSignedMemory(delta);
  }
  if (memoryKnown(start.second) && memoryKnown(end.second)) {
    auto delta = toMegabytes(end.second - start.second);
    os_ << " dpeak:";
    writeSignedMemory(delta);
  }
}

void NajaPerf::writeSignedMemory(double megabytes) {
  auto flags = os_.flags();
  os_ << std::showpos << std::setw(kMemWidth) << std::fixed << std::setprecision(2)
      << megabytes;
  os_.flags(flags);
  os_ << "Mb";
}

std::string NajaPerf::formatPhaseTag(
  const std::string& phase,
  size_t indent,
  bool closing) const {
  std::string tag(indent * kIndentWidth, ' ');
  tag += closing ? "</" : "<";
  tag += phase;
  tag += ">";
  return tag;
}

void NajaPerf::updatePhaseWidth(const std::string& label) {
  phaseWidth_ = std::max(phaseWidth_, label.size());
}

void NajaPerf::writeSummary() {
  auto endClock = std::chrono::steady_clock::now();
  auto totalElapsed =
    std::chrono::duration_cast<std::chrono::milliseconds>(endClock - startClock_);
  os_ << std::string(80, '-') << std::endl;
  os_ << "Summary total:";
  writeSeconds(totalElapsed);
  if (memoryKnown(peakMemoryUsage_.first)) {
    os_ << " max rss:";
    writeMemoryValue(peakMemoryUsage_.first);
  }
  if (memoryKnown(peakMemoryUsage_.second)) {
    os_ << " max peak:";
    writeMemoryValue(peakMemoryUsage_.second);
  }
  os_ << std::endl;
}

void NajaPerf::writeMemoryValue(long value) {
  os_ << std::setw(kMemWidth) << std::fixed << std::setprecision(2)
      << toMegabytes(value) << "Mb";
}

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
