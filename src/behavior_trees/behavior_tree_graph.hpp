//
// Created by tkey on 7/31/25.
//

#ifndef BEHAVIOR_TREE_GRAPH_HPP
#define BEHAVIOR_TREE_GRAPH_HPP

#include "../pipelines/pipeline_graph.hpp"
#include "behavior_tree_node.hpp"
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace hydrogen::behavior_trees {
using namespace hydrogen::pipelines;
using namespace godot;

class BehaviorTreeGraph : public PipelineGraph<BehaviorTreeNode> {
	DECLARE_PIPELINE_GRAPH()
public:
	BehaviorTreeGraph(const StringName &p_group_key) : PipelineGraph(p_group_key) {}
	~BehaviorTreeGraph() override = default;

	const StringName &graph_type() const override {
		static const StringName type_name = StringName("BehaviorTree", true);
		return type_name;
	}

	static void register_core_nodes();
};

} // hydrogen

#endif //BEHAVIOR_TREE_GRAPH_HPP
