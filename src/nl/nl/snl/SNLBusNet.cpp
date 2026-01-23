// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusNet.h"

#include <algorithm>
#include <limits>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLBusNetBit.h"
#include "SNLAttributes.h"
#include "SNLUtils.h"
#include "SNLMacros.h"

namespace naja::NL {

SNLBusNet::SNLBusNet(
    SNLDesign* design,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name):
  super(),
  design_(design),
  name_(name),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusNet::SNLBusNet(
    SNLDesign* design,
    NLID::DesignObjectID id,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusNet* SNLBusNet::create(
    SNLDesign* design,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name) {
  preCreate(design, name);
  SNLBusNet* net = new SNLBusNet(design, msb, lsb, name);
  net->postCreateAndSetID();
  return net;
}

SNLBusNet* SNLBusNet::create(
    SNLDesign* design,
    NLID::DesignID id,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name) {
  preCreate(design, id, name);
  SNLBusNet* net = new SNLBusNet(design, id, msb, lsb, name);
  net->postCreate();
  return net;
}

void SNLBusNet::preCreate(const SNLDesign* design, const NLName& name) {
  super::preCreate();
  if (not design) {
    throw NLException("malformed SNLBusNet creator with NULL design argument");
  }
  if (not name.empty() and design->getNet(name)) {
    std::string reason = "cannot create SNLBusNet with name " + name.getString();
    reason += "A terminal with this name already exists.";
    throw NLException(reason);
  }
}

void SNLBusNet::preCreate(const SNLDesign* design, NLID::DesignObjectID id, const NLName& name) {
  preCreate(design, name);
  if (design->getNet(id)) {
    std::string reason = "cannot create SNLBusNet with id " + std::to_string(id);
    reason += "A terminal with same id already exists.";
    throw NLException(reason);
  }
}

void SNLBusNet::createBits() {
  //create bits
  size_t size = static_cast<size_t>(getWidth());
  bits_.resize(size, nullptr);
  for (size_t i=0; i<size; i++) {
    NLID::Bit bit = (getMSB()>getLSB())?getMSB()-int(i):getMSB()+int(i);
    bits_[i] = SNLBusNetBit::create(this, bit);
  }
}

void SNLBusNet::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addNetAndSetID(this);
  createBits();
}

void SNLBusNet::postCreate() {
  super::postCreate();
  getDesign()->addNet(this);
  createBits();
}

void SNLBusNet::commonPreDestroy() {
  for (auto bit: bits_) {
    if (bit) {
      bit->destroyFromBus();
    }
  }
  super::preDestroy();
}

void SNLBusNet::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLBusNet::preDestroy() {
  commonPreDestroy();
  getDesign()->removeNet(this);
}

void SNLBusNet::removeBit(SNLBusNetBit* bit) {
  auto pos = getBitPosition(bit->getBit());
  if (pos < bits_.size()) {
    bits_[pos] = nullptr;
  }
}

SNLNet* SNLBusNet::clone(SNLDesign* design) const {
  auto newBus = new SNLBusNet(design, id_, msb_, lsb_, name_);
  SNLAttributes::cloneAttributes(this, newBus);
  newBus->createBits();
  for (size_t i=0; i<bits_.size(); i++) {
    newBus->bits_[i]->setType(bits_[i]->getType());
    bits_[i]->cloneComponents(newBus->bits_[i]);
  }
  return newBus;
}

NLID::Bit SNLBusNet::getWidth() const {
  return SNLUtils::getWidth(getMSB(), getLSB());
}

NLID SNLBusNet::getNLID() const {
  return SNLDesignObject::getNLID(NLID::Type::Net, id_, 0, 0);
}

SNLBusNetBit* SNLBusNet::getBit(NLID::Bit bit) const {
  size_t position = getBitPosition(bit);
  if (position < bits_.size()) {
    return bits_[position];
  }
  return nullptr;
}

size_t SNLBusNet::getBitPosition(NLID::Bit bit) const {
  if (SNLDesign::isBetween(bit, getMSB(), getLSB())) {
    size_t pos = static_cast<size_t>(std::abs(getMSB()-bit));
    return pos;
  }
  return std::numeric_limits<size_t>::max();
}

SNLBusNetBit* SNLBusNet::getBitAtPosition(size_t position) const {
  if (position < bits_.size()) {
    return bits_[position];
  }
  return nullptr;
}

NajaCollection<SNLBusNetBit*> SNLBusNet::getBusBits() const {
  auto filter = [](const SNLBusNetBit* bit) {return bit != nullptr; };
  return NajaCollection(new NajaSTLCollection(&bits_)).getSubCollection(filter);
}

NajaCollection<SNLBitNet*> SNLBusNet::getBits() const {
  return getBusBits().getParentTypeCollection<SNLBitNet*>();
}

void SNLBusNet::insertBits(
  std::vector<SNLBitNet*>& bitNets,
  std::vector<SNLBitNet*>::const_iterator position,
  NLID::Bit msb,
  NLID::Bit lsb) {
  if (not SNLDesign::isBetween(msb, getMSB(), getLSB())) {
    //FIXME
  }
  if (not SNLDesign::isBetween(lsb, getMSB(), getLSB())) {
    //FIXME
  }
  int msbPos = std::abs(getMSB()-msb);
  int lsbPos = std::abs(getMSB()-lsb);

  bitNets.insert(position, bits_.begin()+msbPos, bits_.begin()+lsbPos+1); 
}

void SNLBusNet::setType(const Type& type) {
  std::for_each(bits_.begin(), bits_.end(), [type](SNLBusNetBit* b){ if (b) b->setType(type); });
}

DESIGN_OBJECT_SET_NAME(SNLBusNet, Net, net)

bool SNLBusNet::isAllNull() const {
  return std::all_of(bits_.begin(), bits_.end(), [](const SNLBusNetBit* b){ return b == nullptr; });
}

bool SNLBusNet::isAssign0() const {
  return not isAllNull()
    and std::all_of(bits_.begin(), bits_.end(), [](const SNLBusNetBit* b){ return b == nullptr or b->getType().isAssign0(); });
}

bool SNLBusNet::isAssign1() const {
  return not isAllNull()
    and std::all_of(bits_.begin(), bits_.end(), [](const SNLBusNetBit* b){ return b == nullptr or b->getType().isAssign1(); });
}

bool SNLBusNet::isSupply0() const {
  return not isAllNull()
    and std::all_of(bits_.begin(), bits_.end(), [](const SNLBusNetBit* b){ return b == nullptr or b->getType().isSupply0(); });
}

bool SNLBusNet::isSupply1() const {
  return not isAllNull()
    and std::all_of(bits_.begin(), bits_.end(), [](const SNLBusNetBit* b){ return b == nullptr or b->getType().isSupply1(); });
}

//LCOV_EXCL_START
const char* SNLBusNet::getTypeName() const {
  return "SNLBusNet";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusNet::getString() const {
  return getName().getString() + "[" + std::to_string(getMSB()) + ":" + std::to_string(getLSB()) + "]";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusNet::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_.getString() + " " + design_->getName().getString() + ">";  
}
//LCOV_EXCL_STOP

bool SNLBusNet::deepCompare(const SNLNet* other, std::string& reason) const {
  const SNLBusNet* otherBusNet = dynamic_cast<const SNLBusNet*>(other);
  if (not otherBusNet) {
    //LCOV_EXCL_START
    reason = "other term is not a SNLBusNet";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getMSB() not_eq otherBusNet->getMSB()) {
    //LCOV_EXCL_START
    reason = "MSB mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getLSB() not_eq otherBusNet->getLSB()) {
    //LCOV_EXCL_START
    reason = "LSB mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getName() not_eq otherBusNet->getName()) {
    //LCOV_EXCL_START
    reason = "name mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (bits_.size() not_eq otherBusNet->bits_.size()) {
    //LCOV_EXCL_START
    reason = "size mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  for (size_t i=0; i<bits_.size(); i++) {
    if (not bits_[i] and not otherBusNet->bits_[i]) {
      continue;
    } else if (not bits_[i] or not otherBusNet->bits_[i]) {
      //LCOV_EXCL_START
      reason = "bit mismatch";
      return false;
      //LCOV_EXCL_STOP
    }
    if (not bits_[i]->deepCompare(otherBusNet->bits_[i], reason)) {
      return false; //LCOV_EXCL_LINE
    }
  }
  return SNLAttributes::compareAttributes(this, other, reason);
}

//LCOV_EXCL_START
void SNLBusNet::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    if (not getBits().empty()) {
      stream << std::string(indent+2, ' ') << "<bits>" << std::endl;
      for (auto bit: getBits()) {
        bit->debugDump(indent+4, recursive, stream);
      }
      stream << std::string(indent+2, ' ') << "</bits>" << std::endl;
    }
  }
}
//LCOV_EXCL_STOP

}  // namespace naja::NL