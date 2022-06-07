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
#include <stack>
#include <memory>

#include <boost/intrusive/set.hpp>

namespace naja { namespace SNL {

template<class Type>
class SNLBaseIterator {
  public:
    SNLBaseIterator(SNLBaseIterator&&) = delete;
    virtual ~SNLBaseIterator() {}
    virtual Type getElement() const = 0;
    virtual void progress() = 0;
    virtual bool isEqual(const SNLBaseIterator<Type>* r) const = 0;
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

template<class Type>
class SNLSingletonCollection: public SNLBaseCollection<Type*> {
  public:
    using super = SNLBaseCollection<Type*>;

    class SNLSingletonCollectionIterator: public SNLBaseIterator<Type*> {
      public:
        SNLSingletonCollectionIterator(const SNLSingletonCollectionIterator&) = default;
        SNLSingletonCollectionIterator(Type* object, bool beginOrEnd=true): object_(object) {
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
        bool isEqual(const SNLBaseIterator<Type*>* r) override {
          if (const SNLSingletonCollectionIterator* rit = dynamic_cast<const SNLSingletonCollectionIterator*>(r)) {
            return object_ == rit->object_ and begin_ == rit->begin_;
          }
          return false;
        }
        bool isValid() const override {
          return object_ and begin_;
        }
        SNLBaseIterator<Type*>* clone() override {
          return new SNLSingletonCollectionIterator(*this);
        }
      private:
        Type* object_ {nullptr};
        bool  begin_  {false};
    };

    SNLBaseIterator<Type*>* begin() const override {
      return new SNLSingletonCollectionIterator(object_, true);
    }

    SNLBaseIterator<Type*>* end() const override {
      return new SNLSingletonCollectionIterator(object_, false);
    }

    SNLSingletonCollection() = delete;
    SNLSingletonCollection(const SNLSingletonCollection&) = delete;
    SNLSingletonCollection(SNLSingletonCollection&&) = delete;
    SNLSingletonCollection(Type* object): super(), object_(object) {}

    SNLBaseCollection<Type*>* clone() const override {
      return new SNLSingletonCollection(object_);
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
        bool isEqual(const SNLBaseIterator<Type*>* r) const override {
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

template<class STLType>
class SNLSTLCollection: public SNLBaseCollection<typename STLType::value_type> {
  public:
    using super = SNLBaseCollection<typename STLType::value_type>;

    class SNLSTLCollectionIterator: public SNLBaseIterator<typename STLType::value_type> {
      public:
        using STLTypeIterator = typename STLType::const_iterator;

        SNLSTLCollectionIterator(SNLSTLCollectionIterator&) = default;
        SNLSTLCollectionIterator(const STLType* container, bool beginOrEnd=true): container_(container) {
          if (container_) {
            if (beginOrEnd) {
              it_ = container_->begin();
            } else {
              it_ = container_->end();
            }
          }
        }

        SNLBaseIterator<typename STLType::value_type>* clone() override {
          return new SNLSTLCollectionIterator(*this);
        }

        typename STLType::value_type getElement() const override { return *it_; } 
        void progress() override { ++it_; }
        bool isEqual(const SNLBaseIterator<typename STLType::value_type>* r) const override {
          if (auto rit = dynamic_cast<const SNLSTLCollectionIterator*>(r)) {
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

    SNLSTLCollection() = delete;
    SNLSTLCollection(const SNLSTLCollection&) = delete;
    SNLSTLCollection(SNLSTLCollection&&) = delete;
    SNLSTLCollection(const STLType* container): super(), container_(container) {}
    SNLBaseCollection<typename STLType::value_type>* clone() const override {
      return new SNLSTLCollection(container_);
    }
    SNLBaseIterator<typename STLType::value_type>* begin() const override {
      return new SNLSTLCollectionIterator(container_, true);
    }
    SNLBaseIterator<typename STLType::value_type>* end() const override {
      return new SNLSTLCollectionIterator(container_, false);
    }

    size_t size() const override {
      if (container_) {
        return container_->size();
      }
      return 0;
    }

    bool empty() const override {
      if (container_) {
        return container_->empty();
      }
      return true;
    }
  private:
    const STLType* container_ {nullptr};
};

template<class STLMapType>
class SNLSTLMapCollection: public SNLBaseCollection<typename STLMapType::mapped_type> {
  public:
    using super = SNLBaseCollection<typename STLMapType::mapped_type>;

    class SNLSTLMapCollectionIterator: public SNLBaseIterator<typename STLMapType::mapped_type> {
      public:
        using STLMapTypeIterator = typename STLMapType::const_iterator;

        SNLSTLMapCollectionIterator(SNLSTLMapCollectionIterator&) = default;
        SNLSTLMapCollectionIterator(const STLMapType* container, bool beginOrEnd=true): container_(container) {
          if (container_) {
            if (beginOrEnd) {
              it_ = container_->begin();
            } else {
              it_ = container_->end();
            }
          }
        }

        SNLBaseIterator<typename STLMapType::mapped_type>* clone() override {
          return new SNLSTLMapCollectionIterator(*this);
        }

        typename STLMapType::mapped_type getElement() const override { return it_->second; } 
        void progress() override { ++it_; }
        bool isEqual(const SNLBaseIterator<typename STLMapType::mapped_type>* r) const override {
          if (auto rit = dynamic_cast<const SNLSTLMapCollectionIterator*>(r)) {
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

    SNLSTLMapCollection() = delete;
    SNLSTLMapCollection(const SNLSTLMapCollection&) = delete;
    SNLSTLMapCollection(SNLSTLMapCollection&&) = delete;
    SNLSTLMapCollection(const STLMapType* container): super(), container_(container) {}
    SNLBaseCollection<typename STLMapType::mapped_type>* clone() const override {
      return new SNLSTLMapCollection(container_);
    }
    SNLBaseIterator<typename STLMapType::mapped_type>* begin() const override {
      return new SNLSTLMapCollectionIterator(container_, true);
    }
    SNLBaseIterator<typename STLMapType::mapped_type>* end() const override {
      return new SNLSTLMapCollectionIterator(container_, false);
    }

    size_t size() const override {
      if (container_) {
        return container_->size();
      }
      return 0;
    }

    bool empty() const override {
      if (container_) {
        return container_->empty();
      }
      return true;
    }
  private:
    const STLMapType* container_  {nullptr};
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
        bool isEqual(const SNLBaseIterator<SubType>* r) const override {
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

template<class Type, typename Filter> class SNLFilteredCollection: public SNLBaseCollection<Type> {
  public:
    using super = SNLBaseCollection<Type>;

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
        bool isEqual(const SNLBaseIterator<Type>* r) const override {
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

template<class Type, class MasterType, class FlatType, class ReturnType, typename Flattener>
class SNLFlatCollection: public SNLBaseCollection<ReturnType> {
  public:
    using super = SNLBaseCollection<ReturnType>;

    class SNLFlatCollectionIterator: public SNLBaseIterator<ReturnType> {
      public:
        using super = SNLBaseIterator<ReturnType>;
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
        SNLFlatCollectionIterator(const SNLFlatCollectionIterator& it): element_(it.element_), flattener_(it.flattener_) {
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
        ~SNLFlatCollectionIterator() {
          if (it_ not_eq endIt_) {
            delete it_;
          }
          delete endIt_;
          delete flattenIt_;
        }
        SNLBaseIterator<ReturnType>* clone() override {
          return new SNLFlatCollectionIterator(*this);
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
        bool isEqual(const SNLBaseIterator<ReturnType>* r) const override {
          if (it_) {
            if (auto rit = dynamic_cast<const SNLFlatCollectionIterator*>(r)) {
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

        SNLBaseIterator<Type>*      it_         {nullptr};
        SNLBaseIterator<Type>*      endIt_      {nullptr};
        SNLBaseIterator<FlatType>*  flattenIt_  {nullptr};
        ReturnType                  element_    {nullptr};
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
    SNLBaseCollection<ReturnType>* clone() const override {
      return new SNLFlatCollection(collection_, flattener_);
    }
    SNLBaseIterator<ReturnType>* begin() const override {
      return new SNLFlatCollectionIterator(collection_, flattener_, true);
    }
    SNLBaseIterator<ReturnType>* end() const override {
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

template<class Type, typename ChildrenGetter, typename LeafCriterion>
class SNLTreeLeavesCollection: public SNLBaseCollection<Type> {
  public:
    using super = SNLBaseCollection<Type>;
    class SNLTreeLeavesCollectionIterator: public SNLBaseIterator<Type> {
      public:
        using super = SNLBaseIterator<Type>;
        SNLTreeLeavesCollectionIterator(
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
        SNLTreeLeavesCollectionIterator(const SNLTreeLeavesCollectionIterator& it) = default;
        ~SNLTreeLeavesCollectionIterator() {}
        SNLBaseIterator<Type>* clone() override {
          return new SNLTreeLeavesCollectionIterator(*this);
        }
        Type getElement() const override { return element_; } 
        void progress() override {
          if (isValid()) {
            element_ = nullptr;
            findNextElement();
          }
        }
        bool isEqual(const SNLBaseIterator<Type>* r) const override {
          if (auto rit = dynamic_cast<const SNLTreeLeavesCollectionIterator*>(r)) {
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
        bool operator==(const SNLTreeLeavesCollectionIterator&) const = default;
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

    SNLTreeLeavesCollection(const SNLTreeLeavesCollection&) = delete;
    SNLTreeLeavesCollection& operator=(const SNLTreeLeavesCollection&) = delete;
    SNLTreeLeavesCollection(const SNLTreeLeavesCollection&&) = delete;
    SNLTreeLeavesCollection(Type root, const ChildrenGetter& childrenGetter, const LeafCriterion& leafCriterion):
      super(), root_(root), childrenGetter_(childrenGetter), leafCriterion_(leafCriterion)
    {}
    ~SNLTreeLeavesCollection() {}
    SNLTreeLeavesCollection* clone() const override {
      return new SNLTreeLeavesCollection(root_, childrenGetter_, leafCriterion_);
    }

    SNLBaseIterator<Type>* begin() const override {
      return new SNLTreeLeavesCollectionIterator(root_, childrenGetter_, leafCriterion_, true);
    }
    SNLBaseIterator<Type>* end() const override {
      return new SNLTreeLeavesCollectionIterator(root_, childrenGetter_, leafCriterion_, false);
    }
    size_t size() const override {
      size_t size = 0;
      if (root_) {
        auto it = std::make_unique<SNLTreeLeavesCollectionIterator>(root_, childrenGetter_, leafCriterion_, true);
        auto endIt = std::make_unique<SNLTreeLeavesCollectionIterator>(root_, childrenGetter_, leafCriterion_, false);
        while (not it->isEqual(endIt.get())) {
          ++size;
          it->progress();
        }
      }
      return size;
    }
    bool empty() const override {
      if (root_) {
        auto it = std::make_unique<SNLTreeLeavesCollectionIterator>(root_, childrenGetter_, leafCriterion_, true);
        return not it->isValid();
      }
      return true;
    }
  private:
    Type  root_     {nullptr};
    ChildrenGetter  childrenGetter_ {};
    LeafCriterion   leafCriterion_  {};
};

template<class Type> class SNLCollection {
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

    template<typename Filter> SNLCollection<Type> getSubCollection(const Filter& filter) const {
      if (collection_) {
        return SNLCollection<Type>(new SNLFilteredCollection<Type, Filter>(collection_->clone(), filter));
      }
      return SNLCollection<Type>();
    }

    template<class MasterType, class FlatType, class ReturnType, typename Flattener>
      SNLCollection<ReturnType> getFlatCollection(const Flattener& flattener) const {
      if (collection_) {
        return SNLCollection<ReturnType>(new SNLFlatCollection<Type, MasterType, FlatType, ReturnType, Flattener>(collection_->clone(), flattener));
      }
      return SNLCollection<ReturnType>();
    }

    SNLBaseIterator<Type>* begin_() { return collection_->begin(); }
    SNLBaseIterator<Type>* end_() { return collection_->end(); }
    Iterator begin() { return Iterator(begin_()); }
    Iterator end() { return Iterator(end_()); }

    size_t size() const { if (collection_) { return collection_->size(); } return 0; }
    bool empty() const { if (collection_) { return collection_->empty(); } return true; }
  private:
    const SNLBaseCollection<Type>*  collection_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_COLLECTION_H_
