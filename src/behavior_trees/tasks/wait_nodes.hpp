#ifndef WAIT_NODES_HPP
#define WAIT_NODES_HPP

#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/core/memory.hpp"
#include "pipelines/node_interfaces.hpp"
#include "behavior_trees/behavior_tree_context.hpp"
#include "pipelines/pipeline_node.hpp"
#include "godot_cpp/classes/time.hpp"
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace hydrogen::behavior_trees {

using namespace pipelines;

template<typename T>
class WaitForNodeBase : public BehaviorTreeNode, public IPipelineNodeStateful {
protected:

    static constexpr T k_zero = {};
    static constexpr T k_one = k_zero + 1;

    DECLARE_PORT_NAME(delta);
    DECLARE_PORT_NAME(duration);

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
            state->time_remaining = p_context.get<T>(duration_name());
            return RUNNING;
        }
        else if (likely(state->time_remaining > k_zero)) {
            T delta = p_context.get<T>(delta_name());
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
    DECLARE_PIPELINE_NODE(WaitForSecondsNode);

    BEGIN_NODE_PORTS()
        INPUT_PORT(delta, real_t, 0.1)
        INPUT_PORT(duration, real_t, 1.0)
    END_NODE_PORTS()

public:
    DECLARE_GET_PORTS();

    WaitForSecondsNode() = default;
    ~WaitForSecondsNode() override = default;
};

class WaitForMillisecondsNode : public WaitForNodeBase<uint64_t> {
    DECLARE_PIPELINE_NODE(WaitForMillisecondsNode)

    BEGIN_NODE_PORTS()
        INPUT_PORT(delta, uint64_t, 0)
        INPUT_PORT(duration, uint64_t, 1000)
    END_NODE_PORTS()

public:
    DECLARE_GET_PORTS();

    WaitForMillisecondsNode() = default;
    ~WaitForMillisecondsNode() override = default;
};

class WaitForTicksNode : public WaitForNodeBase<uint64_t> {
    DECLARE_PIPELINE_NODE(WaitForTicksNode);

    BEGIN_NODE_PORTS()
        INPUT_PORT(duration, uint64_t, 30)
    END_NODE_PORTS()

protected:
    Result _execute(BehaviorTreeContext &p_context) const override {
        WaitForNodeState *state = _get_state(p_context);

        if (unlikely(state->time_remaining == k_zero)) {
            state->time_remaining = p_context.get<uint32_t>(duration_name());
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
    DECLARE_GET_PORTS();

    WaitForTicksNode() = default;
    ~WaitForTicksNode() override = default;
};

class WaitForRealtimeSeconds : public BehaviorTreeNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(WaitForMillisecondsNode);

    DECLARE_PORT_NAME(duration);

    BEGIN_NODE_PORTS()
        INPUT_PORT(duration, double, 1.0)
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
            double duration = p_context.get<double>(duration_name());
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
    DECLARE_GET_PORTS();

    WaitForRealtimeSeconds() = default;
    ~WaitForRealtimeSeconds() override = default;

    IPipelineNodeState* create_state() const override { return memnew(State); }
    RID state_key() const override { return get_self(); }
};

}

#endif