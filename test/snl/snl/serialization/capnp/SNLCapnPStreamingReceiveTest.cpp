// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <ifaddrs.h>

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

#include "SNLCapnP.h"

using namespace naja::SNL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

class SNLCapnPStreamingReceiveTest: public ::testing::Test {
  //Test libraries
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      db_ = SNLDB::create(universe);
      //
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
  protected:
    SNLDB*      db_;
};

using path = std::filesystem::path;

namespace {

SNLDB* receive(uint16_t port) {
  return SNLCapnP::receive(port);
}

}

TEST_F(SNLCapnPStreamingReceiveTest, test0) {
#if 0
  SNLDB* db = nullptr;
  std::thread receiverThread([&]{ std::cerr << "Starting receiver" << std::endl; db = SNLCapnP::receive(44444); });
  receiverThread.detach();
  std::this_thread::sleep_for(std::chrono::seconds(4));

  ASSERT_NE(nullptr, db_);
  std::cerr << "Sending DB" << std::endl;
  SNLCapnP::send(db_, "127.0.0.1", 44444, 2);

  //Wait for DB to be received, max 5s
  unsigned nbSecs = 0;
  while (not db and nbSecs < 5) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ++nbSecs;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  ASSERT_NE(nullptr, db);
  std::string reason;
  EXPECT_TRUE(db_->deepCompare(db, reason));
  EXPECT_TRUE(reason.empty());
#endif
}