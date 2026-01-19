// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "PNLTechnology.h"
#include "PNLSite.h"
using namespace naja::NL;

TEST(PNLSiteTest, test0) {
    auto universe = NLUniverse::create();
    auto tech = PNLTechnology::create(universe);
    auto site0 = PNLSite::create(tech, NLName("site0"), PNLSite::ClassType::Core, 5, 10);
    EXPECT_EQ(site0->getName().getString(), "site0");
    EXPECT_EQ(site0->getWidth(), 5);
    EXPECT_EQ(site0->getHeight(), 10);
    EXPECT_EQ(site0->getClass(), PNLSite::ClassType::Core);
    EXPECT_EQ(site0->getID(), (NLID::DesignObjectID)0);

    auto site1 = PNLSite::create(tech, NLName("site1"), PNLSite::ClassType::Pad, 15, 20);
    EXPECT_EQ(site1->getName().getString(), "site1");
    EXPECT_EQ(site1->getWidth(), 15);
    EXPECT_EQ(site1->getHeight(), 20);
    EXPECT_EQ(site1->getClass(), PNLSite::ClassType::Pad);
    EXPECT_EQ(site1->getID(), (NLID::DesignObjectID)1);

    NLUniverse::get()->destroy();
}
