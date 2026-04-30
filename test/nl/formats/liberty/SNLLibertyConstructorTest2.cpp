// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <fstream>

#include "NLException.h"
#include "NLUniverse.h"

#include "SNLBundleTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

#include "SNLLibertyConstructor.h"
#include "NLBitVecDynamic.h"
#include "SNLDesignModeling.h"
#include "SNLCapnP.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest2: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary*  library_;
    NLDB* db_ = nullptr;
};

TEST_F(SNLLibertyConstructorTest2, test) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto and2 = library_->getSNLDesign(NLName("and2"));
  ASSERT_NE(nullptr, and2);
  EXPECT_EQ(3, and2->getTerms().size());
  EXPECT_EQ(3, and2->getScalarTerms().size());
  EXPECT_TRUE(and2->getBusTerms().empty());
  auto i0 = and2->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = and2->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = and2->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignModeling::getTruthTable(and2);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(NLBitVecDynamic(0b1000, 4), tt.bits());
  //
  //EXPECT_TRUE(SNLDesignModeling::isAnd(design));

  auto ff = library_->getSNLDesign(NLName("FF"));
  ASSERT_NE(nullptr, ff);
  EXPECT_EQ(3, ff->getTerms().size());
  EXPECT_EQ(3, ff->getScalarTerms().size());
  EXPECT_TRUE(ff->getBusTerms().empty());
  auto d = ff->getScalarTerm(NLName("D"));
  ASSERT_NE(nullptr, d);
  EXPECT_EQ(SNLTerm::Direction::Input, d->getDirection());
  auto ck = ff->getScalarTerm(NLName("CK"));
  ASSERT_NE(nullptr, ck);
  EXPECT_EQ(SNLTerm::Direction::Input, ck->getDirection());
  auto q = ff->getScalarTerm(NLName("Q"));
  ASSERT_NE(nullptr, q);
  EXPECT_EQ(SNLTerm::Direction::Output, q->getDirection());
  auto ff_tt = SNLDesignModeling::getTruthTable(ff);
  EXPECT_FALSE(ff_tt.isInitialized());
  {
    auto clocks = SNLDesignModeling::getInputRelatedClocks(d);
    EXPECT_EQ(1, clocks.size());
  }
  {
    auto clocks = SNLDesignModeling::getOutputRelatedClocks(q);
    EXPECT_EQ(1, clocks.size());
  }
  // dump to naja if
  std::filesystem::path outPath("dump_if");
  SNLCapnP::dump(db_, outPath);
  {
    NLUniverse::get()->destroy();
    NLUniverse* universe = NLUniverse::create();
    db_ = NLDB::create(universe);
    library_ = NLLibrary::create(db_, NLLibrary::Type::Primitives, NLName("MYLIB"));
    SNLLibertyConstructor constructor(library_);
    std::filesystem::path testPath(
        std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
        / std::filesystem::path("benchmarks")
        / std::filesystem::path("tests")
        / std::filesystem::path("small.lib"));
    constructor.construct(testPath);
  }
#if 0
  db_ = SNLCapnP::load(outPath, true);
  NLUniverse::get()->destroy();
  EXPECT_THROW(SNLCapnP::load(outPath, true), NLException);
  {
    NLUniverse::get()->destroy();
    NLUniverse* universe = NLUniverse::create();
    db_ = NLDB::create(universe);
  }
  EXPECT_THROW(SNLCapnP::load(outPath, true), NLException);
#endif
}

TEST_F(SNLLibertyConstructorTest2, TestGZ) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz"));
  constructor.construct(testPath);
}

TEST_F(SNLLibertyConstructorTest2, testBundleCapnpDumpUnsupported) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bundle_issue_120.lib"));
  constructor.construct(testPath);
  EXPECT_THROW(SNLCapnP::dump(db_, "bundle_dump_if"), NLException);
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryBusBitsAreMarkedSequential) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_bus_bits_are_marked_sequential.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array;
    data_type : bit;
    bit_width : 4;
    bit_from : 3;
    bit_to : 0;
    downto : true;
  }
  type (mem_addr) {
    base_type : array;
    data_type : bit;
    bit_width : 2;
    bit_from : 1;
    bit_to : 0;
    downto : true;
  }
  cell(mem) {
    memory() {
      type : ram;
      address_width : 2;
      word_width : 4;
    }
    pin(clk) {
      direction : input;
      clock : true;
    }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() {
        address : addr_in;
      }
      timing() {
        related_pin : "clk";
        timing_type : rising_edge;
      }
    }
    bus(addr_in) {
      bus_type : mem_addr;
      direction : input;
      timing() {
        related_pin : "clk";
        timing_type : setup_rising;
      }
      timing() {
        related_pin : "clk";
        timing_type : hold_rising;
      }
    }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() {
        address : addr_in;
        clocked_on : "clk";
      }
      timing() {
        related_pin : "clk";
        timing_type : setup_rising;
      }
      timing() {
        related_pin : "clk";
        timing_type : hold_rising;
      }
    }
    pin(we_in) {
      direction : input;
      timing() {
        related_pin : "clk";
        timing_type : setup_rising;
      }
      timing() {
        related_pin : "clk";
        timing_type : hold_rising;
      }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  constructor.construct(tempPath);

  auto* memory = library_->getSNLDesign(NLName("mem"));
  ASSERT_NE(nullptr, memory);

  auto* clk = memory->getScalarTerm(NLName("clk"));
  auto* rdOut = memory->getBusTerm(NLName("rd_out"));
  auto* addrIn = memory->getBusTerm(NLName("addr_in"));
  auto* wdIn = memory->getBusTerm(NLName("wd_in"));
  auto* weIn = memory->getScalarTerm(NLName("we_in"));
  ASSERT_NE(nullptr, clk);
  ASSERT_NE(nullptr, rdOut);
  ASSERT_NE(nullptr, addrIn);
  ASSERT_NE(nullptr, wdIn);
  ASSERT_NE(nullptr, weIn);

  for (auto* bit : rdOut->getBits()) {
    EXPECT_EQ(1, SNLDesignModeling::getOutputRelatedClocks(bit).size())
        << "read-data bit " << bit->getName().getString()
        << "[" << bit->getBit() << "] should be clocked";
  }

  for (auto* bit : addrIn->getBits()) {
    EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(bit).size())
        << "address bit " << bit->getName().getString()
        << "[" << bit->getBit() << "] should be clock-related";
  }

  for (auto* bit : wdIn->getBits()) {
    EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(bit).size())
        << "write-data bit " << bit->getName().getString()
        << "[" << bit->getBit() << "] should be clock-related";
  }

  EXPECT_EQ(1, SNLDesignModeling::getInputRelatedClocks(weIn).size());

  EXPECT_TRUE(SNLDesignModeling::hasMemoryInterface(memory));
  const auto interface = SNLDesignModeling::getMemoryInterface(memory);
  EXPECT_EQ(4u, interface.width);
  EXPECT_EQ(4u, interface.depth);
  EXPECT_EQ(2u, interface.abits);
  EXPECT_EQ(clk, interface.clock);
  EXPECT_EQ(1u, interface.readPorts.size());
  EXPECT_EQ(1u, interface.writePorts.size());
  EXPECT_EQ(2u, interface.readPorts.front().address.size());
  EXPECT_EQ(4u, interface.readPorts.front().data.size());
  EXPECT_EQ(2u, interface.writePorts.front().address.size());
  EXPECT_EQ(4u, interface.writePorts.front().data.size());
  EXPECT_TRUE(interface.writePorts.front().mask.empty());
  EXPECT_EQ(1u, interface.writePorts.front().enables.size());
  EXPECT_EQ(weIn, interface.writePorts.front().enables.front());
}

TEST_F(SNLLibertyConstructorTest2, testLibertyMemoryInterfaceCapturesMaskAndControls) {
  auto tempPath = std::filesystem::temp_directory_path()
                  / "naja_liberty_memory_interface_captures_mask_and_controls.lib";
  {
    std::ofstream output(tempPath, std::ios::binary);
    output << R"(library(memlib) {
  type (mem_data) {
    base_type : array;
    data_type : bit;
    bit_width : 4;
    bit_from : 3;
    bit_to : 0;
    downto : true;
  }
  type (mem_addr) {
    base_type : array;
    data_type : bit;
    bit_width : 2;
    bit_from : 1;
    bit_to : 0;
    downto : true;
  }
  cell(mem) {
    memory() {
      type : ram;
      address_width : 2;
      word_width : 4;
    }
    pin(clk) { direction : input; clock : true; }
    pin(we_in) {
      direction : input;
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    pin(ce_in) {
      direction : input;
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(rd_out) {
      bus_type : mem_data;
      direction : output;
      memory_read() { address : addr_in; }
      timing() { related_pin : "clk"; timing_type : rising_edge; }
    }
    bus(addr_in) {
      bus_type : mem_addr;
      direction : input;
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(wd_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
    bus(w_mask_in) {
      bus_type : mem_data;
      direction : input;
      memory_write() { address : addr_in; clocked_on : "clk"; }
      timing() { related_pin : "clk"; timing_type : setup_rising; }
      timing() { related_pin : "clk"; timing_type : hold_rising; }
    }
  }
})";
  }

  SNLLibertyConstructor constructor(library_);
  constructor.construct(tempPath);

  auto* memory = library_->getSNLDesign(NLName("mem"));
  ASSERT_NE(nullptr, memory);
  auto* weIn = memory->getScalarTerm(NLName("we_in"));
  auto* ceIn = memory->getScalarTerm(NLName("ce_in"));
  ASSERT_NE(nullptr, weIn);
  ASSERT_NE(nullptr, ceIn);

  ASSERT_TRUE(SNLDesignModeling::hasMemoryInterface(memory));
  const auto interface = SNLDesignModeling::getMemoryInterface(memory);
  ASSERT_EQ(1u, interface.writePorts.size());
  EXPECT_EQ(4u, interface.writePorts.front().data.size());
  EXPECT_EQ(4u, interface.writePorts.front().mask.size());
  EXPECT_EQ(2u, interface.writePorts.front().enables.size());

  std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> enableTerms(
      interface.writePorts.front().enables.begin(),
      interface.writePorts.front().enables.end());
  EXPECT_EQ(1u, enableTerms.count(weIn));
  EXPECT_EQ(1u, enableTerms.count(ceIn));
}
