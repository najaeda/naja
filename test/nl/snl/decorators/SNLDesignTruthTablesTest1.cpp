// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLDesignTruthTable.h"
#include "NajaDumpableProperty.h"

using namespace naja::NL;

class SNLDesignTruthTablesTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db       = NLDB::create(universe);
      prims_        = NLLibrary::create(db, NLLibrary::Type::Primitives);

      // logic0: const0
      {
        auto logic0 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("logic0"));
        SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0, 0b0));
      }

      // logic1: const1
      {
        auto logic1 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("logic1"));
        SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0, 0b1));
      }

      // inv: inverter (1→0)
      {
        auto inv = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("inv"));
        SNLScalarTerm::create(inv, SNLTerm::Direction::Input,  NLName("I"));
        SNLScalarTerm::create(inv, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(inv, SNLTruthTable(1, 0b01));
      }

      // buf: buffer (0→1)
      {
        auto buf = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("buf"));
        SNLScalarTerm::create(buf, SNLTerm::Direction::Input,  NLName("I"));
        SNLScalarTerm::create(buf, SNLTerm::Direction::Output, NLName("O"));
        // FIXED: call setTruthTable, not setTruthTablesatted
        SNLDesignTruthTable::setTruthTable(buf, SNLTruthTable(1, 0b10));
      }
    }

    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }

    NLLibrary* prims_;
};

//-----------------------------------------------------------------------------
// Original tests (unchanged)
//-----------------------------------------------------------------------------

TEST_F(SNLDesignTruthTablesTest1, testStandardGates) {
  EXPECT_EQ(4, prims_->getSNLDesigns().size());
  using Prims = std::vector<SNLDesign*>;
  Prims prims(prims_->getSNLDesigns().begin(), prims_->getSNLDesigns().end());

  // logic0
  EXPECT_EQ(prims[0]->getName().getString(), "logic0");
  EXPECT_TRUE (SNLDesignTruthTable::isConst0(prims[0]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[0]));
  EXPECT_TRUE (SNLDesignTruthTable::isConst (prims[0]));

  // logic1
  EXPECT_EQ(prims[1]->getName().getString(), "logic1");
  EXPECT_TRUE (SNLDesignTruthTable::isConst1(prims[1]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[1]));
  EXPECT_TRUE (SNLDesignTruthTable::isConst (prims[1]));

  // inv
  EXPECT_EQ(prims[2]->getName().getString(), "inv");
  EXPECT_TRUE (SNLDesignTruthTable::isInv   (prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst (prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isBuf   (prims[2]));

  // buf
  EXPECT_EQ(prims[3]->getName().getString(), "buf");
  EXPECT_TRUE (SNLDesignTruthTable::isBuf   (prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isInv   (prims[3]));

  // return false for primitive with no truth table
  auto design = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("prim"));
  SNLScalarTerm::create(design, SNLTerm::Direction::Output, NLName("O"));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(design));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(design));
  EXPECT_FALSE(SNLDesignTruthTable::isInv   (design));
  EXPECT_FALSE(SNLDesignTruthTable::isBuf   (design));

  // error when setting truth table on non-primitive
  auto designs  = NLLibrary::create(prims_->getDB());
  auto non_prim = SNLDesign::create(designs, NLName("non_prim"));
  EXPECT_THROW(
    SNLDesignTruthTable::setTruthTable(non_prim, SNLTruthTable(1, 0b1)),
    NLException
  );

  // error with more than one output
  auto error = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("error"));
  SNLScalarTerm::create(error, SNLTerm::Direction::Input,  NLName("I"));
  SNLScalarTerm::create(error, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(error, SNLTerm::Direction::Output, NLName("O1"));
  EXPECT_THROW(
    SNLDesignTruthTable::setTruthTable(error, SNLTruthTable(1, 0b10)),
    NLException
  );

  // two-output truth tables + single-get throws
  auto multiple_outputs = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("multiple_outputs"));
  SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Input,  NLName("I0"));
  SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Input,  NLName("I1"));
  SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Output, NLName("O1"));
  SNLTruthTable tt0(2, 0b10), tt1(2, 0b01);
  SNLDesignTruthTable::setTruthTables(multiple_outputs, {tt0, tt1});

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(multiple_outputs),
    NLException
  );
  auto tt0get = SNLDesignTruthTable::getTruthTable(multiple_outputs, 0);
  auto tt1get = SNLDesignTruthTable::getTruthTable(multiple_outputs, 1);
  EXPECT_TRUE(tt0get.isInitialized());
  EXPECT_EQ(tt0, tt0get);
  EXPECT_TRUE(tt1get.isInitialized());
  EXPECT_EQ(tt1, tt1get);

  // multi-chunk + two outputs
  auto multiple_outputs2 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("multiple_outputs2"));
  SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Input,  NLName("I0"));
  SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Input,  NLName("I1"));
  SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Output, NLName("O1"));
  std::vector<bool> bitVect(128);
  for (size_t i = 0; i < bitVect.size(); ++i) bitVect[i] = (i % 2);
  SNLTruthTable tt0Big(7, bitVect), tt1Big(7, bitVect);
  SNLDesignTruthTable::setTruthTables(multiple_outputs2, {tt0Big, tt1Big});

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(multiple_outputs2),
    NLException
  );
  auto tt0getBig = SNLDesignTruthTable::getTruthTable(multiple_outputs2, 0);
  auto tt1getBig = SNLDesignTruthTable::getTruthTable(multiple_outputs2, 1);
  EXPECT_TRUE(tt0getBig.isInitialized());
  EXPECT_EQ(tt0Big, tt0getBig);
  EXPECT_TRUE(tt1getBig.isInitialized());
  EXPECT_EQ(tt1Big, tt1getBig);

  // hybrid 4-output (mask, mask, vector, mask)
  auto hybrid = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("hybrid"));
  SNLScalarTerm::create(hybrid, SNLTerm::Direction::Input,  NLName("I0"));
  SNLScalarTerm::create(hybrid, SNLTerm::Direction::Input,  NLName("I1"));
  SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O1"));
  SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O2"));
  SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O3"));
  SNLTruthTable tt0hybrid(2, 0b10), tt1hybrid(2, 0b01), tt3hybrid(2, 0b01);
  std::vector<bool> bitVect2(128);
  for (size_t i = 0; i < bitVect2.size(); ++i) bitVect2[i] = (i % 2);
  SNLTruthTable tt2hybrid(7, bitVect2);

  SNLDesignTruthTable::setTruthTables(hybrid, {tt0hybrid, tt1hybrid, tt2hybrid, tt3hybrid});

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(hybrid),
    NLException
  );
  EXPECT_EQ(tt0hybrid, SNLDesignTruthTable::getTruthTable(hybrid, 0));
  EXPECT_EQ(tt1hybrid, SNLDesignTruthTable::getTruthTable(hybrid, 1));
  EXPECT_EQ(tt2hybrid, SNLDesignTruthTable::getTruthTable(hybrid, 2));
  EXPECT_EQ(tt3hybrid, SNLDesignTruthTable::getTruthTable(hybrid, 3));

  // single big mask
  auto singleLarge = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("singleLarge"));
  SNLScalarTerm::create(singleLarge, SNLTerm::Direction::Input,  NLName("I0"));
  SNLScalarTerm::create(singleLarge, SNLTerm::Direction::Input,  NLName("I1"));
  SNLScalarTerm::create(singleLarge, SNLTerm::Direction::Output, NLName("O0"));
  std::vector<bool> bitVectSingleLarge(128);
  for (size_t i = 0; i < bitVectSingleLarge.size(); ++i) {
    bitVectSingleLarge[i] = (i % 2);
  }
  SNLTruthTable tt0singleLarge(7, bitVectSingleLarge);

  SNLDesignTruthTable::setTruthTable(singleLarge, tt0singleLarge);
  EXPECT_EQ(tt0singleLarge, SNLDesignTruthTable::getTruthTable(singleLarge));
  
  // error for wrong number outputs for tt
  auto singleLargeError = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("singleLargeError"));
  SNLScalarTerm::create(singleLargeError, SNLTerm::Direction::Output,  NLName("O"));
  std::vector<bool> bitVectSingleLargeError(128);
  for (size_t i = 0; i < bitVectSingleLargeError.size(); ++i) {
    bitVectSingleLargeError[i] = (i % 2);
  }
  SNLTruthTable tt0singleLargeError(7, bitVectSingleLargeError);
  std::vector<SNLTruthTable> singleLargeErrorTables = {tt0singleLargeError, tt0singleLargeError};
  EXPECT_THROW(
    SNLDesignTruthTable::setTruthTables(singleLargeError, singleLargeErrorTables),
    NLException
  );

}

//-----------------------------------------------------------------------------
// Additional edge-case tests for full coverage
//-----------------------------------------------------------------------------

TEST_F(SNLDesignTruthTablesTest1, IndexedGetThrowsOutOfRange) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("one_o"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignTruthTable::setTruthTable(D, SNLTruthTable(1, 0b10));

  EXPECT_ANY_THROW(
    SNLDesignTruthTable::getTruthTable(D, /*outputID=*/1)
  );
}

TEST_F(SNLDesignTruthTablesTest1, SingleChunkDeclaredTooLargeThrows) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("bad_single"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  auto P = naja::NajaDumpableProperty::create(
    static_cast<naja::NajaObject*>(D),
    "SNLDesignTruthTableProperty"
  );
  P->addUInt64Value(7);            // declaredInputs > 6
  P->addUInt64Value(0xDEADBEEF);   // only one chunk

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

TEST_F(SNLDesignTruthTablesTest1, MultiChunkButTooSmallThrows) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("bad_multi_small"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  auto P = naja::NajaDumpableProperty::create(
    static_cast<naja::NajaObject*>(D),
    "SNLDesignTruthTableProperty"
  );
  P->addUInt64Value(4);  // declaredInputs=4 => nBits=16 <= 64
  P->addUInt64Value(0);
  P->addUInt64Value(0);  // values.size()>2

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

TEST_F(SNLDesignTruthTablesTest1, SingleGetAfterMultiTableInstallationThrows) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("two_tables"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Input,  NLName("I"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Input,  NLName("J"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O1"));

  SNLTruthTable t0(2, uint64_t{1});
  SNLTruthTable t1(2, uint64_t{2});
  SNLDesignTruthTable::setTruthTables(D, {t0, t1});

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

// Test error for out of range id in getTruthTable
TEST_F(SNLDesignTruthTablesTest1, GetTruthTableOutOfRangeID) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("out_of_range"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  SNLTruthTable t0(1, 0b1);
  SNLDesignTruthTable::setTruthTable(D, t0);

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D, /*outputID=*/1),
    NLException
  );
}

//-----------------------------------------------------------------------------
// Appended exception‐coverage tests for SNLDesignTruthTable
//-----------------------------------------------------------------------------

// Throws when getTruthTable is called with an invalid output index
TEST_F(SNLDesignTruthTablesTest1, GetTruthTable_InvalidOutputIndex_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("one_out"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignTruthTable::setTruthTable(D, SNLTruthTable(1, 0b10));

  // only one output exists, index 1 is out of range
  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D, /*outputID=*/1),
    NLException
  );
}

// Throws when declaredInputs > 6 but only a single 64-bit chunk is provided
TEST_F(SNLDesignTruthTablesTest1, DeclaredInputsGreaterThan64_SingleChunk_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("too_large_single"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  auto P = naja::NajaDumpableProperty::create(
    static_cast<naja::NajaObject*>(D),
    "SNLDesignTruthTableProperty"
  );
  P->addUInt64Value(7);           // declaredInputs == 7 => 2^7 bits but only 1 chunk
  P->addUInt64Value(0xFEDCBA98);  // single chunk

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

// Throws when multi‐chunk path is taken but total bit-width ≤ 64
TEST_F(SNLDesignTruthTablesTest1, NBitsNotGreaterThan64_MultiChunkPath_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("multi_too_small"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  auto P = naja::NajaDumpableProperty::create(
    static_cast<naja::NajaObject*>(D),
    "SNLDesignTruthTableProperty"
  );
  P->addUInt64Value(4);  // declaredInputs=4 => nBits=16
  P->addUInt64Value(0);
  P->addUInt64Value(1);  // values.size()>2 forces multi-chunk branch

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

// Throws when the number of supplied mask-chunks doesn’t match expectedChunks
TEST_F(SNLDesignTruthTablesTest1, ChunkCountMismatch_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("chunk_mismatch"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  // declaredInputs=3 => nBits=8 => expectedChunks=1, but we provide 2 chunks
  auto P = naja::NajaDumpableProperty::create(
    static_cast<naja::NajaObject*>(D),
    "SNLDesignTruthTableProperty"
  );
  P->addUInt64Value(3);
  P->addUInt64Value(0);  // chunk #0
  P->addUInt64Value(0);  // chunk #1 (extra)

  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

// Throws when multiple tables are set and a single-table getTruthTable() is invoked
TEST_F(SNLDesignTruthTablesTest1, MultipleTables_ThenGetTruthTable_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("many_tables"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Input,  NLName("I"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Input,  NLName("J"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O0"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O1"));

  // install two distinct truth tables
  SNLTruthTable t0(2, uint64_t{1});
  SNLTruthTable t1(2, uint64_t{2});
  SNLDesignTruthTable::setTruthTables(D, {t0, t1});

  // calling the single‐table overload without outputID should fail
  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}
//-----------------------------------------------------------------------------
// Tests for createProperty() exception paths via setTruthTables()
//-----------------------------------------------------------------------------

// Throws when attempting to set an empty vector of truth tables
TEST_F(SNLDesignTruthTablesTest1, SetTruthTables_EmptyVector_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("empty_tt"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  std::vector<SNLTruthTable> noTables;
  EXPECT_THROW(
    SNLDesignTruthTable::setTruthTables(D, noTables),
    NLException
  );
}

// Throws when a truth-table property already exists on the design
TEST_F(SNLDesignTruthTablesTest1, SetTruthTables_AlreadyHasProperty_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("dup_tt"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Input,  NLName("I"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  // First installation succeeds
  SNLTruthTable t0(1, uint64_t{1});
  ASSERT_NO_THROW(
    SNLDesignTruthTable::setTruthTables(D, { t0 })
  );

  // Second installation must throw "Design already has a Truth Table"
  EXPECT_THROW(
    SNLDesignTruthTable::setTruthTables(D, { t0 }),
    NLException
  );
}

//-----------------------------------------------------------------------------
// Exception when declaredInputs == 0 but two chunks follow (special‐case mismatch)
//-----------------------------------------------------------------------------
TEST_F(SNLDesignTruthTablesTest1, DeclaredInputsZero_TwoChunks_Throws) {
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("zero_mismatch"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  // Manually create the underlying property to simulate two trailing chunks
  auto P = naja::NajaDumpableProperty::create(
      static_cast<naja::NajaObject*>(D),
      "SNLDesignTruthTableProperty"
  );
  P->addUInt64Value(0);  // declaredInputs = 0
  P->addUInt64Value(0);  // chunk 0
  P->addUInt64Value(1);  // chunk 1

  // special‐case: declaredInputs == 0 && tableSize == 2 → expectedChunks = 1,
  // but tableSize is actually 2, so we should get a mismatch exception
  EXPECT_THROW(
    SNLDesignTruthTable::getTruthTable(D),
    NLException
  );
}

//-----------------------------------------------------------------------------
// Test: indexed getTruthTable returns empty when no property is set
//-----------------------------------------------------------------------------

TEST_F(SNLDesignTruthTablesTest1, GetTruthTableIndexed_NoProperty_ReturnsUninitialized) {
  // Create a primitive with one output but never call setTruthTable/setTruthTables
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("empty_indexed"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  // indexed lookup on a design with no property should return an uninitialized table
  auto tt = SNLDesignTruthTable::getTruthTable(D, 0);
  EXPECT_FALSE(tt.isInitialized());
}

//-----------------------------------------------------------------------------
// Test: isConst0 on a design without any truth‐table property should be false
//-----------------------------------------------------------------------------

TEST_F(SNLDesignTruthTablesTest1, IsConst0_NoProperty_ReturnsFalse) {
  // Create a primitive with one output but never set any truth table
  auto D = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("empty_const0"));
  SNLScalarTerm::create(D, SNLTerm::Direction::Output, NLName("O"));

  // Without any stored table, isConst0 must return false
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(D));
}
