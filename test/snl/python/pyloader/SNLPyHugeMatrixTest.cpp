// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLException.h"
#include "SNLPyLoader.h"
#include "SNLCapnP.h"
#include "SNLVRLDumper.h"
using namespace naja::SNL;

#ifndef SNL_PYEDIT_TEST_PATH
#define SNL_PYEDIT_TEST_PATH "Undefined"
#endif
#ifndef SNL_DUMP_PATH
#define SNL_DUMP_PATH "Undefined"
#endif

class SNLPyHugeMatrixTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse::create();
      auto db = SNLDB::create(SNLUniverse::get());
      auto primitivesLibrary = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("primitives"));
      auto square = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive, SNLName("square"));
      auto n = SNLScalarTerm::create(square, SNLTerm::Direction::Output, SNLName("n"));
      auto e = SNLScalarTerm::create(square, SNLTerm::Direction::Output, SNLName("e"));
      auto s = SNLScalarTerm::create(square, SNLTerm::Direction::Input, SNLName("s"));
      auto w = SNLScalarTerm::create(square, SNLTerm::Direction::Input, SNLName("w"));
      designsLibrary_ = SNLLibrary::create(db, SNLName("designs"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* designsLibrary_;
};

TEST_F(SNLPyHugeMatrixTest, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "scripts";
  scriptPath /= "huge_matrix.py";
  auto top = SNLDesign::create(designsLibrary_, SNLName("top"));
  SNLPyLoader::loadDesign(top, scriptPath);

  //dump the design
  auto dumpPath = std::filesystem::path(SNL_DUMP_PATH);
  dumpPath /= "huge_matrix";
  if (std::filesystem::exists(dumpPath)) {
    std::filesystem::remove_all(dumpPath);
  }
  SNLCapnP::dump(top->getDB(), dumpPath);
  SNLUniverse::get()->destroy();  
  designsLibrary_ = nullptr;
  SNLCapnP::load(dumpPath);

  //dump verilog
  auto outPath = std::filesystem::path(SNL_DUMP_PATH);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);



}