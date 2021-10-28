#include "SNLDesign.h"

#include "Card.h"

#include "SNLCommon.h"
#include "SNLLibrary.h"

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
  if (not name_.empty()) {
    //designNames_[name_] = id_; 
  }
}

void SNLDesign::commonPreDestroy() {
  struct destroyScalarTermFromDesign {
    void operator()(SNLScalarTerm* net) {
      net->destroyFromDesign();
    }
  };
  scalarTerms_.clear_and_dispose(destroyScalarTermFromDesign());

  struct destroyBusTermFromDesign {
    void operator()(SNLBusTerm* net) {
      net->destroyFromDesign();
    }
  };
  busTerms_.clear_and_dispose(destroyBusTermFromDesign());

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

void SNLDesign::addScalarTerm(SNLScalarTerm* scalarTerm) {
  scalarTerms_.insert(*scalarTerm);
}

void SNLDesign::removeScalarTerm(SNLScalarTerm* scalarTerm) {
  scalarTerms_.erase(*scalarTerm);
}

SNLScalarTerm* SNLDesign::getScalarTerm(const SNLName& name) {
  auto it = scalarTerms_.find(name, SNLNameComp<SNLScalarTerm>());
  if (it != scalarTerms_.end()) {
    return &*it;
  }
  return nullptr;
}

void SNLDesign::addBusTerm(SNLBusTerm* busTerm) {
  busTerms_.insert(*busTerm);
}

void SNLDesign::removeBusTerm(SNLBusTerm* busTerm) {
  busTerms_.erase(*busTerm);
}

SNLBusTerm* SNLDesign::getBusTerm(const SNLName& name) {
  auto it = busTerms_.find(name, SNLNameComp<SNLBusTerm>());
  if (it != busTerms_.end()) {
    return &*it;
  }
  return nullptr;
}

void SNLDesign::addInstance(SNLInstance* instance) {
  instances_.insert(*instance);
}

void SNLDesign::removeInstance(SNLInstance* instance) {
  instances_.erase(*instance);
}

SNLInstance* SNLDesign::getInstance(const SNLName& name) {
  auto it = instances_.find(name, SNLNameComp<SNLInstance>());
  if (it != instances_.end()) {
    return &*it;
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
  auto it = scalarNets_.find(name, SNLNameComp<SNLScalarNet>());
  if (it != scalarNets_.end()) {
    return &*it;
  }
  return nullptr;
}

void SNLDesign::addBusNet(SNLBusNet* busNet) {
  busNets_.insert(*busNet);
}

void SNLDesign::removeBusNet(SNLBusNet* busNet) {
  busNets_.erase(*busNet);
}

SNLBusNet* SNLDesign::getBusNet(const SNLName& name) {
  auto it = busNets_.find(name, SNLNameComp<SNLBusNet>());
  if (it != busNets_.end()) {
    return &*it;
  }
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
  return SNLID(library_->getID(), getID());
}

}
