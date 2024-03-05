// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLID.h"
using namespace naja::SNL;

TEST(SNLIDTest, testSize) {
  EXPECT_EQ(1, sizeof(SNLID::Type));
  EXPECT_EQ(1, sizeof(SNLID::DBID));
  EXPECT_EQ(2, sizeof(SNLID::LibraryID));
  EXPECT_EQ(4, sizeof(SNLID::DesignID));
  EXPECT_EQ(4, sizeof(SNLID::DesignObjectID));
  EXPECT_EQ(4, sizeof(SNLID::Bit)); 
  EXPECT_EQ(1+1+2+4+4+4+4 /*20*/, sizeof(SNLID));
}

TEST(SNLIDTest, testComparisons) {
  EXPECT_LT(SNLID(0), SNLID(1));
  EXPECT_NE(SNLID(0), SNLID(1));
  EXPECT_LE(SNLID(0), SNLID(1));
  EXPECT_EQ(SNLID(0), SNLID(SNLID::Type::DB, 0, 0, 0, 0, 0, 0));
  EXPECT_GT(SNLID(1), SNLID(SNLID::Type::DB, 0, 0, 0, 0, 0, 0));
  EXPECT_GE(SNLID(1), SNLID(SNLID::Type::DB, 0, 0, 0, 0, 0, 0));
}

TEST(SNLIDTest, testDesignReference) {
  SNLID::DesignReference designReference(1, 1, 3);
  EXPECT_EQ(1, designReference.dbID_);
  EXPECT_EQ(1, designReference.libraryID_);
  EXPECT_EQ(3, designReference.designID_);
  EXPECT_EQ(SNLID::DBDesignReference(1, 3), designReference.getDBDesignReference());
}

TEST(SNLIDTest, testBitNetReference) {
  SNLID::BitNetReference scalarNetReference(2, 2, 3, 4);
  EXPECT_FALSE(scalarNetReference.isBusBit_);
  EXPECT_EQ(2, scalarNetReference.dbID_);
  EXPECT_EQ(2, scalarNetReference.libraryID_);
  EXPECT_EQ(3, scalarNetReference.designID_);
  EXPECT_EQ(4, scalarNetReference.designObjectID_);
  EXPECT_EQ(0, scalarNetReference.bit_);
  EXPECT_EQ(SNLID::DesignReference(2, 2, 3), scalarNetReference.getDesignReference());
}
