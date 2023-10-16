#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDesignModeling.h"
#include "SNLScalarTerm.h"
using namespace naja::SNL;

class SNLDesignModelingTest0: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignModelingTest0, test0) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto lut = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("LUT"));
  auto luti0 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I0"));
  auto luti1 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I1"));
  auto luti2 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I2"));
  auto luti3 = SNLScalarTerm::create(lut, SNLTerm::Direction::Input, SNLName("I3"));
  auto luto = SNLScalarTerm::create(lut, SNLTerm::Direction::Output, SNLName("O"));
  SNLDesignModeling::addTimingArc(luti0, luto);

  auto reg = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("REG"));
}