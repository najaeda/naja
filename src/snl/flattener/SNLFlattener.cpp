#include "SNLFlattener.h"

#include "SNLDesign.h"

#include "SNLFlattenerInstanceTree.h"
#include "SNLFlattenerInstanceTreeNode.h"


namespace SNL {

void SNLFlattener::processTop(SNLFlattenerInstanceTree* tree, const SNLDesign* top) {
  auto root = tree->getRoot();
  for (auto instance: top->getInstances()) {
    root->addChild(instance);
  }
}

void SNLFlattener::process(const SNLDesign* top) {
  SNLFlattenerInstanceTree* tree = SNLFlattenerInstanceTree::create();
  processTop(tree, top);
}

}