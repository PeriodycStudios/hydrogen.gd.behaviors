//
// Created by tkey on 7/29/25.
//

#ifndef CONTAINER_NODE_HPP
#define CONTAINER_NODE_HPP
#include "behavior_trees.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class CompositeNode : public TaskNode, public pipelines::ChildNodeContainer<TaskNode> {
protected:
	CompositeNode() = default;
public:
	~CompositeNode() override = default;
};

} // hydrogen

#endif //CONTAINER_NODE_HPP
