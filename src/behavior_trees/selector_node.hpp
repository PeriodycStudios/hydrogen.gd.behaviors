//
// Created by tkey on 7/29/25.
//

#ifndef SELECTOR_NODES_HPP
#define SELECTOR_NODES_HPP

#include "../../godot-cpp/include/godot_cpp/templates/vector.hpp"
#include "behavior_trees.hpp"
#include "composite_node.hpp"

#include <godot_cpp/templates/vector.hpp>

namespace hydrogen::behavior_trees {

using namespace godot;

class SelectorNode : public CompositeNode {
/*
 *children : Task[]
 *run() -> Result {
 * foreach child in children:
 *   if c.run() return true
 * return false
*/

protected:
	Result _run(Blackboard *) const override {
		return FAILURE;
	}

public:
	SelectorNode() = default;
	~SelectorNode() override = default;
};

// TODO: Nondeterministic sequence

} // hydrogen

#endif //SELECTOR_NODES_HPP
