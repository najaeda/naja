// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"
#include "SNLPyLoader.h"

using namespace naja::NL;

#ifndef NAJAEDA_PRIMITIVES_PATH
#define NAJAEDA_PRIMITIVES_PATH "Undefined"
#endif

class SNLRealPrimitivesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse::create();
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLRealPrimitivesTest, testXilinx) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto path = std::filesystem::path(NAJAEDA_PRIMITIVES_PATH) / "xilinx.py";
  SNLPyLoader::loadPrimitives(library, path);
  EXPECT_FALSE(library->getSNLDesigns().empty());
  auto ibuf = library->getSNLDesign(NLName("IBUF"));
  EXPECT_NE(nullptr, ibuf);
  EXPECT_TRUE(ibuf->isPrimitive());
  auto lut6 = library->getSNLDesign(NLName("LUT6"));
  EXPECT_NE(nullptr, lut6);
  EXPECT_TRUE(lut6->isPrimitive());
}

TEST_F(SNLRealPrimitivesTest, testYosys) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto path = std::filesystem::path(NAJAEDA_PRIMITIVES_PATH) / "yosys.py";
  SNLPyLoader::loadPrimitives(library, path);
  EXPECT_FALSE(library->getSNLDesigns().empty());
  auto andGate = library->getSNLDesign(NLName("$_AND_"));
  EXPECT_NE(nullptr, andGate);
  EXPECT_TRUE(andGate->isPrimitive());
  auto dffp = library->getSNLDesign(NLName("$_DFF_P_"));
  EXPECT_NE(nullptr, dffp);
  EXPECT_TRUE(dffp->isPrimitive());
}
