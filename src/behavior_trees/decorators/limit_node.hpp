#ifndef LIMIT_NODE_HPP
#define LIMIT_NODE_HPP

#include "../decorator_node.hpp"
#include "../behavior_tree_context.hpp"
#include "../behavior_tree_node.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
#include <cstdint>

namespace hydrogen::behavior_trees {

class LimitNode : public DecoratorNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(LimitNode)

    DECLARE_PORT_NAME(executeLimit);

    BEGIN_NODE_PORTS()
        INPUT_PORT(executeLimit, uint32_t, 1)
    END_NODE_PORTS()

    struct LimitNodeState : public IPipelineNodeState {
        uint32_t execute_counter = 0;
    };

protected:

    Result _execute(BehaviorTreeContext &p_context) const override {
        ERR_FAIL_NULL_V(_decorated_node, FAILURE);

        LimitNodeState *state = p_context.get_state<LimitNodeState>(state_key());
        ERR_FAIL_NULL_V(state, FAILURE);

        const uint32_t executeLimit = p_context.get<uint32_t>(executeLimit_name());
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
    DECLARE_GET_PORTS();

    LimitNode() = default;
    ~LimitNode() override = default;

    RID state_key() const override { return get_self(); }

    IPipelineNodeState *create_state() const override { return memnew(LimitNodeState); }
};
}

#endif