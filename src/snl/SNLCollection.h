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
    virtual Element getElement() const = 0;
    virtual void progress() = 0;
  protected:
    SNLBaseIterator() = default;
};

template<class Element>
class SNLBaseCollection {
  public:
    virtual SNLBaseIterator<Element>* begin() const = 0;
    virtual SNLBaseIterator<Element>* end() const = 0;

    virtual size_t size() const noexcept = 0;
    virtual bool empty() const noexcept = 0;

    SNLBaseCollection(const SNLBaseCollection&) = delete;
    SNLBaseCollection(SNLBaseCollection&&) = delete;
    virtual ~SNLBaseCollection() = default;
  protected:
    SNLBaseCollection() = default;
};

template<class Element, class HookType>
class SNLIntrusiveConstSetCollection: public SNLBaseCollection<Element*> {
  public:
    using super = SNLBaseCollection<Element*>;
    using Set = boost::intrusive::set<Element, HookType>;

    class SNLIntrusiveConstSetCollectionIterator: public SNLBaseIterator<Element*> {
      public:
        using SetIterator = typename Set::const_iterator;
        SNLIntrusiveConstSetCollectionIterator(const Set* set, bool beginOrEnd=true): set_(set) {
          if (set_) {
            if (beginOrEnd) {
              it_ = set->begin();
            } else {
              it_ = set->end();
            }
          }
        }
        Element* getElement() const override { return const_cast<Element*>(&*it_); }
        void progress() override { ++it_; }
      private:
        const Set*  set_  {nullptr};
        SetIterator it_   {};
    };

    SNLBaseIterator<Element*>* begin() const override {
      return new SNLIntrusiveConstSetCollectionIterator(set_, true);
    }

    SNLBaseIterator<Element*>* end() const override {
      return new SNLIntrusiveConstSetCollectionIterator(set_, false);
    }

    SNLIntrusiveConstSetCollection() = delete;
    SNLIntrusiveConstSetCollection(const SNLIntrusiveConstSetCollection&) = delete;
    SNLIntrusiveConstSetCollection(SNLIntrusiveConstSetCollection&&) = delete;
    SNLIntrusiveConstSetCollection(const Set* set): super(), set_(set) {}

    size_t size() const noexcept override {
      if (set_) {
        return set_->size();
      }
      return 0;
    }

    bool empty() const noexcept override {
      if (set_) {
        return set_->empty();
      }
      return true;
    }

  private:
    const Set*  set_  {nullptr};
};

template<class Element, class HookType>
class SNLIntrusiveSetCollection: public SNLBaseCollection<Element*> {
  public:
    using super = SNLBaseCollection<Element*>;
    using Set = boost::intrusive::set<Element, HookType>;

    class SNLIntrusiveSetCollectionIterator: public SNLBaseIterator<Element*> {
      public:
        using SetIterator = typename Set::iterator;
        SNLIntrusiveSetCollectionIterator(Set* set, bool beginOrEnd=true): set_(set) {
          if (set_) {
            if (beginOrEnd) {
              it_ = set->begin();
            } else {
              it_ = set->end();
            }
          }
        }
        Element* getElement() const override { return &*it_; } 
        void progress() override { if (it_) { ++it_; } }
      private:
        Set*        set_  {nullptr};
        SetIterator it_   {};
    };

    SNLIntrusiveSetCollection() = delete;
    SNLIntrusiveSetCollection(const SNLIntrusiveSetCollection&) = delete;
    SNLIntrusiveSetCollection(SNLIntrusiveSetCollection&&) = delete;
    SNLIntrusiveSetCollection(Set* set): super(), set_(set) {}

    SNLBaseIterator<Element*>* begin() const override {
      return new SNLIntrusiveSetCollectionIterator(set_, true);
    }
    SNLBaseIterator<Element*>* end() const override {
      return new SNLIntrusiveSetCollectionIterator(set_, false);
    }

    size_t size() const noexcept override {
      if (set_) {
        return set_->size();
      }
      return 0;
    }

    bool empty() const noexcept override {
      if (set_) {
        return set_->empty();
      }
      return true;
    }
  private:
    Set*  set_  {nullptr};
};

template<class Element>
class SNLCollection {
  public:
    struct Iterator {
      using iterator_category = std::input_iterator_tag;
      using difference_type   = std::ptrdiff_t;
      using value_type        = Element;
      using pointer           = value_type*;
      using reference         = value_type&;

      Iterator() = delete;
      Iterator(SNLBaseIterator<Element>* iterator): iterator_(iterator) {}

      Iterator& operator++() { iterator_->progress(); return *this; }

      value_type operator*() const { return iterator_->getElement(); }

      friend bool operator== (const Iterator& l, const Iterator& r) { return l.iterator_ == r.iterator_; };
      friend bool operator!= (const Iterator& l, const Iterator& r) { return l.iterator_ != r.iterator_; };

      private:
        SNLBaseIterator<Element>* iterator_ {nullptr};
    };

    Iterator begin() { return Iterator(collection_->begin()); }
    Iterator end() { return Iterator(collection_->end()); }

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
