#include "SNLDesign.h"

#include "Card.h"

#include "SNLDB.h" 
#include "SNLLibrary.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"

namespace SNL {

SNLDesign* SNLDesign::create(SNLLibrary* library, const SNLName& name) {
  preCreate(library, name);
  SNLDesign* design = new SNLDesign(library, name);
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

void SNLDesign::preCreate(const SNLLibrary* library, const SNLName& name) {
  super::preCreate();
  //test if design with same name exists in library
  if (not name.empty()) {
  }
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

  struct destroyNetFromDesign {
    void operator()(SNLNet* net) {
      net->destroyFromDesign();
    }
  };
  nets_.clear_and_dispose(destroyNetFromDesign());

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

SNLTerm* SNLDesign::getTerm(const SNLName& name) {
  auto tit = termNameIDMap_.find(name);
  if (tit != termNameIDMap_.end()) {
    SNLID::DesignObjectID id = tit->second;
    auto it = terms_.find(
        SNLID(SNLID::Type::Term, getDB()->getID(), getLibrary()->getID(), getID(), id, 0, 0),
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
        SNLID(SNLID::Type::Instance, getDB()->getID(), getLibrary()->getID(), getID(), 0, id, 0),
        SNLIDComp<SNLInstance>());
    if (it != instances_.end()) {
      return &*it;
    }
  }
  return nullptr;
}

void SNLDesign::addNet(SNLNet* net) {
  assert(dynamic_cast<SNLScalarNet*>(net) or dynamic_cast<SNLBusNet*>(net));

  if (nets_.empty()) {
    net->setID(0);
  } else {
    auto it = nets_.rbegin();
    SNLNet* lastNet = &(*it);
    SNLID::DesignObjectID netID = lastNet->getID()+1;
    net->setID(netID);
  }
  nets_.insert(*net);
  if (not net->getName().empty()) {
    netNameIDMap_[net->getName()] = net->getID();
  }
}

void SNLDesign::removeNet(SNLNet* net) {
  assert(dynamic_cast<SNLScalarNet*>(net) or dynamic_cast<SNLBusNet*>(net));

  if (not net->getName().empty()) {
    netNameIDMap_.erase(net->getName());
  }
  nets_.erase(*net);
}

SNLNet* SNLDesign::getNet(const SNLName& name) {
  auto tit = netNameIDMap_.find(name);
  if (tit != netNameIDMap_.end()) {
    SNLID::DesignObjectID id = tit->second;
    auto it = nets_.find(
        SNLID(SNLID::Type::Net, getDB()->getID(), getLibrary()->getID(), getID(), id, 0, 0),
        SNLIDComp<SNLNet>());
    if (it != nets_.end()) {
      return &*it;
    }
  }
  return nullptr;
}

SNLScalarNet* SNLDesign::getScalarNet(const SNLName& name) {
  return dynamic_cast<SNLScalarNet*>(getNet(name));
}

SNLBusNet* SNLDesign::getBusNet(const SNLName& name) {
  return dynamic_cast<SNLBusNet*>(getNet(name));
}

SNLDB* SNLDesign::getDB() const {
  return getLibrary()->getDB();
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
