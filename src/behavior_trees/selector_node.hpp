//
// Created by tkey on 7/29/25.
//

#ifndef SELECTOR_NODES_HPP
#define SELECTOR_NODES_HPP
#include "composite_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

struct SelectorNode : public CompositeNode {
/*
 *children : Task[]
 *run() -> Result {
 * foreach child in children:
 *   if c.run() return true
 * return false
*/
	SelectorNode() = default;
	~SelectorNode() override = default;

	Result run(Blackboard *p_blackboard) const override { return FAILURE; }
};

// TODO: Nondeterministic sequence

} // hydrogen

#endif //SELECTOR_NODES_HPP
