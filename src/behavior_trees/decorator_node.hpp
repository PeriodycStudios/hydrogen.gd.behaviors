//
// Created by tkey on 7/29/25.
//

#ifndef DECORATOR_NODE_HPP
#define DECORATOR_NODE_HPP
#include "behavior_trees.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class DecoratorNode : public TaskNode, public pipelines::ChildNodeWrapper<TaskNode> {

protected:
	DecoratorNode() = default;
public:
	~DecoratorNode() override = default;
};
} // hydrogen

#endif //DECORATOR_NODE_HPP
