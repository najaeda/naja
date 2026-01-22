// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLScalarNet.h"

#include <sstream>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"

#include "PNLDesign.h"
//#include "PNLAttributes.h"
#include "SNLMacros.h"

namespace naja::NL {

PNLScalarNet::PNLScalarNet(PNLDesign* design, const NLName& name):
  super(),
  design_(design),
  name_(name)
{}

PNLScalarNet::PNLScalarNet(PNLDesign* design, NLID::DesignObjectID id, const NLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name)
{}

PNLScalarNet* PNLScalarNet::create(PNLDesign* design, const NLName& name) {
  preCreate(design, name);
  PNLScalarNet* net = new PNLScalarNet(design, name);
  net->postCreateAndSetID();
  return net;
}

PNLScalarNet* PNLScalarNet::create(PNLDesign* design, NLID::DesignObjectID id, const NLName& name) {
  preCreate(design, id, name);
  PNLScalarNet* net = new PNLScalarNet(design, id, name);
  net->postCreate();
  return net;
}

void PNLScalarNet::preCreate(const PNLDesign* design, const NLName& name) {
  super::preCreate();
  if (not design) {
    throw NLException("malformed PNLScalarNet creator with NULL design argument");
  }
  if (not name.empty() and design->getNet(name)) {
    std::string reason = "PNLDesign " + design->getString() + " contains already a PNLScalarNet named: " + name.getString();
    throw NLException(reason);
  }
}

void PNLScalarNet::preCreate(const PNLDesign* design, NLID::DesignObjectID id, const NLName& name) {
  preCreate(design, name);
  if (auto conflict = design->getNet(id)) {
    std::ostringstream reason;
    reason << "In PNLDesign " << design->getString();
    reason << ", error while trying to create";
    if (name.empty()) {
      reason << " anonymous ScalarNet";
    } else {
      reason << " " << name.getString() << " ScalarNet";
    }
    reason << ". This design contains already a PNLNet: " << conflict->getDescription() << " with conflicting ID.";
    throw NLException(reason.str());
  }
}

void PNLScalarNet::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addNetAndSetID(this);
}

void PNLScalarNet::postCreate() {
  super::postCreate();
  getDesign()->addNet(this);
}

DESIGN_OBJECT_SET_NAME(PNLScalarNet, Net, net)

void PNLScalarNet::commonPreDestroy() {
  super::preDestroy();
}

void PNLScalarNet::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void PNLScalarNet::preDestroy() {
  commonPreDestroy();
  getDesign()->removeNet(this);
}

// PNLNet* PNLScalarNet::clone(PNLDesign* design) const {
//   auto newNet = new PNLScalarNet(design, id_, name_);
//   newNet->setType(getType());
//   //PNLAttributes::cloneAttributes(this, newNet);
//   cloneComponents(newNet);
//   return newNet;
// }

NLID PNLScalarNet::getNLID() const {
  return PNLDesignObject::getNLID(NLID::Type::Net, id_, 0, 0);
}

NajaCollection<PNLBitNet*> PNLScalarNet::getBits() const {
  return NajaCollection(new NajaSingletonCollection(const_cast<PNLScalarNet*>(this))).getParentTypeCollection<PNLBitNet*>();
}

//LCOV_EXCL_START
const char* PNLScalarNet::getTypeName() const {
  return "PNLScalarNet";
}
//LCOV_EXCL_STOP


//LCOV_EXCL_START
std::string PNLScalarNet::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLScalarNet::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName());
  if (not isUnnamed()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (not getDesign()->isUnnamed()) {
    stream << " " + getDesign()->getName().getString();
  }
  stream << " " << getDesign()->getID();
  stream << ">";
  return stream.str(); 
}
//LCOV_EXCL_STOP

// bool PNLScalarNet::deepCompare(const PNLNet* other, std::string& reason) const {
//   const PNLScalarNet* otherScalarNet = dynamic_cast<const PNLScalarNet*>(other);
//   if (not otherScalarNet) {
//     //LCOV_EXCL_START
//     reason = "other term is not a PNLScalarNet";
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   if (getType() != otherScalarNet->getType()) {
//     //LCOV_EXCL_START
//     reason = "type mismatch";
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   if (getID() != otherScalarNet->getID()) {
//     //LCOV_EXCL_START
//     reason = "ID mismatch";
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   //return PNLAttributes::compareAttributes(this, otherScalarNet, reason);
//   return true;
// }

//LCOV_EXCL_START
void PNLScalarNet::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (not getComponents().empty()) {
    stream << std::string(indent+2, ' ') << "<components>" << std::endl;
    for (auto component: getComponents()) {
      component->debugDump(indent+4, false, stream);
    }
    stream << std::string(indent+2, ' ') << "</components>" << std::endl;
  }
}
//LCOV_EXCL_STOP

}  // namespace naja::NL