/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SNL_COLLECTION_H_
#define __SNL_COLLECTION_H_

#include <vector>
#include <memory>
#include <functional>

#include <boost/intrusive/set.hpp>

namespace SNL {

template<class Type>
class SNLBaseIterator {
  public:
    SNLBaseIterator(SNLBaseIterator&&) = delete;
    virtual ~SNLBaseIterator() {}
    virtual Type getElement() const = 0;
    virtual void progress() = 0;
    virtual bool isEqual(const SNLBaseIterator<Type>* r) = 0;
    virtual bool isValid() const = 0;
    virtual SNLBaseIterator<Type>* clone() = 0;
  protected:
    SNLBaseIterator() = default;
    SNLBaseIterator(const SNLBaseIterator&) = default;
};

template<class Type>
class SNLBaseCollection {
  public:
    SNLBaseCollection(SNLBaseCollection&&) = delete;
    virtual ~SNLBaseCollection() = default;

    virtual SNLBaseCollection<Type>* clone() const = 0;

    virtual SNLBaseIterator<Type>* begin() const = 0;
    virtual SNLBaseIterator<Type>* end() const = 0;

    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
  protected:
    SNLBaseCollection() = default;
    SNLBaseCollection(const SNLBaseCollection&) = default;
};

template<class Type, class HookType>
class SNLIntrusiveSetCollection: public SNLBaseCollection<Type*> {
  public:
    using super = SNLBaseCollection<Type*>;
    using Set = boost::intrusive::set<Type, HookType>;

    class SNLIntrusiveSetCollectionIterator: public SNLBaseIterator<Type*> {
      public:
        using SetIterator = typename Set::const_iterator;
        SNLIntrusiveSetCollectionIterator(const SNLIntrusiveSetCollectionIterator&) = default;
        SNLIntrusiveSetCollectionIterator(const Set* set, bool beginOrEnd=true): set_(set) {
          if (set_) {
            if (beginOrEnd) {
              it_ = set->begin();
            } else {
              it_ = set->end();
            }
          }
        }
        Type* getElement() const override { return const_cast<Type*>(&*it_); }
        void progress() override { ++it_; }
        bool isEqual(const SNLBaseIterator<Type*>* r) override {
          if (const SNLIntrusiveSetCollectionIterator* rit = dynamic_cast<const SNLIntrusiveSetCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isValid() const override {
          return set_ and it_ != set_->end();
        }
        SNLBaseIterator<Type*>* clone() override {
          return new SNLIntrusiveSetCollectionIterator(*this);
        }
      private:
        const Set*  set_  {nullptr};
        SetIterator it_   {};
    };

    SNLBaseIterator<Type*>* begin() const override {
      return new SNLIntrusiveSetCollectionIterator(set_, true);
    }

    SNLBaseIterator<Type*>* end() const override {
      return new SNLIntrusiveSetCollectionIterator(set_, false);
    }

    SNLIntrusiveSetCollection() = delete;
    SNLIntrusiveSetCollection(const SNLIntrusiveSetCollection&) = delete;
    SNLIntrusiveSetCollection(SNLIntrusiveSetCollection&&) = delete;
    SNLIntrusiveSetCollection(const Set* set): super(), set_(set) {}

    SNLBaseCollection<Type*>* clone() const override {
      return new SNLIntrusiveSetCollection(set_);
    }
    size_t size() const override {
      if (set_) {
        return set_->size();
      }
      return 0;
    }
    bool empty() const override {
      if (set_) {
        return set_->empty();
      }
      return true;
    }

  private:
    const Set*  set_  {nullptr};
};

template<class Type>
class SNLVectorCollection: public SNLBaseCollection<Type> {
  public:
    using super = SNLBaseCollection<Type>;
    using Vector = std::vector<Type>;

    class SNLVectorCollectionIterator: public SNLBaseIterator<Type> {
      public:
        using VectorIterator = typename Vector::const_iterator;

        SNLVectorCollectionIterator(SNLVectorCollectionIterator&) = default;
        SNLVectorCollectionIterator(const Vector* bits, bool beginOrEnd=true): bits_(bits) {
          if (bits_) {
            if (beginOrEnd) {
              it_ = bits->begin();
            } else {
              it_ = bits->end();
            }
          }
        }

        SNLBaseIterator<Type>* clone() override {
          return new SNLVectorCollectionIterator(*this);
        }

        Type getElement() const override { return *it_; } 
        void progress() override { ++it_; }
        bool isEqual(const SNLBaseIterator<Type>* r) override {
          if (auto rit = dynamic_cast<const SNLVectorCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isValid() const override {
          return bits_ and it_ != bits_->end();
        }
      private:
        const Vector*   bits_ {nullptr};
        VectorIterator  it_   {};
    };

    SNLVectorCollection() = delete;
    SNLVectorCollection(const SNLVectorCollection&) = delete;
    SNLVectorCollection(SNLVectorCollection&&) = delete;
    SNLVectorCollection(const Vector* bits): super(), bits_(bits) {}
    SNLBaseCollection<Type>* clone() const override {
      return new SNLVectorCollection(bits_);
    }
    SNLBaseIterator<Type>* begin() const override {
      return new SNLVectorCollectionIterator(bits_, true);
    }
    SNLBaseIterator<Type>* end() const override {
      return new SNLVectorCollectionIterator(bits_, false);
    }

    size_t size() const override {
      if (bits_) {
        return bits_->size();
      }
      return 0;
    }

    bool empty() const override {
      if (bits_) {
        return bits_->empty();
      }
      return true;
    }
  private:
    const Vector* bits_ {nullptr};
};

template<class Type, class SubType> class SNLSubTypeCollection: public SNLBaseCollection<SubType> {
  public:
    using super = SNLBaseCollection<SubType>;

    class SNLSubTypeCollectionIterator: public SNLBaseIterator<SubType> {
      public:
        using super = SNLBaseIterator<SubType>;
        SNLSubTypeCollectionIterator(const SNLBaseCollection<Type>* collection, bool beginOrEnd=true):
          super() {
          if (collection) {
            endIt_ = collection->end();
            if (not beginOrEnd) {
              it_ = endIt_;
            } else {
              it_ = collection->begin();
              while (isValid() and not dynamic_cast<SubType>(it_->getElement())) {
                it_->progress();
              }
            }
          }
        }
        SNLSubTypeCollectionIterator(const SNLSubTypeCollectionIterator& it) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
        ~SNLSubTypeCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
        }
        SNLBaseIterator<SubType>* clone() override {
          return new SNLSubTypeCollectionIterator(*this);
        }
        SubType getElement() const override { return static_cast<SubType>(it_->getElement()); } 
        void progress() override {
          if (isValid()) {
            do {
              it_->progress();
            } while (isValid() and not dynamic_cast<SubType>(it_->getElement()));
          }
        }
        bool isEqual(const SNLBaseIterator<SubType>* r) override {
          if (it_) {
            if (auto rit = dynamic_cast<const SNLSubTypeCollectionIterator*>(r)) {
              return it_->isEqual(rit->it_);
            }
          }
          return false;
        }
        bool isValid() const override {
          return it_ and endIt_ and not it_->isEqual(endIt_);
        }
      private:

        SNLBaseIterator<Type>*  it_     {nullptr};
        SNLBaseIterator<Type>*  endIt_  {nullptr};
    };

    SNLSubTypeCollection(const SNLSubTypeCollection&) = delete;
    SNLSubTypeCollection& operator=(const SNLSubTypeCollection&) = delete;
    SNLSubTypeCollection(const SNLSubTypeCollection&&) = delete;
    SNLSubTypeCollection(const SNLBaseCollection<Type>* collection):
      super(), collection_(collection)
    {}
    ~SNLSubTypeCollection() {
      delete collection_;
    }
    SNLBaseCollection<SubType>* clone() const override {
      return new SNLSubTypeCollection(collection_);
    }
    SNLBaseIterator<SubType>* begin() const override {
      return new SNLSubTypeCollectionIterator(collection_, true);
    }
    SNLBaseIterator<SubType>* end() const override {
      return new SNLSubTypeCollectionIterator(collection_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<SNLSubTypeCollectionIterator>(collection_, true);
        auto endIt = std::make_unique<SNLSubTypeCollectionIterator>(collection_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      if (collection_) {
        auto it = std::make_unique<SNLSubTypeCollectionIterator>(collection_, true);
        return not it->isValid();
      }
      return true;
    }

  private:
    const SNLBaseCollection<Type>* collection_;
};

template<class Type> class SNLFilteredCollection: public SNLBaseCollection<Type> {
  public:
    using super = SNLBaseCollection<Type>;
    using Filter = std::function<bool(Type)>;

    class SNLFilteredCollectionIterator: public SNLBaseIterator<Type> {
      public:
        using super = SNLBaseIterator<Type>;
        SNLFilteredCollectionIterator(const SNLBaseCollection<Type>* collection, const Filter& filter, bool beginOrEnd=true):
          super(),
          filter_(filter) {
          if (collection) {
            endIt_ = collection->end();
            if (not beginOrEnd) {
              it_ = endIt_;
            } else {
              it_ = collection->begin();
              while (isValid() and not filter_(it_->getElement())) {
                it_->progress();
              }
            }
          }
        }
        SNLFilteredCollectionIterator(const SNLFilteredCollectionIterator& it): filter_(it.filter_) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
        ~SNLFilteredCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
        }
        SNLBaseIterator<Type>* clone() override {
          return new SNLFilteredCollectionIterator(*this);
        }
        Type getElement() const override { return static_cast<Type>(it_->getElement()); } 
        void progress() override {
          if (isValid()) {
            do {
              it_->progress();
            } while (isValid() and not filter_(it_->getElement()));
          }
        }
        bool isEqual(const SNLBaseIterator<Type>* r) override {
          if (it_) {
            if (auto rit = dynamic_cast<const SNLFilteredCollectionIterator*>(r)) {
              return it_->isEqual(rit->it_);
            }
          }
          return false;
        }
        bool isValid() const override {
          return it_ and endIt_ and not it_->isEqual(endIt_);
        }
      private:
        SNLBaseIterator<Type>*  it_     {nullptr};
        SNLBaseIterator<Type>*  endIt_  {nullptr};
        Filter                  filter_;
    };

    SNLFilteredCollection(const SNLFilteredCollection&) = delete;
    SNLFilteredCollection& operator=(const SNLFilteredCollection&) = delete;
    SNLFilteredCollection(const SNLFilteredCollection&&) = delete;
    SNLFilteredCollection(const SNLBaseCollection<Type>* collection, const Filter& filter):
      super(), collection_(collection), filter_(filter) 
    {}
    ~SNLFilteredCollection() {
      delete collection_;
    }
    SNLBaseCollection<Type>* clone() const override {
      return new SNLFilteredCollection(collection_, filter_);
    }
    SNLBaseIterator<Type>* begin() const override {
      return new SNLFilteredCollectionIterator(collection_, filter_, true);
    }
    SNLBaseIterator<Type>* end() const override {
      return new SNLFilteredCollectionIterator(collection_, filter_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<SNLFilteredCollectionIterator>(collection_, filter_, true);
        auto endIt = std::make_unique<SNLFilteredCollectionIterator>(collection_, filter_,false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      if (collection_) {
        auto it = std::make_unique<SNLFilteredCollectionIterator>(collection_, filter_,true);
        return not it->isValid();
      }
      return true;
    }

  private:
    const SNLBaseCollection<Type>*  collection_;
    Filter                          filter_;
};

template<class Type, class MasterType, class FlatType, typename Flattener>
class SNLFlatCollection: public SNLBaseCollection<FlatType> {
  public:
    using super = SNLBaseCollection<FlatType>;

    class SNLFlatCollectionIterator: public SNLBaseIterator<FlatType> {
      public:
        using super = SNLBaseIterator<FlatType>;
        SNLFlatCollectionIterator(const SNLBaseCollection<Type>* collection, const Flattener& flattener, bool beginOrEnd=true):
          super(),
          flattener_(flattener) {
          if (collection) {
            endIt_ = collection->end();
            if (not beginOrEnd) {
              it_ = endIt_;
              flattenIt_ = nullptr;
              element_ = nullptr;
            } else {
              it_ = collection->begin();
              Type e = it_->getElement();
              if (not dynamic_cast<FlatType>(e)) {
                //MasterType master = static_cast<MasterType>(e);
                //flattenIt_ = master->getXX().begin();
                assert(flattenIt_->isValid());
                element_ = flattenIt_->getElement();
              } else {
                //element_ = e;
              }
            }
          }
        }
#if 0
        SNLSubTypeCollectionIterator(const SNLSubTypeCollectionIterator& it) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
#endif
        ~SNLFlatCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
          delete flattenIt_;
        }
        SNLBaseIterator<FlatType>* clone() override {
          return new SNLFlatCollectionIterator(*this);
        }
        FlatType getElement() const override { return element_; } 
        void progress() override {
#if 0
          if (isValid()) {
            if (flattenIt_->isValid()) {
              flattenIt_->progress();
              if (flattenIt_->isValid()) {
                element_ = flattenIt_->getElement();
              }
            }
          }
#endif
        }
        bool isEqual(const SNLBaseIterator<FlatType>* r) override {
          //if (it_) {
          //  if (auto rit = dynamic_cast<const SNLSubTypeCollectionIterator*>(r)) {
          //    return it_->isEqual(rit->it_);
          //  }
          //}
          return false;
        }
        bool isValid() const override {
          return element_ != nullptr;
        }
      private:

        SNLBaseIterator<Type>*      it_         {nullptr};
        SNLBaseIterator<Type>*      endIt_      {nullptr};
        SNLBaseIterator<FlatType>*  flattenIt_  {nullptr};
        FlatType                    element_    {nullptr};
        Flattener                   flattener_;
    };

    SNLFlatCollection(const SNLFlatCollection&) = delete;
    SNLFlatCollection& operator=(const SNLFlatCollection&) = delete;
    SNLFlatCollection(const SNLFlatCollection&&) = delete;
    SNLFlatCollection(const SNLBaseCollection<Type>* collection, const Flattener& flattener):
      super(), collection_(collection), flattener_(flattener)
    {}
    ~SNLFlatCollection() {
      delete collection_;
    }
    SNLBaseCollection<FlatType>* clone() const override {
      return new SNLFlatCollection(collection_, flattener_);
    }
    SNLBaseIterator<FlatType>* begin() const override {
      return new SNLFlatCollectionIterator(collection_, flattener_, true);
    }
    SNLBaseIterator<FlatType>* end() const override {
      return new SNLFlatCollectionIterator(collection_, flattener_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<SNLFlatCollectionIterator>(collection_, flattener_, true);
        auto endIt = std::make_unique<SNLFlatCollectionIterator>(collection_, flattener_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      if (collection_) {
        auto it = std::make_unique<SNLFlatCollectionIterator>(collection_, flattener_, true);
        return not it->isValid();
      }
      return true;
    }

  private:
    const SNLBaseCollection<Type>* collection_;
    Flattener                      flattener_;
};

template<class Type> class SNLCollection {
  public:
    class Iterator: public std::iterator<std::input_iterator_tag, Type> {
      public:
        Iterator() = delete;
        Iterator(const Iterator& it) {
          if (it.baseIt_) {
            baseIt_ = it.baseIt_->clone();
          }
        }
        Iterator(SNLBaseIterator<Type>* iterator): baseIt_(iterator) {}
        ~Iterator() { if (baseIt_) { delete baseIt_; } }

        Iterator& operator++() { baseIt_->progress(); return *this; }

        Type operator*() const { return baseIt_->getElement(); }

        bool operator==(const Iterator& r) const { return baseIt_->isEqual(r.baseIt_); }
        bool operator!=(const Iterator& r) const { return not baseIt_->isEqual(r.baseIt_); }
      private:
        SNLBaseIterator<Type>* baseIt_ {nullptr};
    };

    SNLCollection() = default;
    SNLCollection(SNLCollection&&) = delete;
    SNLCollection(const SNLBaseCollection<Type>* collection): collection_(collection) {}
    ~SNLCollection() { delete collection_; }

    template<class SubType> SNLCollection<SubType> getSubCollection() const {
      if (collection_) {
        return SNLCollection<SubType>(new SNLSubTypeCollection<Type, SubType>(collection_->clone()));
      }
      return SNLCollection<SubType>();
    }

    SNLCollection<Type> getSubCollection(const std::function<bool(Type)>& filter) const {
      if (collection_) {
        return SNLCollection<Type>(new SNLFilteredCollection<Type>(collection_->clone(), filter));
      }
      return SNLCollection<Type>();
    }

    template<class MasterType, class FlatType, typename Flattener> SNLCollection<FlatType> getFlatCollection(const Flattener& flattener) const {
      if (collection_) {
        return SNLCollection<FlatType>(new SNLFlatCollection<Type, MasterType, FlatType, Flattener>(collection_->clone(), flattener));
      }
      return SNLCollection<FlatType>();
    }

    Iterator begin() { return Iterator(collection_->begin()); }
    Iterator end() { return Iterator(collection_->end()); }

    size_t size() const { if (collection_) { return collection_->size(); } return 0; }
    bool empty() const { if (collection_) { return collection_->empty(); } return true; }
  private:
    const SNLBaseCollection<Type>*  collection_ {nullptr};
};

}

#endif /* __SNL_COLLECTION_H_ */
