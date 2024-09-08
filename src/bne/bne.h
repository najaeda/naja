// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ActionTree.h"

class BNE {
 public:
  BNE(bool blockNormalization = false, bool blockOptimization = false)
      : tree_(blockNormalization, blockOptimization) {}
  void addDeleteAction(const std::vector<SNLID::DesignObjectID>& pathToDelete);
  void addDriveWithConstantAction(
      const std::vector<SNLID::DesignObjectID>& context,
      const SNLID::DesignObjectID& pathToDrive,
      const SNLID::DesignObjectID& termToDrive,
      const double& value,
      SNLBitTerm* topTermToDrive = nullptr);
  void addReductionCommand(
      const std::vector<SNLID::DesignObjectID>& context,
      SNLID::DesignObjectID instance,
      const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result);
  void process();
 private:
  ActionTree tree_;
};