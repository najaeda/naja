#ifndef __SNL_COLLECTION_H_
#define __SNL_COLLECTION_H_

#include <boost/intrusive/set.hpp>
#include <vector>

namespace SNL {

template<class Element>
class SNLBaseIterator {
  public:
    SNLBaseIterator(const SNLBaseIterator&) = delete;
    SNLBaseIterator(SNLBaseIterator&&) = delete;
    virtual ~SNLBaseIterator() {}
    virtual Element getElement() const = 0;
    virtual void progress() = 0;
    virtual bool isEqual(const SNLBaseIterator<Element>* r) = 0;
    virtual bool isDifferent(const SNLBaseIterator<Element>* r) = 0;
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
        bool isEqual(const SNLBaseIterator<Element*>* r) override {
          if (const SNLIntrusiveConstSetCollectionIterator* rit = dynamic_cast<const SNLIntrusiveConstSetCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isDifferent(const SNLBaseIterator<Element*>* r) override {
          if (const SNLIntrusiveConstSetCollectionIterator* rit = dynamic_cast<const SNLIntrusiveConstSetCollectionIterator*>(r)) {
            return it_ != rit->it_;
          }
          return true;
        }
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
        void progress() override { ++it_; }
        bool isEqual(const SNLBaseIterator<Element*>* r) override {
          if (const SNLIntrusiveSetCollectionIterator* rit = dynamic_cast<const SNLIntrusiveSetCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isDifferent(const SNLBaseIterator<Element*>* r) override {
          if (const SNLIntrusiveSetCollectionIterator* rit = dynamic_cast<const SNLIntrusiveSetCollectionIterator*>(r)) {
            return it_ != rit->it_;
          }
          return true;
        }
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
class SNLBitsCollection: public SNLBaseCollection<Element> {
  public:
    using super = SNLBaseCollection<Element>;
    using Bits = std::vector<Element>;

    class SNLBitsCollectionIterator: public SNLBaseIterator<Element> {
      public:
        using BitsIterator = typename Bits::const_iterator;
        SNLBitsCollectionIterator(const Bits* bits, bool beginOrEnd=true): bits_(bits) {
          if (bits_) {
            if (beginOrEnd) {
              it_ = bits->begin();
            } else {
              it_ = bits->end();
            }
          }
        }
        Element getElement() const override { return *it_; } 
        void progress() override { ++it_; }
        bool isEqual(const SNLBaseIterator<Element>* r) override {
          if (const SNLBitsCollectionIterator* rit = dynamic_cast<const SNLBitsCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isDifferent(const SNLBaseIterator<Element>* r) override {
          if (const SNLBitsCollectionIterator* rit = dynamic_cast<const SNLBitsCollectionIterator*>(r)) {
            return it_ != rit->it_;
          }
          return true;
        }
      private:
        const Bits*   bits_ {nullptr};
        BitsIterator  it_   {};
    };

    SNLBitsCollection() = delete;
    SNLBitsCollection(const SNLBitsCollection&) = delete;
    SNLBitsCollection(SNLBitsCollection&&) = delete;
    SNLBitsCollection(const Bits* bits): super(), bits_(bits) {}

    SNLBaseIterator<Element>* begin() const override {
      return new SNLBitsCollectionIterator(bits_, true);
    }
    SNLBaseIterator<Element>* end() const override {
      return new SNLBitsCollectionIterator(bits_, false);
    }

    size_t size() const noexcept override {
      if (bits_) {
        return bits_->size();
      }
      return 0;
    }

    bool empty() const noexcept override {
      if (bits_) {
        return bits_->empty();
      }
      return true;
    }
  private:
    const Bits* bits_ {nullptr};
};

template<class Element>
class SNLCollection {
  public:
    class Iterator: public std::iterator<std::input_iterator_tag, Element> {
      public:
        Iterator() = delete;
        Iterator(SNLBaseIterator<Element>* iterator): it_(iterator) {}
        ~Iterator() { if (it_) { delete it_; } }

        Iterator& operator++() { it_->progress(); return *this; }

        Element operator*() const { return it_->getElement(); }

        bool operator==(const Iterator& r) const { return it_->isEqual(r.it_); }
        bool operator!=(const Iterator& r) const { return it_->isDifferent(r.it_); }
      private:
        SNLBaseIterator<Element>* it_ {nullptr};
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
