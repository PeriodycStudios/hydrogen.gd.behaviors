//
// Created by tkey on 7/29/25.
//

#ifndef SELECTOR_NODES_HPP
#define SELECTOR_NODES_HPP
#include "composite_node.hpp"

namespace hydrogen::behavior_trees {

class SelectorNode : public CompositeNode, public pipelines::IPipelineNodeStateful {
	//
	// struct SelectorNodeState : public pipelines::IPipelineNodeState{
	// 	int current_child_index = 0;
	// };
	//
/*
 *children : Task[]
 *run() -> Result {
 * foreach child in children:
 *   if c.run() return true
 * return false
*/
public:
	SelectorNode() = default;
	~SelectorNode() override = default;

	pipelines::IPipelineNodeState *create_state() const override { return nullptr; }

	Result execute(BehaviorTreeContext &p_context) const override { return FAILURE; }

	void halt(BehaviorTreeContext &p_context) const override {}
};

// TODO: Nondeterministic sequence

} // hydrogen

#endif //SELECTOR_NODES_HPP
