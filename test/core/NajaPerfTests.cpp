// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <thread>

#include "NajaPerf.h"
using namespace naja;

#ifndef NAJA_CORE_TESTS_PATH
#define NAJA_CORE_TESTS_PATH "Undefined"
#endif

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

TEST(NajaPerfTests, testErrors) {
  EXPECT_THROW(NajaPerf::Scope scope("test0"), NajaException);
}