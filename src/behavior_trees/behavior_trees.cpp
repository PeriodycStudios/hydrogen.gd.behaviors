
#include "behavior_trees.hpp"
#include "behavior_tree_node.hpp"
#include <godot_cpp/core/defs.hpp>

namespace hydrogen::behavior_trees {

_FORCE_INLINE_ const BehaviorTreeNode *BehaviorTree::get_root() const {
    return dynamic_cast<const BehaviorTreeNode *>(get_pipeline_root());
}

_FORCE_INLINE_ BehaviorTreeContext BehaviorTree::create_context() {
    return {get_state_blackboard(), &node_states(), &aliases()};
}

void BehaviorTree::register_types() {
    Blackboard::register_type<BehaviorTreeNode::Result>();
}

BehaviorTree::BehaviorTree(const Blackboard *p_blackboard, BehaviorTreeGraph *p_graph) : Pipeline(p_blackboard, p_graph) {
    get_state_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
}

BehaviorTree::~BehaviorTree() {
    halt();
}

void BehaviorTree::execute() {
    std::scoped_lock<std::mutex> lock(*mutex());
    BehaviorTreeContext context = create_context();
    const BehaviorTreeNode::Result result = get_root()->execute(context);
    get_state_blackboard()->set_entry_fast(last_result_name(), result);
}

void BehaviorTree::halt() {
    std::scoped_lock<std::mutex> lock(*mutex());
    BehaviorTreeContext context = create_context();
    get_root()->halt(context);
    get_state_blackboard()->set_entry_fast(last_result_name(), BehaviorTreeNode::SUCCESS);
}

}