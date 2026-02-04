// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <stack>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>

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
        Scope(const std::string& phase);
        ~Scope();
        Scope(const Scope&) = delete;
      private:
        std::string phase_;
        MemoryUsage startMemoryUsage_   { NajaPerf::UnknownMemoryUsage, NajaPerf::UnknownMemoryUsage };
        Clock       startClock_;
        bool        started_ {false};
    };
    static MemoryUsage getMemoryUsage();
    static NajaPerf* create(const std::filesystem::path& logPath, const std::string& topName);
    static NajaPerf* get();
    ~NajaPerf();
    NajaPerf(const NajaPerf&) = delete;
    NajaPerf& operator=(const NajaPerf&) = delete;

    using ScopeStack = std::stack<Scope*>;
    const ScopeStack& getStack();
  private:
    NajaPerf(const std::filesystem::path& logPath, const std::string& topName);

    static void registerDestructor();
    static void destroy();
    
    void start(Scope* scope);
    void end(Scope* scope);
    static bool memoryKnown(long value);
    static double toMegabytes(long kbytes);
    static double toSeconds(const std::chrono::milliseconds& duration);
    void updatePeak(const MemoryUsage& usage);
    void writeSeconds(const std::chrono::milliseconds& duration);
    void writeMemoryField(const char* label, long value);
    void writeDeltaMemoryFields(const MemoryUsage& start, const MemoryUsage& end);
    void writeSignedMemory(double megabytes);
    std::string formatPhaseTag(
      const std::string& phase,
      size_t indent,
      bool closing) const;
    void updatePhaseWidth(const std::string& label);
    void writeSummary();
    void writeMemoryValue(long value);
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
