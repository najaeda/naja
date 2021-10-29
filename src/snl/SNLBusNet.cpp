#include "SNLBusNet.h"

#include "Card.h"

#include "SNLLibrary.h"
#include "SNLDesign.h"

namespace SNL {

SNLBusNet::SNLBusNet(SNLDesign* design, const SNLName& name):
  super(),
  name_(name),
  design_(design)
{}

SNLBusNet* SNLBusNet::create(SNLDesign* design, const SNLName& name) {
  preCreate(design, name);
  SNLBusNet* net = new SNLBusNet(design, name);
  net->postCreate();
  return net;
}

void SNLBusNet::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLBusNet::postCreate() {
  super::postCreate();
  getDesign()->addBusNet(this);
}

void SNLBusNet::commonPreDestroy() {
  super::preDestroy();
}

void SNLBusNet::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLBusNet::preDestroy() {
  commonPreDestroy();
  getDesign()->removeBusNet(this);
}

SNLID SNLBusNet::getSNLID() const {
  return SNLID(
      SNLID::Type::Net,
      getDesign()->getLibrary()->getID(),
      getDesign()->getID(),
      id_);
}

constexpr const char* SNLBusNet::getTypeName() const {
  return "SNLBusNet";
}

std::string SNLBusNet::getString() const {
  return std::string();
}

std::string SNLBusNet::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_ + " " + design_->getName() + ">";  
}

Card* SNLBusNet::getCard() const {
  Card* card = super::getCard();
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLDesign*>("Design", design_));
  return card;
}

}
