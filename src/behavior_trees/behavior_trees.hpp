#ifndef BEHAVIOR_TREES_HPP
#define BEHAVIOR_TREES_HPP

#include "../blackboard.hpp"
#include "../pipelines/pipeline.hpp"
#include "behavior_tree_context.hpp"
#include "behavior_tree_graph.hpp"
#include "behavior_tree_node.hpp"

#include <godot_cpp/core/defs.hpp>

namespace hydrogen::behavior_trees {

using namespace godot;

class BehaviorTree final : public Pipeline {

	[[nodiscard]] const BehaviorTreeNode *get_root() const;

	[[nodiscard]] BehaviorTreeContext create_context();

public:

	static void register_types();

	explicit BehaviorTree(const Blackboard *p_blackboard, BehaviorTreeGraph *p_graph);
	~BehaviorTree() override;

	void execute() override;

	void halt() override;
};

}

#endif