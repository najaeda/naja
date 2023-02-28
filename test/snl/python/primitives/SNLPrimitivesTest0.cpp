#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLPrimitivesLoader.h"
using namespace naja::SNL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPrimitivesTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse::create();
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPrimitivesTest0, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "primitives0";
  SNLPrimitivesLoader::load(library, primitives0Path);
}