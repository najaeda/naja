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

#ifndef __SNL_FLATTENER_NET_TREE_NODE_H_
#define __SNL_FLATTENER_NET_TREE_NODE_H_

#include <set>
#include "SNLFlattenerInstanceTreeNode.h"

namespace naja { namespace SNL {

class SNLBitNet;
class SNLBitTerm;
class SNLInstTerm;

class SNLFlattenerNetTree;
class SNLFlattenerNetForest;

class SNLFlattenerNetTreeNode {
  friend class SNLFlattenerNetTree;
  public:
    class Type {
      public:
        enum TypeEnum {
          Root, InstTerm, Term
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

    using Children = std::map<const SNLDesignObject*, SNLFlattenerNetTreeNode*, SNLDesignObject::PointerLess>;

    SNLFlattenerNetTreeNode() = delete;
    SNLFlattenerNetTreeNode(const SNLFlattenerNetTreeNode&) = delete;
    SNLFlattenerNetTreeNode(SNLFlattenerNetTreeNode&&) = delete;

    SNLFlattenerNetTreeNode(
      SNLFlattenerNetTree* tree,
      SNLFlattenerInstanceTreeNode* instanceTreeNode,
      const SNLBitNet* rootNet);
    SNLFlattenerNetTreeNode(
      SNLFlattenerNetTreeNode* parent,
      SNLFlattenerInstanceTreeNode* instanceTreeNode,
      const SNLInstTerm* instTerm);
    SNLFlattenerNetTreeNode(SNLFlattenerNetTreeNode* parent, const SNLBitTerm* term);

    ~SNLFlattenerNetTreeNode();

    const SNLBitNet* getNet() const;
    const SNLBitTerm* getTerm() const;
    const SNLInstTerm* getInstTerm() const;

    SNLFlattenerInstanceTreeNode* getInstanceTreeNode() const;

    SNLFlattenerNetTreeNode* getParent() const;
    SNLFlattenerNetTree* getTree() const;
    SNLFlattenerNetForest* getForest() const;
    Type getType() const { return type_; }
    std::string getObjectString() const;

    bool isRoot() const { return type_ == Type::Root; }
    bool isLeaf() const { return children_.empty(); }
    bool isTerm() const { return type_ == Type::Term; }
    bool isInstTerm() const { return type_ == Type::InstTerm; }

    void print(std::ostream& stream, unsigned indent=0) const;
    std::string getString() const;

    ///\return the collection of direct children
    SNLCollection<SNLFlattenerNetTreeNode*> getChildren() const;
    ///\return the collection of leaf nodes
    SNLCollection<SNLFlattenerNetTreeNode*> getLeaves() const;
  private:
    void addChild(SNLFlattenerNetTreeNode* child, const SNLDesignObject* object);
    void removeChild(SNLFlattenerNetTreeNode* child);

    void*                         parent_           {nullptr};
    SNLFlattenerInstanceTreeNode* instanceTreeNode_ {nullptr};
    Type                          type_             {Type::Root};
    const SNLDesignObject*        object_           {nullptr};
    Children                      children_         {};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_FLATTENER_NET_TREE_NODE_H_ */
