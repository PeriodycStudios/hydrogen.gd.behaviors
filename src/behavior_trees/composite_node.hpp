//
// Created by tkey on 7/29/25.
//

#ifndef CONTAINER_NODE_HPP
#define CONTAINER_NODE_HPP
#include "behavior_tree_node.hpp"

namespace hydrogen::behavior_trees {

class CompositeNode : public BehaviorTreeNode, public pipelines::ChildNodeContainer<BehaviorTreeNode> {
protected:
	CompositeNode() = default;
public:
	~CompositeNode() override = default;
};

} // hydrogen

#endif //CONTAINER_NODE_HPP
