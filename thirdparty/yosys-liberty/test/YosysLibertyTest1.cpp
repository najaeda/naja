// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "YosysLibertyParser.h"
using namespace Yosys;

#ifndef YOSYS_LIBERTY_BENCHMARKS
#define YOSYS_LIBERTY_BENCHMARKS "Undefined"
#endif

TEST(YosysLibertyTest1, test0) {
  std::filesystem::path test0Path(
      std::filesystem::path(YOSYS_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("test1.lib"));
  std::ifstream inFile(test0Path);
  ASSERT_TRUE(inFile.good());
  auto parser = std::make_unique<Yosys::LibertyParser>(inFile);
  ASSERT_NE(nullptr, parser);
  auto ast = parser->ast;
  ASSERT_NE(nullptr, ast);
  EXPECT_EQ(ast->id, "library");
  //find cells
  size_t cellsNb = 0;
  for (auto child: ast->children) {
    if (child->id == "cell") {
      cellsNb++;
    }
  }
  EXPECT_EQ(2, cellsNb);
}
