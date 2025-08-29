#ifndef PARALLEL_NODE_HPP
#define PARALLEL_NODE_HPP

#include "behavior_trees/behavior_tree_node.hpp"
#include "composite_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/templates/local_vector.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "pipelines/node_interfaces.hpp"
#include <cstdint>
#include <optional>

namespace hydrogen::behavior_trees {

class ParallelNodeBase : public CompositeNode {
    ABSTRACT_PIPELINE_NODE(ParallelNodeBase, CompositeNode);
protected:
    struct ParallelNodeState : public CompositeNodeState {
        Vector<const BehaviorTreeNode *> running_nodes = {};
        std::optional<Result> result = {};

        ParallelNodeState() = default;
        ~ParallelNodeState() override = default;
    };

    ParallelNodeState *_get_parallel_state(BehaviorTreeContext &p_context) const {
        return p_context.get_state<ParallelNodeState>(state_key());
    }

    static void _halt_running_children(BehaviorTreeContext &p_context, ParallelNodeState *state) {
        for( const BehaviorTreeNode *child : state->running_nodes) {
            child->halt(p_context);
        }
        state->running_nodes.clear();
    }

    void _halt(BehaviorTreeContext &p_context) const override {
        ParallelNodeState *state = _get_parallel_state(p_context);
        ERR_FAIL_NULL(state);

        state->result.reset();
        _halt_running_children(p_context, state);
    }

    ParallelNodeState *_get_and_prepare_state(BehaviorTreeContext &p_context) const {
        ParallelNodeState *state = _get_parallel_state(p_context);
        ERR_FAIL_NULL_V(state, nullptr);

        if (unlikely(state->running_nodes.size() == 0)) {
            state->result.reset();
            state->running_nodes.append_array(_children);   
        }

        return state;
    }

    virtual bool _inner_execute(BehaviorTreeContext &p_context, ParallelNodeState *p_state, TightLocalVector<int32_t> &p_remove_idx) const = 0;

    Result _execute(BehaviorTreeContext &p_context) const override {
        ParallelNodeState *state = _get_and_prepare_state(p_context);
        ERR_FAIL_NULL_V(state, FAILURE);

        TightLocalVector<int32_t> remove_idx = {};
        remove_idx.reserve(state->running_nodes.size());

        bool halt_children = _inner_execute(p_context, state, remove_idx);

        if (likely(!remove_idx.is_empty())) {
            for(int32_t idx : remove_idx) {
                state->running_nodes.remove_at(idx);
            }
        }

        if (unlikely(halt_children)) {
            _halt_running_children(p_context, state);
        }

        if (likely(state->running_nodes.size() > 0)) {
            return RUNNING;
        }

        Result final_result = state->result.value();
        state->result.reset();

        return final_result;
    }

    ParallelNodeBase() = default;

public:
    ~ParallelNodeBase() = default;
    ParallelNodeState *create_state() const override { return memnew(ParallelNodeState); }
};

class ParallelSequenceNode : public ParallelNodeBase {
    DECLARE_PIPELINE_NODE(ParallelSequenceNode, ParallelNodeBase);

protected:

    bool _inner_execute(BehaviorTreeContext &p_context, ParallelNodeState *p_state, TightLocalVector<int32_t> &p_remove_idx) const override {
        for (int32_t idx = 0; idx < p_state->running_nodes.size(); ++idx) {
            Result result = p_state->running_nodes[idx]->execute(p_context);

            if (likely(result == RUNNING)) {
                continue;
            }
            else {
                p_remove_idx.push_back(idx);
                p_state->result = result;

                if (unlikely(result == FAILURE)) {
                    return true;
                }
            }
        }

        return false;
    }
};

class ParallelSelectorNode : public ParallelNodeBase {
    DECLARE_PIPELINE_NODE(ParallelSelectorNode, ParallelNodeBase);

protected:

    bool _inner_execute(BehaviorTreeContext &p_context, ParallelNodeState *p_state, TightLocalVector<int32_t> &p_remove_idx) const override {
        for (int32_t idx = 0; idx < p_state->running_nodes.size(); ++idx) {
            Result result = p_state->running_nodes[idx]->execute(p_context);

            if (likely(result == RUNNING)) {
                continue;
            }
            else {
                p_remove_idx.push_back(idx);
                p_state->result = result;

                if (unlikely(result == SUCCESS)) {
                    return true;
                }
            }
        }

        return false;
    }
};
}

#endif