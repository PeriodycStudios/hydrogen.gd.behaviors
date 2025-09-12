#ifndef INTERRUPTER_NODE_HPP
#define INTERRUPTER_NODE_HPP

#include "../decorator_node.hpp"
#include "behavior_trees/behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
#include <optional>

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

class InterrupterNode : public DecoratorNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(InterrupterNode, DecoratorNode);

    struct State : public IPipelineNodeState {
        bool is_running = false;
        std::optional<Result> override_result = {};
    };

    static void _reset_state(State *p_state) {
        p_state->is_running = false;
        p_state->override_result.reset();
    }

protected:

    void _halt(BehaviorTreeContext &p_context) const override {
        GET_STATE(State);
        if (likely(state->is_running)) {
            _decorated_node->halt(p_context);
        }
        _reset_state(state);
    }

    Result _execute(BehaviorTreeContext &p_context) const override {
        DECORATOR_FAILURE_IF_NULL();
        GET_STATE_V(State, FAILURE);

        if (state->override_result.has_value()) {
            Result result = state->override_result.value();
            if (state->is_running) {
                _decorated_node->halt(p_context);
            }
            _reset_state(state);
            return result;
        }

        if (unlikely(!state->is_running)) {
            state->is_running = true;
        }

        Result result = _decorated_node->execute(p_context);
        if (likely(result == RUNNING)) {
            return RUNNING;
        }
        else {
            _reset_state(state);
            return result;
        }
    }

public:

    DEFINE_STATEFUL_FUNCS(State);

    void set_result(BehaviorTreeContext &p_context, Result p_override) const {
        GET_STATE(State);
        state->override_result = p_override;
    }
};
}

#endif