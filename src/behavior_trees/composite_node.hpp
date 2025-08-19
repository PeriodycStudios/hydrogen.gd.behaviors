//
// Created by tkey on 7/29/25.
//

#ifndef CONTAINER_NODE_HPP
#define CONTAINER_NODE_HPP
#include "behavior_tree_node.hpp"
#include "../pipelines/composite.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "mutex_helpers.hpp"
#include "pipelines/node_interfaces.hpp"

namespace hydrogen::behavior_trees {
using namespace pipelines;
using namespace godot;

class BehaviorTreeNodeComposite : public BehaviorTreeNode, public pipelines::PipelineNodeComposite<BehaviorTreeNode> {

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
	BehaviorTreeNodeComposite() = default;
public:
	~BehaviorTreeNodeComposite() override = default;

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
