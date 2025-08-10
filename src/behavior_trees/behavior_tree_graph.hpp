//
// Created by tkey on 7/31/25.
//

#ifndef BEHAVIOR_TREE_GRAPH_HPP
#define BEHAVIOR_TREE_GRAPH_HPP

#include "../pipelines/pipeline_graph.hpp"
#include "behavior_tree_node.hpp"
#include "godot_cpp/variant/rid.hpp"
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace hydrogen::behavior_trees {
using namespace hydrogen::pipelines;

class BehaviorTreeGraph : public PipelineGraph<BehaviorTreeNode> {

public:
	BehaviorTreeGraph() = default;
	~BehaviorTreeGraph() override = default;

	RID create_node(const StringName &p_node_type_name) override { return {}; }
	bool destroy_node(RID p_node_id) override { return false; }
};

} // hydrogen

#endif //BEHAVIOR_TREE_GRAPH_HPP
