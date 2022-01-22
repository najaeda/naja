#ifndef __SNL_COLLECTION_H_
#define __SNL_COLLECTION_H_

#include <boost/intrusive/set.hpp>
#include <vector>

namespace SNL {

template<class Element>
class SNLBaseIterator {
  public:
    SNLBaseIterator(const SNLBaseIterator&) = default;
    SNLBaseIterator(SNLBaseIterator&&) = delete;
    virtual ~SNLBaseIterator() {}
    virtual Element getElement() const = 0;
    virtual void progress() = 0;
    virtual bool isEqual(const SNLBaseIterator<Element>* r) = 0;
    virtual bool isDifferent(const SNLBaseIterator<Element>* r) = 0;
    virtual SNLBaseIterator<Element>* clone() = 0;
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
        SNLIntrusiveConstSetCollectionIterator(const SNLIntrusiveConstSetCollectionIterator&) = default;
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
        SNLBaseIterator<Element*>* clone() override {
          return new SNLIntrusiveConstSetCollectionIterator(*this);
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
        SNLBitsCollectionIterator(SNLBitsCollectionIterator&) = default;
        SNLBaseIterator<Element>* clone() override {
          return new SNLBitsCollectionIterator(*this);
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

#if 0

template<class Element, SubElement> class SNLSubElementCollection: public SNLCollection<SubElement> {
  public:
    using super = SNLCollection<SubElement>;

    SNLSubElementCollection(): super(), collection_ {}
    SNLSubElementCollection(const SNLSubElementCollection&) = delete;
    SNLSubElementCollection& operator=(const SNLSubElementCollection&) = delete;

    public: virtual Hurricane::Locator<SubType>* getLocator() const
    // ************************************************************
    {
        return new Locator(_collection);
    }


};

#endif

template<class Element>
class SNLCollection {
  public:
    class Iterator: public std::iterator<std::input_iterator_tag, Element> {
      public:
        Iterator() = delete;
        Iterator(const Iterator& it) {
          if (it.baseIt_) {
            baseIt_ = it.baseIt_->clone();
          }
        }
        Iterator(SNLBaseIterator<Element>* iterator): baseIt_(iterator) {}
        ~Iterator() { if (baseIt_) { delete baseIt_; } }

        Iterator& operator++() { baseIt_->progress(); return *this; }

        Element operator*() const { return baseIt_->getElement(); }

        bool operator==(const Iterator& r) const { return baseIt_->isEqual(r.baseIt_); }
        bool operator!=(const Iterator& r) const { return baseIt_->isDifferent(r.baseIt_); }
      private:
        SNLBaseIterator<Element>* baseIt_ {nullptr};
    };

    Iterator begin() { return Iterator(collection_->begin()); }
    Iterator end() { return Iterator(collection_->end()); }

    template<class SubElement> SNLBaseCollection<SubElement> getSubCollection() const {
      return SubTypeCollection<Element, SubElement>(this);
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
