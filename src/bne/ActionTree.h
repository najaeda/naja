#include "actions.h"
#include "bne.h"
#include "SNLID.h"

using namespace naja::SNL;

//simple tree structure
class ActionTreeNode {
public:
    ActionTreeNode(const BNE::ActionID& action, SNLID::DesignObjectID instance, SNLID snlid) : 
        action_(action), instance_(instance), snlid_(snlid) {}
    void addChild(const ActionTreeNode& child) { children_.push_back(child); }
private:
    BNE::ActionID action_;
    SNLID::DesignObjectID instance_;
    SNLID snlid_;
    std::vector<ActionTreeNode> children_;
};
