#ifndef BEHAVIOR_TREES_HPP
#define BEHAVIOR_TREES_HPP

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/pair.hpp>

#include "../pipeline.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;

MAKE_BLACKBOARD_ENTRY_NAME(last_result)
MAKE_BLACKBOARD_ENTRY_NAME(behavior_tree)

class BehaviorTree;

class TaskNode : public pipelines::PipelineNode {

public:
	enum Result {
		SUCCESS = 0,
		FAILURE = 1,
		RUNNING = 2,
	};

protected:
	friend class BehaviorTree;

	[[nodiscard]] static _FORCE_INLINE_ BehaviorTree * get_behavior_tree(const Blackboard * p_blackboard) {
		return p_blackboard->get_entry<BehaviorTree *>(behavior_tree_name());
	}

	virtual Result _run(Blackboard *) const = 0;

	virtual Blackboard * push_blackboard(Blackboard *p_blackboard) const {
		return p_blackboard;
	}

	virtual void pop_blackboard(Blackboard *p_blackboard) const {}

public:

	Result run(Blackboard *p_blackboard, bool p_resume = false) const;
};

class BehaviorTree final : public pipelines::Pipeline {

	friend class TaskNode;

	Vector<Pair<const TaskNode *, Blackboard *>> execution_stack = {};

	[[nodiscard]] _FORCE_INLINE_ bool is_stack_empty() const { return execution_stack.is_empty(); }
	[[nodiscard]] _FORCE_INLINE_ const Pair<const TaskNode *, Blackboard *> &peek_stack() const { return execution_stack[execution_stack.size() - 1]; }

	void _enter(const TaskNode *p_node, Blackboard *p_blackboard) {
		const auto& pair = peek_stack();
		CRASH_COND(p_node == pair.first);
		CRASH_COND(p_blackboard == pair.second);

		execution_stack.push_back(Pair(p_node, p_blackboard));
	}

	void _exit(const TaskNode *p_node, const Blackboard *p_blackboard) {
		const auto &bb_pair = peek_stack();
		CRASH_COND(p_node != bb_pair.first);
		CRASH_COND(p_blackboard != bb_pair.second);

		execution_stack.remove_at(execution_stack.size() - 1);
	}

	void execute() override;

public:

	static void register_types();

	BehaviorTree(const Blackboard *p_blackboard, const TaskNode *p_root_node);
	~BehaviorTree() override;

	[[nodiscard]] bool is_fully_halted() const override;

	[[nodiscard]] _FORCE_INLINE_ const TaskNode *get_task_root() const { return dynamic_cast<const TaskNode *>(get_root()); }
};

}

#endif