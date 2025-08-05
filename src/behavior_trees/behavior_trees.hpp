#ifndef BEHAVIOR_TREES_HPP
#define BEHAVIOR_TREES_HPP
#include <godot_cpp/core/defs.hpp>

#include "../pipelines/pipeline.hpp"
#include "behavior_tree_context.hpp"
#include "behavior_tree_graph.hpp"
#include "behavior_tree_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class BehaviorTree final : public Pipeline {

	[[nodiscard]] _FORCE_INLINE_ const BehaviorTreeNode *get_root() const {
		return dynamic_cast<const BehaviorTreeNode *>(get_pipeline_root());
	}

	[[nodiscard]] _FORCE_INLINE_ BehaviorTreeContext create_context() {
		return BehaviorTreeContext(get_state_blackboard(), &node_states(), &aliases());
	}

public:

	static void register_types();

	explicit BehaviorTree(const Blackboard *p_blackboard, BehaviorTreeGraph *p_graph);
	~BehaviorTree() override;

	void execute() override;
	void halt() override;
};

}

#endif