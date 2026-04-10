
#pragma once

#include "behavior_trees/behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
#include <cstdint>

namespace hydrogen::behavior_trees {

class CountDownNode : public BehaviorTreeNode, public IPipelineNodeStateful {
	DECLARE_PIPELINE_NODE(CountDownNode, BehaviorTreeNode);

	DECLARE_INPUT_PORT(should_succeed, bool, true);
	DECLARE_INPUT_PORT(count, uint32_t, 10);

	BEGIN_NODE_PORTS()
	INPUT_PORT(should_succeed)
	INPUT_PORT(count)
	END_NODE_PORTS()

	struct State : public IPipelineNodeState {
		uint32_t count = 0;
	};

protected:
	Result _execute(BehaviorTreeContext &p_context) const override {
		GET_STATE_V(State, FAILURE);
		if (unlikely(state->count == 0)) {
			state->count = GET_PORT(count);
		}

		state->count--;

		if (unlikely(state->count > 0)) {
			return RUNNING;
		} else {
			bool should_succeed = GET_PORT(should_succeed);
			if (likely(should_succeed)) {
				return SUCCESS;
			} else {
				return FAILURE;
			}
		}
	}

	void _halt(BehaviorTreeContext &p_context) const override {
		GET_STATE(State);
		state->count = 0;
	}

public:
	DEFINE_GET_PORTS();
	DEFINE_STATEFUL_FUNCS(State);
};
} //namespace hydrogen::behavior_trees
