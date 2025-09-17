// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "PNLTransform.h"

using namespace naja::NL;

TEST(PNLTransformTest, test0) {
    auto transform1 = PNLTransform(PNLPoint(1, 2), PNLOrientation::Type(PNLOrientation::Type::R90));
    auto transform2 = PNLTransform(PNLPoint(3, 4), PNLOrientation::Type(PNLOrientation::Type::R180));
    auto transform3 = PNLTransform(PNLPoint(5, 6), PNLOrientation::Type(PNLOrientation::Type::R270));
    EXPECT_EQ(transform1.getOffset().getX(), 1);
    EXPECT_EQ(transform1.getOffset().getY(), 2);
    EXPECT_EQ(transform2.getOffset().getX(), 3);
    EXPECT_EQ(transform2.getOffset().getY(), 4);
    EXPECT_EQ(transform3.getOffset().getX(), 5);
    EXPECT_EQ(transform3.getOffset().getY(), 6);

    // Comperators testng
    EXPECT_EQ(transform1, transform1);
    EXPECT_FALSE(transform1 != transform1);
    EXPECT_FALSE(transform1 < transform1);
    EXPECT_FALSE(transform1 > transform1);
    EXPECT_TRUE(transform1 <= transform1);
    EXPECT_TRUE(transform1 >= transform1);
}