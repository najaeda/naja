// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <algorithm>

#include "NLBitDependencies.h"
#include "NLDB0.h"
#include "NLUniverse.h"
#include "NLException.h"
#include "NLLibraryTruthTables.h"
#include "SNLDesignModeling.h"
#include "SNLScalarTerm.h"
using namespace naja::NL;

class NLLibraryTruthTablesTest: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      primitives_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    SNLDesign* createSingleOutputPrimitive(const std::string& name, size_t inputCount) {
      auto* design =
          SNLDesign::create(primitives_, SNLDesign::Type::Primitive, NLName(name));
      for (size_t i = 0; i < inputCount; ++i) {
        SNLScalarTerm::create(
            design, SNLTerm::Direction::Input, NLName("I" + std::to_string(i)));
      }
      SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
      return design;
    }
    NLLibrary* primitives_ {};
};

TEST_F(NLLibraryTruthTablesTest, test) {
  EXPECT_EQ(NLLibraryTruthTables::LibraryTruthTables(), NLLibraryTruthTables::construct(primitives_));
  EXPECT_EQ(NLLibraryTruthTables::LibraryTruthTables(), NLLibraryTruthTables::getTruthTables(primitives_));
}

TEST_F(NLLibraryTruthTablesTest, testConstructStandardLibraryReturnsEmpty) {
  auto* standardLibrary =
      NLLibrary::create(primitives_->getDB(), NLLibrary::Type::Standard, NLName("WORK"));

  EXPECT_EQ(
      NLLibraryTruthTables::LibraryTruthTables(),
      NLLibraryTruthTables::construct(standardLibrary));
  EXPECT_EQ(
      NLLibraryTruthTables::LibraryTruthTables(),
      NLLibraryTruthTables::getTruthTables(standardLibrary));
}

TEST_F(NLLibraryTruthTablesTest, testCanonicalizedLookupMatchesDB0GenericTables) {
  auto* gateLibrary = NLDB0::getOrCreateGateLibrary(NLDB0::GateType::Xor);
  auto* xor3 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Xor, 3);

  auto truthTables = NLLibraryTruthTables::construct(gateLibrary);
  EXPECT_FALSE(truthTables.empty());
  EXPECT_EQ(truthTables, NLLibraryTruthTables::getTruthTables(gateLibrary));

  auto [genericMatch, genericIndexes] = NLLibraryTruthTables::getDesignForTruthTable(
      gateLibrary,
      SNLTruthTable(
          3,
          SNLTruthTable::GenericType::XOR,
          SNLTruthTable::fullDependencies(3)));
  EXPECT_EQ(xor3, genericMatch);
  EXPECT_TRUE(genericIndexes.empty());
}

TEST_F(NLLibraryTruthTablesTest, testCanonicalizedLookupMatchesSparseVectorTables) {
  auto* vector = createSingleOutputPrimitive("sparse_vector", 8);
  std::vector<bool> bits(1u << 7, false);
  for (size_t i = 0; i < bits.size(); ++i) {
    bits[i] = (i % 3) == 1;
  }
  SNLDesignModeling::setTruthTable(
      vector,
      SNLTruthTable(7, bits, NLBitDependencies::encodeBits({0, 1, 2, 3, 4, 5, 7})));

  auto truthTables = NLLibraryTruthTables::construct(primitives_);
  EXPECT_EQ(1, truthTables.size());
  EXPECT_EQ(truthTables, NLLibraryTruthTables::getTruthTables(primitives_));

  auto [vectorMatch, vectorIndexes] = NLLibraryTruthTables::getDesignForTruthTable(
      primitives_,
      SNLTruthTable(7, bits, SNLTruthTable::fullDependencies(7)));
  EXPECT_EQ(vector, vectorMatch);
  EXPECT_TRUE(vectorIndexes.empty());
}

TEST_F(NLLibraryTruthTablesTest, testDB0MemoryAndDivModAreSkipped) {
  NLDB0::MemorySignature memorySignature;
  memorySignature.width = 8;
  memorySignature.depth = 16;
  memorySignature.abits = 4;
  memorySignature.readPorts = 1;
  memorySignature.writePorts = 1;
  auto* memory = NLDB0::getOrCreateMemory(memorySignature);
  ASSERT_NE(nullptr, memory);
  ASSERT_TRUE(NLDB0::isMemory(memory));

  NLDB0::DivModSignature divModSignature;
  divModSignature.width = 8;
  auto* divMod = NLDB0::getOrCreateDivMod(divModSignature);
  ASSERT_NE(nullptr, divMod);
  ASSERT_TRUE(NLDB0::isDivMod(divMod));

  auto* assign = NLDB0::getAssign();
  ASSERT_NE(nullptr, assign);
  auto assignTruthTable = NLDB0::getPrimitiveTruthTable(assign);
  auto truthTables = NLLibraryTruthTables::construct(NLDB0::getDB0RootLibrary());

  auto assignIt = truthTables.find(assignTruthTable);
  ASSERT_NE(truthTables.end(), assignIt);
  EXPECT_NE(
      assignIt->second.end(),
      std::find(assignIt->second.begin(), assignIt->second.end(), assign));

  for (const auto& [truthTable, primitives]: truthTables) {
    (void) truthTable;
    EXPECT_EQ(primitives.end(), std::find(primitives.begin(), primitives.end(), memory));
    EXPECT_EQ(primitives.end(), std::find(primitives.begin(), primitives.end(), divMod));
  }
}
