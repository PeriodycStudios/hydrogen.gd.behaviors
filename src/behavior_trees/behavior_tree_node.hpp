
#ifndef TASK_NODE_HPP
#define TASK_NODE_HPP

#include "../pipeline_nodes.hpp"

namespace hydrogen::behavior_trees {

class BehaviorTree;
class BehaviorTreeContext;

MAKE_BLACKBOARD_ENTRY_NAME(last_result)
MAKE_BLACKBOARD_ENTRY_NAME(behavior_tree)

class BehaviorTreeNode : public pipelines::IPipelineNode {
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

	virtual Result execute(BehaviorTreeContext &p_context) const = 0;
	virtual void halt(BehaviorTreeContext &p_context) const = 0;
};
}

#endif