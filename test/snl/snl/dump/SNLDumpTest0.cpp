#include "gtest/gtest.h"
#include <filesystem>

#include "SNLUniverse.h"
#include "SNLDump.h"
using namespace naja::SNL;

#include "SNLNetlist0.h"

#ifndef SNL_DUMP_PATHS
#define SNL_DUMP_PATHS "Undefined"
#endif

namespace {

std::filesystem::path createDumpsDir() {
  std::filesystem::path dumpsPath(SNL_DUMP_PATHS);
  dumpsPath /= "dumps";
  if (not std::filesystem::exists(dumpsPath)) {
    std::filesystem::create_directory(dumpsPath);
  }
  return dumpsPath;
}

}

class SNLDumpTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      dumpsPath_ = createDumpsDir();
      auto u = SNLUniverse::create();
      auto db = SNLDB::create(u);
      top_ = SNLNetlist0::create(db);
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
    }
    SNLDesign*            top_;
    std::filesystem::path dumpsPath_;
};

TEST_F(SNLDumpTest0, test) {
  std::filesystem::path test0Path(dumpsPath_);
  test0Path /= "test0.snl";
  SNLDump::dump(top_, test0Path);

  {
    SNLDump::load(test0Path);
  }
}