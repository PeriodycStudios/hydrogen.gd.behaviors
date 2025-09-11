#include "behavior_trees/behavior_tree_node.hpp"
// #include "behavior_trees/selector_node.hpp"
// #include "behavior_trees/sequence_node.hpp"
// #include "behavior_trees/tasks/count_down_node.hpp"
// #include "behavior_trees/tasks/wait_nodes.hpp"
// #include "behavior_trees/tasks/set_bool_node.hpp"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_DLL
#include <doctest.h>

#include "behavior_server.hpp"

namespace hydrogen::test {
using namespace godot;
using namespace pipelines;
using namespace behavior_trees;
using BTNodeResult = BehaviorTreeNode::Result;

TEST_CASE("[Hydrogen][Game AI][Behavior Trees] Simple Create and Destroy") {
    BehaviorServer *server = BehaviorServer::get_singleton();
    REQUIRE(server);

    RID blackboard = server->blackboard_create();
    CHECK(blackboard.is_valid());

    RID graph = server->graph_create(BehaviorServer::BehaviorTree_name());
    CHECK(graph.is_valid());
    CHECK_FALSE(server->graph_is_bound(graph));
    CHECK_EQ(server->graph_get_node_count(graph), 0);
    
    RID selector_node = server->graph_create_node(graph, "SelectorNode");
    CHECK(selector_node.is_valid());
    CHECK_EQ(server->graph_get_node_count(graph), 1);

    CHECK(server->node_is_composite(graph, selector_node));

    server->graph_set_root(graph, selector_node);

    CHECK_EQ(server->graph_get_root(graph), selector_node);

    RID behavior_tree = server->pipeline_create(graph, blackboard);
    RID bt2 = server->pipeline_create(graph);

    CHECK(server->graph_is_bound(graph));
    CHECK_EQ(server->graph_get_bind_count(graph), 2);

    RID exec_bb_1 = server->pipeline_get_execution_blackboard(behavior_tree);
    RID exec_bb_2 = server->pipeline_get_execution_blackboard(bt2);

    CHECK(exec_bb_1.is_valid());
    CHECK(exec_bb_2.is_valid());

    BTNodeResult result_1 = server->blackboard_get_entry<BTNodeResult>(exec_bb_1, behavior_trees::_last_result_name());
    BTNodeResult result_2 = server->blackboard_get_entry<BTNodeResult>(exec_bb_2, behavior_trees::_last_result_name());

    CHECK_EQ(result_1, BTNodeResult::SUCCESS);
    CHECK_EQ(result_2, BTNodeResult::SUCCESS);

    server->pipeline_execute(behavior_tree);
    server->pipeline_execute(bt2);

    result_1 = server->blackboard_get_entry<BTNodeResult>(exec_bb_1, behavior_trees::_last_result_name());
    result_2 = server->blackboard_get_entry<BTNodeResult>(exec_bb_2, behavior_trees::_last_result_name());

    CHECK_EQ(result_1, BTNodeResult::FAILURE);
    CHECK_EQ(result_2, BTNodeResult::FAILURE);

    server->pipeline_halt(behavior_tree);
    server->pipeline_halt(bt2);

    result_1 = server->blackboard_get_entry<BTNodeResult>(exec_bb_1, behavior_trees::_last_result_name());
    result_2 = server->blackboard_get_entry<BTNodeResult>(exec_bb_2, behavior_trees::_last_result_name());

    CHECK_EQ(result_1, BTNodeResult::SUCCESS);
    CHECK_EQ(result_2, BTNodeResult::SUCCESS);

    server->free_rid(behavior_tree);

    CHECK(server->graph_is_bound(graph));

    server->free_rid(bt2);

    CHECK_FALSE(server->graph_is_bound(graph));

    server->free_rid(graph);
    server->free_rid(blackboard);
}

// TEST_CASE("[Hydrogen][Game AI][Behavior Trees] Test Sequence Node") {
//     BehaviorServer *server = BehaviorServer::get_singleton();
//     REQUIRE(server);

//     RID graph = server->behavior_tree_graph_create();
//     CHECK(graph.is_valid());
//     RID sequence = server->graph_create_node(graph, SequenceNode::get_node_name());
//     CHECK(sequence.is_valid());

//     server->graph_set_root(graph, sequence);

//     RID countdown1 = server->graph_create_node(graph, CountDownNode::get_node_name());
//     RID wait_seconds1 = server->graph_create_node(graph, WaitForSecondsNode::get_node_name());
// }

// TEST_CASE("[Hydrogen][Game AI][Behavior Trees] Test Selector Node") {
//     BehaviorServer *server = BehaviorServer::get_singleton();
//     REQUIRE(server);

    

//     RID graph = server->behavior_tree_graph_create();
//     CHECK(graph.is_valid());

//     RID selector_node = server->graph_create_node(graph, SelectorNode::get_node_name());
//     CHECK(selector_node.is_valid());


// }

}