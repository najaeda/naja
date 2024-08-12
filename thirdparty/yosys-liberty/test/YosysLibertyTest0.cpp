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

TEST(YosysLibertyTest0, test0) {
  LibertyConstructorTest constructor;
  std::filesystem::path test0Path(
      std::filesystem::path(YOSYS_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("test0.lib"));
  auto ast = constructor.parse(test0Path);
  ASSERT_NE(nullptr, ast);
  EXPECT_EQ(ast->id, "library");
  ASSERT_EQ(3, ast->children.size());
  auto child0 = ast->children[0];
  EXPECT_EQ("delay_model", child0->id);
  EXPECT_EQ("table_lookup", child0->value);
  EXPECT_TRUE(child0->children.empty());
  auto child1 = ast->children[1];
  EXPECT_EQ("define", child1->id);
  EXPECT_TRUE(child1->children.empty());
  EXPECT_EQ(3, child1->args.size());
  EXPECT_EQ("process_corner", child1->args[0]);
  EXPECT_EQ("operating_conditions", child1->args[1]);
  EXPECT_EQ("string", child1->args[2]);
  auto child2 = ast->children[2];
  EXPECT_EQ("cell", child2->id);
  EXPECT_EQ(1, child2->args.size());
  EXPECT_EQ("AND2_X1", child2->args[0]);
  EXPECT_EQ(1, child2->children.size());
  auto child2_0 = child2->children[0];
  EXPECT_EQ("drive_strength", child2_0->id);
  EXPECT_TRUE(child2_0->args.empty());
  EXPECT_TRUE(child2_0->children.empty());
  EXPECT_EQ("1", child2_0->value);
}
