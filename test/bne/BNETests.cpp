// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniquifier.h"

#include "ActionTree.h"
#include "actions.h"
#include "BNE.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::BNE;

class BNETests : public ::testing::Test {
 protected:
  BNETests() {
    // You can do set-up work for each test here
  }
  ~BNETests() override {
    // You can do clean-up work that doesn't throw exceptions here
  }
  void SetUp() override {
    // top
      //  |-> h0
      //       |-> h1
      //       |    |-> h2
      //       |         |-> prim
      //       |-> h3
      //            |-> h2
      //                 |-> prim
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto primitivesLib = NLLibrary::create(db, NLLibrary::Type::Primitives);
      auto designsLib = NLLibrary::create(db);
      auto prim = SNLDesign::create(primitivesLib, SNLDesign::Type::Primitive, NLName("PRIM"));
      top_ = SNLDesign::create(designsLib, NLName("TOP"));
      auto h0 = SNLDesign::create(designsLib, NLName("H0"));
      auto h1 = SNLDesign::create(designsLib, NLName("H1"));
      auto h2 = SNLDesign::create(designsLib, NLName("H2"));
      auto h3 = SNLDesign::create(designsLib, NLName("H3"));
      primInstance_ = SNLInstance::create(h2, prim, NLName("prim"));
      h1h2Instance_ = SNLInstance::create(h1, h2, NLName("h2"));
      h1Instance_ = SNLInstance::create(h0, h1, NLName("h1"));
      h3Instance_ = SNLInstance::create(h0, h3, NLName("h3"));
      h3h2Instance_ = SNLInstance::create(h3, h2, NLName("h2")); 
      h0Instance_ = SNLInstance::create(top_, h0, NLName("h0"));
  }
  void TearDown() override {
    NLUniverse::get()->destroy();
  }
  SNLInstance* primInstance_  {nullptr};
  SNLInstance* h1h2Instance_  {nullptr};
  SNLInstance* h1Instance_    {nullptr};
  SNLInstance* h3Instance_    {nullptr};
  SNLInstance* h3h2Instance_  {nullptr};
  SNLInstance* h0Instance_    {nullptr};
  SNLDesign*   top_           {nullptr};
};

TEST_F(BNETests, ActionComparators) {
  // Create artificial DriveWithConstantActions and compare them
  /*const std::vector<NLID::DesignObjectID>& context,
                        const NLID::DesignObjectID& pathToDrive,
                        const NLID::DesignObjectID& termToDrive,
                        const double& value,
                        SNLBitTerm* topTermToDrive = nullptr
  bool operator<(const Action& action) const override {
  if (action.getType() != ActionType::DRIVE_WITH_CONSTANT) {
    return getType() < action.getType();
  }

  const DriveWithConstantAction& driveWithConstantAction =
      dynamic_cast<const DriveWithConstantAction&>(action);
  if (topTermToDrive_ != nullptr) {
    if (topTermToDrive_ < driveWithConstantAction.topTermToDrive_) {
      return true;
    } else if (topTermToDrive_ == driveWithConstantAction.topTermToDrive_) {
      if (value_ < driveWithConstantAction.value_) {
        return true;
      }
    }
    return false;
  }
  if (pathToDrive_ < driveWithConstantAction.pathToDrive_) {
    return true;
  } else if (pathToDrive_ == driveWithConstantAction.pathToDrive_) {
    if (termToDrive_ < driveWithConstantAction.termToDrive_) {
      return true;
    } else if (termToDrive_ == driveWithConstantAction.termToDrive_) {
      if (value_ < driveWithConstantAction.value_) {
        return true;
      }
    }
  }
  return false;
}*/
  std::vector<NLID::DesignObjectID> context;
  {
    DriveWithConstantAction action1(context, 0, 0, 0);
    DriveWithConstantAction action2(context, 0, 0, 0);
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr == *action2ptr;
    EXPECT_EQ(compare, true);
  }
  {
    DriveWithConstantAction action1(context, 0, 0, 0);
    DriveWithConstantAction action2(context, 1, 0, 0);
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr < *action2ptr;
    EXPECT_EQ(compare, true);
  }
  {
    DriveWithConstantAction action1(context, 0, 0, 0);
    DriveWithConstantAction action2(context, 0, 1, 0);
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr < *action2ptr;
    EXPECT_EQ(compare, true);
  }
  {
    DriveWithConstantAction action1(context, 0, 0, 0);
    DriveWithConstantAction action2(context, 0, 0, 1);
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr < *action2ptr;
    EXPECT_EQ(compare, true);
  }
  {
    ReductionAction action1(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    ReductionAction action2(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr == *action2ptr;
    EXPECT_EQ(compare, true);
    compare = *action2ptr < *action1ptr;
    EXPECT_EQ(compare, false);
  }
  {
    ReductionAction action1(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    ReductionAction action2(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr == *action2ptr;
    EXPECT_EQ(compare, true);
    compare = *action2ptr < *action1ptr;
    EXPECT_EQ(compare, false);
  }
  {
    ReductionAction action1(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    NLLibraryTruthTables::Indexes indexes;
    indexes.push_back(0);
    ReductionAction action2(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, indexes));
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action2ptr < *action1ptr;
    EXPECT_EQ(compare, false);
  }
  {
    ReductionAction action1(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    ReductionAction action2(
        context, 1,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr < *action2ptr;
    EXPECT_EQ(compare, true);
  }
  {
    auto pathToDelete = std::vector<NLID::DesignObjectID>();
    pathToDelete.push_back(0);
    DeleteAction action1(pathToDelete);
    DriveWithConstantAction action2(context, 0, 0, 0);
    ReductionAction action3(
        context, 0,
        std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>(
            nullptr, NLLibraryTruthTables::Indexes()));
    Action* action1ptr = &action1;
    Action* action2ptr = &action2;
    Action* action3ptr = &action3;
    // Test equality comparison
    bool compare = *action1ptr == *action2ptr;
    EXPECT_EQ(compare, false);
    compare = *action2ptr == *action1ptr;
    EXPECT_EQ(compare, false);
    compare = *action1ptr == *action3ptr;
    EXPECT_EQ(compare, false);
    compare = *action3ptr == *action1ptr;
    EXPECT_EQ(compare, false);
    compare = *action2ptr == *action3ptr;
    EXPECT_EQ(compare, false);
    compare = *action3ptr == *action2ptr;
    EXPECT_EQ(compare, false);
  }
}

TEST_F(BNETests, testCompare) {
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h3", "h2", "prim"};

  auto path0 = SNLPath(h0Instance_->getDesign(), pathDescriptor0);
  auto path1 = SNLPath(h0Instance_->getDesign(), pathDescriptor1);

  EXPECT_FALSE(path0.empty());
  EXPECT_FALSE(path1.empty());
  EXPECT_EQ(path0.getDesign(), path1.getDesign());
  EXPECT_EQ(path0.getModel(), path1.getModel());
  EXPECT_NE(path0, path1);
  EXPECT_LT(path0, path1);

  SNLUniquifier uniquifier0(path0);
  SNLUniquifier uniquifier1(path1);

  //Test Uniquifier comparators
  EXPECT_EQ(uniquifier0 == uniquifier0, true);
  EXPECT_EQ(uniquifier0 != uniquifier0, false);
  EXPECT_EQ(uniquifier0 < uniquifier0, false);
  EXPECT_EQ(uniquifier0 > uniquifier0, false);
  EXPECT_EQ(uniquifier0 <= uniquifier0, true);
  EXPECT_EQ(uniquifier0 >= uniquifier0, true);
  EXPECT_EQ(uniquifier0 == uniquifier1, false);
  EXPECT_EQ(uniquifier0 != uniquifier1, true);
  EXPECT_EQ(uniquifier0 < uniquifier1, true);
  EXPECT_EQ(uniquifier0 > uniquifier1, false);
  EXPECT_EQ(uniquifier0 <= uniquifier1, true);
  EXPECT_EQ(uniquifier0 >= uniquifier1, false);

  EXPECT_NE(uniquifier0.getString(), uniquifier1.getString());
}

TEST_F(BNETests, normalizeNodeDeletion) {
  NLUniverse::get()->setTopDesign(top_);
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h3", "h2", "prim"};

  auto path0 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor0);
  auto path1 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor1);

  auto path0IDs = path0.getIDDescriptor();
  auto path1IDs = path1.getIDDescriptor();

  BNE bne;
  bne.addDeleteAction(path0IDs);
  bne.addDeleteAction(path1IDs);
  path0IDs.pop_back();
  path1IDs.pop_back();
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel() == getInstanceForPath(path1IDs)->getModel(), true);
  bne.process();
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel() == getInstanceForPath(path1IDs)->getModel(), true);
}

TEST_F(BNETests, blockedNormalizeNodeDeletion) {
  NLUniverse::get()->setTopDesign(top_);
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h1", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h3", "h2", "prim"};

  auto path0 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor0);
  auto path1 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor1);

  auto path0IDs = path0.getIDDescriptor();
  auto path1IDs = path1.getIDDescriptor();
  const bool blockNormalization = true;
  BNE bne(blockNormalization);
  bne.addDeleteAction(path0IDs);
  bne.addDeleteAction(path1IDs);
  path0IDs.pop_back();
  path1IDs.pop_back();
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel() == getInstanceForPath(path1IDs)->getModel(), true);
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel()->getInstances().size(), 1);
  EXPECT_EQ(getInstanceForPath(path1IDs)->getModel()->getInstances().size(), 1);
  bne.process();
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel()->getInstances().size(), 0);
  EXPECT_EQ(getInstanceForPath(path1IDs)->getModel()->getInstances().size(), 0);
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel() == getInstanceForPath(path1IDs)->getModel(), false);
}

TEST_F(BNETests, blockedNormalizeNodeDeletionOrphanNodeRemoval) {
  NLUniverse::get()->setTopDesign(top_);
  auto h4 = SNLInstance::create(h0Instance_->getModel(), h3Instance_->getModel(), NLName("h4"));
  SNLInstance::create(h1h2Instance_->getModel(), primInstance_->getModel(), NLName("prim2"));
  SNLPath::PathStringDescriptor pathDescriptor0 = { "h0", "h4", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor1 = { "h0", "h3", "h2", "prim"};
  SNLPath::PathStringDescriptor pathDescriptor2 = { "h0", "h4", "h2"};
  SNLPath::PathStringDescriptor pathDescriptor3 = { "h0", "h3", "h2"};

  auto path0 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor0);
  auto path1 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor1);
  auto path2 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor2);
  auto path3 = SNLPath(NLUniverse::get()->getTopDesign(), pathDescriptor3);

  auto path0IDs = path0.getIDDescriptor();
  auto path1IDs = path1.getIDDescriptor();
  auto path2IDs = path2.getIDDescriptor();
  auto path3IDs = path3.getIDDescriptor();
  const bool blockNormalization = false;
  const bool keepOrder = true;
  BNE bne(blockNormalization, keepOrder);
  bne.addDeleteAction(path2IDs);
  bne.addDeleteAction(path3IDs);
  bne.addDeleteAction(path0IDs);
  bne.addDeleteAction(path1IDs);
  path0IDs.pop_back();
  path1IDs.pop_back();
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel() == getInstanceForPath(path1IDs)->getModel(), true);
  EXPECT_EQ(getInstanceForPath(path0IDs)->getModel()->getInstances().size(), 2);
  EXPECT_EQ(getInstanceForPath(path1IDs)->getModel()->getInstances().size(), 2);
  bne.process();
  //EXPECT_EQ(getInstanceForPath(path0IDs)->getModel()->getInstances().size(), 1);
  //EXPECT_EQ(getInstanceForPath(path1IDs)->getModel()->getInstances().size(), 1);
  //EXPECT_EQ(getInstanceForPath(path0IDs)->getModel() == getInstanceForPath(path1IDs)->getModel(), false);
}

TEST_F(BNETests, missingTop) {
  EXPECT_THROW(new BNE(), NLException);
}

