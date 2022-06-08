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

#ifndef __SNL_FLATTENER_INSTANCE_TREE_NODE_H_
#define __SNL_FLATTENER_INSTANCE_TREE_NODE_H_

#include "SNLInstance.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"

namespace naja { namespace SNL {

class SNLFlattenerInstanceTree;
class SNLFlattenerNetTreeNode;

class SNLFlattenerInstanceTreeNode {
  public:
    friend class SNLFlattener;
    friend class SNLFlattenerInstanceTree;
    friend class SNLFlattenerNetTreeNode;
    
    using Children =
      std::map<const SNLInstance*, SNLFlattenerInstanceTreeNode*, SNLDesignObject::PointerLess>;
    using NetNodes =
      std::map<const SNLBitNet*, SNLFlattenerNetTreeNode*, SNLDesignObject::PointerLess>;
    using InstTermNodes =
      std::map<const SNLBitTerm*, SNLFlattenerNetTreeNode*, SNLDesignObject::PointerLess>;

    SNLFlattenerInstanceTreeNode(const SNLFlattenerInstanceTreeNode&) = delete;
    SNLFlattenerInstanceTreeNode(SNLFlattenerInstanceTreeNode&&) = delete;
    ~SNLFlattenerInstanceTreeNode();

    SNLFlattenerInstanceTreeNode* getParent() const;
    SNLFlattenerInstanceTree* getTree() const;

    bool isRoot() const { return isRoot_; }
    bool isLeaf() const { return children_.empty(); }
    
    SNLFlattenerInstanceTreeNode* getChildNode(const SNLInstance* instance) const;
    SNLFlattenerNetTreeNode* getNetNode(const SNLBitNet* net) const;
    SNLFlattenerNetTreeNode* getTermNode(const SNLBitTerm* term) const;

    const SNLDesign* getDesign() const;
    const SNLInstance* getInstance() const;

    ///\return the collection of direct children
    SNLCollection<SNLFlattenerInstanceTreeNode*> getChildren() const;
    ///\return the collection of leaf nodes
    SNLCollection<SNLFlattenerInstanceTreeNode*> getLeaves() const;

    void print(std::ostream& stream, unsigned indent=0) const;
    std::string getString() const;
  private:
    SNLFlattenerInstanceTreeNode(SNLFlattenerInstanceTree* tree, const SNLDesign* top);
    SNLFlattenerInstanceTreeNode(SNLFlattenerInstanceTreeNode* parent, const SNLInstance* instance);
    SNLFlattenerInstanceTreeNode* addChild(const SNLInstance* instance);
    void removeChild(const SNLInstance* instance);
    void addNetNode(SNLFlattenerNetTreeNode* node, const SNLBitNet* net);
    void addInstTermNode(SNLFlattenerNetTreeNode* node, const SNLBitTerm* term);

    bool          isRoot_         { false };
    void*         parent_         { nullptr };
    const void*   object_         { nullptr };
    Children      children_       {};
    NetNodes      netNodes_       {};
    InstTermNodes instTermNodes_  {};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_FLATTENER_INSTANCE_TREE_NODE_H_ */