
#include "behavior_tree_graph.hpp"
#include "selector_node.hpp"
#include "sequence_node.hpp"

namespace hydrogen::behavior_trees {

void BehaviorTreeGraph::register_core_nodes() {
    BehaviorTreeGraph::register_node<SequenceNode>();
    BehaviorTreeGraph::register_node<SelectorNode>();
}

}