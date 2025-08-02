
#include "behavior_trees.hpp"
#include <godot_cpp/templates/local_vector.hpp>

namespace hydrogen::behavior_trees {

void BehaviorTree::register_types() {
	Blackboard::register_type<BehaviorTreeNode::Result>();
	Blackboard::register_type<BehaviorTree *>();
}

BehaviorTree::BehaviorTree(const Blackboard *p_blackboard, const BehaviorTreeNode *p_root_node) : Pipeline(p_blackboard, p_root_node) {
	get_blackboard()->set_entry_fast(behavior_tree_name(), this);
	get_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
}

BehaviorTree::~BehaviorTree() {
}

void BehaviorTree::execute() {
	const BehaviorTreeNode::Result result = get_task_root()->execute(get_blackboard());
	get_blackboard()->set_entry_fast(last_result_name(), result);
}

bool BehaviorTree::is_fully_halted() const {
	return is_halting();
}

} //namespace hydrogen