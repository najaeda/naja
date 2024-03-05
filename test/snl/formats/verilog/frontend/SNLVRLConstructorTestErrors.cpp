// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

#include "SNLPyLoader.h"
#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"

using namespace naja::SNL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestErrors: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestErrors, test0) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.parse(benchmarksPath/"errors"/"error0.v");
  constructor.setFirstPass(false);
  EXPECT_THROW(constructor.parse(benchmarksPath/"errors"/"error0.v"), SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test1) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error1.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test2) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error2.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test3) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error3.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test4) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error4.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test5) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error5.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test6) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error6.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test7) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error7.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test8) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error8.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test9) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error9.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test10) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error10.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestErrors, test11) {
  SNLVRLConstructor constructor(library_);
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"errors"/"error11.v"),
    SNLVRLConstructorException);
}