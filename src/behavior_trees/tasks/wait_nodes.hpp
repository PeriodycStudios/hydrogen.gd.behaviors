#ifndef WAIT_NODES_HPP
#define WAIT_NODES_HPP

#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/math.hpp"
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

    virtual T _get_duration_default() const { return {}; }
    virtual T _get_delta_default() const { return {}; }

    Result _execute(BehaviorTreeContext &p_context) const override {
        GET_STATE_V(WaitForNodeState, FAILURE);

        if (state->time_remaining == k_zero) {
            state->time_remaining = _get_port<T>(p_context.blackboard(), duration_name(), _get_duration_default());
            return RUNNING;
        }
        else if (likely(state->time_remaining > k_zero)) {
            T delta = _get_port<T>(p_context.blackboard(), delta_name(), _get_delta_default());
            state->time_remaining -= delta;
            state->time_remaining = Math::max<T>(state->time_remaining, k_zero);
            if (likely(state->time_remaining > k_zero)) {
                return RUNNING;
            }
        }
        
        state->time_remaining = {};
        return SUCCESS;
    }

    void _halt(BehaviorTreeContext &p_context) const override {
        GET_STATE(WaitForNodeState);
        state->time_remaining = {};
    }

    WaitForNodeBase() = default;

public:

    ~WaitForNodeBase() override = default;

    DEFINE_STATEFUL_FUNCS(WaitForNodeState);
};

class WaitForSecondsNode : public WaitForNodeBase<real_t> {
    DECLARE_PIPELINE_NODE(WaitForSecondsNode, WaitForNodeBase<real_t>);

protected:

    typedef real_t delta_type;
    typedef real_t duration_type;

    static constexpr real_t k_default_delta = 0.1;
    static constexpr real_t k_default_duration = 1.0;

    real_t _get_duration_default() const override {
        return k_default_duration;
    }

    real_t _get_delta_default() const override {
        return k_default_delta;
    }

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

    uint64_t _get_duration_default() const override {
        return k_default_duration;
    }

    uint64_t _get_delta_default() const override {
        return k_default_delta;
    }

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
        GET_STATE_V(WaitForNodeState, FAILURE);

        if (unlikely(state->time_remaining == k_zero)) {
            state->time_remaining = GET_PORT(duration);
            return RUNNING;
        }
        else if (likely(state->time_remaining > k_one)) {
            state->time_remaining--;
            return RUNNING;
        }
        
        state->time_remaining = 0;
        return SUCCESS;
    }

public:
    DEFINE_GET_PORTS();
};

class WaitForRealtimeSeconds : public BehaviorTreeNode, public IPipelineNodeStateful {
    DECLARE_PIPELINE_NODE(WaitForRealtimeSeconds, BehaviorTreeNode);

    DECLARE_CONSTEXPR_INPUT_PORT(duration, double, 1.0);

    BEGIN_NODE_PORTS()
        INPUT_PORT(duration)
    END_NODE_PORTS()

    struct State : public IPipelineNodeState {
        double time_point = 0.0;
    };

protected:

    void _halt(BehaviorTreeContext &p_context) const override {
        GET_STATE(State);
        state->time_point = 0.0;
    }

    Result _execute(BehaviorTreeContext &p_context) const override {
        GET_STATE_V(State, FAILURE);

        if (unlikely(state->time_point == 0.0)) {
            double duration = GET_PORT(duration);
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