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

#ifndef __SNL_FLATTENER_NET_TREE_H_
#define __SNL_FLATTENER_NET_TREE_H_

#include <ostream>

namespace naja { namespace SNL {

class SNLBitNet;
class SNLFlattenerNetForest;
class SNLFlattenerNetTreeNode;

class SNLFlattenerNetTree {
  public:
    friend class SNLFlattenerNetForest;
    using ID = unsigned int;

    class Type {
      public:
        enum TypeEnum {
          Standard, Constant0, Constant1
        };
        Type() = delete;
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };

    struct Less {
      bool operator() (const SNLFlattenerNetTree* leftTree, const SNLFlattenerNetTree* rightTree) const {
        return leftTree->getID() < rightTree->getID();
      }
    };

    SNLFlattenerNetTree() = delete;
    SNLFlattenerNetTree(const SNLFlattenerNetTree&) = delete;
    SNLFlattenerNetTree(SNLFlattenerNetTree&&) = delete;
    static SNLFlattenerNetTree* create(SNLFlattenerNetForest* forest, const SNLBitNet* net);
    void destroy();

    ID getID() const { return id_; }
    SNLFlattenerNetTreeNode* getRoot() const { return root_; }
    SNLFlattenerNetForest* getForest() const { return forest_; }

    void print(std::ostream& stream) const;
  private:
    SNLFlattenerNetTree(SNLFlattenerNetForest* forest, const SNLBitNet* net);
    ~SNLFlattenerNetTree() = default;
    void destroyFromForest();

    SNLFlattenerNetForest*    forest_ {nullptr};
    SNLFlattenerNetTreeNode*  root_   {nullptr};
    ID                        id_     {0};
    Type                      type_   {Type::Standard};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_FLATTENER_NET_TREE_H_ */
