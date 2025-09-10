#ifndef BEHAVIOR_TREE_CONTEXT_HPP
#define BEHAVIOR_TREE_CONTEXT_HPP

#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <type_traits>

#include "../blackboard.hpp"
#include "../pipelines/node_interfaces.hpp"
#include "behavior_tree_node.hpp"
#include "godot_cpp/core/error_macros.hpp"

namespace hydrogen::behavior_trees {

using namespace godot;
using namespace pipelines;

class BehaviorTreeContext {

	friend class BehaviorTree;
	friend class BehaviorTreeNode;

	Blackboard *_blackboard;
	const NodeStateMap *_node_states;

	BehaviorTreeContext(Blackboard *p_blackboard, const NodeStateMap *p_node_states)
    : _blackboard(p_blackboard), _node_states (p_node_states) {}

	// TODO: Handle profiling, editor debugging via context
	void _node_enter(const BehaviorTreeNode *p_node) {}
	void _node_exit(const BehaviorTreeNode *p_node) {}

	void _node_halt_enter(const BehaviorTreeNode *p_node) {}
	void _node_halt_exit(const BehaviorTreeNode *p_node) {}

public:
	IPipelineNodeState *get_state(RID p_state_key) const {
		auto iter = _node_states->find(p_state_key);
		if (unlikely(iter == _node_states->end())) {
			return nullptr;
		}
		return iter->value;
	}

	template<typename T>
	_FORCE_INLINE_ T *get_state(RID p_state_key) {
		if constexpr (!std::is_base_of_v<IPipelineNodeState, T>) {
			WARN_PRINT_ONCE("Unknown pipeline node state type!");
			return nullptr;
		}
		else {
			T *state = dynamic_cast<T *>(get_state(p_state_key));
			ERR_FAIL_NULL_V(state, nullptr);
			return state;
		}
	}

	_FORCE_INLINE_ Blackboard *blackboard() const { return _blackboard; }
};

}

#endif
