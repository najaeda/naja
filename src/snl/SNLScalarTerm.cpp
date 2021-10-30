#include "SNLScalarTerm.h"

#include "SNLLibrary.h"
#include "SNLDesign.h"

namespace SNL {

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, const Direction& direction):
  super(),
  design_(design),
  direction_(direction)
{}

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, const SNLName& name, const Direction& direction):
  super(),
  design_(design),
  name_(name),
  direction_(direction)
{}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, const SNLName& name, const Direction& direction) {
  preCreate(design, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, name, direction);
  net->postCreate();
  return net;
}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, const Direction& direction) {
  preCreate(design);
  SNLScalarTerm* term = new SNLScalarTerm(design, direction);
  term->postCreate();
  return term;
}

void SNLScalarTerm::preCreate(const SNLDesign* design) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLScalarTerm::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLScalarTerm::postCreate() {
  super::postCreate();
  getDesign()->addTerm(this);
}

void SNLScalarTerm::commonPreDestroy() {
  super::preDestroy();
}

void SNLScalarTerm::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLScalarTerm::preDestroy() {
  commonPreDestroy();
  getDesign()->removeTerm(this);
}

SNLID SNLScalarTerm::getSNLID() const {
  return SNLID(
      SNLID::Type::Term,
      getDesign()->getLibrary()->getID(),
      getDesign()->getID(),
      id_);
}

constexpr const char* SNLScalarTerm::getTypeName() const {
  return "SNLScalarTerm";
}
 
std::string SNLScalarTerm::getString() const {
  return std::string();
}

std::string SNLScalarTerm::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_ + " " + design_->getName() + ">";  
}

}
