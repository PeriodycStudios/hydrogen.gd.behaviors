//
// Created by tkey on 7/29/25.
//

#ifndef SEQUENCE_NODE_HPP
#define SEQUENCE_NODE_HPP
#include "composite_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

struct SequenceNode final : public CompositeNode {

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

	Result run(Blackboard *p_blackboard) const override { return FAILURE; }
};

// TODO: NonDeterministic Selector

} // hydrogen

#endif //SEQUENCE_NODE_HPP
