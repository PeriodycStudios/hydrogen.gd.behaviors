#ifndef BEHAVIOR_TREE_CONTEXT_HPP
#define BEHAVIOR_TREE_CONTEXT_HPP

#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "../blackboard.hpp"
#include "../pipelines/node_interfaces.hpp"
#include "behavior_tree_node.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

class BehaviorTreeContext {

	friend class BehaviorTree;
	friend class BehaviorTreeNode;

	Blackboard *blackboard;
	const NodeStateMap *node_states;
	const HashMap<StringName, StringName> *aliases;

	BehaviorTreeContext(Blackboard *p_blackboard, const NodeStateMap *p_node_states, const godot::HashMap<StringName, StringName> *p_aliases);

	void _node_enter(const BehaviorTreeNode *p_node);
	void _node_exit(const BehaviorTreeNode *p_node);

	void _node_halt_enter(const BehaviorTreeNode *p_node);
	void _node_halt_exit(const BehaviorTreeNode *p_node);

public:
	IPipelineNodeState *get_state(RID p_state_key) const;

	template<typename T>
	void set(const StringName &p_output_port_name, const T &p_input_value) {
		auto iter = aliases->find(p_output_port_name);
		if (likely(iter == aliases->end())) {
			blackboard->set_entry_fast(p_output_port_name, p_input_value);
		}
		else {
			const StringName &alias_name = iter->value;
			blackboard->set_entry_fast(alias_name, p_input_value);
		}
	}

	template<typename T>
	const T &get(const StringName &p_output_port_name, const T &p_default = {}) const {
		auto iter = aliases->find(p_output_port_name);
		if (likely(iter == aliases->end())) {
			return blackboard->get_entry_fast<T>(p_output_port_name, p_default);
		}

		const StringName &alias_name = iter->value;
		return blackboard->get_entry_fast<T>(alias_name, p_default);
	}
};

}

#endif
