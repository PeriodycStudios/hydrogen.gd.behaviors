
#pragma once

#include "../decorator_node.hpp"
#include "behavior_trees/behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"

namespace hydrogen::behavior_trees {
class UntilFailNode : public DecoratorNode {
	DECLARE_PIPELINE_NODE(UntilFailNode, DecoratorNode);

protected:
	Result _execute(BehaviorTreeContext &p_context) const override {
		DECORATOR_FAILURE_IF_NULL();

		Result result = _decorated_node->execute(p_context);
		if (unlikely(result == FAILURE)) {
			return SUCCESS;
		} else {
			return RUNNING;
		}
	}
};
} //namespace hydrogen::behavior_trees
