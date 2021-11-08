#include "SNLScalarNet.h"

#include "Card.h"

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"

namespace SNL {

SNLScalarNet::SNLScalarNet(SNLDesign* design, const SNLName& name):
  super(),
  name_(name),
  design_(design)
{}

SNLScalarNet* SNLScalarNet::create(SNLDesign* design, const SNLName& name) {
  preCreate(design, name);
  SNLScalarNet* net = new SNLScalarNet(design, name);
  net->postCreate();
  return net;
}

void SNLScalarNet::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLScalarNet::postCreate() {
  super::postCreate();
  getDesign()->addNet(this);
}

void SNLScalarNet::commonPreDestroy() {
  super::preDestroy();
}

void SNLScalarNet::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLScalarNet::preDestroy() {
  commonPreDestroy();
  getDesign()->removeNet(this);
}

SNLID SNLScalarNet::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Net, id_, 0, 0);
}

constexpr const char* SNLScalarNet::getTypeName() const {
  return "SNLScalarNet";
}

std::string SNLScalarNet::getString() const {
  return std::string();
}

std::string SNLScalarNet::getDescription() const {
  return "<" + std::string(typeid(this).name()) + " " + name_ + " " + design_->getName() + ">";  
}

Card* SNLScalarNet::getCard() const {
  Card* card = super::getCard();
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLDesign*>("Design", design_));
  return card;
}

}
