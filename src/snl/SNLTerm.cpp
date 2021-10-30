#include "SNLTerm.h"

namespace SNL {

SNLTerm::Direction::Direction(const DirectionEnum& dirEnum):
  dirEnum_(dirEnum) 
{}

std::string SNLTerm::Direction::getString() const {
  switch (dirEnum_) {
    case Direction::Input: return "Input";
    case Direction::Output: return "Output";
    case Direction::InOut: return "InOut";
  }
  return "Unknown";
}

void SNLTerm::preCreate() {
  super::preCreate();
}

void SNLTerm::postCreate() {
  super::postCreate();
}

void SNLTerm::preDestroy() {
  super::preDestroy();
}


}
