
#include "behavior_trees.hpp"

namespace hydrogen {

void BehaviorTree::register_types() {
	Blackboard::register_type<Vector<Blackboard *>*>();
	Blackboard::register_type<Vector<BehaviorTreeTaskNode *>*>();
	Blackboard::register_type<BehaviorTreeTaskNode::Result>();
}

BehaviorTree::BehaviorTree(Blackboard *p_blackboard, BehaviorTreeTaskNode *p_root) : Pipeline(p_blackboard, p_root), state_blackboard(memnew(Blackboard)) {
	state_blackboard->set_parent(p_blackboard);

	blackboards.push_back(state_blackboard);
	nodes.push_back(p_root);

	state_blackboard->set_entry(blackboards_name(), &blackboards);
	state_blackboard->set_entry(nodes_name(), &nodes);
	state_blackboard->set_entry(halting_name(), false);
}

BehaviorTree::~BehaviorTree() {
	memdelete(state_blackboard);
	blackboards.clear();
	nodes.clear();
}

void BehaviorTree::execute() {
	Blackboard *current_blackboard = get_current_blackboard();
	const BehaviorTreeTaskNode *current_node = get_current_node();

	const BehaviorTreeTaskNode::Result result = current_node->execute(current_blackboard);
	state_blackboard->set_entry_fast(last_result_name(), result);
}

void BehaviorTree::halt() {
	Pipeline::halt();
	state_blackboard->set_entry(halting_name(), true);
}

bool BehaviorTree::is_fully_halted() const {
	return is_halting() && state_blackboard->get_entry<bool>(halting_name()) &&
		nodes.size() == 1 && blackboards.size() == 1;
}

void BehaviorTree::clear_halt() {
	Pipeline::clear_halt();
	state_blackboard->set_entry(halting_name(), false);
}

String BehaviorTree::get_error() const {
	return state_blackboard->get_entry_fast(error_name(), "", false);
}






}