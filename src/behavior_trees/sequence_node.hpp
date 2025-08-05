//
// Created by tkey on 7/29/25.
//

#ifndef SEQUENCE_NODE_HPP
#define SEQUENCE_NODE_HPP
#include "composite_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

struct SequenceNode final : public CompositeNode, pipelines::IPipelineNodeStateful {

	// struct SequenceNodeState : public pipelines::IPipelineNodeState {
	// 	int current_child_index = 0;
	// };
	// Need to take into account resuming a paused node.
	// const Vector<BehaviorTreeNode*> &children = get_children();
	// for (const BehaviorTreeNode *child : children) {
	// 	const Result child_result = child->run(p_blackboard);
	// 	if (child_result != SUCCESS) {
	// 		return child_result;
	// 	}
	// }
	//
	// return SUCCESS;
	SequenceNode() = default;
	~SequenceNode() override = default;

	pipelines::IPipelineNodeState *create_state() const override { return nullptr; }

	Result execute(BehaviorTreeContext &p_context) const override { return FAILURE; }
	void halt(BehaviorTreeContext &p_context) const override { }
};

// TODO: NonDeterministic Selector

} // hydrogen

#endif //SEQUENCE_NODE_HPP
