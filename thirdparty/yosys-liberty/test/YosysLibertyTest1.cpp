// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "LibertyConstructor.h"
using namespace naja::liberty;

#include "LibertyParser.h"
using namespace Yosys;

#include "LibertyConstructorTest.h"

#ifndef YOSYS_LIBERTY_BENCHMARKS
#define YOSYS_LIBERTY_BENCHMARKS "Undefined"
#endif

TEST(YosysLibertyTest1, test0) {
  LibertyConstructorTest constructor;
  std::filesystem::path test0Path(
      std::filesystem::path(YOSYS_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("test1.lib"));
  constructor.parse(test0Path);
  auto ast = constructor.parse(test0Path);
  ASSERT_NE(nullptr, ast);
  EXPECT_EQ(ast->id, "library");
  //find cells
  for (auto child: ast->children) {
    if (child->id == "cell") {
    }
  }
}
