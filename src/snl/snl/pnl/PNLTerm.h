// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLDesignObject.h"
#include "SNLID.h"
#include "SNLName.h"

namespace naja {
namespace PNL {

class PNLNet;
class PNLInstance;

class PNLTerm : public PNLDesignObject {
 public:
  using super = PNLDesignObject;

  /**
   * \brief class describing Direction.
   */
  class Direction {
   public:
    enum DirectionEnum {
      Input,   ///< Input direction.
      Output,  ///< Output direction.
      InOut    ///< InOut direction.
    };
    Direction(const DirectionEnum& dirEnum);
    Direction(const Direction& direction) = default;
    Direction& operator=(const Direction& direction) = default;
    operator const DirectionEnum&() const { return dirEnum_; }
    std::string getString() const;

   private:
    DirectionEnum dirEnum_;
  };

  static PNLTerm* create(PNLDesign* design,
                         Direction direction,
                         naja::SNL::SNLID::DesignObjectID id,
                         const naja::SNL::SNLName& name);

  /// \return this PNLTerm Direction.
  Direction getDirection();

  /// \return this PNLTerm SNLNet.
  PNLNet* getNet();

  /**
   * \brief Change this PNLTerm PNLNet.
   * \remark This PNLTerm and net must have the same size.
   * \remark If net is null, this PNLTerm will be disconnected.
   */
  void setNet(PNLNet* net);

  PNLDesign* getDesign() const override;

  naja::SNL::SNLID::DesignObjectID getID() const;

  void setID(naja::SNL::SNLID::DesignObjectID id);

  void destroyFromDesign();

  const char* getTypeName() const override { return "PNLTerm"; }
  std::string getString() const override { return "PNLTerm"; }
  std::string getDescription() const override { return "PNLTerm"; }
  void debugDump(size_t indent,
                 bool recursive = true,
                 std::ostream& stream = std::cerr) const override {}

 protected:
  PNLTerm() = default;

  static void preCreate();
  void postCreate();
  void preDestroy() override;

 private:
  naja::SNL::SNLName name_;
  naja::SNL::SNLID::DesignObjectID id_ = 0;
  Direction direction_ = Direction::Input;
  PNLNet* net_;
  PNLDesign* design_;
  boost::intrusive::set_member_hook<> netComponentsHook_{};
};
}  // namespace PNL
}  // namespace naja