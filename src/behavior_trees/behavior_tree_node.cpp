
#include "behavior_tree_node.hpp"
#include "behavior_trees/behavior_tree_context.hpp"

namespace hydrogen::behavior_trees {


    BehaviorTreeNode::Result BehaviorTreeNode::execute(BehaviorTreeContext &p_context) const {

        p_context._node_enter(this);

        Result result = _execute(p_context);

        p_context._node_exit(this);

        return result;
    }

    void BehaviorTreeNode::halt(BehaviorTreeContext &p_context) const {

        p_context._node_halt_enter(this);

        _halt(p_context);

        p_context._node_halt_exit(this);
    }
}