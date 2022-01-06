#include "SNLInstTerm.h"

#include <sstream>

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLBitTerm.h"

namespace SNL {

SNLInstTerm::SNLInstTerm(SNLInstance* instance, SNLBitTerm* term):
  instance_(instance),
  term_(term)
{}

SNLInstTerm* SNLInstTerm::create(SNLInstance* instance, SNLBitTerm* term) {
  preCreate(instance, term);
  SNLInstTerm* instTerm = new SNLInstTerm(instance, term);
  instTerm->postCreate();
  return instTerm;
}

void SNLInstTerm::preCreate(const SNLInstance* instance, const SNLBitTerm* term) {
  super::preCreate();
}

void SNLInstTerm::postCreate() {
  super::postCreate();
}

void SNLInstTerm::preDestroy() {
  super::preDestroy();
}

void SNLInstTerm::destroyFromInstance() {
  preDestroy();
  delete this;
}

void SNLInstTerm::destroy() {
  throw SNLException("Unauthorized destroy of SNLInstTerm");
}

SNLDesign* SNLInstTerm::getDesign() const {
  return getInstance()->getDesign();
}

SNLID SNLInstTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::InstTerm, getTerm()->getID(), getInstance()->getID(), getTerm()->getBit());
}

bool SNLInstTerm::isAnonymous() const {
  return getTerm()->isAnonymous();
}

constexpr const char* SNLInstTerm::getTypeName() const {
  return "SNLInstTerm";
}

std::string SNLInstTerm::getString() const {
  std::ostringstream str;
  if (not getInstance()->isAnonymous()) {
    str << getInstance()->getName().getString();
  }
  str << "(" << getInstance()->getID() << ")";
  str << ":";
  if (not getTerm()->isAnonymous()) {
    str << getTerm()->getName().getString();
  }
  str << "(" << getTerm()->getID() << ")";
  return str.str();
}

std::string SNLInstTerm::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + getDesign()->getName().getString() + ">";  
}

}
