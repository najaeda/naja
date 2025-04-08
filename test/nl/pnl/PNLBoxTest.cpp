// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "PNLBox.h"
using namespace naja::NL;

TEST(PNLBoxTest, test0) {
    auto box0 = PNLBox(5, 100, 25, 200);
    EXPECT_EQ(box0.getLeft(), 5);
    EXPECT_EQ(box0.getBottom(), 100);
    EXPECT_EQ(box0.getRight(), 25);
    EXPECT_EQ(box0.getTop(), 200);
    EXPECT_EQ(box0.getLowerLeft(), PNLPoint(5, 100));
    EXPECT_EQ(box0.getUpperRight(), PNLPoint(25, 200));

    // Comperators testng
    auto box1 = PNLBox(5, 100, 25, 200);
    EXPECT_EQ(box0, box1);
    EXPECT_FALSE(box0 != box1);
    EXPECT_FALSE(box0 < box1);
    EXPECT_FALSE(box0 > box1);
    EXPECT_TRUE(box0 <= box1);
    EXPECT_TRUE(box0 >= box1);
}