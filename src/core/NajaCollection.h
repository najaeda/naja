// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_COLLECTION_H_
#define __NAJA_COLLECTION_H_

#include <vector>
#include <stack>
#include <memory>

#include <boost/intrusive/set.hpp>

namespace naja {

template<class Type>
class NajaBaseIterator {
  public:
    NajaBaseIterator(NajaBaseIterator&&) = delete;
    virtual ~NajaBaseIterator() {}
    virtual Type getElement() const = 0;
    virtual void progress() = 0;
    virtual bool isEqual(const NajaBaseIterator<Type>* r) const = 0;
    virtual bool isValid() const = 0;
    virtual NajaBaseIterator<Type>* clone() = 0;
  protected:
    NajaBaseIterator() = default;
    NajaBaseIterator(const NajaBaseIterator&) = default;
};

template<class Type>
class NajaBaseCollection {
  public:
    NajaBaseCollection(NajaBaseCollection&&) = delete;
    virtual ~NajaBaseCollection() = default;

    virtual NajaBaseCollection<Type>* clone() const = 0;

    virtual NajaBaseIterator<Type>* begin() const = 0;
    virtual NajaBaseIterator<Type>* end() const = 0;

    virtual size_t size() const = 0;
    virtual bool empty() const = 0;
  protected:
    NajaBaseCollection() = default;
    NajaBaseCollection(const NajaBaseCollection&) = default;
};

template<class Type>
class NajaSingletonCollection: public NajaBaseCollection<Type*> {
  public:
    using super = NajaBaseCollection<Type*>;

    class NajaSingletonCollectionIterator: public NajaBaseIterator<Type*> {
      public:
        NajaSingletonCollectionIterator(const NajaSingletonCollectionIterator&) = default;
        NajaSingletonCollectionIterator(Type* object, bool beginOrEnd=true): object_(object) {
          if (object_) {
            if (beginOrEnd) {
              begin_ = true;
            } else {
              begin_ = false;
            }
          }
        }
        Type* getElement() const override { return object_; }
        void progress() override { begin_ = false; }
        bool isEqual(const NajaBaseIterator<Type*>* r) const override {
          if (const NajaSingletonCollectionIterator* rit = dynamic_cast<const NajaSingletonCollectionIterator*>(r)) {
            return object_ == rit->object_ and begin_ == rit->begin_;
          }
          return false;
        }
        bool isValid() const override {
          return object_ and begin_;
        }
        NajaBaseIterator<Type*>* clone() override {
          return new NajaSingletonCollectionIterator(*this);
        }
      private:
        Type* object_ {nullptr};
        bool  begin_  {false};
    };

    NajaBaseIterator<Type*>* begin() const override {
      return new NajaSingletonCollectionIterator(object_, true);
    }

    NajaBaseIterator<Type*>* end() const override {
      return new NajaSingletonCollectionIterator(object_, false);
    }

    NajaSingletonCollection() = delete;
    NajaSingletonCollection(const NajaSingletonCollection&) = delete;
    NajaSingletonCollection(NajaSingletonCollection&&) = delete;
    NajaSingletonCollection(Type* object): super(), object_(object) {}

    NajaBaseCollection<Type*>* clone() const override {
      return new NajaSingletonCollection(object_);
    }
    size_t size() const override {
      if (object_) {
        return 1;
      }
      return 0;
    }
    bool empty() const override {
      return not object_;
    }

  private:
    Type* object_ {nullptr};
};

template<class Type, class HookType>
class NajaIntrusiveSetCollection: public NajaBaseCollection<Type*> {
  public:
    using super = NajaBaseCollection<Type*>;
    using Set = boost::intrusive::set<Type, HookType>;

    class NajaIntrusiveSetCollectionIterator: public NajaBaseIterator<Type*> {
      public:
        using SetIterator = typename Set::const_iterator;
        NajaIntrusiveSetCollectionIterator(const NajaIntrusiveSetCollectionIterator&) = default;
        NajaIntrusiveSetCollectionIterator(const Set* set, bool beginOrEnd=true): set_(set) {
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
        bool isEqual(const NajaBaseIterator<Type*>* r) const override {
          if (const NajaIntrusiveSetCollectionIterator* rit = dynamic_cast<const NajaIntrusiveSetCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isValid() const override {
          return set_ and it_ != set_->end();
        }
        NajaBaseIterator<Type*>* clone() override {
          return new NajaIntrusiveSetCollectionIterator(*this);
        }
      private:
        const Set*  set_  {nullptr};
        SetIterator it_   {};
    };

    NajaBaseIterator<Type*>* begin() const override {
      return new NajaIntrusiveSetCollectionIterator(set_, true);
    }

    NajaBaseIterator<Type*>* end() const override {
      return new NajaIntrusiveSetCollectionIterator(set_, false);
    }

    NajaIntrusiveSetCollection() = delete;
    NajaIntrusiveSetCollection(const NajaIntrusiveSetCollection&) = delete;
    NajaIntrusiveSetCollection(NajaIntrusiveSetCollection&&) = delete;
    NajaIntrusiveSetCollection(const Set* set): super(), set_(set) {}

    NajaBaseCollection<Type*>* clone() const override {
      return new NajaIntrusiveSetCollection(set_);
    }
    size_t size() const override {
      return set_->size();
    }
    bool empty() const override {
      return set_->empty();
    }

  private:
    const Set*  set_  {nullptr};
};

template<class STLType>
class NajaSTLCollection: public NajaBaseCollection<typename STLType::value_type> {
  public:
    using super = NajaBaseCollection<typename STLType::value_type>;

    class NajaSTLCollectionIterator: public NajaBaseIterator<typename STLType::value_type> {
      public:
        using STLTypeIterator = typename STLType::const_iterator;

        NajaSTLCollectionIterator(NajaSTLCollectionIterator&) = default;
        NajaSTLCollectionIterator(const STLType* container, bool beginOrEnd=true): container_(container) {
          if (container_) {
            if (beginOrEnd) {
              it_ = container_->begin();
            } else {
              it_ = container_->end();
            }
          }
        }

        NajaBaseIterator<typename STLType::value_type>* clone() override {
          return new NajaSTLCollectionIterator(*this);
        }

        typename STLType::value_type getElement() const override { return *it_; } 
        void progress() override { ++it_; }
        bool isEqual(const NajaBaseIterator<typename STLType::value_type>* r) const override {
          if (auto rit = dynamic_cast<const NajaSTLCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isValid() const override {
          return container_ and it_ != container_->end();
        }
      private:
        const STLType*  container_  {nullptr};
        STLTypeIterator it_         {};
    };

    NajaSTLCollection() = delete;
    NajaSTLCollection(const NajaSTLCollection&) = delete;
    NajaSTLCollection(NajaSTLCollection&&) = delete;
    NajaSTLCollection(const STLType* container): super(), container_(container) {}
    NajaBaseCollection<typename STLType::value_type>* clone() const override {
      return new NajaSTLCollection(container_);
    }
    NajaBaseIterator<typename STLType::value_type>* begin() const override {
      return new NajaSTLCollectionIterator(container_, true);
    }
    NajaBaseIterator<typename STLType::value_type>* end() const override {
      return new NajaSTLCollectionIterator(container_, false);
    }

    size_t size() const override {
      return container_->size();
    }

    bool empty() const override {
      return container_->empty();
    }
  private:
    const STLType* container_ {nullptr};
};

template<class STLMapType>
class NajaSTLMapCollection: public NajaBaseCollection<typename STLMapType::mapped_type> {
  public:
    using super = NajaBaseCollection<typename STLMapType::mapped_type>;

    class NajaSTLMapCollectionIterator: public NajaBaseIterator<typename STLMapType::mapped_type> {
      public:
        using STLMapTypeIterator = typename STLMapType::const_iterator;

        NajaSTLMapCollectionIterator(NajaSTLMapCollectionIterator&) = default;
        NajaSTLMapCollectionIterator(const STLMapType* container, bool beginOrEnd=true): container_(container) {
          if (container_) {
            if (beginOrEnd) {
              it_ = container_->begin();
            } else {
              it_ = container_->end();
            }
          }
        }

        NajaBaseIterator<typename STLMapType::mapped_type>* clone() override {
          return new NajaSTLMapCollectionIterator(*this);
        }

        typename STLMapType::mapped_type getElement() const override { return it_->second; } 
        void progress() override { ++it_; }
        bool isEqual(const NajaBaseIterator<typename STLMapType::mapped_type>* r) const override {
          if (auto rit = dynamic_cast<const NajaSTLMapCollectionIterator*>(r)) {
            return it_ == rit->it_;
          }
          return false;
        }
        bool isValid() const override {
          return container_ and it_ != container_->end();
        }
      private:
        const STLMapType*   container_  {nullptr};
        STLMapTypeIterator  it_         {};
    };

    NajaSTLMapCollection() = delete;
    NajaSTLMapCollection(const NajaSTLMapCollection&) = delete;
    NajaSTLMapCollection(NajaSTLMapCollection&&) = delete;
    NajaSTLMapCollection(const STLMapType* container): super(), container_(container) {}
    NajaBaseCollection<typename STLMapType::mapped_type>* clone() const override {
      return new NajaSTLMapCollection(container_);
    }
    NajaBaseIterator<typename STLMapType::mapped_type>* begin() const override {
      return new NajaSTLMapCollectionIterator(container_, true);
    }
    NajaBaseIterator<typename STLMapType::mapped_type>* end() const override {
      return new NajaSTLMapCollectionIterator(container_, false);
    }

    size_t size() const override {
      return container_->size();
    }

    bool empty() const override {
      return container_->empty();
    }
  private:
    const STLMapType* container_  {nullptr};
};

template<class Type, class ParentType> class NajaParentTypeCollection: public NajaBaseCollection<ParentType> {
  public:
    using super = NajaBaseCollection<ParentType>;

    class NajaParentTypeCollectionIterator: public NajaBaseIterator<ParentType> {
      public:
        using super = NajaBaseIterator<ParentType>;
        NajaParentTypeCollectionIterator(const NajaBaseCollection<Type>* collection, bool beginOrEnd=true):
          super() {
          if (collection) {
            endIt_ = collection->end();
            if (not beginOrEnd) {
              it_ = endIt_;
            } else {
              it_ = collection->begin();
            }
          }
        }
        NajaParentTypeCollectionIterator(const NajaParentTypeCollectionIterator& it) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
        ~NajaParentTypeCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
        }
        NajaBaseIterator<ParentType>* clone() override {
          return new NajaParentTypeCollectionIterator(*this);
        }
        ParentType getElement() const override { return it_->getElement(); } 
        void progress() override {
          if (isValid()) {
            it_->progress();
          }
        }
        bool isEqual(const NajaBaseIterator<ParentType>* r) const override {
          if (it_) {
            if (auto rit = dynamic_cast<const NajaParentTypeCollectionIterator*>(r)) {
              return it_->isEqual(rit->it_);
            }
          }
          return false;
        }
        bool isValid() const override {
          return it_ and endIt_ and not it_->isEqual(endIt_);
        }
      private:

        NajaBaseIterator<Type>*  it_     {nullptr};
        NajaBaseIterator<Type>*  endIt_  {nullptr};
    };

    NajaParentTypeCollection(const NajaParentTypeCollection&) = delete;
    NajaParentTypeCollection& operator=(const NajaParentTypeCollection&) = delete;
    NajaParentTypeCollection(const NajaParentTypeCollection&&) = delete;
    NajaParentTypeCollection(const NajaBaseCollection<Type>* collection):
      super(), collection_(collection)
    {}
    ~NajaParentTypeCollection() {
      delete collection_;
    }
    NajaBaseCollection<ParentType>* clone() const override {
      return new NajaParentTypeCollection(collection_->clone());
    }
    NajaBaseIterator<ParentType>* begin() const override {
      return new NajaParentTypeCollectionIterator(collection_, true);
    }
    NajaBaseIterator<ParentType>* end() const override {
      return new NajaParentTypeCollectionIterator(collection_, false);
    }
    size_t size() const override {
      if (collection_) {
        return collection_->size();
      }
      return 0;
    }
    bool empty() const override {
      if (collection_) {
        return collection_->empty();
      }
      return true;
    }

  private:
    const NajaBaseCollection<Type>* collection_;
};

template<class Type, class SubType> class NajaSubTypeCollection: public NajaBaseCollection<SubType> {
  public:
    using super = NajaBaseCollection<SubType>;

    class NajaSubTypeCollectionIterator: public NajaBaseIterator<SubType> {
      public:
        using super = NajaBaseIterator<SubType>;
        NajaSubTypeCollectionIterator(const NajaBaseCollection<Type>* collection, bool beginOrEnd=true):
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
        NajaSubTypeCollectionIterator(const NajaSubTypeCollectionIterator& it) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
        ~NajaSubTypeCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
        }
        NajaBaseIterator<SubType>* clone() override {
          return new NajaSubTypeCollectionIterator(*this);
        }
        SubType getElement() const override { return static_cast<SubType>(it_->getElement()); } 
        void progress() override {
          if (isValid()) {
            do {
              it_->progress();
            } while (isValid() and not dynamic_cast<SubType>(it_->getElement()));
          }
        }
        bool isEqual(const NajaBaseIterator<SubType>* r) const override {
          if (it_) {
            if (auto rit = dynamic_cast<const NajaSubTypeCollectionIterator*>(r)) {
              return it_->isEqual(rit->it_);
            }
          }
          return false;
        }
        bool isValid() const override {
          return it_ and endIt_ and not it_->isEqual(endIt_);
        }
      private:

        NajaBaseIterator<Type>*  it_     {nullptr};
        NajaBaseIterator<Type>*  endIt_  {nullptr};
    };

    NajaSubTypeCollection(const NajaSubTypeCollection&) = delete;
    NajaSubTypeCollection& operator=(const NajaSubTypeCollection&) = delete;
    NajaSubTypeCollection(const NajaSubTypeCollection&&) = delete;
    NajaSubTypeCollection(const NajaBaseCollection<Type>* collection):
      super(), collection_(collection)
    {}
    ~NajaSubTypeCollection() {
      delete collection_;
    }
    NajaBaseCollection<SubType>* clone() const override {
      return new NajaSubTypeCollection(collection_->clone());
    }
    NajaBaseIterator<SubType>* begin() const override {
      return new NajaSubTypeCollectionIterator(collection_, true);
    }
    NajaBaseIterator<SubType>* end() const override {
      return new NajaSubTypeCollectionIterator(collection_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<NajaSubTypeCollectionIterator>(collection_, true);
        auto endIt = std::make_unique<NajaSubTypeCollectionIterator>(collection_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      auto it = std::make_unique<NajaSubTypeCollectionIterator>(collection_, true);
      return not it->isValid();
    }

  private:
    const NajaBaseCollection<Type>* collection_;
};

template<class Type, typename Filter> class NajaFilteredCollection: public NajaBaseCollection<Type> {
  public:
    using super = NajaBaseCollection<Type>;

    class NajaFilteredCollectionIterator: public NajaBaseIterator<Type> {
      public:
        using super = NajaBaseIterator<Type>;
        NajaFilteredCollectionIterator(const NajaBaseCollection<Type>* collection, const Filter& filter, bool beginOrEnd=true):
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
        NajaFilteredCollectionIterator(const NajaFilteredCollectionIterator& it): filter_(it.filter_) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
        ~NajaFilteredCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
        }
        NajaBaseIterator<Type>* clone() override {
          return new NajaFilteredCollectionIterator(*this);
        }
        Type getElement() const override { return static_cast<Type>(it_->getElement()); } 
        void progress() override {
          if (isValid()) {
            do {
              it_->progress();
            } while (isValid() and not filter_(it_->getElement()));
          }
        }
        bool isEqual(const NajaBaseIterator<Type>* r) const override {
          if (it_) {
            if (auto rit = dynamic_cast<const NajaFilteredCollectionIterator*>(r)) {
              return it_->isEqual(rit->it_);
            }
          }
          return false;
        }
        bool isValid() const override {
          return it_ and endIt_ and not it_->isEqual(endIt_);
        }
      private:
        NajaBaseIterator<Type>*  it_     {nullptr};
        NajaBaseIterator<Type>*  endIt_  {nullptr};
        Filter                  filter_;
    };

    NajaFilteredCollection(const NajaFilteredCollection&) = delete;
    NajaFilteredCollection& operator=(const NajaFilteredCollection&) = delete;
    NajaFilteredCollection(const NajaFilteredCollection&&) = delete;
    NajaFilteredCollection(const NajaBaseCollection<Type>* collection, const Filter& filter):
      super(), collection_(collection), filter_(filter) 
    {}
    ~NajaFilteredCollection() {
      delete collection_;
    }
    NajaBaseCollection<Type>* clone() const override {
      return new NajaFilteredCollection(collection_->clone(), filter_);
    }
    NajaBaseIterator<Type>* begin() const override {
      return new NajaFilteredCollectionIterator(collection_, filter_, true);
    }
    NajaBaseIterator<Type>* end() const override {
      return new NajaFilteredCollectionIterator(collection_, filter_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<NajaFilteredCollectionIterator>(collection_, filter_, true);
        auto endIt = std::make_unique<NajaFilteredCollectionIterator>(collection_, filter_,false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      auto it = std::make_unique<NajaFilteredCollectionIterator>(collection_, filter_,true);
      return not it->isValid();
    }

  private:
    const NajaBaseCollection<Type>* collection_;
    Filter                          filter_;
};

template<class Type, class MasterType, class FlatType, class ReturnType, typename Flattener>
class NajaFlatCollection: public NajaBaseCollection<ReturnType> {
  public:
    using super = NajaBaseCollection<ReturnType>;

    class NajaFlatCollectionIterator: public NajaBaseIterator<ReturnType> {
      public:
        using super = NajaBaseIterator<ReturnType>;
        NajaFlatCollectionIterator(const NajaBaseCollection<Type>* collection, const Flattener& flattener, bool beginOrEnd=true):
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
              if (it_->isValid()) {
                Type e = it_->getElement();
                if (auto r = dynamic_cast<ReturnType>(e)) {
                  element_ = r;
                } else {
                  MasterType master = static_cast<MasterType>(e);
                  flattenIt_ = flattener_(master).begin_();
                  assert(flattenIt_->isValid());
                  element_ = flattenIt_->getElement();
                }
              }
            }
          }
        }
        NajaFlatCollectionIterator(const NajaFlatCollectionIterator& it): element_(it.element_), flattener_(it.flattener_) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
          if (it.flattenIt_) {
            flattenIt_ = it.flattenIt_->clone();
          }
        }
        ~NajaFlatCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
          delete flattenIt_;
        }
        NajaBaseIterator<ReturnType>* clone() override {
          return new NajaFlatCollectionIterator(*this);
        }
        ReturnType getElement() const override { return element_; } 
        void progress() override {
          if (isValid()) {
            element_ = nullptr;
            if (flattenIt_ and flattenIt_->isValid()) {
              flattenIt_->progress();
              if (flattenIt_->isValid()) {
                element_ = flattenIt_->getElement();
                return;
              } else {
                delete flattenIt_;
                flattenIt_ = nullptr;
              }
            }
            if (it_->isValid()) {
              it_->progress();
            }
            if (it_->isValid()) {
              Type e = it_->getElement();
              if (auto r = dynamic_cast<ReturnType>(e)) {
                element_ = r;
                if (flattenIt_) {
                  delete flattenIt_;
                }
                flattenIt_ = nullptr;
              } else {
                MasterType master = static_cast<MasterType>(e);
                flattenIt_ = flattener_(master).begin_();
                assert(flattenIt_->isValid());
                element_ = flattenIt_->getElement();
              }
            }
          }
        }
        bool isEqual(const NajaBaseIterator<ReturnType>* r) const override {
          if (it_) {
            if (auto rit = dynamic_cast<const NajaFlatCollectionIterator*>(r)) {
              if (it_->isEqual(rit->it_)) {
                if (flattenIt_) {
                  return flattenIt_->isEqual(rit->flattenIt_);
                } else {
                  return not rit->flattenIt_;
                }
              }
            }
          }
          return false;
        }
        bool isValid() const override {
          return element_ != nullptr;
        }
      private:

        NajaBaseIterator<Type>*     it_         {nullptr};
        NajaBaseIterator<Type>*     endIt_      {nullptr};
        NajaBaseIterator<FlatType>* flattenIt_  {nullptr};
        ReturnType                  element_    {nullptr};
        Flattener                   flattener_;
    };

    NajaFlatCollection(const NajaFlatCollection&) = delete;
    NajaFlatCollection& operator=(const NajaFlatCollection&) = delete;
    NajaFlatCollection(const NajaFlatCollection&&) = delete;
    NajaFlatCollection(const NajaBaseCollection<Type>* collection, const Flattener& flattener):
      super(), collection_(collection), flattener_(flattener)
    {}
    ~NajaFlatCollection() {
      delete collection_;
    }
    NajaBaseCollection<ReturnType>* clone() const override {
      return new NajaFlatCollection(collection_->clone(), flattener_);
    }
    NajaBaseIterator<ReturnType>* begin() const override {
      return new NajaFlatCollectionIterator(collection_, flattener_, true);
    }
    NajaBaseIterator<ReturnType>* end() const override {
      return new NajaFlatCollectionIterator(collection_, flattener_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<NajaFlatCollectionIterator>(collection_, flattener_, true);
        auto endIt = std::make_unique<NajaFlatCollectionIterator>(collection_, flattener_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      auto it = std::make_unique<NajaFlatCollectionIterator>(collection_, flattener_, true);
      return not it->isValid();
    }

  private:
    const NajaBaseCollection<Type>* collection_;
    Flattener                      flattener_;
};

template<class Type, class ReturnType, typename Transformer>
class NajaTransformerCollection: public NajaBaseCollection<ReturnType> {
  public:
    using super = NajaBaseCollection<ReturnType>;

    class NajaTransformerCollectionIterator: public NajaBaseIterator<ReturnType> {
      public:
        using super = NajaBaseIterator<ReturnType>;
        NajaTransformerCollectionIterator(const NajaBaseCollection<Type>* collection, const Transformer& transformer, bool beginOrEnd=true):
          super(),
          transformer_(transformer) {
          if (collection) {
            endIt_ = collection->end();
            if (not beginOrEnd) {
              it_ = endIt_;
              element_ = nullptr;
            } else {
              it_ = collection->begin();
              if (it_->isValid()) {
                Type e = it_->getElement();
                element_ = transformer_(e);
              }
            }
          }
        }
        NajaTransformerCollectionIterator(const NajaTransformerCollectionIterator& it):
          element_(it.element_), transformer_(it.transformer_) {
          endIt_ = it.endIt_->clone();
          if (it.it_ not_eq it.endIt_) {
            it_ = it.it_->clone();
          } else {
            it_ = endIt_;
          }
        }
        ~NajaTransformerCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
        }
        NajaBaseIterator<ReturnType>* clone() override {
          return new NajaTransformerCollectionIterator(*this);
        }
        ReturnType getElement() const override { return element_; } 
        void progress() override {
          if (isValid()) {
            element_ = nullptr;
            if (it_->isValid()) {
              it_->progress();
            }
            if (it_->isValid()) {
              Type e = it_->getElement();
              element_ = transformer_(e);
            }
          }
        }
        bool isEqual(const NajaBaseIterator<ReturnType>* r) const override {
          if (it_) {
            if (auto rit = dynamic_cast<const NajaTransformerCollectionIterator*>(r)) {
              return it_->isEqual(rit->it_);
            }
          }
          return false;
        }
        bool isValid() const override {
          return element_ != nullptr;
        }
      private:

        NajaBaseIterator<Type>*     it_         {nullptr};
        NajaBaseIterator<Type>*     endIt_      {nullptr};
        ReturnType                  element_    {nullptr};
        Transformer                 transformer_;
    };

    NajaTransformerCollection(const NajaTransformerCollection&) = delete;
    NajaTransformerCollection& operator=(const NajaTransformerCollection&) = delete;
    NajaTransformerCollection(const NajaTransformerCollection&&) = delete;
    NajaTransformerCollection(const NajaBaseCollection<Type>* collection, const Transformer& transformer):
      super(), collection_(collection), transformer_(transformer)
    {}
    ~NajaTransformerCollection() {
      delete collection_;
    }
    NajaBaseCollection<ReturnType>* clone() const override {
      return new NajaTransformerCollection(collection_->clone(), transformer_);
    }
    NajaBaseIterator<ReturnType>* begin() const override {
      return new NajaTransformerCollectionIterator(collection_, transformer_, true);
    }
    NajaBaseIterator<ReturnType>* end() const override {
      return new NajaTransformerCollectionIterator(collection_, transformer_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (collection_) {
        auto it = std::make_unique<NajaTransformerCollectionIterator>(collection_, transformer_, true);
        auto endIt = std::make_unique<NajaTransformerCollectionIterator>(collection_, transformer_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      auto it = std::make_unique<NajaTransformerCollectionIterator>(collection_, transformer_, true);
      return not it->isValid();
    }

  private:
    const NajaBaseCollection<Type>* collection_;
    Transformer                     transformer_;
};

template<class Type, typename ChildrenGetter, typename LeafCriterion>
class NajaTreeLeavesCollection: public NajaBaseCollection<Type> {
  public:
    using super = NajaBaseCollection<Type>;
    class NajaTreeLeavesCollectionIterator: public NajaBaseIterator<Type> {
      public:
        using super = NajaBaseIterator<Type>;
        NajaTreeLeavesCollectionIterator(
          const Type& root,
          const ChildrenGetter& childrenGetter,
          const LeafCriterion& leafCriterion,
          bool beginOrEnd=true):
          super(),
          root_(root),
          childrenGetter_(childrenGetter),
          leafCriterion_(leafCriterion),
          stack_() {
          if (beginOrEnd) {
            if (root) {
              stack_.push(root);
              findNextElement(); 
            }
          }
        }
        NajaTreeLeavesCollectionIterator(const NajaTreeLeavesCollectionIterator& it) = default;
        ~NajaTreeLeavesCollectionIterator() {}
        NajaBaseIterator<Type>* clone() override {
          return new NajaTreeLeavesCollectionIterator(*this);
        }
        Type getElement() const override { return element_; } 
        void progress() override {
          if (isValid()) {
            element_ = nullptr;
            findNextElement();
          }
        }
        bool isEqual(const NajaBaseIterator<Type>* r) const override {
          if (auto rit = dynamic_cast<const NajaTreeLeavesCollectionIterator*>(r)) {
            return root_ == rit->root_
              and element_ == rit->element_ 
              and childrenGetter_ == rit->childrenGetter_
              and leafCriterion_ == rit->leafCriterion_
              and stack_ == rit->stack_;
          }
          return false;
        }
        bool isValid() const override {
          return element_ != nullptr;
        } 
      private:
        void findNextElement() {
          while (not stack_.empty()) {
            auto object = stack_.top();
            stack_.pop();
            if (leafCriterion_(object)) {
              element_ = object;
              return;
            } else {
              for (auto child: childrenGetter_(object)) {
                stack_.push(child);
              }
            }
          }
        }
        using Stack = std::stack<Type>;
        Type            root_           {nullptr};
        ChildrenGetter  childrenGetter_ {};
        LeafCriterion   leafCriterion_  {};
        Stack           stack_          {};
        Type            element_        {nullptr};
    };

    NajaTreeLeavesCollection(const NajaTreeLeavesCollection&) = delete;
    NajaTreeLeavesCollection& operator=(const NajaTreeLeavesCollection&) = delete;
    NajaTreeLeavesCollection(const NajaTreeLeavesCollection&&) = delete;
    NajaTreeLeavesCollection(Type root, const ChildrenGetter& childrenGetter, const LeafCriterion& leafCriterion):
      super(), root_(root), childrenGetter_(childrenGetter), leafCriterion_(leafCriterion)
    {}
    ~NajaTreeLeavesCollection() {}
    NajaTreeLeavesCollection* clone() const override {
      return new NajaTreeLeavesCollection(root_, childrenGetter_, leafCriterion_);
    }

    NajaBaseIterator<Type>* begin() const override {
      return new NajaTreeLeavesCollectionIterator(root_, childrenGetter_, leafCriterion_, true);
    }
    NajaBaseIterator<Type>* end() const override {
      return new NajaTreeLeavesCollectionIterator(root_, childrenGetter_, leafCriterion_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (root_) {
        auto it = std::make_unique<NajaTreeLeavesCollectionIterator>(root_, childrenGetter_, leafCriterion_, true);
        auto endIt = std::make_unique<NajaTreeLeavesCollectionIterator>(root_, childrenGetter_, leafCriterion_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      if (root_) {
        auto it = std::make_unique<NajaTreeLeavesCollectionIterator>(root_, childrenGetter_, leafCriterion_, true);
        return not it->isValid();
      }
      return true;
    }
  private:
    Type  root_     {nullptr};
    ChildrenGetter  childrenGetter_ {};
    LeafCriterion   leafCriterion_  {};
};

template<class Type> class NajaCollection {
  public:
    class Iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Type;
        using difference_type = std::ptrdiff_t;
        using pointer = Type*;
        using reference = Type&;

        Iterator() = delete;
        Iterator(const Iterator& it) {
          if (it.baseIt_) {
            baseIt_ = it.baseIt_->clone();
          }
        }
        Iterator(NajaBaseIterator<Type>* iterator): baseIt_(iterator) {}
        ~Iterator() { if (baseIt_) { delete baseIt_; } }

        Iterator& operator++() { if (baseIt_) { baseIt_->progress(); } return *this; }

        Type operator*() const { if (baseIt_) { return baseIt_->getElement(); } return Type(); }

        bool operator==(const Iterator& r) const { 
          if (baseIt_) {
            if (r.baseIt_) {
              return baseIt_->isEqual(r.baseIt_);
            } else {
              return false;
            }
          }
          return r.baseIt_ == nullptr;
        }
        bool operator!=(const Iterator& r) const { return !(*this == r); }
      private:
        NajaBaseIterator<Type>* baseIt_ {nullptr};
    };

    NajaCollection() = default;
    NajaCollection(NajaCollection&&) = delete;
    NajaCollection(const NajaBaseCollection<Type>* collection): collection_(collection) {}
    ~NajaCollection() { delete collection_; }

    template<class ParentType> NajaCollection<ParentType> getParentTypeCollection() const {
      if (collection_) {
        return NajaCollection<ParentType>(new NajaParentTypeCollection<Type, ParentType>(collection_->clone()));
      }
      return NajaCollection<ParentType>();
    }

    template<class SubType> NajaCollection<SubType> getSubCollection() const {
      if (collection_) {
        return NajaCollection<SubType>(new NajaSubTypeCollection<Type, SubType>(collection_->clone()));
      }
      return NajaCollection<SubType>();
    }

    template<typename Filter> NajaCollection<Type> getSubCollection(const Filter& filter) const {
      if (collection_) {
        return NajaCollection<Type>(new NajaFilteredCollection<Type, Filter>(collection_->clone(), filter));
      }
      return NajaCollection<Type>();
    }

    template<class MasterType, class FlatType, class ReturnType, typename Flattener>
      NajaCollection<ReturnType> getFlatCollection(const Flattener& flattener) const {
      if (collection_) {
        return NajaCollection<ReturnType>(new NajaFlatCollection<Type, MasterType, FlatType, ReturnType, Flattener>(collection_->clone(), flattener));
      }
      return NajaCollection<ReturnType>();
    }

    template<class ReturnType, typename Transformer>
      NajaCollection<ReturnType> getTransformerCollection(const Transformer& transformer) const {
      if (collection_) {
        return NajaCollection<ReturnType>(new NajaTransformerCollection<Type, ReturnType, Transformer>(collection_->clone(), transformer));
      }
      return NajaCollection<ReturnType>();
    }

    NajaBaseIterator<Type>* begin_() { if (collection_) { return collection_->begin(); } return nullptr; }
    NajaBaseIterator<Type>* end_() { if (collection_) { return collection_->end(); } return nullptr; }
    Iterator begin() { return Iterator(begin_()); }
    Iterator end() { return Iterator(end_()); }

    size_t size() const { if (collection_) { return collection_->size(); } return 0; }
    bool empty() const { if (collection_) { return collection_->empty(); } return true; }
  private:
    const NajaBaseCollection<Type>*  collection_ {nullptr};
};

} // namespace naja

#endif // __NAJA_COLLECTION_H_
