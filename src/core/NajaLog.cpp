// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NajaLog.h"

#include <cstdlib>
#include <mutex>

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace naja::log {
namespace {
std::shared_ptr<spdlog::logger> logger_;
std::mutex logger_mutex_;
std::string pattern_ = "[naja] [%^%l%$] %v";

std::shared_ptr<spdlog::logger> ensureLogger() {
  std::lock_guard<std::mutex> lock(logger_mutex_);
  if (logger_) {
    return logger_;
  }
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  console_sink->set_pattern(pattern_);
  logger_ = std::make_shared<spdlog::logger>("naja", console_sink);
  logger_->set_level(spdlog::level::trace);
  logger_->set_pattern(pattern_);
  spdlog::set_default_logger(logger_);
  return logger_;
}
}  // namespace

void init() {
  (void)ensureLogger();
}

void initFromEnv() {
  init();

  if (const char* pattern = std::getenv("NAJA_LOG_PATTERN")) {
    setPattern(pattern);
  }

  if (const char* file = std::getenv("NAJA_LOG_FILE")) {
    addFileSink(file);
  }

  spdlog::cfg::load_env_levels();

  if (const char* level = std::getenv("NAJA_LOG_LEVEL")) {
    setLevel(levelFromString(level));
  }
}

std::shared_ptr<spdlog::logger> get() {
  return ensureLogger();
}

void setLevel(spdlog::level::level_enum level) {
  ensureLogger()->set_level(level);
}

void setPattern(const std::string& pattern) {
  pattern_ = pattern;
  ensureLogger()->set_pattern(pattern_);
}

void addConsoleSink(spdlog::level::level_enum level) {
  auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  sink->set_level(level);
  sink->set_pattern(pattern_);
  ensureLogger()->sinks().push_back(sink);
}

void addFileSink(const std::string& path, spdlog::level::level_enum level) {
  auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path);
  sink->set_level(level);
  sink->set_pattern(pattern_);
  ensureLogger()->sinks().push_back(sink);
}

//LCOV_EXCL_START
void flushEvery(std::chrono::seconds interval) {
  spdlog::flush_every(interval);
}
//LCOV_EXCL_STOP

void clearSinks() {
  auto logger = ensureLogger();
  logger->flush();
  logger->sinks().clear();
  addConsoleSink(spdlog::level::info);
}

void shutdown() {
  std::lock_guard<std::mutex> lock(logger_mutex_);
  logger_.reset();
  spdlog::shutdown();
}

spdlog::level::level_enum levelFromString(const std::string& level) {
  return spdlog::level::from_str(level);
}

}  // namespace naja::log
