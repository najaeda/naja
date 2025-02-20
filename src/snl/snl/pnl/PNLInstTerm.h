// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesignObject.h"
#include "PNLTerm.h"
#include "SNLID.h"
#include "SNLName.h"
#include "PNLNet.h"
#include "PNLInstance.h"
#include "PNLDesign.h"

#pragma once

namespace naja {
namespace PNL {

class PNLDesign;
class PNLNet;
class PNLInstance;

class PNLInstTerm : public PNLDesignObject {
 public:
  using super = PNLDesignObject;

  static PNLInstTerm* create(PNLInstance* instance,
                              PNLTerm* term,
                              PNLNet* net,
                              PNLTerm::Direction direction,
                              naja::SNL::SNLID::DesignObjectID id);

  /// \return this PNLInstTerm Direction.
  PNLTerm::Direction getDirection();

  /// \return this PNLInstTerm SNLNet.
  PNLNet* getNet();

  /**
   * \brief Change this PNLInstTerm PNLNet.
   * \remark This PNLInstTerm and net must have the same size.
   * \remark If net is null, this PNLInstTerm will be disconnected.
   */
  void setNet(PNLNet* net);

  PNLInstance* getInstance() const;
  
  naja::SNL::SNLID::DesignObjectID getID() const;

  void destroyFromInstance();

  const char* getTypeName() const override { return "PNLInstTerm"; }
  std::string getString() const override { return "PNLInstTerm"; }
  std::string getDescription() const override { return "PNLInstTerm"; }
  void debugDump(size_t indent,
                 bool recursive = true,
                 std::ostream& stream = std::cerr) const override {}
  
  PNLDesign* getDesign() const override { return instance_->getDesign(); }

 protected:
  PNLInstTerm() = default;

  static void preCreate();
  void postCreate();
  void preDestroy() override;

 private:
  naja::SNL::SNLID::DesignObjectID id_;
  PNLTerm::Direction direction_ = PNLTerm::Direction::Input;
  PNLNet* net_ = nullptr;
  PNLInstance* instance_ = nullptr;
  PNLTerm* term_ = nullptr;
};

}  // namespace PNL
}  // namespace naja