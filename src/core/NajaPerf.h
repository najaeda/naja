// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <stack>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>

#include "NajaUtils.h"
#include "NajaException.h"

namespace naja {

class NajaPerf {
  public:
    static const long UnknownMemoryUsage;
    using Clock = std::chrono::steady_clock::time_point;
    using MemoryUsage = std::pair<long, long>;
    class Scope {
      friend class NajaPerf;
      public:
        Scope(const std::string& phase): phase_(phase) {
          auto perf = NajaPerf::get();
          if (perf != nullptr) {
            perf->start(this);
            started_ = true;
          }
        }
        ~Scope() {
          if (!started_) {
            return;
          }
          auto perf = NajaPerf::get();
          if (perf != nullptr) {
            perf->end(this);
          }
        }
        Scope(const Scope&) = delete;
      private:
        std::string phase_;
        MemoryUsage startMemoryUsage_   { NajaPerf::UnknownMemoryUsage, NajaPerf::UnknownMemoryUsage };
        Clock       startClock_;
        bool        started_ {false};
    };
    static MemoryUsage getMemoryUsage();
    static NajaPerf* create(const std::filesystem::path& logPath, const std::string& topName) {
      if (singleton_ == nullptr) {
        singleton_ = new NajaPerf(logPath, topName);
        registerDestructor(); // Register atexit cleanup
        //start top scope
        new Scope(topName);
      }
      return singleton_;
    }
    static NajaPerf* get() {
      return singleton_;
    }
    ~NajaPerf() {
      while (!scopeStack_.empty()) {
        auto scope = scopeStack_.top();
        delete scope;
      }
      writeSummary();
    }
    NajaPerf(const NajaPerf&) = delete;
    NajaPerf& operator=(const NajaPerf&) = delete;

    using ScopeStack = std::stack<Scope*>;
    const ScopeStack& getStack() {
      return scopeStack_;
    }
  private:
    NajaPerf(const std::filesystem::path& logPath, const std::string& topName):
      startClock_(std::chrono::steady_clock::now()) {
      os_.open(logPath);
      NajaUtils::createBanner(os_, "Naja Performance Report", "#");
      os_ << std::endl;
    }

    static void registerDestructor() {
      std::atexit(&NajaPerf::destroy);
    }

    static void destroy() {
      delete singleton_;
      singleton_ = nullptr;
    }
    
    void start(Scope* scope) {
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
    void end(Scope* scope) {
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
    static bool memoryKnown(long value) {
      return value != NajaPerf::UnknownMemoryUsage;
    }
    static double toMegabytes(long kbytes) {
      return static_cast<double>(kbytes) / 1024.0;
    }
    static double toSeconds(const std::chrono::milliseconds& duration) {
      return static_cast<double>(duration.count()) / 1000.0;
    }
    void updatePeak(const MemoryUsage& usage) {
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
    void writeSeconds(const std::chrono::milliseconds& duration) {
      os_ << std::setw(kTimeWidth) << std::fixed << std::setprecision(3)
          << toSeconds(duration) << "s";
    }
    void writeMemoryField(const char* label, long value) {
      os_ << label;
      if (memoryKnown(value)) {
        os_ << std::setw(kMemWidth) << std::fixed << std::setprecision(2)
            << toMegabytes(value) << "Mb";
      } else {
        os_ << std::setw(kMemWidth) << "n/a" << "  ";
      }
    }
    void writeDeltaMemoryFields(const MemoryUsage& start, const MemoryUsage& end) {
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
    void writeSignedMemory(double megabytes) {
      auto flags = os_.flags();
      os_ << std::showpos << std::setw(kMemWidth) << std::fixed << std::setprecision(2)
          << megabytes;
      os_.flags(flags);
      os_ << "Mb";
    }
    std::string formatPhaseTag(
      const std::string& phase,
      size_t indent,
      bool closing) const {
      std::string tag(indent * kIndentWidth, ' ');
      tag += closing ? "</" : "<";
      tag += phase;
      tag += ">";
      return tag;
    }
    void updatePhaseWidth(const std::string& label) {
      if (label.size() > phaseWidth_) {
        phaseWidth_ = label.size();
      }
    }
    void writeSummary() {
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
    void writeMemoryValue(long value) {
      os_ << std::setw(kMemWidth) << std::fixed << std::setprecision(2)
          << toMegabytes(value) << "Mb";
    }
    static constexpr int kIndentWidth {2};
    static constexpr int kTimeWidth {8};
    static constexpr int kMemWidth {8};
    static constexpr int kBasePhaseWidth {32};
    static NajaPerf*    singleton_;
    Clock               startClock_;
    std::ofstream       os_;
    ScopeStack          scopeStack_ {};
    MemoryUsage         peakMemoryUsage_ { UnknownMemoryUsage, UnknownMemoryUsage };
    size_t              phaseWidth_ { kBasePhaseWidth };
};

} // namespace naja
