#include "SNLBusTerm.h"

#include "SNLDesign.h"
#include "SNLBusTermBit.h"

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
  //create bits
  bits_.resize(getSize(), nullptr);
  for (size_t i=0; i<getSize(); i++) {
    SNLID::Bit bit = (getMSB()>getLSB())?getMSB()-i:getMSB()+i;
    bits_[i] = SNLBusTermBit::create(this, bit);
  }
}

void SNLBusTerm::commonPreDestroy() {
  for (SNLBusTermBit* bit: bits_) {
    bit->destroyFromBus();
  }
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

size_t SNLBusTerm::getSize() const {
  return std::abs(getLSB() - getMSB()) + 1;
}

SNLID SNLBusTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Term, id_, 0, 0);
}

constexpr const char* SNLBusTerm::getTypeName() const {
  return "SNLBusTerm";
}

std::string SNLBusTerm::getString() const {
  return getName() + "[" + std::to_string(getMSB()) + ":" + std::to_string(getLSB()) + "]";
}

std::string SNLBusTerm::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_ + " " + design_->getName() + ">";  
}

}
