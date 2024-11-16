#ifndef __DESIGN_TREE_VIEW_H_
#define __DESIGN_TREE_VIEW_H_

namespace naja::SNL {
  class SNLDesign;
}

class DesignTreeView {
  public:
    static void render(const naja::SNL::SNLDesign* top);
};

#endif /* __DESIGN_TREE_VIEW_H_ */