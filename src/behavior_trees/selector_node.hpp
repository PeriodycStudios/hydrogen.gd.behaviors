//
// Created by tkey on 7/29/25.
//

#ifndef SELECTOR_NODES_HPP
#define SELECTOR_NODES_HPP
#include "behavior_trees/behavior_tree_node.hpp"
#include "composite_node.hpp"
#include "behavior_tree_context.hpp"
#include "godot_cpp/core/error_macros.hpp"

namespace hydrogen::behavior_trees {

class SelectorNode : public CompositeNode {
	DECLARE_PIPELINE_NODE(SelectorNode, CompositeNode);

protected:

	Result _execute(BehaviorTreeContext &p_context) const override {
		CompositeNodeState *state = _get_state(p_context);
		ERR_FAIL_NULL_V(state, FAILURE);

		while (state->current_child_index < child_count()) {
			const BehaviorTreeNode *child = _children.get(state->current_child_index);
			Result result = child->execute(p_context);
			switch (result) {
				case BehaviorTreeNode::RUNNING:
					return RUNNING;
				case BehaviorTreeNode::SUCCESS:
					_reset_state(state);
					return SUCCESS;
				case BehaviorTreeNode::FAILURE:
					state->current_child_index++;
					continue;
				default:
					_reset_state(state);
					unknown_result_handler(result);
					return FAILURE;
			}
		}

		_reset_state(state);
		return FAILURE;
	}
};

// TODO: Nondeterministic selector

} // hydrogen

#endif //SELECTOR_NODES_HPP
