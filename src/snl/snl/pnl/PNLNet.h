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

class PNLDesign;
class PNLTerm;
class PNLInstTerm;

class PNLNet : public PNLDesignObject {
 public:
  using super = PNLDesignObject;

  static PNLNet* create(PNLDesign* design,
                        const naja::SNL::SNLName& name,
                        naja::SNL::SNLID::DesignObjectID id);

  PNLDesign* getDesign() const override;

  naja::SNL::SNLID::DesignObjectID getID() const;

  void destroyFromDesign();

  void addTerm(PNLTerm* term);
  PNLTerm* getTerm(naja::SNL::SNLID::DesignObjectID id) const;
  void detachTerm(naja::SNL::SNLID::DesignObjectID id);

  void addInstTerm(PNLInstTerm* instTerm);
  PNLInstTerm* getInstTerm(naja::SNL::SNLID::DesignObjectID id) const;
  void detachInstTerm(naja::SNL::SNLID::DesignObjectID id);

  const char* getTypeName() const override { return "PNLNet"; }
  std::string getString() const override { return "PNLNet"; }
  std::string getDescription() const override { return "PNLNet"; }
  void debugDump(size_t indent,
                 bool recursive = true,
                 std::ostream& stream = std::cerr) const override {}

 protected:
  PNLNet() = default;

  static void preCreate();
  void postCreate();
  void preDestroy() override;

 private:
  naja::SNL::SNLName name_;
  naja::SNL::SNLID::DesignObjectID id_;
  PNLDesign* design_;
  std::vector<PNLTerm*> terms_;
  std::vector<PNLInstTerm*> instTerms_;
};

}  // namespace PNL
}  // namespace naja