#ifndef LIMIT_NODE_HPP
#define LIMIT_NODE_HPP

#include "../decorator_node.hpp"
#include "../behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/memory.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
#include <cstdint>

namespace hydrogen::behavior_trees {

class LimitNode : public DecoratorNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(LimitNode)

    DECLARE_PORT_NAME(runLimit);

    BEGIN_NODE_PORTS()
        INPUT_PORT(runLimit, uint32_t, 1)
    END_NODE_PORTS()

    struct LimitNodeState : public IPipelineNodeState {
        uint32_t runCounter = 0;
    };

protected:

    Result _execute(BehaviorTreeContext &p_context) const override {
        if (unlikely(_decorated_node == nullptr)) {
            return FAILURE;
        }

        LimitNodeState *state = p_context.get_state<LimitNodeState>(state_key());
        const uint32_t runLimit = p_context.get<uint32_t>(runLimit_name());
        while (state->runCounter < runLimit) {
            Result result = _decorated_node->execute(p_context);
            switch (result) {
            case BehaviorTreeNode::RUNNING:
                return RUNNING;
            default:
                state->runCounter++;
                return result;
            }
        }

        return FAILURE;
    }

    void _halt(BehaviorTreeContext &p_context) const override {
        p_context.get_state<LimitNodeState>(state_key())->runCounter = 0;
        DecoratorNode::_halt(p_context);
    }

public:
    DECLARE_GET_PORTS();

    RID state_key() const override { return get_self(); }

    IPipelineNodeState *create_state() const override { return memnew(LimitNodeState); }
};
}

#endif