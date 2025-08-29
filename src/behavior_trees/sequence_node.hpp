//
// Created by tkey on 7/29/25.
//

#ifndef SEQUENCE_NODE_HPP
#define SEQUENCE_NODE_HPP
#include "behavior_trees/behavior_tree_node.hpp"
#include "composite_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "pipelines/node_interfaces.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class SequenceNode : public CompositeNode {
	DECLARE_PIPELINE_NODE(SequenceNode, CompositeNode);

protected:
	Result _execute(BehaviorTreeContext &p_context) const override { 
		CompositeNodeState * state = _get_state(p_context);
		if (unlikely(state == nullptr)) {
			return FAILURE;
		}

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
					_reset_state(state);
					return FAILURE;
				default:
					_reset_state(state);
					unknown_result_handler(result);
					return FAILURE;
			}
		}

		_reset_state(state);
		return SUCCESS;
	}
};

// TODO: NonDeterministic Selector

} // hydrogen

#endif //SEQUENCE_NODE_HPP
