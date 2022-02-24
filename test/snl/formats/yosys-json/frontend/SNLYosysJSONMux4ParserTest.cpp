#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "SNLUniverse.h"
#include "SNLException.h"
#include "SNLScalarTerm.h"
#include "SNLYosysJSONParser.h"
#include "SNLVRLDumper.h"

using namespace naja::SNL;

#ifndef SNL_YOSYS_JSON_TEST_PATH
#define SNL_YOSYS_JSON_TEST_PATH "Undefined"
#endif

class SNLYosysJSONMux4ParserTest: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      primitives_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("Primitives"));
      designs_ = SNLLibrary::create(db, SNLName("Designs"));
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* primitives_;
    SNLLibrary* designs_;
};

TEST_F(SNLYosysJSONMux4ParserTest, test) {
  EXPECT_THROW(SNLYosysJSONParser::parse(std::filesystem::path("ERROR"), primitives_, designs_), SNLException);

  ASSERT_NE(nullptr, primitives_);
  ASSERT_NE(nullptr, designs_);
  std::filesystem::path jsonNetlist0Path(SNL_YOSYS_JSON_TEST_PATH);
  jsonNetlist0Path /= "files";
  jsonNetlist0Path /= "mux4.json";
  SNLYosysJSONParser::parse(jsonNetlist0Path, primitives_, designs_);

  EXPECT_EQ(1, primitives_->getDesigns().size());

  EXPECT_EQ(2, designs_->getDesigns().size());
  auto mux2 = designs_->getDesign(SNLName("MUX2"));
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

  EXPECT_EQ(1, mux2->getInstances().size());

  auto mux4 = designs_->getDesign(SNLName("MUX4"));
  ASSERT_NE(nullptr, mux4);
  EXPECT_EQ(3, mux4->getInstances().size());

  std::filesystem::path outPath("mux4.v");
  std::ofstream ofs(outPath, std::ofstream::out);
  SNLVRLDumper dumper;
  dumper.dumpDesign(mux4, ofs);
  ofs.close();
}
