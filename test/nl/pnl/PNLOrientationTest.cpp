// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "PNLOrientation.h"
using namespace naja::NL;

TEST(PNLOrientationTest, test0) {
    auto orientation0 = PNLOrientation::Type(PNLOrientation::Type::R0);
    auto orientation1 = PNLOrientation::Type(PNLOrientation::Type::R90);
    auto orientation2 = PNLOrientation::Type(PNLOrientation::Type::R180);
    auto orientation3 = PNLOrientation::Type(PNLOrientation::Type::R270);
    auto orientation4 = PNLOrientation::Type(PNLOrientation::Type::MY);
    auto orientation5 = PNLOrientation::Type(PNLOrientation::Type::MYR90);
    auto orientation6 = PNLOrientation::Type(PNLOrientation::Type::MX);
    auto orientation7 = PNLOrientation::Type(PNLOrientation::Type::MXR90);
    
    EXPECT_EQ(orientation0.getString(), "R0");
    EXPECT_EQ(orientation1.getString(), "R90");
    EXPECT_EQ(orientation2.getString(), "R180");
    EXPECT_EQ(orientation3.getString(), "R270");
    EXPECT_EQ(orientation4.getString(), "MY");
    EXPECT_EQ(orientation5.getString(), "MYR90");
    EXPECT_EQ(orientation6.getString(), "MX");
    EXPECT_EQ(orientation7.getString(), "MXR90");
}