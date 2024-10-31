// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ActionTree.h"

namespace naja::BNE {

class BNE {
 public:
 /**
  * \brief Create a BNE.
  * \param blockNormalization True if the normalization is blocked.
  * \param blockOptimization True if the optimization is blocked.
  */
  BNE(bool blockNormalization = false, bool blockOptimization = false)
      : tree_(blockNormalization, blockOptimization) {}
  /**
   * \brief Add a delete action to the BNE.
   * \param pathToDelete The path to delete.
   */
  void addDeleteAction(const std::vector<SNLID::DesignObjectID>& pathToDelete);
  /**
   * \brief Add a delete action to the BNE.
   * \param pathToDelete The path to delete.
   */
  void addDriveWithConstantAction(
      const std::vector<SNLID::DesignObjectID>& context,
      const SNLID::DesignObjectID& pathToDrive,
      const SNLID::DesignObjectID& termToDrive,
      const double& value,
      SNLBitTerm* topTermToDrive = nullptr);
  /**
   * \brief Add a delete action to the BNE.
   * \param pathToDelete The path to delete.
   */
  void addReductionCommand(
      const std::vector<SNLID::DesignObjectID>& context,
      SNLID::DesignObjectID instance,
      const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result);
  /**
   * \brief Add a delete action to the BNE.
   * \param pathToDelete The path to delete.
   */
  void process();
 private:
  ActionTree tree_;
};

}  // namespace naja::BNE