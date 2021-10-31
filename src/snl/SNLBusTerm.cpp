#include "SNLBusTerm.h"

#include "SNLDesign.h"

namespace SNL {

SNLBusTerm::SNLBusTerm(SNLDesign* design, const SNLName& name, const Direction& direction):
  super(),
  name_(name),
  design_(design),
  direction_(direction)
{}

SNLBusTerm::SNLBusTerm(SNLDesign* design, const Direction& direction):
  super(),
  design_(design),
  direction_(direction)
{}

SNLBusTerm* SNLBusTerm::create(SNLDesign* design, const SNLName& name, const Direction& direction) {
  preCreate(design, name);
  SNLBusTerm* term = new SNLBusTerm(design, name, direction);
  term->postCreate();
  return term;
}

SNLBusTerm* SNLBusTerm::create(SNLDesign* design, const Direction& direction) {
  preCreate(design);
  SNLBusTerm* term = new SNLBusTerm(design, direction);
  term->postCreate();
  return term;
}

void SNLBusTerm::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLBusTerm::preCreate(const SNLDesign* design) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLBusTerm::postCreate() {
  super::postCreate();
  getDesign()->addTerm(this);
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
  getDesign()->removeTerm(this);
}

SNLID SNLBusTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Term, id_);
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

}
