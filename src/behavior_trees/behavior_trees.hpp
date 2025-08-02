#ifndef BEHAVIOR_TREES_HPP
#define BEHAVIOR_TREES_HPP

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/pair.hpp>

#include "../pipeline.hpp"
#include "behavior_tree_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

class BehaviorTree final : public pipelines::Pipeline {

	friend class BehaviorTreeNode;

	// Vector<Pair<const BehaviorTreeNode *, Blackboard *>> execution_stack = {};

	// [[nodiscard]] _FORCE_INLINE_ bool is_stack_empty() const { return execution_stack.is_empty(); }
	// [[nodiscard]] _FORCE_INLINE_ const Pair<const BehaviorTreeNode *, Blackboard *> &peek_stack() const { return execution_stack[execution_stack.size() - 1]; }
	//
	// void _enter(const BehaviorTreeNode *p_node, Blackboard *p_blackboard) {
	// 	const auto& pair = peek_stack();
	// 	CRASH_COND(p_node == pair.first);
	// 	CRASH_COND(p_blackboard == pair.second);
	//
	// 	execution_stack.push_back(Pair(p_node, p_blackboard));
	// }
	//
	// void _exit(const BehaviorTreeNode *p_node, const Blackboard *p_blackboard) {
	// 	const auto &bb_pair = peek_stack();
	// 	CRASH_COND(p_node != bb_pair.first);
	// 	CRASH_COND(p_blackboard != bb_pair.second);
	//
	// 	execution_stack.remove_at(execution_stack.size() - 1);
	// }

public:

	static void register_types();

	explicit BehaviorTree(const Blackboard *p_blackboard, const BehaviorTreeNode *p_root_node = nullptr);
	~BehaviorTree() override;

	void execute() override;

	[[nodiscard]] bool is_fully_halted() const override;

	[[nodiscard]] _FORCE_INLINE_ bool set_task_root(const BehaviorTreeNode *p_root_node) {
		return set_pipeline_root(p_root_node);
	}

	[[nodiscard]] _FORCE_INLINE_ const BehaviorTreeNode *get_task_root() const { return dynamic_cast<const BehaviorTreeNode *>(get_pipeline_root()); }
};

}

#endif