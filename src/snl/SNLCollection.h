#ifndef __SNL_COLLECTION_H_
#define __SNL_COLLECTION_H_

#include <boost/intrusive/set.hpp>

namespace SNL {

template<class Element>
class SNLBaseIterator {
  public:
    SNLBaseIterator(const SNLBaseIterator&) = delete;
    SNLBaseIterator(SNLBaseIterator&&) = delete;
    virtual ~SNLBaseIterator() {}
    virtual Element* getElement() const = 0;
    virtual void progress() = 0;
    virtual bool isValid() const = 0;
  protected:
    SNLBaseIterator() = default;
};

template<class Element>
class SNLBaseCollection {
  public:
    virtual SNLBaseIterator<Element>* getIterator() const = 0;

    SNLBaseCollection(const SNLBaseCollection&) = delete;
    SNLBaseCollection(SNLBaseCollection&&) = delete;
    virtual ~SNLBaseCollection() = default;
  protected:
    SNLBaseCollection() = default;
};

template<class Element, class HookType>
class SNLIntrusiveConstSetCollection: public SNLBaseCollection<Element> {
  public:
    using super = SNLBaseCollection<Element>;
    using Set = boost::intrusive::set<Element, HookType>;

    class SNLIntrusiveConstSetCollectionIterator: public SNLBaseIterator<Element> {
      public:
        using SetIterator = typename Set::const_iterator;
        SNLIntrusiveConstSetCollectionIterator(const Set* set): set_(set) {
          if (set_) {
            it_ = set->begin();
          }
        }
        Element* getElement() const override { if (isValid()) { return const_cast<Element*>(&*it_); } return nullptr; } 
        void progress() override { if (isValid()) { ++it_; } }
        bool isValid() const override { return set_ and it_ != set_->end(); }
      private:
        const Set*  set_  {nullptr};
        SetIterator it_   {};
    };

    SNLBaseIterator<Element>* getIterator() const override {
      return new SNLIntrusiveConstSetCollectionIterator(set_);
    }
    SNLIntrusiveConstSetCollection() = delete;
    SNLIntrusiveConstSetCollection(const SNLIntrusiveConstSetCollection&) = delete;
    SNLIntrusiveConstSetCollection(SNLIntrusiveConstSetCollection&&) = delete;
    SNLIntrusiveConstSetCollection(const Set* set): super(), set_(set) {}
  private:
    const Set*  set_  {nullptr};
};

template<class Element, class HookType>
class SNLIntrusiveSetCollection: public SNLBaseCollection<Element> {
  public:
    using super = SNLBaseCollection<Element>;
    using Set = boost::intrusive::set<Element, HookType>;

    class SNLIntrusiveSetCollectionIterator: public SNLBaseIterator<Element> {
      public:
        using SetIterator = typename Set::iterator;
        SNLIntrusiveSetCollectionIterator(Set* set): set_(set) {
          if (set_) {
            it_ = set->begin();
          }
        }

        Element* getElement() const override { if (isValid()) { return &*it_; } return nullptr; } 
        void progress() override { if (it_) { ++it_; } }
        bool isValid() const override { return set_ and it_ != set_->end(); }
      private:
        Set*        set_  {nullptr};
        SetIterator it_   {};
    };

    SNLBaseIterator<Element>* getIterator() const override {
      return new SNLIntrusiveSetCollectionIterator(set_);
    }
    SNLIntrusiveSetCollection() = delete;
    SNLIntrusiveSetCollection(const SNLIntrusiveSetCollection&) = delete;
    SNLIntrusiveSetCollection(SNLIntrusiveSetCollection&&) = delete;
    SNLIntrusiveSetCollection(Set* set): super(), set_(set) {}
  private:
    Set*  set_  {nullptr};
};

template<class Element>
class SNLIterator {
  public:
    SNLIterator() = default;
    SNLIterator(SNLBaseIterator<Element>* iterator): iterator_(iterator) {}
    SNLIterator(const SNLIterator&) = delete;
    SNLIterator(SNLIterator&&) = delete;
    ~SNLIterator() { delete iterator_; }
    Element* getElement() const { if (iterator_) { return iterator_->getElement(); } return nullptr; }
    SNLIterator& operator++() { if (iterator_) { iterator_->progress(); } return *this; }
    bool isValid() const { if (iterator_) { return iterator_->isValid(); } return false; }
  private:
    SNLBaseIterator<Element>* iterator_ {nullptr};
};

template<class Element>
class SNLCollection {
  public:
    virtual SNLIterator<Element> getIterator() const {
      if (collection_) {
        return SNLIterator(collection_->getIterator());
      }
      return SNLIterator<Element>();
    } 

    SNLCollection() = delete;
    SNLCollection(const SNLBaseCollection<Element>* collection): collection_(collection) {}
    SNLCollection(const SNLCollection&) = delete;
    SNLCollection(SNLCollection&&) = delete;
    virtual ~SNLCollection() { delete collection_; }

    size_t size() const noexcept { if (collection_) { return collection_->size(); } return 0; }
    bool empty() const noexcept { if (collection_) { return collection_->empty(); } return true; }
  private:
    const SNLBaseCollection<Element>*  collection_ {nullptr};
};

}

#endif /* __SNL_COLLECTION_H_ */
