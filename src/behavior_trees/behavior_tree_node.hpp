
#ifndef TASK_NODE_HPP
#define TASK_NODE_HPP

#include "../pipeline_nodes.hpp"

namespace hydrogen::behavior_trees {

class BehaviorTree;

MAKE_BLACKBOARD_ENTRY_NAME(last_result)
MAKE_BLACKBOARD_ENTRY_NAME(behavior_tree)


class Context {

	friend class BehaviorTree;

	Blackboard *_blackboard;
	BehaviorTree *_behavior_tree;
	std::atomic_bool _halt;


public:

	[[nodiscard]] _FORCE_INLINE_ Blackboard *get_blackboard() const { return _blackboard; }
	[[nodiscard]] _FORCE_INLINE_ BehaviorTree *get_behavior_tree() const { return _behavior_tree; }
	[[nodiscard]] _FORCE_INLINE_ bool is_halting() const { return _halt; }
};

class BehaviorTreeNode : public pipelines::PipelineNode {
protected:
	friend class BehaviorTree;

	[[nodiscard]] static _FORCE_INLINE_ BehaviorTree * get_behavior_tree(const Blackboard * p_blackboard) {
		return p_blackboard->get_entry<BehaviorTree *>(behavior_tree_name());
	}

public:
	enum Result {
		SUCCESS = 0,
		FAILURE = 1,
		RUNNING = 2,
	};

	virtual Result execute(Blackboard *p_blackboard) const = 0;
	virtual Result resume(Blackboard *p_blackboard) const = 0;
	virtual void halt(Blackboard *p_blackboard) const = 0;
};
}

#endif