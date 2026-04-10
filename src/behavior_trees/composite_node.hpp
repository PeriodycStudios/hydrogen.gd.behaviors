//
// Created by tkey on 7/29/25.
//

#pragma once

#include "../pipelines/composite.hpp"
#include "behavior_tree_context.hpp"
#include "behavior_tree_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline_node.hpp"
#include <cstdint>

namespace hydrogen::behavior_trees {
using namespace pipelines;
using namespace godot;

class CompositeNode : public BehaviorTreeNode, public PipelineNodeComposite<BehaviorTreeNode>, public IPipelineNodeStateful {
	ABSTRACT_PIPELINE_NODE(CompositeNode, BehaviorTreeNode);

	template <typename T>
	void _get_children(Vector<const T *> &p_nodes) const {
		for (const T *node : _children) {
			p_nodes.push_back(node);
		}
	}

	template <typename T>
	void _get_descendants(Vector<const T *> &p_nodes) const {
		for (const T *node : _children) {
			p_nodes.push_back(node);
			node->get_descendants(p_nodes);
		}
	}

protected:
	CompositeNode() = default;

	EMPTY_CONNECTION_LIST();
	EMPTY_PORT_LIST();

	struct CompositeNodeState : IPipelineNodeState {
		int64_t current_child_index = 0;

		CompositeNodeState() = default;
		~CompositeNodeState() override = default;
	};

	void _halt(BehaviorTreeContext &p_context) const override {
		GET_STATE(CompositeNodeState);
		state->current_child_index = 0;

		for (const BehaviorTreeNode *child : _children) {
			child->halt(p_context);
		}
	}

public:
	~CompositeNode() override = default;

	DEFINE_STATEFUL_FUNCS(CompositeNodeState);

	bool supports_children() const override { return true; }
	bool has_children() const override { return !is_empty(); }

	void get_children(Vector<const IPipelineNode *> &p_nodes) const override {
		_get_children(p_nodes);
	}

	void get_children(Vector<const BehaviorTreeNode *> &p_nodes) const override {
		_get_children(p_nodes);
	}

	void get_descendants(Vector<const IPipelineNode *> &p_nodes) const override {
		_get_descendants(p_nodes);
	}

	void get_descendants(Vector<const BehaviorTreeNode *> &p_nodes) const override {
		_get_descendants(p_nodes);
	}

	bool is_child(const BehaviorTreeNode *p_node) const {
		return _children.has(p_node);
	}

	bool has_child(const IPipelineNode *p_node) const override {
		const BehaviorTreeNode *node = dynamic_cast<const BehaviorTreeNode *>(p_node);
		if (unlikely(node == nullptr)) {
			return false;
		}

		return is_child(node);
	}

	bool is_descendent(const BehaviorTreeNode *p_node) const {
		if (unlikely(_children.has(p_node))) {
			return true;
		}

		for (const BehaviorTreeNode *child : _children) {
			if (unlikely(child->has_descendant(p_node))) {
				return true;
			}
		}

		return false;
	}

	bool has_descendant(const IPipelineNode *p_node) const override {
		const BehaviorTreeNode *node = dynamic_cast<const BehaviorTreeNode *>(p_node);
		if (unlikely(node == nullptr)) {
			return false;
		}

		return is_descendent(node);
	}
};

} //namespace hydrogen::behavior_trees
