#ifndef BEHAVIOR_TREE_NODE_HPP
#define BEHAVIOR_TREE_NODE_HPP

#include "../name_helpers.hpp"
#include "../pipelines/pipeline_node.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/string.hpp"

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

	static String get_result_name(Result p_result) {
		switch (p_result) {
			case SUCCESS: return "SUCCESS";
			case FAILURE: return "FAILURE";
			case RUNNING: return "RUNNING";
			default: return "Unknown";
		}
	}

protected:
	friend class BehaviorTree;

	virtual Result _execute(BehaviorTreeContext &p_context) const = 0;
	virtual void _halt(BehaviorTreeContext &p_context) const = 0;

	static void unknown_result_handler(Result result) {
		ERR_PRINT(vformat("Unimplemented result behavior: {}", static_cast<int>(result)));
	}

public:
	
	Result execute(BehaviorTreeContext &p_context) const;

	void halt(BehaviorTreeContext &p_context) const;

	virtual void get_children(Vector<const BehaviorTreeNode *> &p_nodes) const {}
	virtual void get_descendants(Vector<const BehaviorTreeNode *> &p_nodes) const {}
};
}

#endif