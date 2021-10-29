#include "SNLBusTerm.h"

#include "SNLLibrary.h"
#include "SNLDesign.h"

namespace SNL {

SNLBusTerm::SNLBusTerm(SNLDesign* design, const SNLName& name):
  super(),
  name_(name),
  design_(design)
{}

SNLBusTerm* SNLBusTerm::create(SNLDesign* design, const SNLName& name) {
  preCreate(design, name);
  SNLBusTerm* term = new SNLBusTerm(design, name);
  term->postCreate();
  return term;
}

SNLID SNLBusTerm::getSNLID() const {
  return SNLID(
      SNLID::Type::Term,
      getDesign()->getLibrary()->getID(),
      getDesign()->getID(),
      id_);
}

constexpr const char* SNLBusTerm::getTypeName() const {
  return "SNLBusTerm";
}

std::string SNLBusTerm::getString() const {
  return std::string();
}

std::string SNLBusTerm::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_ + " " + design_->getName() + ">";  
}

void SNLBusTerm::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLBusTerm::postCreate() {
  super::postCreate();
  getDesign()->addBusTerm(this);
}

void SNLBusTerm::commonPreDestroy() {
  super::preDestroy();
}

void SNLBusTerm::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLBusTerm::preDestroy() {
  commonPreDestroy();
  getDesign()->removeBusTerm(this);
}

}
