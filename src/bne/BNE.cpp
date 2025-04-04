// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "BNE.h"

using namespace naja::BNE;

void BNE::addDeleteAction(
    const std::vector<NLID::DesignObjectID>& pathToDelete) {
  DeleteAction* deleteAction = new DeleteAction(pathToDelete);
  tree_.addAction(deleteAction);
}
void BNE::addDriveWithConstantAction(
    const std::vector<NLID::DesignObjectID>& context,
    const NLID::DesignObjectID& pathToDrive,
    const NLID::DesignObjectID& termToDrive,
    const double& value,
    SNLBitTerm* topTermToDrive) {
  DriveWithConstantAction* driveWithConstantAction =
      new DriveWithConstantAction(context, pathToDrive, termToDrive, value,
                                  topTermToDrive);
  tree_.addAction(driveWithConstantAction);
}
void BNE::addReductionCommand(
    const std::vector<NLID::DesignObjectID>& context,
    NLID::DesignObjectID instance,
    const std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>& result) {
  ReductionAction* reductionAction =
      new ReductionAction(context, instance, result);
  tree_.addAction(reductionAction);
}

void BNE::process() {
    tree_.process();    
}
