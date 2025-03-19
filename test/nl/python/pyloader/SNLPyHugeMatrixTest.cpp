// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLPyLoader.h"
#include "SNLCapnP.h"
#include "SNLVRLDumper.h"
using namespace naja::NL;

#ifndef SNL_PYEDIT_TEST_PATH
#define SNL_PYEDIT_TEST_PATH "Undefined"
#endif
#ifndef SNL_DUMP_PATH
#define SNL_DUMP_PATH "Undefined"
#endif

class SNLPyHugeMatrixTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse::create();
      auto db = NLDB::create(NLUniverse::get());
      auto primitivesLibrary = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
      auto square = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive, NLName("square"));
      auto n = SNLScalarTerm::create(square, SNLTerm::Direction::Output, NLName("n"));
      auto e = SNLScalarTerm::create(square, SNLTerm::Direction::Output, NLName("e"));
      auto s = SNLScalarTerm::create(square, SNLTerm::Direction::Input, NLName("s"));
      auto w = SNLScalarTerm::create(square, SNLTerm::Direction::Input, NLName("w"));
      designsLibrary_ = NLLibrary::create(db, NLName("designs"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    NLLibrary*   designsLibrary_;
};

TEST_F(SNLPyHugeMatrixTest, test) {
  auto db = NLDB::create(NLUniverse::get());
  auto scriptPath = std::filesystem::path(SNL_PYEDIT_TEST_PATH);
  scriptPath /= "scripts";
  scriptPath /= "huge_matrix.py";
  auto top = SNLDesign::create(designsLibrary_, NLName("top"));
  SNLPyLoader::loadDesign(top, scriptPath);

  //dump the design
  auto dumpPath = std::filesystem::path(SNL_DUMP_PATH);
  dumpPath /= "huge_matrix";
  if (std::filesystem::exists(dumpPath)) {
    std::filesystem::remove_all(dumpPath);
  }
  SNLCapnP::dump(top->getDB(), dumpPath);

  //dump verilog
  auto outPath = std::filesystem::path(SNL_DUMP_PATH);
  SNLVRLDumper dumper;
  dumper.setTopFileName(top->getName().getString() + ".v");
  dumper.setSingleFile(true);
  dumper.dumpDesign(top, outPath);
  
  NLUniverse::get()->destroy();  
  top = nullptr;
  designsLibrary_ = nullptr;
  
  SNLCapnP::load(dumpPath);
}
