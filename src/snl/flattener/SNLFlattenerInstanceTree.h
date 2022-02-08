#ifndef __SNL_FLATTENER_INSTANCE_TREE_H_
#define __SNL_FLATTENER_INSTANCE_TREE_H_

namespace SNL {

class SNLFlattenerInstanceTreeNode;

class SNLFlattenerInstanceTree {
  public:
    SNLFlattenerInstanceTree(const SNLFlattenerInstanceTree&) = delete;
    SNLFlattenerInstanceTree(const SNLFlattenerInstanceTree&&) = delete;
    static SNLFlattenerInstanceTree* create();

    SNLFlattenerInstanceTreeNode* getRoot() const { return root_; }
  private:
    SNLFlattenerInstanceTree() = default;

    SNLFlattenerInstanceTreeNode* root_;
};

}

#endif /* __SNL_FLATTENER_INSTANCE_TREE_H_ */