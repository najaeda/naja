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