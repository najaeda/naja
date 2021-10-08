#ifndef __CARD_H_
#define __CARD_H_

#include <vector>

#include "CardItem.h"

class Card {
  public:
    using CardItems = std::vector<std::unique_ptr<CardItem>>;
    Card() = default;
    void addItem(CardItem* item);
  private:
    CardItems items_;  
};

#endif /* __CARD_H_ */
