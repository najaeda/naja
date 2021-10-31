#include "SNLDesign.h"

#include "Card.h"

#include "SNLDB.h" 
#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

namespace SNL {

SNLDesign* SNLDesign::create(SNLLibrary* library, const SNLName& name) {
  preCreate(library, name);
  SNLDesign* design = new SNLDesign(library, name);
  design->postCreate();
  return design;
}

SNLDesign* SNLDesign::create(SNLLibrary* library) {
  preCreate(library);
  SNLDesign* design = new SNLDesign(library);
  design->postCreate();
  return design;
}

SNLDesign::SNLDesign(SNLLibrary* library):
  super(),
  library_(library)
{}
  
  
SNLDesign::SNLDesign(SNLLibrary* library, const SNLName& name):
  super(),
  library_(library),
  name_(name)
{}

void SNLDesign::preCreate(const SNLLibrary* library) {
  super::preCreate();
}

void SNLDesign::preCreate(const SNLLibrary* library, const SNLName& name) {
  preCreate(library);
  //test if design with same name exists in library
}

void SNLDesign::postCreate() {
  super::postCreate();
  library_->addDesign(this);
}

void SNLDesign::commonPreDestroy() {
  struct destroyTermFromDesign {
    void operator()(SNLTerm* term) {
      term->destroyFromDesign();
    }
  };
  terms_.clear_and_dispose(destroyTermFromDesign());

  struct destroyInstanceFromDesign {
    void operator()(SNLInstance* instance) {
      instance->destroyFromDesign();
    }
  };
  instances_.clear_and_dispose(destroyInstanceFromDesign());

  struct destroyScalarNetFromDesign {
    void operator()(SNLScalarNet* net) {
      net->destroyFromDesign();
    }
  };
  scalarNets_.clear_and_dispose(destroyScalarNetFromDesign());

  struct destroyBusNetFromDesign {
    void operator()(SNLBusNet* net) {
      net->destroyFromDesign();
    }
  };
  busNets_.clear_and_dispose(destroyBusNetFromDesign());
  super::preDestroy();
}

void SNLDesign::destroyFromLibrary() {
  commonPreDestroy();
  delete this;
}

void SNLDesign::preDestroy() {
  library_->removeDesign(this);
  commonPreDestroy();
}

void SNLDesign::addTerm(SNLTerm* term) {
  assert(dynamic_cast<SNLScalarTerm*>(term) or dynamic_cast<SNLBusTerm*>(term));

  if (terms_.empty()) {
    term->setID(0);
  } else {
    auto it = terms_.rbegin();
    SNLTerm* lastTerm = &(*it);
    SNLID::DesignObjectID termID = lastTerm->getID()+1;
    term->setID(termID);
  }
  terms_.insert(*term);
  if (not term->getName().empty()) {
    termNameIDMap_[term->getName()] = term->getID();
  }
}

void SNLDesign::removeTerm(SNLTerm* term) {
  assert(dynamic_cast<SNLScalarTerm*>(term) or dynamic_cast<SNLBusTerm*>(term));

  if (not term->getName().empty()) {
    termNameIDMap_.erase(term->getName());
  }
  terms_.erase(*term);
}

SNLDB* SNLDesign::getDB() const {
  return getLibrary()->getDB();
}

SNLTerm* SNLDesign::getTerm(const SNLName& name) {
  auto tit = termNameIDMap_.find(name);
  if (tit != termNameIDMap_.end()) {
    SNLID::DesignObjectID id = tit->second;
    auto it = terms_.find(
        SNLID(SNLID::Type::Term, getDB()->getID(), getLibrary()->getID(), getID(), id),
        SNLIDComp<SNLTerm>());
    if (it != terms_.end()) {
      return &*it;
    }
  }
  return nullptr;
}

SNLScalarTerm* SNLDesign::getScalarTerm(const SNLName& name) {
  return dynamic_cast<SNLScalarTerm*>(getTerm(name));
}

SNLBusTerm* SNLDesign::getBusTerm(const SNLName& name) {
  return dynamic_cast<SNLBusTerm*>(getTerm(name));
}

void SNLDesign::addInstance(SNLInstance* instance) {
  if (instances_.empty()) {
    instance->id_ = 0;
  } else {
    auto it = instances_.rbegin();
    SNLInstance* lastInstance = &(*it);
    SNLID::InstanceID instanceID = lastInstance->id_+1;
    instance->id_ = instanceID;
  }
  instances_.insert(*instance);
  if (not instance->getName().empty()) {
    instanceNameIDMap_[instance->getName()] = instance->id_;
  }
}

void SNLDesign::removeInstance(SNLInstance* instance) {
  if (not instance->getName().empty()) {
    instanceNameIDMap_.erase(instance->getName());
  }
  instances_.erase(*instance);
}

SNLInstance* SNLDesign::getInstance(const SNLName& name) {
  auto iit = instanceNameIDMap_.find(name);
  if (iit != instanceNameIDMap_.end()) {
    SNLID::InstanceID id = iit->second;
    auto it = instances_.find(
        SNLID(SNLID::Type::Instance, getDB()->getID(), getLibrary()->getID(), getID(), id),
        SNLIDComp<SNLInstance>());
    if (it != instances_.end()) {
      return &*it;
    }
  }
  return nullptr;
}

void SNLDesign::addScalarNet(SNLScalarNet* scalarNet) {
  scalarNets_.insert(*scalarNet);
}

void SNLDesign::removeScalarNet(SNLScalarNet* scalarNet) {
  scalarNets_.erase(*scalarNet);
}

SNLScalarNet* SNLDesign::getScalarNet(const SNLName& name) {
  //auto it = scalarNets_.find(name, SNLNameComp<SNLScalarNet>());
  //if (it != scalarNets_.end()) {
  //  return &*it;
  //}
  return nullptr;
}

void SNLDesign::addBusNet(SNLBusNet* busNet) {
  busNets_.insert(*busNet);
}

void SNLDesign::removeBusNet(SNLBusNet* busNet) {
  busNets_.erase(*busNet);
}

SNLBusNet* SNLDesign::getBusNet(const SNLName& name) {
  //auto it = busNets_.find(name, SNLNameComp<SNLBusNet>());
  //if (it != busNets_.end()) {
  //  return &*it;
  //}
  return nullptr;
}

//SNLCollection<SNLInstance> SNLDesign::getInstances() {
//  return SNLCollection<SNLInstance>(new SNLIntrusiveSetCollection<SNLInstance, SNLDesignInstancesHook>(&instances_));
//}

SNLCollection<SNLInstance> SNLDesign::getInstances() const {
  return SNLCollection<SNLInstance>(new SNLIntrusiveConstSetCollection<SNLInstance, SNLDesignInstancesHook>(&instances_));
}

constexpr const char* SNLDesign::getTypeName() const {
  return "SNLDesign";
}

std::string SNLDesign::getString() const {
  return std::string();
}

std::string SNLDesign::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_ + " " + library_->getName() + ">";  
}

Card* SNLDesign::getCard() const {
  Card* card = super::getCard();
  //card->addItem(new CardDataItem<SNLID>("ID", id_));
  card->addItem(new CardDataItem<const SNLName>("Name", name_));
  card->addItem(new CardDataItem<const SNLLibrary*>("Library", library_));
  return card;
}

SNLID SNLDesign::getSNLID() const {
  return SNLID(getDB()->getID(), library_->getID(), getID());
}

}
