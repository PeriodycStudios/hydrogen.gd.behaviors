
#pragma once

#include "behavior_trees/behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/math.hpp"
#include "name_helpers.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
#include <cstdint>
#include <type_traits>

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

template <typename T>
class WaitForNodeBase : public BehaviorTreeNode, public IPipelineNodeStateful {
	ABSTRACT_PIPELINE_NODE(WaitForNodeBase, BehaviorTreeNode);

protected:
	DEFINE_NAME_STATIC(duration);
	DEFINE_NAME_STATIC(delta);

	static constexpr T k_zero = {};
	static constexpr T k_one = k_zero + 1;

	virtual T _get_default_duration() const { return k_zero; }
	virtual T _get_default_delta() const { return k_zero; }

	struct WaitForNodeState : public IPipelineNodeState {
		std::enable_if_t<std::is_scalar_v<T>, T> time_remaining = {};
	};

	Result _execute(BehaviorTreeContext &p_context) const override {
		GET_STATE_V(WaitForNodeState, FAILURE);

		if (state->time_remaining == k_zero) {
			state->time_remaining = _get_port<T>(p_context.blackboard(), duration_name(), _get_default_duration());
		}

		T delta = _get_port<T>(p_context.blackboard(), delta_name(), _get_default_delta());
		state->time_remaining -= delta;
		state->time_remaining = Math::max<T>(state->time_remaining, k_zero);
		if (likely(state->time_remaining > k_zero)) {
			return RUNNING;
		} else {
			state->time_remaining = {};
			return SUCCESS;
		}
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

	_FORCE_INLINE_ constexpr static real_t get_default_delta() {
		return k_default_delta;
	}

	_FORCE_INLINE_ constexpr static real_t get_default_duration() {
		return k_default_duration;
	}

	real_t _get_default_delta() const override {
		return get_default_delta();
	}

	real_t _get_default_duration() const override {
		return get_default_duration();
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

	_FORCE_INLINE_ constexpr static uint64_t get_default_duration() {
		return k_default_duration;
	}

	_FORCE_INLINE_ constexpr static uint64_t get_default_delta() {
		return k_default_delta;
	}

	uint64_t _get_default_duration() const override {
		return get_default_duration();
	}

	uint64_t _get_default_delta() const override {
		return get_default_delta();
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

	_FORCE_INLINE_ constexpr static uint64_t get_default_duration() {
		return k_default_duration;
	}

	BEGIN_NODE_PORTS()
	INPUT_PORT(duration)
	END_NODE_PORTS()

protected:
	Result _execute(BehaviorTreeContext &p_context) const override {
		GET_STATE_V(WaitForNodeState, FAILURE);

		if (unlikely(state->time_remaining == k_zero)) {
			state->time_remaining = GET_PORT(duration);
		}

		state->time_remaining--;

		if (likely(state->time_remaining > k_one)) {
			return RUNNING;
		} else {
			state->time_remaining = 0;
			return SUCCESS;
		}
	}

public:
	DEFINE_GET_PORTS();
};

class WaitForRealtimeSeconds : public BehaviorTreeNode, public IPipelineNodeStateful {
	DECLARE_PIPELINE_NODE(WaitForRealtimeSeconds, BehaviorTreeNode);

	DECLARE_INPUT_PORT(duration, real_t, 1.0);

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
			real_t duration = GET_PORT(duration);
			state->time_point = Time::get_singleton()->get_unix_time_from_system() + duration;
		}

		double current_time = Time::get_singleton()->get_unix_time_from_system();
		if (likely(current_time < state->time_point)) {
			return RUNNING;
		} else {
			state->time_point = 0.0;
			return SUCCESS;
		}
	};

public:
	DEFINE_GET_PORTS();

	DEFINE_STATEFUL_FUNCS(State);
};

} //namespace hydrogen::behavior_trees
