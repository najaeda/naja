// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_PERF_H_
#define __NAJA_PERF_H_

#include <stack>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>

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
          if (perf == nullptr) {
            throw NajaException("NajaPerf is not initialized");
          }
          perf->start(this);
        }
        ~Scope() {
          auto perf = NajaPerf::get();
          perf->end(this);
        }
        Scope(const Scope&) = delete;
      private:
        std::string phase_;
        MemoryUsage startMemoryUsage_   { NajaPerf::UnknownMemoryUsage, NajaPerf::UnknownMemoryUsage };
        Clock       startClock_;
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
      os_ << std::string(indent, ' ');
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(scope->startClock_ - startClock_);
      os_ << "<" << scope->phase_;
      os_ << " total:" << elapsed.count() / 1000.0 << "s";
      if (scope->startMemoryUsage_.first != NajaPerf::UnknownMemoryUsage) {
        os_ << " VM(RSS):" << scope->startMemoryUsage_.first / 1024.0 << "Mb";
        os_ << " VM(Peak):" << scope->startMemoryUsage_.second / 1024.0 << "Mb";
      } 
      os_ << ">" << std::endl;
    }
    void end(Scope* scope) {
      if (scopeStack_.empty()) {
        throw NajaException("Scope stack is empty");
      }
      if (scopeStack_.top() != scope) {
        throw NajaException("Scope stack is corrupted");
      }
      scopeStack_.pop();
      auto indent = scopeStack_.size();
      auto endMemoryUsage = NajaPerf::getMemoryUsage();
      auto endClock = std::chrono::steady_clock::now();
      os_ << std::string(indent, ' ');
      auto totalElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endClock - startClock_);
      auto scopeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endClock - scope->startClock_);
      os_ << "</" << scope->phase_ << " total:" << totalElapsed.count() / 1000.0 << "s";
      os_ << " scope:" << scopeElapsed.count() / 1000.0 << "s";
      if (endMemoryUsage.first != NajaPerf::UnknownMemoryUsage) {
        os_ << " VM(RSS):" << endMemoryUsage.first / 1024.0 << "Mb";
        os_ << " VM(Peak):" << endMemoryUsage.second / 1024.0 << "Mb";
      } 
      os_ << ">" << std::endl;
    }
    static NajaPerf*    singleton_;
    Clock               startClock_;
    std::ofstream       os_;
    ScopeStack          scopeStack_ {};
};

} // namespace naja

#endif // __NAJA_PERF_H_