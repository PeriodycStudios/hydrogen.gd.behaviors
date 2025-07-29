
#include "behavior_trees.hpp"
#include <godot_cpp/templates/local_vector.hpp>

namespace hydrogen {

BehaviorTreeNode::Result BehaviorTreeNode::run(Blackboard *p_blackboard, bool p_resume) const  {
	BehaviorTree *behavior_tree = get_behavior_tree(p_blackboard);

	if (likely(!p_resume)) {
		p_blackboard = push_blackboard(p_blackboard);
		behavior_tree->_enter(this, p_blackboard);
	}

	const Result result = _run(p_blackboard);

	if (likely(result != RUNNING)) {
		behavior_tree->_exit(this, p_blackboard);
		pop_blackboard(p_blackboard);
	}

	return result;
}

void BehaviorTree::register_types() {
	Blackboard::register_type<BehaviorTreeNode::Result>();
	Blackboard::register_type<BehaviorTree *>();
}

BehaviorTree::BehaviorTree(const Blackboard *p_blackboard, const BehaviorTreeNode *p_root_node) : Pipeline(p_blackboard, p_root_node) {
	_state_blackboard->set_entry_fast(behavior_trees::behavior_tree_name(), this);
	_state_blackboard->set_entry_fast(behavior_trees::last_result_name(), BehaviorTreeNode::SUCCESS);
}

BehaviorTree::~BehaviorTree() {
	execution_stack.clear();
}

void BehaviorTree::execute() {

	const BehaviorTreeNode *node;
	Blackboard *blackboard;
	if (likely(is_stack_empty())) {
		node = get_task_root();
		blackboard = _state_blackboard;
	} else {
		const auto &bb_pair = peek_stack();
		node = bb_pair.first;
		blackboard = bb_pair.second;
	}

	const BehaviorTreeNode::Result result = node->run(blackboard);
	_state_blackboard->set_entry_fast(behavior_trees::last_result_name(), result);
}

bool BehaviorTree::is_fully_halted() const {
	return is_halting() && is_stack_empty();
}

} //namespace hydrogen