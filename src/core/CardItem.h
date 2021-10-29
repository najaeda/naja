#ifndef __CARD_ITEM_H_
#define __CARD_ITEM_H_

#include <string>

class Card;

class CardItem {
  public:
    CardItem() = delete;
    CardItem(const CardItem&) = delete;
    virtual ~CardItem() = default;

    virtual Card* getDataCard() const = 0;
    std::string getDescription() const {
      return description_;
    }
  protected:
    CardItem(const std::string& description): description_(description) {}
  private:
    std::string description_;
};

template<typename Data>
class CardDataItem: public CardItem {
  public:
    using super = CardItem;
    CardDataItem(const std::string& description, Data data):
      super(description) {}
    Card* getDataCard() const override { return nullptr; }
};

template<typename Data>
class CardDataItem<const Data*>: public CardItem {
  public:
    using super = CardItem;
    CardDataItem(const std::string& description, const Data* data):
      super(description), data_(data) {}
    Card* getDataCard() const override {
      return data_->getCard();
    }
  private:
    const Data* data_;  
};

#endif /* __CARD_ITEM_H_ */
