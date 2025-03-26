// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using namespace std;

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLException.h"

using namespace naja::NL;

class PNLInstanceTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      db_ = NLDB::create(universe);
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }
    NLDB* db_;
};

TEST_F(PNLInstanceTest0, testCreation) {
  NLLibrary* library = NLLibrary::create(db_, NLName("MYLIB"));
  PNLDesign* design = PNLDesign::create(library, NLName("design"));
  PNLDesign* model = PNLDesign::create(library, NLName("model"));
  EXPECT_EQ(NLName("design"), design->getName());
  EXPECT_EQ(NLName("model"), model->getName());
  EXPECT_EQ(0, design->getID());
  EXPECT_EQ(1, model->getID());

  auto instance = PNLInstance::create(design, model, NLName("instance"));
  instance->destroy();
}