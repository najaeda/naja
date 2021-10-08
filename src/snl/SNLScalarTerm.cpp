#include "SNLScalarTerm.h"

#include "SNLDesign.h"

namespace SNL {

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, const SNLName& name):
  super(),
  name_(name),
  design_(design)
{}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, const SNLName& name) {
  preCreate(design, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, name);
  net->postCreate();
  return net;
}

void SNLScalarTerm::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  //verify that there is not an instance of name in this design
}

void SNLScalarTerm::postCreate() {
  super::postCreate();
  getDesign()->addScalarTerm(this);
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
  getDesign()->removeScalarTerm(this);
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
