//
// Created by tkey on 7/29/25.
//

#ifndef SELECTOR_NODES_HPP
#define SELECTOR_NODES_HPP
#include "behavior_trees/behavior_tree_node.hpp"
#include "composite_node.hpp"
#include "behavior_tree_context.hpp"
#include "pipelines/pipeline_node.hpp"

namespace hydrogen::behavior_trees {

class SelectorNode : public CompositeNode {
	DECLARE_PIPELINE_NODE(SelectorNode);
protected:

	Result _execute(BehaviorTreeContext &p_context) const override {
		CompositeNodeState *state = p_context.get_state<CompositeNodeState>(state_key());
		if (unlikely(state == nullptr)) {
			return FAILURE;
		}

		try_init_state(state);

		while (state->current_child_index < get_node_count()) {
			const BehaviorTreeNode *child = _children.get(state->current_child_index);
			Result result = child->execute(p_context);
			switch (result) {
				case BehaviorTreeNode::RUNNING:
					return RUNNING;
				case BehaviorTreeNode::SUCCESS:
					state->current_child_index = -1;
					return SUCCESS;
				case BehaviorTreeNode::FAILURE:
					state->current_child_index++;
					continue;
				default:
					unknown_result_handler(result);
					return FAILURE;
			}
		}

		return FAILURE;
	}

public:
	SelectorNode() = default;
	~SelectorNode() override = default;
};

// TODO: Nondeterministic selector

} // hydrogen

#endif //SELECTOR_NODES_HPP
