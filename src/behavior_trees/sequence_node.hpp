//
// Created by tkey on 7/29/25.
//

#ifndef SEQUENCE_NODE_HPP
#define SEQUENCE_NODE_HPP
#include "../../godot-cpp/include/godot_cpp/templates/vector.hpp"
#include "behavior_trees.hpp"
#include "composite_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class SequenceNode final : public CompositeNode {

protected:
	SequenceNode() = default;

	Result _run(Blackboard * p_blackboard) const override {

		// Need to take into account children utilizing RUNNING
		return FAILURE;
		// const Vector<TaskNode*> &children = get_children();
		// for (const TaskNode *child : children) {
		// 	const Result child_result = child->run(p_blackboard);
		// 	if (child_result != SUCCESS) {
		// 		return child_result;
		// 	}
		// }
		//
		// return SUCCESS;
	}

public:
	~SequenceNode() override = default;
};

// TODO: NonDeterministic Selector

} // hydrogen

#endif //SEQUENCE_NODE_HPP
