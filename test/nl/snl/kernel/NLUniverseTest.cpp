// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <set>
#include <vector>

#include "NLUniverse.h"

#include "NLException.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLObject.h"
#include "PNLTechnology.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
using namespace naja::NL;

class NLUniverseTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(NLUniverseTest, testGetSNLObjects1) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  auto universe = NLUniverse::get();
  ASSERT_TRUE(universe);

  EXPECT_FALSE(universe->getDBs().empty());
  EXPECT_EQ(1, universe->getDBs().size());
  EXPECT_TRUE(universe->getUserDBs().empty());
  EXPECT_EQ(0, universe->getUserDBs().size());
  EXPECT_EQ(nullptr, universe->getTerm(NLID::DesignObjectReference(1, 1, 2, 3)));
  EXPECT_EQ(nullptr, universe->getBusTermBit(NLID(NLID::Type::TermBit, 1, 1, 1, 0, 1, 1)));
  EXPECT_EQ(nullptr, universe->getNet(NLID::DesignObjectReference(2, 3, 1, 1)));
  EXPECT_EQ(nullptr, universe->getBitNet(NLID::BitNetReference(1, 1, 1, 1, 1)));
  EXPECT_EQ(nullptr, universe->getInstance(NLID::DesignObjectReference(1, 1, 1, 1)));
}

TEST_F(NLUniverseTest, testGetObjectFromTypedNLIDs) {
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto library = NLLibrary::create(db, NLName("work"));
  auto model = SNLDesign::create(library, NLName("model"));
  auto modelInput = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("i"));
  auto modelOutput = SNLBusTerm::create(model, SNLTerm::Direction::Output, 3, 0, NLName("o"));
  auto top = SNLDesign::create(library, NLName("top"));
  auto net = SNLScalarNet::create(top, NLName("n"));
  auto busNet = SNLBusNet::create(top, 3, 0, NLName("b"));
  auto instance = SNLInstance::create(top, model, NLName("u0"));
  auto instTerm = instance->getInstTerm(modelInput);
  ASSERT_NE(nullptr, instTerm);

  struct ExpectedObject {
    NLID id_;
    const void* object_;
  };
  std::vector<ExpectedObject> objects = {
    {top->getNLID(), static_cast<const void*>(top)},
    {net->getNLID(), static_cast<const void*>(net)},
    {busNet->getBit(2)->getNLID(), static_cast<const void*>(busNet->getBit(2))},
    {modelInput->getNLID(), static_cast<const void*>(modelInput)},
    {modelOutput->getBit(1)->getNLID(), static_cast<const void*>(modelOutput->getBit(1))},
    {instance->getNLID(), static_cast<const void*>(instance)},
    {instTerm->getNLID(), static_cast<const void*>(instTerm)},
  };
  std::set<NLID> ids;
  for (const auto& object: objects) {
    ASSERT_NE(nullptr, object.object_);
    EXPECT_TRUE(ids.insert(object.id_).second) << object.id_.getString();
    EXPECT_EQ(object.object_, static_cast<const void*>(universe->getObject(object.id_)));
  }

  EXPECT_NE(top->getNLID(), net->getNLID());
  EXPECT_NE(top->getNLID(), instance->getNLID());
  EXPECT_NE(net->getNLID(), instance->getNLID());
  EXPECT_EQ(nullptr, universe->getObject(NLID(NLID::Type::Instance, 1, 1, 1, 0, 999, 0)));
}

TEST_F(NLUniverseTest, testUniverseClashError) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  NLUniverse::create();
  ASSERT_NE(nullptr, NLUniverse::get());
  EXPECT_THROW(NLUniverse::create(), NLException);
}

TEST_F(NLUniverseTest, testEmptyUniverse) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  EXPECT_EQ(nullptr, NLUniverse::getTopDB());
  EXPECT_EQ(nullptr, NLUniverse::getTopDesign());
}

TEST_F(NLUniverseTest, testNameStringAndTechnology) {
  ASSERT_EQ(nullptr, NLUniverse::get());
  NLUniverse::create();
  auto universe = NLUniverse::get();
  ASSERT_NE(nullptr, universe);

  NLName name("foo");
  EXPECT_EQ("foo", name.getString());
}

TEST_F(NLUniverseTest, testTechnologyCreation) {
  auto universe = NLUniverse::create();
  EXPECT_EQ(nullptr, universe->getTechnology());
  auto tech = PNLTechnology::create(universe);
  EXPECT_EQ(tech, universe->getTechnology());
}
