// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLID.h"
using namespace naja::SNL;

TEST(NLIDTest, testSize) {
  EXPECT_EQ(1, sizeof(NLID::Type));
  EXPECT_EQ(1, sizeof(NLID::DBID));
  EXPECT_EQ(2, sizeof(NLID::LibraryID));
  EXPECT_EQ(4, sizeof(NLID::DesignID));
  EXPECT_EQ(4, sizeof(NLID::DesignObjectID));
  EXPECT_EQ(4, sizeof(NLID::Bit)); 
  EXPECT_EQ(1+1+2+4+4+4+4 /*20*/, sizeof(NLID));
}

TEST(NLIDTest, testComparisons) {
  EXPECT_LT(NLID(0), NLID(1));
  EXPECT_NE(NLID(0), NLID(1));
  EXPECT_LE(NLID(0), NLID(1));
  EXPECT_EQ(NLID(0), NLID(NLID::Type::DB, 0, 0, 0, 0, 0, 0));
  EXPECT_GT(NLID(1), NLID(NLID::Type::DB, 0, 0, 0, 0, 0, 0));
  EXPECT_GE(NLID(1), NLID(NLID::Type::DB, 0, 0, 0, 0, 0, 0));
}

TEST(NLIDTest, testDesignReference) {
  NLID::DesignReference designReference(1, 1, 3);
  EXPECT_EQ(1, designReference.dbID_);
  EXPECT_EQ(1, designReference.libraryID_);
  EXPECT_EQ(3, designReference.designID_);
  EXPECT_EQ(NLID::DBDesignReference(1, 3), designReference.getDBDesignReference());
}

TEST(NLIDTest, testBitNetReference) {
  NLID::BitNetReference scalarNetReference(2, 2, 3, 4);
  EXPECT_FALSE(scalarNetReference.isBusBit_);
  EXPECT_EQ(2, scalarNetReference.dbID_);
  EXPECT_EQ(2, scalarNetReference.libraryID_);
  EXPECT_EQ(3, scalarNetReference.designID_);
  EXPECT_EQ(4, scalarNetReference.designObjectID_);
  EXPECT_EQ(0, scalarNetReference.bit_);
  EXPECT_EQ(NLID::DesignReference(2, 2, 3), scalarNetReference.getDesignReference());
}
