// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "NLDB0.h"
#include "NLUniverse.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLVRLConstructor.h"

using namespace naja::NL;

class SNLVRLConstructorTestSignedNets: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }

  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLVRLConstructorTestSignedNets, testSignedNetDeclarations) {
  auto testPath = std::filesystem::temp_directory_path() / "naja_vrl_signed_nets.v";
  {
    std::ofstream stream(testPath);
    stream
      << "module top(a);\n"
      << "  output a;\n"
      << "  wire signed [42:0] _0000_;\n"
      << "  wire signed [7:0] signed_bus;\n"
      << "  wire        [7:0] unsigned_bus;\n"
      << "  wire signed       signed_scalar;\n"
      << "  wire              unsigned_scalar;\n"
      << "  assign a = _0000_[0];\n"
      << "endmodule\n";
  }

  SNLVRLConstructor constructor(library_);
  ASSERT_NO_THROW(constructor.construct(testPath));

  auto top = library_->getSNLDesign(NLName("top"));
  ASSERT_NE(nullptr, top);

  auto a = top->getScalarTerm(NLName("a"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Output, a->getDirection());

  auto signedNet = top->getBusNet(NLName("_0000_"));
  ASSERT_NE(nullptr, signedNet);
  EXPECT_EQ(42, signedNet->getMSB());
  EXPECT_EQ(0, signedNet->getLSB());
  EXPECT_EQ(43, signedNet->getWidth());

  auto signedBus = top->getBusNet(NLName("signed_bus"));
  ASSERT_NE(nullptr, signedBus);
  EXPECT_EQ(8, signedBus->getWidth());
  auto unsignedBus = top->getBusNet(NLName("unsigned_bus"));
  ASSERT_NE(nullptr, unsignedBus);
  EXPECT_EQ(8, unsignedBus->getWidth());
  EXPECT_NE(nullptr, top->getScalarNet(NLName("signed_scalar")));
  EXPECT_NE(nullptr, top->getScalarNet(NLName("unsigned_scalar")));

  ASSERT_EQ(1, top->getInstances().size());
  auto assign = *top->getInstances().begin();
  ASSERT_EQ(NLDB0::getAssign(), assign->getModel());
  ASSERT_NE(nullptr, signedNet->getBit(0));
  ASSERT_NE(nullptr, a->getNet());
  EXPECT_EQ(
    signedNet->getBit(0),
    assign->getInstTerm(NLDB0::getAssignInput())->getNet());
  EXPECT_EQ(
    a->getNet(),
    assign->getInstTerm(NLDB0::getAssignOutput())->getNet());
}
