#ifndef WAIT_NODES_HPP
#define WAIT_NODES_HPP

#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/core/memory.hpp"
#include "name_helpers.hpp"
#include "pipelines/node_interfaces.hpp"
#include "behavior_trees/behavior_tree_context.hpp"
#include "pipelines/pipeline_node.hpp"
#include "godot_cpp/classes/time.hpp"
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

template<typename T>
class WaitForNodeBase : public BehaviorTreeNode, public IPipelineNodeStateful {
    ABSTRACT_PIPELINE_NODE(WaitForNodeBase, BehaviorTreeNode);

protected:

    DEFINE_NAME_STATIC(duration);
    DEFINE_NAME_STATIC(delta);

    static constexpr T k_zero = {};
    static constexpr T k_one = k_zero + 1;

    struct WaitForNodeState : public IPipelineNodeState {
        std::enable_if_t<std::is_scalar_v<T>, T> time_remaining = {};
    };

    WaitForNodeState *_get_state(BehaviorTreeContext &p_context) const {
        WaitForNodeState *state = p_context.get_state<WaitForNodeState>(state_key());
        ERR_FAIL_NULL_V(state, nullptr);
        return nullptr;
    }

    void _reset_state(WaitForNodeState *p_state) const {
        ERR_FAIL_NULL(p_state);
        p_state->time_remaining = {};
    }

    Result _execute(BehaviorTreeContext &p_context) const override {
        WaitForNodeState *state = _get_state(p_context);

        if (state->time_remaining == k_zero) {
            state->time_remaining = _get_port<T>(p_context.blackboard(), duration_name());
            return RUNNING;
        }
        else if (likely(state->time_remaining > k_zero)) {
            T delta = _get_port<T>(p_context.blackboard(), delta_name());
            state->time_remaining -= delta;
            state->time_remaining = Math::max<T>(state->time_remaining, k_zero);
            if (likely(state->time_remaining > k_zero)) {
                return RUNNING;
            }
        }
        
        _reset_state(state);
        return SUCCESS;
    }

    void _halt(BehaviorTreeContext &p_context) const override {
        WaitForNodeState *state = _get_state(p_context);
        _reset_state(state);
    }

    WaitForNodeBase() = default;

public:

    ~WaitForNodeBase() override = default;

    virtual IPipelineNodeState * create_state() const override { return memnew(WaitForNodeState); }
    RID state_key() const override { return get_self(); }
};

class WaitForSecondsNode : public WaitForNodeBase<real_t> {
    DECLARE_PIPELINE_NODE(WaitForSecondsNode, WaitForNodeBase<real_t>);

protected:

    typedef real_t delta_type;
    typedef real_t duration_type;

    static constexpr real_t k_default_delta = 0.1;
    static constexpr real_t k_default_duration = 1.0;

    BEGIN_NODE_PORTS()
        INPUT_PORT(delta)
        INPUT_PORT(duration)
    END_NODE_PORTS()
    
public:
    DEFINE_GET_PORTS();
};

class WaitForMillisecondsNode : public WaitForNodeBase<uint64_t> {
    DECLARE_PIPELINE_NODE(WaitForMillisecondsNode, WaitForNodeBase<uint64_t>);

    typedef uint64_t delta_type;
    typedef uint64_t duration_type;

    static constexpr uint64_t k_default_delta = 0;
    static constexpr uint64_t k_default_duration = 1000;

    BEGIN_NODE_PORTS()
        INPUT_PORT(delta)
        INPUT_PORT(duration)
    END_NODE_PORTS()

public:
    DEFINE_GET_PORTS();
};

class WaitForTicksNode : public WaitForNodeBase<uint64_t> {
    DECLARE_PIPELINE_NODE(WaitForTicksNode, WaitForNodeBase<uint64_t>);

    typedef uint64_t duration_type;
    static constexpr uint64_t k_default_duration = 30;

    BEGIN_NODE_PORTS()
        INPUT_PORT(duration)
    END_NODE_PORTS()

protected:
    Result _execute(BehaviorTreeContext &p_context) const override {
        WaitForNodeState *state = _get_state(p_context);

        if (unlikely(state->time_remaining == k_zero)) {
            state->time_remaining = _get_port<uint32_t>(p_context.blackboard(), duration_name());
            return RUNNING;
        }
        else if (likely(state->time_remaining > k_one)) {
            state->time_remaining--;
            return RUNNING;
        }
        
        _reset_state(state);
        return SUCCESS;
    }

public:
    DEFINE_GET_PORTS();
};

class WaitForRealtimeSeconds : public BehaviorTreeNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(WaitForRealtimeSeconds, BehaviorTreeNode);

    DECLARE_INPUT_PORT(duration, double, 1.0);

    BEGIN_NODE_PORTS()
        INPUT_PORT(duration)
    END_NODE_PORTS()

    struct State : public IPipelineNodeState {
        double time_point = 0.0;
    };

protected:

    void _halt(BehaviorTreeContext &p_context) const override {
        State *state = p_context.get_state<State>(state_key());
        ERR_FAIL_NULL(state);
        state->time_point = 0.0;
    }

    Result _execute(BehaviorTreeContext &p_context) const override {
        State *state = p_context.get_state<State>(state_key());
        ERR_FAIL_NULL_V(state, FAILURE);

        if (unlikely(state->time_point == 0.0)) {
            double duration = _get_port<uint32_t>(p_context.blackboard(), duration_name());
            state->time_point = Time::get_singleton()->get_unix_time_from_system() + duration;
            return RUNNING;
        }
        else {
            double current_time = Time::get_singleton()->get_unix_time_from_system();
            if (likely(current_time < state->time_point)) {
                return RUNNING;
            }
        }

        state->time_point = 0.0;
        return SUCCESS;
    };

public:
    DEFINE_GET_PORTS();

    DEFINE_STATEFUL_FUNCS(State);
};

}

#endif