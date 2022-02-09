#ifndef __SNL_FLATTENER_INSTANCE_TREE_NODE_H_
#define __SNL_FLATTENER_INSTANCE_TREE_NODE_H_

#include "SNLInstance.h"

namespace naja { namespace SNL {

class SNLFlattenerInstanceTree;

class SNLFlattenerInstanceTreeNode {
  public:
    friend class SNLFlattener;
    using Children =
      std::map<const SNLInstance*, SNLFlattenerInstanceTreeNode*, SNLDesignObject::PointerLess>;

    SNLFlattenerInstanceTreeNode(const SNLFlattenerInstanceTreeNode&) = delete;
    SNLFlattenerInstanceTreeNode(SNLFlattenerInstanceTreeNode&&) = delete;
    ~SNLFlattenerInstanceTreeNode();

    SNLFlattenerInstanceTreeNode* getParent() const;
    SNLFlattenerInstanceTree* getTree() const;

    bool isRoot() const { return isRoot_; }
  private:
    SNLFlattenerInstanceTreeNode(SNLFlattenerInstanceTreeNode* parent, const SNLInstance* instance);
    void addChild(const SNLInstance* instance);

    bool                isRoot_   { false };
    void*               parent_   { nullptr };
    const SNLInstance*  instance_ { nullptr };
    Children            children_ {};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_FLATTENER_INSTANCE_TREE_NODE_H_ */