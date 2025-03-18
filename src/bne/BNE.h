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
  void addDeleteAction(const std::vector<NLID::DesignObjectID>& pathToDelete);
  /**
   * \brief Add a drive with constant action to the BNE.
   * \param pathToDrive The context.
   * \param termToDrive The term to drive.
   * \param value The value to drive.
   */
  void addDriveWithConstantAction(
      const std::vector<NLID::DesignObjectID>& context,
      const NLID::DesignObjectID& pathToDrive,
      const NLID::DesignObjectID& termToDrive,
      const double& value,
      SNLBitTerm* topTermToDrive = nullptr);
  /**
   * \brief Add a reduction action to the BNE.
   * \param context The context.
   * \param instance The instance that can be reduced.
   * \param result The interface reduction needed. 
   */
  void addReductionCommand(
      const std::vector<NLID::DesignObjectID>& context,
      NLID::DesignObjectID instance,
      const std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>& result);
  /**
   * \brief Process the commands.
   */
  void process();
 private:
  ActionTree tree_;
};

}  // namespace naja::BNE
