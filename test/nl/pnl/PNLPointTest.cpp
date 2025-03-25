// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "PNLPoint.h"
using namespace naja::NL;

TEST(PNLPointTest, test0) {
  auto point0 = PNLPoint(0, 0);
  auto point1 = PNLPoint(1, 1);
  auto point2 = PNLPoint(255, 357);
  auto point3 = PNLPoint(-1000, 1000);

  EXPECT_EQ(point0.getX(), 0);
  EXPECT_EQ(point0.getY(), 0);
  EXPECT_EQ(point1.getX(), 1);
  EXPECT_EQ(point1.getY(), 1);
  EXPECT_EQ(point2.getX(), 255);
  EXPECT_EQ(point2.getY(), 357);
  EXPECT_EQ(point3.getX(), -1000);
  EXPECT_EQ(point3.getY(), 1000);
  EXPECT_NE(point0, point1);
  EXPECT_NE(point0, point3);
  EXPECT_EQ(point0, point0);
  EXPECT_EQ(point0, PNLPoint(0, 0));
  EXPECT_EQ(point3, PNLPoint(-1000, 1000));
}