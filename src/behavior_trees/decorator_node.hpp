//
// Created by tkey on 7/29/25.
//

#ifndef DECORATOR_NODE_HPP
#define DECORATOR_NODE_HPP
#include "behavior_tree_node.hpp"

namespace hydrogen::behavior_trees {

class DecoratorNode : public BehaviorTreeNode, public pipelines::ChildNodeWrapper<BehaviorTreeNode> {
protected:
	DecoratorNode() = default;
public:
	~DecoratorNode() override = default;
};
} // hydrogen

#endif //DECORATOR_NODE_HPP
