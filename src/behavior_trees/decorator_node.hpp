//
// Created by tkey on 7/29/25.
//

#ifndef DECORATOR_NODE_HPP
#define DECORATOR_NODE_HPP
#include "behavior_tree_node.hpp"
#include "../pipelines/node_wrapper.hpp"

namespace hydrogen::behavior_trees {

using namespace pipelines;

class DecoratorNode : public BehaviorTreeNode, public PipelineNodeWrapper<BehaviorTreeNode> {
protected:
	DecoratorNode() = default;
public:
	~DecoratorNode() override = default;
};
} // hydrogen

#endif //DECORATOR_NODE_HPP
