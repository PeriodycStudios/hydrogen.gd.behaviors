
#pragma once

#include "behavior_trees/behavior_tree_context.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"

namespace hydrogen::behavior_trees {

class PrintMessageNode : public BehaviorTreeNode {
	DECLARE_PIPELINE_NODE(PrintMessageNode, BehaviorTreeNode);

	DECLARE_INPUT_PORT(message, String, "");

	BEGIN_NODE_PORTS()
	INPUT_PORT(message)
	END_NODE_PORTS()

protected:
	Result _execute(BehaviorTreeContext &p_context) const override {
		const String msg = GET_PORT(message);
		print_line(msg);
		return SUCCESS;
	}

public:
	DEFINE_GET_PORTS()
};

class PrintVariantNode : public BehaviorTreeNode {
	DECLARE_PIPELINE_NODE(PrintVariantNode, BehaviorTreeNode);

	DECLARE_INPUT_PORT(value, Variant, Variant());

	BEGIN_NODE_PORTS()
	INPUT_PORT(value)
	END_NODE_PORTS()

protected:
	Result _execute(BehaviorTreeContext &p_context) const override {
		Variant value = GET_PORT(value);
		print_line(value);
		return SUCCESS;
	}

public:
	DEFINE_GET_PORTS()
};

} //namespace hydrogen::behavior_trees
