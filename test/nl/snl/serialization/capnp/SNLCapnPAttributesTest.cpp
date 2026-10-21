// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <filesystem>
#include <vector>

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "SNLDesign.h"
#include "SNLAttributes.h"
#include "SNLRTLInfos.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLCapnP.h"

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

using namespace naja::NL;

class SNLCapnPAttributesTest: public ::testing::TestWithParam<bool> {
  protected:
    void TearDown() override {
      if (NLUniverse::get()) NLUniverse::get()->destroy();
    }

    static std::vector<SNLAttribute> attributes() {
      return {
        SNLAttribute(NLName("tag"), SNLAttributeValue("23")),
        SNLAttribute(NLName("tag"), SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "23")),
        SNLAttribute(NLName("marker")),
        SNLAttribute(NLName("empty"), SNLAttributeValue("")),
        SNLAttribute(NLName("wide"), SNLAttributeValue(SNLAttributeValue::Type::NUMBER,
          "160'h123456789abcdef0123456789abcdef0123456789")),
        SNLAttribute(NLName("unknown"), SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "8'b10xz0011")),
        SNLAttribute(NLName("empty_number"), SNLAttributeValue(SNLAttributeValue::Type::NUMBER, "")),
      };
    }

    static std::vector<NLObject*> objects(SNLDesign* top) {
      auto instance = top->getInstance(NLName("u"));
      auto model = instance->getModel();
      auto modelBus = model->getBusTerm(NLName("b"));
      return {top, model, top->getScalarTerm(NLName("a")), top->getBusTerm(NLName("b")),
        top->getBusTerm(NLName("b"))->getBit(-2), instance,
        instance->getInstTerm(model->getScalarTerm(NLName("a"))),
        instance->getInstTerm(modelBus->getBit(-2)),
        top->getScalarNet(NLName("n")), top->getBusNet(NLName("bus")),
        top->getBusNet(NLName("bus"))->getBit(2)};
    }
};

TEST_P(SNLCapnPAttributesTest, roundTrip) {
  auto universe = NLUniverse::create();
  auto db = NLDB::create(universe);
  auto library = NLLibrary::create(db, NLName("designs"));
  auto model = SNLDesign::create(library, NLName("leaf"));
  SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("a"));
  SNLBusTerm::create(model, SNLTerm::Direction::Output, -1, -3, NLName("b"));
  auto top = SNLDesign::create(library, NLName("top"));
  universe->setTopDesign(top);
  SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("a"));
  SNLBusTerm::create(top, SNLTerm::Direction::Output, -1, -3, NLName("b"));
  SNLScalarNet::create(top, NLName("n"));
  SNLScalarNet::create(top, NLName("plain"));
  auto bus = SNLBusNet::create(top, 0, 3, NLName("bus"));
  bus->getBit(1)->destroy();
  auto instance = SNLInstance::create(top, model, NLName("u"));
  instance->getInstTerm(model->getScalarTerm(NLName("a")))->setNet(top->getScalarNet(NLName("n")));
  top->getScalarTerm(NLName("a"))->setNet(top->getScalarNet(NLName("n")));
  for (auto object: objects(top)) {
    if (auto design = dynamic_cast<SNLDesign*>(object)) {
      for (const auto& attribute: attributes()) SNLAttributes::addAttribute(design, attribute);
      if (GetParam()) SNLRTLInfos::create(design)->setInfo(NLName("source"), "synthetic");
    } else {
      auto designObject = dynamic_cast<SNLDesignObject*>(object);
      for (const auto& attribute: attributes()) SNLAttributes::addAttribute(designObject, attribute);
      if (GetParam()) SNLRTLInfos::create(designObject)->setInfo(NLName("source"), "synthetic");
    }
  }
  auto path = std::filesystem::path(SNL_CAPNP_TEST_PATH) /
    (GetParam() ? "attributes_with_metadata.snl" : "attributes_only.snl");
  std::filesystem::remove_all(path);
  SNLCapnP::dump(db, path);
  universe->destroy();
  auto loadedDB = SNLCapnP::load(path);
  auto loaded = loadedDB->getTopDesign();
  ASSERT_NE(nullptr, loaded);
  for (auto object: objects(loaded)) {
    auto actual = SNLAttributes::getAttributes(object);
    EXPECT_EQ(attributes(), std::vector<SNLAttribute>(actual.begin(), actual.end()));
    if (auto design = dynamic_cast<SNLDesign*>(object)) {
      EXPECT_EQ(GetParam(), design->hasRTLInfos());
    } else {
      EXPECT_EQ(GetParam(), dynamic_cast<SNLDesignObject*>(object)->hasRTLInfos());
    }
  }
  EXPECT_TRUE(SNLAttributes::getAttributes(loaded->getScalarNet(NLName("plain"))).empty());
  EXPECT_TRUE(SNLAttributes::getAttributes(loaded->getBusTerm(NLName("b"))->getBit(-1)).empty());
  EXPECT_EQ(nullptr, loaded->getBusNet(NLName("bus"))->getBit(1));
  EXPECT_EQ(loaded->getScalarNet(NLName("n")), loaded->getScalarTerm(NLName("a"))->getNet());
  auto loadedInstance = loaded->getInstance(NLName("u"));
  EXPECT_EQ(loaded->getScalarNet(NLName("n")),
    loadedInstance->getInstTerm(loadedInstance->getModel()->getScalarTerm(NLName("a")))->getNet());
}

INSTANTIATE_TEST_SUITE_P(Metadata, SNLCapnPAttributesTest, ::testing::Bool());
