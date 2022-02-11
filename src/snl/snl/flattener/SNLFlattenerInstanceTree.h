#ifndef __SNL_FLATTENER_INSTANCE_TREE_H_
#define __SNL_FLATTENER_INSTANCE_TREE_H_

namespace naja { namespace SNL {

class SNLFlattenerInstanceTreeNode;

class SNLFlattenerInstanceTree {
  public:
    SNLFlattenerInstanceTree(const SNLFlattenerInstanceTree&) = delete;
    SNLFlattenerInstanceTree(const SNLFlattenerInstanceTree&&) = delete;
    static SNLFlattenerInstanceTree* create();
    void destroy();

    SNLFlattenerInstanceTreeNode* getRoot() const { return root_; }
  private:
    SNLFlattenerInstanceTree() = default;
    ~SNLFlattenerInstanceTree();

    SNLFlattenerInstanceTreeNode* root_;
};

}} // namespace SNL // namespace naja

#endif /* __SNL_FLATTENER_INSTANCE_TREE_H_ */