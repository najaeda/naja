// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
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
