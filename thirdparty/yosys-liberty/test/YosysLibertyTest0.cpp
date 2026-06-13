// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "YosysLibertyParser.h"
using namespace Yosys;

#ifndef YOSYS_LIBERTY_BENCHMARKS
#define YOSYS_LIBERTY_BENCHMARKS "Undefined"
#endif

namespace {

const LibertyAst* findChild(const LibertyAst* ast, const std::string& id) {
  for (auto child: ast->children) {
    if (child->id == id) {
      return child;
    }
  }
  return nullptr;
}

size_t countChildren(const LibertyAst* ast, const std::string& id) {
  size_t count = 0;
  for (auto child: ast->children) {
    if (child->id == id) {
      count++;
    }
  }
  return count;
}

}

TEST(YosysLibertyTest0, test0) {
  std::filesystem::path test0Path(
      std::filesystem::path(YOSYS_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("test0.lib"));
  std::ifstream inFile(test0Path);
  ASSERT_TRUE(inFile.good());
  auto parser = std::make_unique<Yosys::LibertyParser>(inFile);
  ASSERT_NE(nullptr, parser);
  auto ast = parser->ast;
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

TEST(YosysLibertyTest0, structuralModeSkipsNonStructuralGroups) {
  std::istringstream inFile(R"liberty(
library(test) {
  delay_model : table_lookup;
  lu_table_template(delay_template) {
    variable_1 : input_net_transition;
    index_1("1, 2, 3");
  }
  power_lut_template(power_template) {
    variable_1 : input_transition_time;
    index_1("1, 2, 3");
  }
  type(bus4) {
    bit_from : 3;
    bit_to : 0;
    downto : true;
    bit_width : 4;
  }
  cell(AND2_X1) {
    area : 1.0;
    leakage_power() {
      value : 0.1;
    }
    pin(A) {
      direction : input;
      capacitance : 0.1;
      internal_power() {
        rise_power(power_template) {
          values("1, 2, 3");
        }
      }
    }
    pin(Y) {
      direction : output;
      function : "A";
      timing() {
        related_pin : "A";
        timing_type : combinational;
        timing_sense : positive_unate;
        cell_rise(delay_template) {
          values("1, 2, 3");
        }
        rise_transition(delay_template) {
          values("1, 2, 3");
        }
      }
      internal_power() {
        rise_power(power_template) {
          values("1, 2, 3");
        }
      }
    }
  }
}
)liberty");

  auto parser = std::make_unique<Yosys::LibertyParser>(
      inFile, Yosys::LibertyParser::ParseMode::Structural);
  ASSERT_NE(nullptr, parser);
  auto ast = parser->ast;
  ASSERT_NE(nullptr, ast);
  EXPECT_EQ("library", ast->id);
  EXPECT_EQ(2, ast->children.size());
  EXPECT_EQ(nullptr, findChild(ast, "delay_model"));
  EXPECT_EQ(nullptr, findChild(ast, "lu_table_template"));
  EXPECT_EQ(nullptr, findChild(ast, "power_lut_template"));

  auto type = findChild(ast, "type");
  ASSERT_NE(nullptr, type);
  EXPECT_NE(nullptr, findChild(type, "bit_from"));
  EXPECT_NE(nullptr, findChild(type, "bit_to"));
  EXPECT_NE(nullptr, findChild(type, "downto"));
  EXPECT_EQ(nullptr, findChild(type, "bit_width"));

  auto cell = findChild(ast, "cell");
  ASSERT_NE(nullptr, cell);
  EXPECT_EQ(2, countChildren(cell, "pin"));
  EXPECT_EQ(nullptr, findChild(cell, "area"));
  EXPECT_EQ(nullptr, findChild(cell, "leakage_power"));

  auto pinA = cell->children[0];
  ASSERT_EQ("pin", pinA->id);
  EXPECT_NE(nullptr, findChild(pinA, "direction"));
  EXPECT_EQ(nullptr, findChild(pinA, "capacitance"));
  EXPECT_EQ(nullptr, findChild(pinA, "internal_power"));

  auto pinY = cell->children[1];
  ASSERT_EQ("pin", pinY->id);
  EXPECT_NE(nullptr, findChild(pinY, "direction"));
  EXPECT_NE(nullptr, findChild(pinY, "function"));
  EXPECT_EQ(nullptr, findChild(pinY, "internal_power"));

  auto timing = findChild(pinY, "timing");
  ASSERT_NE(nullptr, timing);
  EXPECT_NE(nullptr, findChild(timing, "related_pin"));
  EXPECT_NE(nullptr, findChild(timing, "timing_type"));
  EXPECT_EQ(nullptr, findChild(timing, "timing_sense"));
  EXPECT_EQ(nullptr, findChild(timing, "cell_rise"));
  EXPECT_EQ(nullptr, findChild(timing, "rise_transition"));
}
