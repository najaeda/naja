// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "VHDLConstructor.h"
#include "VHDLTestUtils.h"
#include <fstream>
#include <iostream>

#include <cstdlib>
#include <filesystem>

using namespace naja::NL;

class VHDLExternalTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto* db = NLDB::create(NLUniverse::create());
    library_ = NLLibrary::create(db, NLLibrary::Type::Standard);
  }
  void TearDown() override { NLUniverse::get()->destroy(); }

  NLLibrary* library_ {};
};

TEST_F(VHDLExternalTest, AESDecryptBenchmark) {
  const auto* directory = std::getenv("VHDL_AES_BENCHMARK");
  ASSERT_NE(directory, nullptr) << "Set VHDL_AES_BENCHMARK to the downloaded AES rtl directory";
  const auto root = std::filesystem::path(directory);
  VHDLConstructor constructor(library_);
  EXPECT_EQ(constructor.constructFile(root / "aes_pkg.vhdl"), nullptr);
  ASSERT_NE(constructor.constructFile(root / "key_expansion.vhdl"), nullptr);
  auto* top = constructor.constructFile(root / "aes_dec.vhdl", "aes_dec");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getTerms().size(), 10u);
  ASSERT_NE(top->getInstance(NLName("KEXP0")), nullptr);
  auto* ram = top->getInstance(NLName("t_state_ram0_mem"));
  ASSERT_NE(ram, nullptr);
  ASSERT_TRUE(NLDB0::isMemory(ram->getModel()));
  const auto ramSignature = NLDB0::getMemorySignature(ram);
  EXPECT_EQ(ramSignature.width, 32u);
  EXPECT_EQ(ramSignature.depth, 4u);
  EXPECT_EQ(ramSignature.readPorts, 1u);
  EXPECT_EQ(ramSignature.writePorts, 1u);
  EXPECT_EQ(ramSignature.resetMode, NLDB0::MemoryResetMode::None);
  for (auto* design : {top, top->getInstance(NLName("KEXP0"))->getModel()}) {
    size_t flops = 0, flopBits = 0, muxes = 0, muxBits = 0;
    for (auto* instance : design->getInstances()) {
      auto* model = instance->getModel();
      if (NLDB0::isDFF(model)) {
        ++flops;
        flopBits += model->getTerm(NLName("Q"))->getWidth();
      }
      if (NLDB0::isMux2(model)) {
        ++muxes;
        muxBits += NLDB0::getMux2Output(model)->getWidth();
      }
    }
    EXPECT_LT(flops, flopBits);
    EXPECT_LT(muxes, muxBits);
    std::cout << design->getName().getString() << ": " << design->getInstances().size()
              << " instances, " << flops << " DFFs (" << flopBits << " bits), "
              << muxes << " muxes (" << muxBits << " bits)\n";
  }
}

TEST_F(VHDLExternalTest, FIRBenchmark) {
  const auto* file = std::getenv("VHDL_FIR_BENCHMARK");
  ASSERT_NE(file, nullptr);
  auto* top = VHDLConstructor(library_).constructFile(file, "cf_fir_12_16_10");
  ASSERT_NE(top, nullptr);
  EXPECT_NE(top->getInstance(NLName("s1")), nullptr);
}

TEST_F(VHDLExternalTest, FIR16BenchmarkCycles) {
  const auto* file = std::getenv("VHDL_FIR16_BENCHMARK");
  ASSERT_NE(file, nullptr);
  const auto* trace = std::getenv("VHDL_FIR_REFERENCE");
  ASSERT_NE(trace, nullptr);
  auto* top = VHDLConstructor(library_).constructFile(file);
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getName(), NLName("fir16"));
  const auto outputs = naja::NL::test::simulateFIR(top, naja::NL::test::firInputs(16));
  std::ifstream reference(trace);
  ASSERT_TRUE(reference);
  for (size_t cycle = 3; cycle < outputs.size(); ++cycle) {
    unsigned expected;
    ASSERT_TRUE(reference >> expected);
    EXPECT_EQ(outputs[cycle], expected) << "NVC cycle " << cycle;
  }
  std::string trailing;
  EXPECT_FALSE(reference >> trailing);
}

TEST_F(VHDLExternalTest, FIRDecDSPBenchmarkAndSignedStageCycles) {
  const auto* directory = std::getenv("VHDL_FIRDEC_BENCHMARK");
  ASSERT_NE(directory, nullptr);
  const auto root = std::filesystem::path(directory);
  VHDLConstructor constructor(library_);
  ASSERT_NE(constructor.constructFile(root / "fir16.vhd"), nullptr);
  EXPECT_EQ(constructor.constructFile(root / "hbfc1.vhd"), nullptr);
  EXPECT_EQ(constructor.constructFile(root / "hbfc2.vhd"), nullptr);
  auto* top = constructor.constructFile(root / "firdec.vhd", "firdec_DSP");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getTerms().size(), 7u);
  for (const auto* stage : {"fir16", "hbfc1", "hbfc2"}) {
    auto* i = top->getInstance(NLName(std::string("inst_") + stage + "_i"));
    auto* q = top->getInstance(NLName(std::string("inst_") + stage + "_q"));
    ASSERT_NE(i, nullptr);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(i->getModel(), q->getModel());
  }
  // Check both signed half-band stages against their integer recurrence. State
  // starts at zero in this two-state harness; each clock advances independently.
  for (const auto* name : {"inst_hbfc1_i", "inst_hbfc2_i"}) {
    auto* stage = top->getInstance(NLName(name))->getModel();
    auto* input = stage->getBusTerm(NLName("x_in"));
    auto* output = stage->getBusTerm(NLName("y_out"));
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::unordered_map<SNLBitNet*, bool> state;
    for (const auto& flop : naja::NL::test::dffBits(stage)) state[flop.output] = false;
    std::vector<int64_t> fast(12, 0), slow(6, 0);
    int64_t delayed = 0;
    const int coefficients[] = {-1, 1, -2, 3, -6, 20, 20, -6, 3, -2, 1, -1};
    const auto sign = int64_t(1) << (input->getWidth()-1);
    for (unsigned cycle = 0; cycle < 40; ++cycle) {
      const int64_t sample = cycle % 4 == 0 ? -sign : cycle % 4 == 1 ? sign-1 :
          (int64_t(cycle * 7919) % (2*sign)) - sign;
      auto values = state;
      for (unsigned bit = 0; bit < input->getWidth(); ++bit)
        values[input->getBit(bit)->getNet()] = (uint64_t(sample) >> bit) & 1;
      std::unordered_set<SNLBitNet*> visiting;
      for (const auto& flop : naja::NL::test::dffBits(stage)) {
        auto* clock = flop.clock;
        const bool fastClock = clock == stage->getScalarTerm(NLName("ck"))->getNet();
        ASSERT_TRUE(fastClock || clock == stage->getScalarTerm(NLName("ck2"))->getNet());
        if (fastClock || cycle % 2 == 0)
          state[flop.output] =
              naja::NL::test::evaluateRTL(flop.data, values, visiting);
      }
      if (cycle % 2 == 0) {
        fast.insert(fast.begin(), sample); fast.pop_back();
        slow.insert(slow.begin(), delayed); slow.pop_back();
      }
      delayed = sample;
      int64_t expected = 32 * slow.back();
      for (size_t i = 0; i < fast.size(); ++i) expected += coefficients[i] * fast[i];
      values = state;
      for (unsigned bit = 0; bit < input->getWidth(); ++bit)
        values[input->getBit(bit)->getNet()] = (uint64_t(sample) >> bit) & 1;
      visiting.clear();
      for (unsigned bit = 0; bit < output->getWidth(); ++bit)
        EXPECT_EQ(naja::NL::test::evaluateRTL(output->getBit(bit)->getNet(), values, visiting),
                  bool((uint64_t(expected) >> bit) & 1)) << name << " cycle " << cycle << " bit " << bit;
    }
  }
}
