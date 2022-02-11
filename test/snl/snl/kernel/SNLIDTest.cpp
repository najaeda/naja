#include "gtest/gtest.h"

#include "SNLID.h"
using namespace naja::SNL;

TEST(SNLIDTest, testSize) {
  EXPECT_EQ(1, sizeof(SNLID::Type));
  EXPECT_EQ(1, sizeof(SNLID::DBID));
  EXPECT_EQ(2, sizeof(SNLID::LibraryID));
  EXPECT_EQ(4, sizeof(SNLID::DesignID));
  EXPECT_EQ(4, sizeof(SNLID::InstanceID));
  EXPECT_EQ(4, sizeof(SNLID::DesignObjectID));
  EXPECT_EQ(4, sizeof(SNLID::Bit)); 
  EXPECT_EQ(1+1+2+4+4+4+4 /*20*/, sizeof(SNLID));
}