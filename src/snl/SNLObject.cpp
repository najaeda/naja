#include "SNLObject.h"

#include "Card.h"

namespace SNL {

void SNLObject::destroy() {
  preDestroy();
  delete this;
}

Card* SNLObject::getCard() const {
  Card* card = new Card();
  return card;
}

void SNLObject::postCreate() {
}

void SNLObject::preDestroy() {
}

}
