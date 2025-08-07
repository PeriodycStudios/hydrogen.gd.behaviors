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

	[[nodiscard]] _FORCE_INLINE_ const BehaviorTreeNode *get_root() const {
		return dynamic_cast<const BehaviorTreeNode *>(get_pipeline_root());
	}

	[[nodiscard]] _FORCE_INLINE_ BehaviorTreeContext create_context() {
		return {get_state_blackboard(), &node_states(), &aliases()};
	}

public:

	static void register_types() {
		Blackboard::register_type<BehaviorTreeNode::Result>();
	}

	explicit BehaviorTree(const Blackboard *p_blackboard, BehaviorTreeGraph *p_graph) : Pipeline(p_blackboard, p_graph) {
		get_state_blackboard()->set_entry_fast(behavior_tree_name(), this);
		get_state_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
	}

	~BehaviorTree() override {
		halt();
	}

	void execute() override {
		std::scoped_lock<std::mutex> lock(*mutex());
		BehaviorTreeContext context = create_context();
		const BehaviorTreeNode::Result result = get_root()->execute(context);
		get_state_blackboard()->set_entry_fast(last_result_name(), result);
	}
	void halt() override {
		std::scoped_lock<std::mutex> lock(*mutex());
		BehaviorTreeContext context = create_context();
		get_root()->halt(context);
		get_state_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
	}
};

}

#endif