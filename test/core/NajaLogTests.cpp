// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>

#include "NajaLog.h"

using namespace naja;

#ifndef NAJA_CORE_TESTS_PATH
#define NAJA_CORE_TESTS_PATH "Undefined"
#endif

namespace {
std::string readFile(const std::filesystem::path& path) {
  std::ifstream in(path);
  std::string contents((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
  return contents;
}

void setEnvVar(const char* name, const std::string& value) {
#ifdef _WIN32
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void unsetEnvVar(const char* name) {
#ifdef _WIN32
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}
}  // namespace

TEST(NajaLogTests, initAndGet) {
  log::init();
  EXPECT_NE(log::get(), nullptr);
}

TEST(NajaLogTests, fileSinkWrites) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  auto filePath = logPath / "naja_log_test_file_sink.log";

  log::clearSinks();
  log::addFileSink(filePath.string());
  log::setLevel(spdlog::level::trace);

  NAJA_LOG_INFO("file sink test message");
  log::get()->flush();

  auto contents = readFile(filePath);
  EXPECT_NE(contents.find("file sink test message"), std::string::npos);
}

TEST(NajaLogTests, clearSinksStopsFileLogging) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  auto filePath = logPath / "naja_log_test_clear_sinks.log";

  log::clearSinks();
  log::addFileSink(filePath.string());
  log::setLevel(spdlog::level::trace);

  NAJA_LOG_INFO("before clear");
  log::get()->flush();

  log::clearSinks();
  NAJA_LOG_INFO("after clear");
  log::get()->flush();

  auto contents = readFile(filePath);
  EXPECT_NE(contents.find("before clear"), std::string::npos);
  EXPECT_EQ(contents.find("after clear"), std::string::npos);
}

TEST(NajaLogTests, setLogLevelFiltersMessages) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  auto filePath = logPath / "naja_log_test_log_level.log";

  log::clearSinks();
  log::addFileSink(filePath.string());
  log::setLevel(spdlog::level::warn);

  NAJA_LOG_INFO("level info suppressed");
  NAJA_LOG_WARN("level warn visible");
  log::get()->flush();

  auto contents = readFile(filePath);
  EXPECT_EQ(contents.find("level info suppressed"), std::string::npos);
  EXPECT_NE(contents.find("level warn visible"), std::string::npos);
}

TEST(NajaLogTests, levelFromStringParsesLevels) {
  EXPECT_EQ(log::levelFromString("info"), spdlog::level::info);
  EXPECT_EQ(log::levelFromString("warn"), spdlog::level::warn);
  EXPECT_EQ(log::levelFromString("debug"), spdlog::level::debug);
  EXPECT_EQ(log::levelFromString("off"), spdlog::level::off);
}

TEST(NajaLogTests, shutdownResetsLogger) {
  log::init();
  auto logger = log::get();
  EXPECT_NE(logger, nullptr);

  log::shutdown();

  auto newLogger = log::get();
  EXPECT_NE(newLogger, nullptr);
}

TEST(NajaLogTests, setPatternFormatsOutput) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  auto filePath = logPath / "naja_log_test_pattern.log";

  log::clearSinks();
  log::addFileSink(filePath.string());
  log::setPattern("TESTPATTERN %v");
  log::setLevel(spdlog::level::info);

  NAJA_LOG_INFO("pattern check");
  log::get()->flush();

  auto contents = readFile(filePath);
  EXPECT_NE(contents.find("TESTPATTERN"), std::string::npos);
  EXPECT_NE(contents.find("pattern check"), std::string::npos);
}

TEST(NajaLogTests, initFromEnvUsesEnvVars) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  auto filePath = logPath / "naja_log_test_env.log";

  std::filesystem::remove(filePath);
  setEnvVar("NAJA_LOG_PATTERN", "ENVTEST %v");
  setEnvVar("NAJA_LOG_FILE", filePath.string());
  setEnvVar("NAJA_LOG_LEVEL", "warn");

  log::clearSinks();
  log::initFromEnv();

  NAJA_LOG_INFO("env info suppressed");
  NAJA_LOG_WARN("env warn visible");
  log::get()->flush();

  auto contents = readFile(filePath);
  EXPECT_NE(contents.find("ENVTEST"), std::string::npos);
  EXPECT_EQ(contents.find("env info suppressed"), std::string::npos);
  EXPECT_NE(contents.find("env warn visible"), std::string::npos);

  unsetEnvVar("NAJA_LOG_PATTERN");
  unsetEnvVar("NAJA_LOG_FILE");
  unsetEnvVar("NAJA_LOG_LEVEL");
}
