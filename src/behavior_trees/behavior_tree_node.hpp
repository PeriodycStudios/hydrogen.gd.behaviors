#ifndef BEHAVIOR_TREE_NODE_HPP
#define BEHAVIOR_TREE_NODE_HPP

#include "../pipelines/pipeline_nodes.hpp"

namespace hydrogen::behavior_trees {

class BehaviorTree;
class BehaviorTreeContext;

MAKE_BLACKBOARD_ENTRY_NAME(last_result)

class BehaviorTreeNode : public pipelines::IPipelineNode {
public:
	enum Result {
		SUCCESS = 0,
		FAILURE = 1,
		RUNNING = 2,
	};

protected:
	friend class BehaviorTree;

	virtual Result _execute(BehaviorTreeContext &p_context) const = 0;
	virtual void _halt(BehaviorTreeContext &p_context) const = 0;

public:
	
	Result execute(BehaviorTreeContext &p_context) const;

	void halt(BehaviorTreeContext &p_context) const;
};
}

#endif