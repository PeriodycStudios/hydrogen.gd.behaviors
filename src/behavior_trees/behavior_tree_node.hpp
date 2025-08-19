#ifndef BEHAVIOR_TREE_NODE_HPP
#define BEHAVIOR_TREE_NODE_HPP

#include "../name_helpers.hpp"
#include "../pipelines/pipeline_node.hpp"
#include "godot_cpp/templates/vector.hpp"

namespace hydrogen::behavior_trees {

class BehaviorTree;
class BehaviorTreeContext;

DEFINE_NAME_STATIC(last_result)

class BehaviorTreeNode : public pipelines::PipelineNode {
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

	virtual void get_children(Vector<const BehaviorTreeNode *> &p_nodes) const {}
	virtual void get_descendants(Vector<const BehaviorTreeNode *> &p_nodes) const {}
};
}

#endif