// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "PNLTechnology.h"
#include "PNLSite.h"
using namespace naja::NL;

TEST(PNLTechnologyTest, test0) {
    auto tech = PNLTechnology::getOrCreate();
    EXPECT_EQ(tech->getManufacturingGrid(), 0);
    EXPECT_EQ(tech->getSites().size(), 0);

    auto site0 = PNLSite::create(NLName("site0"), PNLSite::ClassType::Core, 5, 10);
    EXPECT_EQ(site0->getName().getString(), "site0");
    EXPECT_EQ(site0->getWidth(), 5);
    EXPECT_EQ(site0->getHeight(), 10);
    EXPECT_EQ(site0->getClass(), PNLSite::ClassType::Core);
    EXPECT_EQ(site0->getID(), (NLID::DesignObjectID)0);
    EXPECT_EQ(tech->getSites().size(), 1);

    // Clean up
    delete site0;
    PNLTechnology::getOrCreate()->destroy();
}