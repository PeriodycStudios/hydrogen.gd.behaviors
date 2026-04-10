//
// Created by tkey on 7/29/25.
//

#pragma once

#include "behavior_trees/behavior_tree_node.hpp"
#include "composite_node.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class SequenceNode : public CompositeNode {
	DECLARE_PIPELINE_NODE(SequenceNode, CompositeNode);

protected:
	Result _execute(BehaviorTreeContext &p_context) const override {
		GET_STATE_V(CompositeNodeState, FAILURE);

		while (state->current_child_index < child_count()) {
			const BehaviorTreeNode *child = _children.get(state->current_child_index);

			Result result = child->execute(p_context);
			switch (result) {
				case BehaviorTreeNode::RUNNING:
					return RUNNING;
				case BehaviorTreeNode::SUCCESS:
					state->current_child_index++;
					continue;
				case BehaviorTreeNode::FAILURE:
					state->current_child_index = 0;
					return FAILURE;
				default:
					state->current_child_index = 0;
					unknown_result_handler(result);
					return FAILURE;
			}
		}

		state->current_child_index = 0;
		return SUCCESS;
	}
};

// TODO: NonDeterministic Selector

} //namespace hydrogen::behavior_trees
