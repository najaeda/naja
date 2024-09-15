// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "bne.h"

using namespace naja::BNE;

void BNE::addDeleteAction(
    const std::vector<SNLID::DesignObjectID>& pathToDelete) {
  DeleteAction* deleteAction = new DeleteAction(pathToDelete);
  tree_.addAction(deleteAction);
}
void BNE::addDriveWithConstantAction(
    const std::vector<SNLID::DesignObjectID>& context,
    const SNLID::DesignObjectID& pathToDrive,
    const SNLID::DesignObjectID& termToDrive,
    const double& value,
    SNLBitTerm* topTermToDrive) {
  DriveWithConstantAction* driveWithConstantAction =
      new DriveWithConstantAction(context, pathToDrive, termToDrive, value,
                                  topTermToDrive);
  tree_.addAction(driveWithConstantAction);
}
void BNE::addReductionCommand(
    const std::vector<SNLID::DesignObjectID>& context,
    SNLID::DesignObjectID instance,
    const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result) {
  ReductionAction* reductionAction =
      new ReductionAction(context, instance, result);
  tree_.addAction(reductionAction);
}

void BNE::process() {
    tree_.process();    
}