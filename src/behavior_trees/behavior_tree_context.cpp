
#include "behavior_tree_context.hpp"
#include "behavior_tree_node.hpp"
#include "../pipelines/node_interfaces.hpp"

namespace hydrogen::behavior_trees {

BehaviorTreeContext::BehaviorTreeContext(Blackboard *p_blackboard, const NodeStateMap *p_node_states, const godot::HashMap<StringName, StringName> *p_aliases)
    : blackboard(p_blackboard), node_states (p_node_states), aliases(p_aliases) {}

// TODO: Handle profiling, editor debugging via context
void BehaviorTreeContext::_node_enter(const BehaviorTreeNode *p_node) {

}

void BehaviorTreeContext::_node_exit(const BehaviorTreeNode *p_node) {

}

void BehaviorTreeContext::_node_halt_enter(const BehaviorTreeNode *p_node) {

}

void BehaviorTreeContext::_node_halt_exit(const BehaviorTreeNode *p_node) {

}

IPipelineNodeState *BehaviorTreeContext::get_state(RID p_state_key) const {
    auto iter = node_states->find(p_state_key);
    if (unlikely(iter == node_states->end())) {
        return nullptr;
    }
    return iter->value;
}

}