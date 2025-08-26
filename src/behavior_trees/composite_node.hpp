//
// Created by tkey on 7/29/25.
//

#ifndef CONTAINER_NODE_HPP
#define CONTAINER_NODE_HPP
#include "behavior_tree_node.hpp"
#include "behavior_tree_context.hpp"
#include "../pipelines/composite.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "mutex_helpers.hpp"
#include "pipelines/node_interfaces.hpp"
#include <cstdint>

namespace hydrogen::behavior_trees {
using namespace pipelines;
using namespace godot;

class CompositeNode : public BehaviorTreeNode, public pipelines::PipelineNodeComposite<BehaviorTreeNode>, public IPipelineNodeStateful {

	template <typename T>
	void _get_children(Vector<const T *> &p_nodes) const {
		LOCK_ONE(_mutex);
		for (const T *node : _children) {
			p_nodes.push_back(node);
		}
	}

	template<typename T>
	void _get_descendants(Vector<const T *> &p_nodes) const {
		LOCK_ONE(_mutex);
		for (const T *node : _children) {
			p_nodes.push_back(node);
			node->get_descendants(p_nodes);
		}
	}

protected:
	CompositeNode() = default;

	struct CompositeNodeState : IPipelineNodeState {
		int64_t current_child_index = 0;

		CompositeNodeState() = default;
		~CompositeNodeState() override = default;
	};

	CompositeNodeState *_get_state(BehaviorTreeContext &p_context) const {
		return p_context.get_state<CompositeNodeState>(state_key());
	}

	void _reset_state(CompositeNodeState *state) const {
		ERR_FAIL_NULL(state);
		state->current_child_index = 0;
	}

	void _halt(BehaviorTreeContext &p_context) const override {
		CompositeNodeState *state = _get_state(p_context);
		_reset_state(state);

		for (const BehaviorTreeNode *child : _children) {
			child->halt(p_context);
		}
	}

public:
	~CompositeNode() override = default;

	RID state_key() const override { return get_self(); }

	IPipelineNodeState * create_state() const override { return memnew(CompositeNodeState); }

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
};

} // hydrogen

#endif //CONTAINER_NODE_HPP
