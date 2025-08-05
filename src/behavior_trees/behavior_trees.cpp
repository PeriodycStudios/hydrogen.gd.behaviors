
#include "behavior_trees.hpp"
#include <mutex>
#include "behavior_tree_context.hpp"

namespace hydrogen::behavior_trees {

void BehaviorTree::register_types() {
	Blackboard::register_type<BehaviorTreeNode::Result>();
}

BehaviorTree::BehaviorTree(const Blackboard *p_blackboard, BehaviorTreeGraph *p_graph) : Pipeline(p_blackboard, p_graph) {
	get_state_blackboard()->set_entry_fast(behavior_tree_name(), this);
	get_state_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
}

BehaviorTree::~BehaviorTree() {
	halt();
}

void BehaviorTree::execute() {
	std::scoped_lock<std::mutex> lock(mutex());
	BehaviorTreeContext context = create_context();
	const BehaviorTreeNode::Result result = get_root()->execute(context);
	get_state_blackboard()->set_entry_fast(last_result_name(), result);
}

void BehaviorTree::halt() {
	std::scoped_lock<std::mutex> lock(mutex());
	BehaviorTreeContext context = create_context();
	get_root()->halt(context);
	get_state_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
}

} //namespace hydrogen