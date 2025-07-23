
#include "behavior_trees.hpp"

#include <godot_cpp/templates/local_vector.hpp>

namespace hydrogen {

void BehaviorTree::_update_dirty() {

	// Rebuild node cache data, check if any important nodes are removed and if so, reset completely
	// if it's just adding nodes, then alloc state for them and we should resume as normal.
	BehaviorTreeTaskNode *task_root_node = get_task_root();
	Vector nodes = {task_root_node};
	task_root_node->collect_descendents(nodes);

	NodeSet collected_nodes = {};
	collected_nodes.reserve(nodes.size());
	for (const auto node : nodes) {
		collected_nodes.insert(node);
	}

	const NodeSet existing_nodes = bound_nodes;
	bound_nodes = collected_nodes;

	LocalVector<BehaviorTreeTaskNode *> added_nodes = {};
	added_nodes.reserve(nodes.size());
	for (auto node : nodes) {
		if (!existing_nodes.has(node)) {
			added_nodes.push_back(node);
		}
	}

	LocalVector<BehaviorTreeTaskNode *> removed_nodes = {};
	removed_nodes.reserve(existing_nodes.size());
	for (auto node : existing_nodes) {
		if (!collected_nodes.has(node)) {
			removed_nodes.push_back(node);
		}
	}

	for (const auto node : added_nodes) {
		const auto *stateful_node = dynamic_cast<IStatefulBehaviorTreeTaskNode *>(node);
		if (unlikely(stateful_node != nullptr)) {
			node_states[node->get_self()] = stateful_node->create_state_value();
		}
	}

	for (const auto node : removed_nodes) {
		const auto *stateful_node = dynamic_cast<IStatefulBehaviorTreeTaskNode *>(node);
		if (likely(!stateful_node)) {
			continue;
		}

		auto iter = node_states.find(node->get_self());
		if (likely(iter != node_states.end())) {
			IBehaviorTreeTaskState *state = iter->value;
			memdelete(state);
			node_states.remove(iter);
		}
	}

	// check if

	Pipeline::_update_dirty();
}

void BehaviorTree::register_types() {
	Blackboard::register_type<Vector<Blackboard *>*>();
	Blackboard::register_type<Vector<BehaviorTreeTaskNode *>*>();
	Blackboard::register_type<BehaviorTreeTaskNode::Result>();
}

BehaviorTree::BehaviorTree(Blackboard *p_blackboard, BehaviorTreeTaskNode *p_root) : Pipeline(p_blackboard, p_root), state_blackboard(memnew(Blackboard)) {
	state_blackboard->set_parent(p_blackboard);

	blackboards_stack.push_back(Pair(p_root->get_self(), state_blackboard));
	nodes_stack.push_back(p_root);

	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::blackboards_name(), &blackboards_stack);
	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::nodes_name(), &nodes_stack);
	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::error_name(), String(""));
	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::halting_name(), false);
	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::last_result_name(), BehaviorTreeTaskNode::SUCCESS);
	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::node_states_name(), &node_states);

	_update_dirty();
}

BehaviorTree::~BehaviorTree() {
	memdelete(state_blackboard);
	blackboards_stack.clear();
	nodes_stack.clear();
	for (const auto& kvp : node_states) {
		memdelete(kvp.value);
	}
	node_states.clear();
}

void BehaviorTree::execute() {

	if (_is_dirty) {
		_update_dirty();
	}

	const Pair<RID, Blackboard *> bb_pair = get_current_blackboard();
	const BehaviorTreeTaskNode *current_node = get_current_node();

	const BehaviorTreeTaskNode::Result result = current_node->execute(bb_pair.second);
	state_blackboard->set_entry_fast(BehaviorTreeTaskNode::last_result_name(), result);
}

void BehaviorTree::halt() {
	Pipeline::halt();
	state_blackboard->set_entry(BehaviorTreeTaskNode::halting_name(), true);
}

bool BehaviorTree::is_fully_halted() const {
	return is_halting() && state_blackboard->get_entry<bool>(BehaviorTreeTaskNode::halting_name()) &&
		nodes_stack.size() == 1 && blackboards_stack.size() == 1;
}

void BehaviorTree::clear_halt() {
	Pipeline::clear_halt();
	state_blackboard->set_entry(BehaviorTreeTaskNode::halting_name(), false);
}

String BehaviorTree::get_error() const {
	return state_blackboard->get_entry_fast<String>(BehaviorTreeTaskNode::error_name(), "", false);
}

}