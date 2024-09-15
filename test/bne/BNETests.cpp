// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "ActionTree.h"
#include "actions.h"
#include "bne.h"
#include "gtest/gtest.h"

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
    // Code here will be called immediately after the constructor (right
    // before each test).
  }
  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    // Destroy the SNL
  }
};

TEST_F(BNETests, ActionComperators) {
  // Create artificial DriveWithConstantActions and compare them
  /*const std::vector<SNLID::DesignObjectID>& context,
                        const SNLID::DesignObjectID& pathToDrive,
                        const SNLID::DesignObjectID& termToDrive,
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
  std::vector<SNLID::DesignObjectID> context;
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
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
    ReductionAction action2(
        context, 0,
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
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
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
    ReductionAction action2(
        context, 0,
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
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
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
    SNLLibraryTruthTables::Indexes indexes;
    indexes.push_back(0);
    ReductionAction action2(
        context, 0,
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, indexes));
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action2ptr < *action1ptr;
    EXPECT_EQ(compare, false);
  }
  {
    ReductionAction action1(
        context, 0,
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
    ReductionAction action2(
        context, 1,
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
    Action* action2ptr = &action2;
    Action* action1ptr = &action1;
    bool compare = *action1ptr < *action2ptr;
    EXPECT_EQ(compare, true);
  }
  {
    auto pathToDelete = std::vector<SNLID::DesignObjectID>();
    pathToDelete.push_back(0);
    DeleteAction action1(pathToDelete);
    DriveWithConstantAction action2(context, 0, 0, 0);
    ReductionAction action3(
        context, 0,
        std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>(
            nullptr, SNLLibraryTruthTables::Indexes()));
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