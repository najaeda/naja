#include "SNLInstance.h"

#include "Card.h"

#include "SNLDesign.h"

namespace SNL {

SNLInstance::SNLInstance(SNLDesign* design, SNLDesign* model, const SNLName& name):
  super(),
  design_(design),
  model_(model),
  name_(name)
{}

SNLInstance* SNLInstance::create(SNLDesign* design, SNLDesign* model, const SNLName& name) {
  preCreate(design, name);
  SNLInstance* instance = new SNLInstance(design, model, name);
  instance->postCreate();
  return instance;
}

void SNLInstance::preCreate(SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLInstance::postCreate() {
  super::postCreate();
  getDesign()->addInstance(this);
  //create instance terminals
  //FIXME
}

void SNLInstance::commonPreDestroy() {
  super::preDestroy();
}

void SNLInstance::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLInstance::preDestroy() {
  commonPreDestroy();
  getDesign()->removeInstance(this);
}

SNLID SNLInstance::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Instance, 0, id_, 0);
}

constexpr const char* SNLInstance::getTypeName() const {
  return "SNLInstance";
}

std::string SNLInstance::getString() const {
  return std::string();
}

std::string SNLInstance::getDescription() const {
  return "<" + std::string(getTypeName())
    + " " + name_
    + " " + design_->getName()
    + " " + model_->getName()
    + ">";  
}

Card* SNLInstance::getCard() const {
  Card* card = super::getCard();
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLDesign*>("Design", design_));
  card->addItem(new CardDataItem<const SNLDesign*>("Model", model_));
  return card;
}

}
