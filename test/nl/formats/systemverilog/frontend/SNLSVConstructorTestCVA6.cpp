// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "DNL.h"
#include "LogicCone.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBitTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLOccurrence.h"
#include "SNLSVConstructor.h"
#include "SNLTerm.h"

using namespace naja::NL;
using namespace naja::NAJA_METRICS;

#ifndef SNL_SV_DUMPER_TEST_PATH
#define SNL_SV_DUMPER_TEST_PATH "Undefined"
#endif

namespace {

void setEnvVar(const char* name, const std::string& value) {
#if defined(_WIN32)
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void unsetEnvVar(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

class ScopedEnvVar {
  public:
    explicit ScopedEnvVar(const char* name):
      name_(name) {
      if (const char* value = std::getenv(name)) {
        previous_ = value;
      }
    }

    ~ScopedEnvVar() {
      if (previous_) {
        setEnvVar(name_.c_str(), *previous_);
      } else {
        unsetEnvVar(name_.c_str());
      }
    }

    void setIfMissing(const std::string& value) const {
      if (not std::getenv(name_.c_str())) {
        setEnvVar(name_.c_str(), value);
      }
    }

  private:
    std::string name_;
    std::optional<std::string> previous_;
};

template <typename Cone>
std::pair<Cone, std::chrono::microseconds> makeTimedCone(
    const SNLOccurrence& start,
    typename Cone::Direction direction) {
  const auto begin = std::chrono::steady_clock::now();
  Cone cone(start, direction);
  const auto end = std::chrono::steady_clock::now();
  return {
      std::move(cone),
      std::chrono::duration_cast<std::chrono::microseconds>(end - begin)};
}

template <typename Cone>
void printConeStats(
    const std::string& bitName,
    const char* implementationName,
    const Cone& cone,
    std::chrono::microseconds elapsed) {
  std::cout
      << "[CVA6 logic cone] bit=" << bitName
      << " impl=" << implementationName
      << " nodes=" << cone.getNodeCount()
      << " time_us=" << elapsed.count()
      << std::endl;
}

}  // namespace

class SNLSVConstructorTestCVA6: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("SVLIB"));
    }

    void TearDown() override {
      naja::DNL::destroy();
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
      library_ = nullptr;
    }

    NLLibrary* library_ {nullptr};
};

TEST_F(SNLSVConstructorTestCVA6, cva6NocReqOLogicConeBuilds) {
  const char* cva6RepoEnv = std::getenv("CVA6_REPO_DIR");
  if (cva6RepoEnv == nullptr or std::string(cva6RepoEnv).empty()) {
    GTEST_SKIP() << "Set CVA6_REPO_DIR to a CVA6 checkout to run this test";
  }

  const std::filesystem::path cva6Repo(cva6RepoEnv);
  const auto flist = cva6Repo / "core" / "Flist.cva6";
  if (not std::filesystem::exists(flist)) {
    GTEST_SKIP() << "Missing CVA6 flist: " << flist;
  }

  const ScopedEnvVar hpdcacheDir("HPDCACHE_DIR");
  const ScopedEnvVar targetCfg("TARGET_CFG");
  hpdcacheDir.setIfMissing(
      (cva6Repo / "core" / "cache_subsystem" / "hpdcache").string());
  targetCfg.setIfMissing("cv64a6_imafdc_sv39");

  const auto wrapperFlist =
      std::filesystem::path(SNL_SV_DUMPER_TEST_PATH) / "cva6_top.f";
  {
    std::ofstream stream(wrapperFlist);
    ASSERT_TRUE(stream.good());
    stream << "--top cva6\n";
    stream << "-f \"" << flist.string() << "\"\n";
  }

  SNLSVConstructor constructor(library_);
  constructor.construct(SNLSVConstructor::Paths {
      std::filesystem::path("-f"),
      wrapperFlist,
  });

  auto top = library_->getSNLDesign(NLName("cva6"));
  ASSERT_NE(nullptr, top);
  NLUniverse::get()->setTopDesign(top);

  auto nocReqO = top->getTerm(NLName("noc_req_o"));
  ASSERT_NE(nullptr, nocReqO);
  auto nocReqOBus = dynamic_cast<SNLBusTerm*>(nocReqO);
  ASSERT_NE(nullptr, nocReqOBus);

  naja::DNL::destroy();
  naja::DNL::get();
  const std::vector<NLID::Bit> checkedBits {468, 469};
  for (auto bitIndex : checkedBits) {
    auto bit = nocReqOBus->getBit(bitIndex);
    ASSERT_NE(nullptr, bit) << "Missing noc_req_o[" << bitIndex << "]";
    SCOPED_TRACE(bit->getString());
    const auto bitName = bit->getString();
    const auto start = SNLOccurrence(bit);
    auto [cone, elapsed] = makeTimedCone<LogicCone>(
        start, LogicCone::Direction::FanIn);
    printConeStats(bitName, "LogicCone", cone, elapsed);
    EXPECT_GT(cone.getNodeCount(), 0);
    EXPECT_EQ(start, cone.getNodes()[cone.getRoot()].occurrence);
  }
}
