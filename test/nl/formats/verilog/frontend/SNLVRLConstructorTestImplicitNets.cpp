// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

#include "NLUniverse.h"

#include "SNLVRLConstructor.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestImplicitNets: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
      library_ = nullptr;
    }
  protected:
    NLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestImplicitNets, test0) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"implicit_nets.v");
  auto test0 = library_->getSNLDesign(NLName("implicit_net0"));
}