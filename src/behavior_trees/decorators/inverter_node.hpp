#ifndef INVERTER_NODE_HPP
#define INVERTER_NODE_HPP

#include "../decorator_node.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "pipelines/pipeline_node.hpp"

namespace hydrogen::behavior_trees {
class InverterNode : public DecoratorNode {
    DECLARE_PIPELINE_NODE(InverterNode);

protected:
    Result _execute(BehaviorTreeContext &p_context) const override {
        ERR_FAIL_NULL_V(_decorated_node, SUCCESS);

        Result result = _decorated_node->execute(p_context);
        switch (result) {
        case BehaviorTreeNode::SUCCESS:
            return FAILURE;
        case BehaviorTreeNode::RUNNING:
            return RUNNING;
        case BehaviorTreeNode::FAILURE:
            return SUCCESS;
        }
    }
public:
    InverterNode() = default;
    ~InverterNode() override = default;
};
}

#endif