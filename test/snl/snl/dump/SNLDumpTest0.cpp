#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLDumper.h"
using namespace naja::SNL;

#include "SNLNetlists.h"

class SNLDumpTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      top_ = SNLNetlists::create();

    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
};

TEST_F(SNLDumpTest0, test) {
}