#include "SNLBusNet.h"

#include "Card.h"

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusNetBit.h"

namespace SNL {

SNLBusNet::SNLBusNet(
    SNLDesign* design,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name):
  super(),
  design_(design),
  name_(name),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusNet* SNLBusNet::create(
    SNLDesign* design,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name) {
  preCreate(design, name);
  SNLBusNet* net = new SNLBusNet(design, msb, lsb, name);
  net->postCreate();
  return net;
}

void SNLBusNet::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLBusNet::postCreate() {
  super::postCreate();
  getDesign()->addNet(this);
  //create bits
  bits_.resize(getSize(), nullptr);
  for (size_t i=0; i<getSize(); i++) {
    SNLID::Bit bit = (getMSB()>getLSB())?getMSB()-int(i):getMSB()+int(i);
    bits_[i] = SNLBusNetBit::create(this, bit);
  }
}

void SNLBusNet::commonPreDestroy() {
  for (auto bit: bits_) {
    bit->destroyFromBus();
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

size_t SNLBusNet::getSize() const {
  return static_cast<size_t>(std::abs(getLSB() - getMSB()) + 1);
}

SNLID SNLBusNet::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Net, id_, 0, 0);
}

constexpr const char* SNLBusNet::getTypeName() const {
  return "SNLBusNet";
}

std::string SNLBusNet::getString() const {
  return std::string();
}

std::string SNLBusNet::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_.getString() + " " + design_->getName().getString() + ">";  
}

Card* SNLBusNet::getCard() const {
  Card* card = super::getCard();
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLDesign*>("Design", design_));
  return card;
}

}
