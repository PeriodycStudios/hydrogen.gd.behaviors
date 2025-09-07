#define DOCTEST_CONFIG_IMPLEMENT_WITH_DLL
#include <doctest.h>

#include "behavior_server.hpp"

namespace hydrogen::test {
using namespace godot;

TEST_CASE("[Hydrogen][Game AI][Behavior Trees] Simple Create and Destroy") {
    BehaviorServer *server = BehaviorServer::get_singleton()->get_singleton();
    REQUIRE(server);

    RID blackboard = server->blackboard_create();
    CHECK(blackboard.is_valid());

    RID graph = server->graph_create(BehaviorServer::BehaviorTree_name());
    CHECK(graph.is_valid());
    CHECK_FALSE(server->graph_is_bound(graph));
    
    RID selector_node = server->graph_create_node(graph, "SelectorNode", {}, {});
    CHECK(selector_node.is_valid());

    CHECK(server->node_is_composite(graph, selector_node));

    RID behavior_tree = server->pipeline_create(graph, blackboard);
    RID bt2 = server->pipeline_create(graph);

    CHECK(server->graph_is_bound(graph));

    server->free_rid(behavior_tree);

    CHECK_FALSE(server->graph_is_bound(graph));

    server->free_rid(graph);
    server->free_rid(bt2);
    server->free_rid(blackboard);
}

}