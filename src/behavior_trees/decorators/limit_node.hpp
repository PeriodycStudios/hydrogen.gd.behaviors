#ifndef LIMIT_NODE_HPP
#define LIMIT_NODE_HPP

#include "../decorator_node.hpp"
#include "../behavior_tree_context.hpp"
#include "../behavior_tree_node.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "pipelines/node_interfaces.hpp"
#include <cstdint>

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

class LimitNode : public DecoratorNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(LimitNode, DecoratorNode);

    DECLARE_INPUT_PORT(executeLimit, uint32_t, 1);

    BEGIN_NODE_PORTS()
        INPUT_PORT(executeLimit)
    END_NODE_PORTS()

    struct LimitNodeState : public IPipelineNodeState {
        uint32_t execute_counter = 0;
    };

protected:

    Result _execute(BehaviorTreeContext &p_context) const override {
        ERR_FAIL_NULL_V(_decorated_node, FAILURE);

        LimitNodeState *state = p_context.get_state<LimitNodeState>(state_key());
        ERR_FAIL_NULL_V(state, FAILURE);

        const uint32_t executeLimit = _get_port<uint32_t>(p_context.blackboard(), executeLimit_name());
        while (state->execute_counter < executeLimit) {
            Result result = _decorated_node->execute(p_context);
            switch (result) {
            case BehaviorTreeNode::RUNNING:
                return RUNNING;
            default:
                state->execute_counter++;
                return result;
            }
        }

        return FAILURE;
    }

    void _halt(BehaviorTreeContext &p_context) const override {
        LimitNodeState *state = p_context.get_state<LimitNodeState>(state_key());
        ERR_FAIL_NULL(state);
        state->execute_counter = 0;
        DecoratorNode::_halt(p_context);
    }

public:
    DEFINE_GET_PORTS();

    DEFINE_STATEFUL_FUNCS(LimitNodeState);
};
}

#endif