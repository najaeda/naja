// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLVRLDumper.h"
using namespace naja::NL;

TEST(SNLVRLDumperTestUtils, testbinStrToHexStr) {
  EXPECT_EQ("0", SNLVRLDumper::binStrToHexStr("0"));
  EXPECT_EQ("0", SNLVRLDumper::binStrToHexStr("00"));
  EXPECT_EQ("0", SNLVRLDumper::binStrToHexStr("000"));
  EXPECT_EQ("0", SNLVRLDumper::binStrToHexStr("0000"));
  EXPECT_EQ("00", SNLVRLDumper::binStrToHexStr("00000"));
  EXPECT_EQ("1", SNLVRLDumper::binStrToHexStr("1"));
  EXPECT_EQ("1", SNLVRLDumper::binStrToHexStr("01"));
  EXPECT_EQ("01", SNLVRLDumper::binStrToHexStr("00001"));
  EXPECT_EQ("2", SNLVRLDumper::binStrToHexStr("10"));
  EXPECT_EQ("02", SNLVRLDumper::binStrToHexStr("000010"));
  EXPECT_EQ("A", SNLVRLDumper::binStrToHexStr("1010"));
  EXPECT_EQ("AB", SNLVRLDumper::binStrToHexStr("10101011"));
  EXPECT_EQ("1234", SNLVRLDumper::binStrToHexStr("0001001000110100"));
  EXPECT_EQ("123456", SNLVRLDumper::binStrToHexStr("000100100011010001010110"));
  EXPECT_EQ("89ABCDEF", SNLVRLDumper::binStrToHexStr("10001001101010111100110111101111"));
  EXPECT_EQ("0A", SNLVRLDumper::binStrToHexStr("001010"));
  EXPECT_EQ("08A", SNLVRLDumper::binStrToHexStr("010001010"));
  EXPECT_EQ("0FF1", SNLVRLDumper::binStrToHexStr("00111111110001"));
  EXPECT_THROW(SNLVRLDumper::binStrToHexStr("ERROR"), SNLVRLDumperException);
}
