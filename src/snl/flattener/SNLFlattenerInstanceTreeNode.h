#ifndef __SNL_FLATTENER_INSTANCE_TREE_NODE_H_
#define __SNL_FLATTENER_INSTANCE_TREE_NODE_H_

namespace SNL {

class SNLFlattenerInstanceTree;

class SNLFlattenerInstanceTreeNode {
  public:
    SNLFlattenerInstanceTreeNode* getParent() const;
    SNLFlattenerInstanceTree* getTree() const;

    bool isRoot() const { return isRoot_; }
  private:
    bool  isRoot_;
    void* parent_;
};

}

#endif /* __SNL_FLATTENER_INSTANCE_TREE_NODE_H_ */