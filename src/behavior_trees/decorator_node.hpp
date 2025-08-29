//
// Created by tkey on 7/29/25.
//

#ifndef DECORATOR_NODE_HPP
#define DECORATOR_NODE_HPP
#include "behavior_tree_node.hpp"
#include "../pipelines/decorator.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "pipelines/node_interfaces.hpp"

namespace hydrogen::behavior_trees {

using namespace pipelines;

class DecoratorNode : public BehaviorTreeNode, public PipelineNodeDecorator<BehaviorTreeNode> {
	ABSTRACT_PIPELINE_NODE(DecoratorNode, BehaviorTreeNode);

	template<typename T>
	void _get_children(Vector<const T *> &p_nodes) const {
		if (likely(_decorated_node != nullptr)) {
			p_nodes.push_back(_decorated_node);
		}
	}

	template<typename T>
	void _get_descendents(Vector<const T *> &p_nodes) const {
		if (likely(_decorated_node != nullptr)) {
			const T *node = _decorated_node;
			p_nodes.push_back(node);
			node->get_descendants(p_nodes);
		}
	}

protected:
	DecoratorNode() = default;

	EMPTY_CONNECTION_LIST();
	EMPTY_PORT_LIST();

	void _halt(BehaviorTreeContext &p_context) const override {
		ERR_FAIL_NULL(_decorated_node);
		_decorated_node->halt(p_context);
	}

public:
	~DecoratorNode() override = default;

	bool supports_children() const override { return true; }
	bool has_children() const override {
		return _decorated_node != nullptr;
	}

	void get_children(Vector<const IPipelineNode *> &p_nodes) const override {
		_get_children(p_nodes);
	}

	void get_children(Vector<const BehaviorTreeNode *> &p_nodes) const override {
		_get_children(p_nodes);
	}

	void get_descendants(Vector<const IPipelineNode *> &p_nodes) const override {
		_get_descendents(p_nodes);
	}

	void get_descendants(Vector<const BehaviorTreeNode *> &p_nodes) const override {
		_get_descendents(p_nodes);
	}
};
} // hydrogen

#endif //DECORATOR_NODE_HPP
