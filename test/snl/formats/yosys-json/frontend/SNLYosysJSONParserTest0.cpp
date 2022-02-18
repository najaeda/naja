#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLException.h"
#include "SNLScalarTerm.h"
#include "SNLYosysJSONParser.h"

using namespace naja::SNL;

#ifndef SNL_YOSYS_JSON_TEST_PATH
#define SNL_YOSYS_JSON_TEST_PATH "Undefined"
#endif

class SNLYosysJSONParserTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLName("MYLIB"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* library_;
};

TEST_F(SNLYosysJSONParserTest0, test) {
  EXPECT_THROW(SNLYosysJSONParser::parse(std::filesystem::path("ERROR"), library_), SNLException);

  ASSERT_NE(nullptr, library_);
  std::filesystem::path jsonNetlist0Path(SNL_YOSYS_JSON_TEST_PATH);
  jsonNetlist0Path /= "files";
  jsonNetlist0Path /= "mux4.json";
  SNLYosysJSONParser::parse(jsonNetlist0Path, library_);

  EXPECT_EQ(2, library_->getDesigns().size());
  auto mux2 = library_->getDesign(SNLName("MUX2"));
  ASSERT_NE(nullptr, mux2);
  EXPECT_EQ(4, mux2->getTerms().size());
  EXPECT_EQ(4, mux2->getScalarTerms().size());
  EXPECT_TRUE(mux2->getBusTerms().empty());

  auto i0 = mux2->getScalarTerm(SNLName("I0"));
  ASSERT_NE(nullptr, i0);
  auto i1 = mux2->getScalarTerm(SNLName("I1"));
  ASSERT_NE(nullptr, i1);
  auto s0 = mux2->getScalarTerm(SNLName("S0"));
  ASSERT_NE(nullptr, s0);
  auto o = mux2->getScalarTerm(SNLName("O"));
  ASSERT_NE(nullptr, o);

  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, s0->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output, o->getDirection());

#if 0
  auto lib = db_->getLibrary(SNLName("MYLIB"));  
  ASSERT_TRUE(lib);
  auto top = lib->getDesign(SNLName("design"));
  ASSERT_TRUE(top);
  std::filesystem::path outPath("test.v");
  std::ofstream ofs(outPath, std::ofstream::out);
  SNLVRLDumper dumper;
  dumper.dumpDesign(top, ofs);
  ofs.close();
#endif
}