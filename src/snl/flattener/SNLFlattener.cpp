#include "SNLFlattener.h"

#include "SNLDesign.h"

#include "SNLFlattenerInstanceTree.h"

namespace SNL {

void SNLFlattener::process(const SNLDesign* top) {
  SNLFlattenerInstanceTree* tree = SNLFlattenerInstanceTree::create();
  auto root = tree->getRoot();
  for (auto instance: top->getInstances()) {
    root->addChild(instance->getID());
  }
}

}