// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <cstdlib>
#include <thread>

#include "NajaPerf.h"
using namespace naja;

#ifndef NAJA_CORE_TESTS_PATH
#define NAJA_CORE_TESTS_PATH "Undefined"
#endif

namespace {
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

TEST(NajaPerfTests, getLogPathFromEnvUsesDefaultWhenUnset) {
  unsetEnvVar("NAJA_PERF_TEST_PATH");
  EXPECT_EQ(
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log"),
    std::filesystem::path("naja_perf.log"));
}

TEST(NajaPerfTests, getLogPathFromEnvUsesEnvValue) {
  setEnvVar("NAJA_PERF_TEST_PATH", "custom_perf.log");
  EXPECT_EQ(
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log"),
    std::filesystem::path("custom_perf.log"));
  unsetEnvVar("NAJA_PERF_TEST_PATH");
}

TEST(NajaPerfTests, getLogPathFromEnvFallsBackForEmptyAndLegacyEnable) {
  setEnvVar("NAJA_PERF_TEST_PATH", "");
  EXPECT_EQ(
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log"),
    std::filesystem::path("naja_perf.log"));

  setEnvVar("NAJA_PERF_TEST_PATH", "1");
  EXPECT_EQ(
    NajaPerf::getLogPathFromEnv("NAJA_PERF_TEST_PATH", "naja_perf.log"),
    std::filesystem::path("naja_perf.log"));
  unsetEnvVar("NAJA_PERF_TEST_PATH");
}

TEST(NajaPerfTests, test0) {
  std::filesystem::path logPath(NAJA_CORE_TESTS_PATH);
  NajaPerf::create(logPath/"naja_perf_test0.log", "top");
  auto perf = NajaPerf::get();
  EXPECT_EQ(1, perf->getStack().size());
  ASSERT_NE(perf, nullptr);
  {
    NajaPerf::Scope scope("test0");
    EXPECT_EQ(2, perf->getStack().size());
    {
      NajaPerf::Scope scope("test1");
      EXPECT_EQ(3, perf->getStack().size());
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      {
        NajaPerf::Scope scope("test2");
        EXPECT_EQ(4, perf->getStack().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      EXPECT_EQ(3, perf->getStack().size());
      {
        NajaPerf::Scope scope("test3");
        EXPECT_EQ(4, perf->getStack().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      EXPECT_EQ(3, perf->getStack().size());
    }
    EXPECT_EQ(2, perf->getStack().size());
  }
  EXPECT_EQ(1, perf->getStack().size());
}
