
#include "behavior_tree_graph.hpp"
#include "behavior_trees/tasks/count_down_node.hpp"
#include "behavior_trees/tasks/print_message_node.hpp"
#include "behavior_trees/tasks/set_bool_node.hpp"
#include "behavior_trees/tasks/set_variant_node.hpp"
#include "selector_node.hpp"
#include "sequence_node.hpp"
#include "parallel_node.hpp"

#include "decorators/interrupter_node.hpp"
#include "decorators/inverter_node.hpp"
#include "decorators/limit_node.hpp"
#include "decorators/until_fail_node.hpp"

#include "tasks/perform_interruption_node.hpp"
#include "tasks/wait_nodes.hpp"

namespace hydrogen::behavior_trees {

void BehaviorTreeGraph::register_core_nodes() {
    BehaviorTreeGraph::register_node<SequenceNode>();
    BehaviorTreeGraph::register_node<SelectorNode>();
    BehaviorTreeGraph::register_node<ParallelSelectorNode>();
    BehaviorTreeGraph::register_node<ParallelSequenceNode>();

    BehaviorTreeGraph::register_node<InterrupterNode>();
    BehaviorTreeGraph::register_node<InverterNode>();
    BehaviorTreeGraph::register_node<LimitNode>();
    BehaviorTreeGraph::register_node<UntilFailNode>();

    BehaviorTreeGraph::register_node<CountDownNode>();
    BehaviorTreeGraph::register_node<PerformInterruptionNode>();
    BehaviorTreeGraph::register_node<PrintMessageNode>();
    BehaviorTreeGraph::register_node<PrintVariantNode>();
    BehaviorTreeGraph::register_node<SetBoolNode>();
    BehaviorTreeGraph::register_node<SetVariantNode>();
    BehaviorTreeGraph::register_node<WaitForSecondsNode>();
    BehaviorTreeGraph::register_node<WaitForMillisecondsNode>();
    BehaviorTreeGraph::register_node<WaitForTicksNode>();
    BehaviorTreeGraph::register_node<WaitForRealtimeSeconds>();
}

}