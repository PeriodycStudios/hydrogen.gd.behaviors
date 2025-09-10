
#include "behavior_trees.hpp"
#include "behavior_tree_node.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "pipelines/pipeline.hpp"
#include <godot_cpp/core/defs.hpp>

namespace hydrogen::behavior_trees {

_FORCE_INLINE_ const BehaviorTreeNode *BehaviorTree::get_root() const {
    return dynamic_cast<const BehaviorTreeNode *>(get_pipeline_root());
}

_FORCE_INLINE_ BehaviorTreeContext BehaviorTree::create_context() {
    return {get_execution_blackboard(), &node_states()};
}

void BehaviorTree::register_types() {
    Blackboard::register_type<BehaviorTreeNode::Result>();
}

BehaviorTree::BehaviorTree(const StringName &p_name_key, const Blackboard *p_parent, BehaviorTreeGraph *p_graph, bool p_auto_delete_blackboard)
    : Pipeline(p_name_key, p_parent, p_graph, p_auto_delete_blackboard) {
    get_execution_blackboard()->set_entry_fast(_last_result_name(), BehaviorTreeNode::SUCCESS);
}

BehaviorTree::~BehaviorTree() {
}

void BehaviorTree::execute() {
    std::scoped_lock lock(*mutex());
    const BehaviorTreeNode *root = get_root();
    if (unlikely(root == nullptr)) {
        WARN_PRINT("Unable to execute BehaviorTree without root!");
        return;
    }

    BehaviorTreeContext context = create_context();
    const BehaviorTreeNode::Result result = get_root()->execute(context);
    get_execution_blackboard()->set_entry_fast(_last_result_name(), result);
}

void BehaviorTree::halt() {
    std::scoped_lock lock(*mutex());

    const BehaviorTreeNode *root = get_root();
    if (unlikely(root == nullptr)) {
        WARN_PRINT("Unable to halt BehaviorTree without root!");
        return;
    }

    BehaviorTreeContext context = create_context();
    get_root()->halt(context);
    clear_error();
    get_execution_blackboard()->set_entry_fast(_last_result_name(), BehaviorTreeNode::SUCCESS);
}

}