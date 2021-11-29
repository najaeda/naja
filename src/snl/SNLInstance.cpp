#include "SNLInstance.h"

#include <sstream>

#include "Card.h"

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLInstTerm.h"

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
  if (not design) {
    throw SNLException("malformed SNLInstance creator with NULL design argument");
  }
  if (not name.empty() and design->getInstance(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLInstance named: " + name;
    throw SNLException(reason);
  }
}

void SNLInstance::postCreate() {
  super::postCreate();
  getDesign()->addInstance(this);
  if (not getModel()->isPrimitive()) {
    //Always execute addSlaveInstance after addInstance.
    //addInstance determines the instance ID.
    getModel()->addSlaveInstance(this);
  }
  //create instance terminals
  size_t nbTerms = getModel()->getTerms().size();
  instTerms_.reserve(nbTerms);
  SNLCollection<SNLTerm*> terms = getModel()->getTerms();
  for (SNLTerm* term: getModel()->getTerms()) {
    if (SNLBusTerm* busTerm = dynamic_cast<SNLBusTerm*>(term)) {
      //FIXME
    } else {
      auto scalarTerm = static_cast<SNLScalarTerm*>(term);
      instTerms_.push_back(SNLInstTerm::create(this, scalarTerm));
    }
  }
}

void SNLInstance::destroyFromModel() {
  getDesign()->removeInstance(this);
  delete this;
}

void SNLInstance::destroyFromDesign() {
  if (not getModel()->isPrimitive()) {
    getModel()->removeSlaveInstance(this);
  }
  delete this;
}

void SNLInstance::preDestroy() {
  if (not getModel()->isPrimitive()) {
    getModel()->removeSlaveInstance(this);
  }
  getDesign()->removeInstance(this);
}

SNLID SNLInstance::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Instance, 0, id_, 0);
}

constexpr const char* SNLInstance::getTypeName() const {
  return "SNLInstance";
}

std::string SNLInstance::getString() const {
  std::ostringstream str; 
  if (not isAnonymous()) {
    str << getName();
  }
  str << "(" << getID() << ")";
  return str.str();
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
