//
// Created by tkey on 7/31/25.
//

#ifndef BEHAVIOR_TREE_GRAPH_HPP
#define BEHAVIOR_TREE_GRAPH_HPP

#include "../pipeline_graph.hpp"
#include "behavior_tree_node.hpp"

namespace hydrogen::behavior_trees {
using namespace hydrogen::pipelines;

class BehaviorTreeGraph : public PipelineGraph<BehaviorTreeNode> {

public:
	BehaviorTreeGraph() = default;
	~BehaviorTreeGraph() override = default;

};

} // hydrogen

#endif //BEHAVIOR_TREE_GRAPH_HPP
