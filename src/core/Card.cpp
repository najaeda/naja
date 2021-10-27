#include "Card.h"

void Card::addItem(CardItem* item) {
  items_.push_back(std::unique_ptr<CardItem>(item));
}
