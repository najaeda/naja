#include "SNLBusTerm.h"

#include "SNLDesign.h"

namespace SNL {

SNLBusTerm::SNLBusTerm(
    SNLDesign* design,
    const Direction& direction,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name):
  super(),
  name_(name),
  design_(design),
  direction_(direction),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusTerm* SNLBusTerm::create(
    SNLDesign* design,
    const Direction& direction,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name) {
  preCreate(design, name);
  SNLBusTerm* term = new SNLBusTerm(design, direction, msb, lsb, name);
  term->postCreate();
  return term;
}

void SNLBusTerm::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
  if (not name.empty()) {
  }
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
  return SNLDesignObject::getSNLID(SNLID::Type::Term, id_, 0, 0);
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
