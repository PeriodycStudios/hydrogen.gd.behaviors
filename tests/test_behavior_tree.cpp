#include "behavior_trees/behavior_tree_node.hpp"
#include "behavior_trees/selector_node.hpp"
#include "behavior_trees/sequence_node.hpp"
#include "behavior_trees/tasks/count_down_node.hpp"
#include "behavior_trees/tasks/wait_nodes.hpp"
#include "behavior_trees/tasks/set_bool_node.hpp"
#include "behavior_trees/tasks/set_variant_node.hpp"
#include "behavior_trees/tasks/print_message_node.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/string_name.hpp" 
#include <chrono>
#include <cstdint>
#include <thread>

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

PortAliases aliases(std::initializer_list<Pair<StringName, StringName>> initializer_list) {
	PortAliases aliases = {};
	auto iter = initializer_list.begin();
	while (iter != initializer_list.end()) {
		aliases.set(iter->first, iter->second);
		++iter;
	}

	return aliases;
}

TEST_CASE("[Hydrogen][Game AI][Behavior Trees] Test Sequence and Wait Nodes") {
    BehaviorServer *server = BehaviorServer::get_singleton();
    REQUIRE(server);

    RID graph = server->behavior_tree_graph_create();
    CHECK(graph.is_valid());
    RID sequence = server->graph_create_node(graph, SequenceNode::get_node_name());
    CHECK(sequence.is_valid());

    server->graph_set_root(graph, sequence);

	const StringName count_name = "count";
	const StringName one_name = "one";
	const StringName two_name = "two";
	const StringName three_name = "three";
	const StringName four_name = "four";
	const StringName value_name = "value";
	const StringName target_name = "target";
	const StringName true_name = "true";
	const StringName complete_name = "complete";
	const StringName stage_name = "stage";
	const StringName delta_name = "delta";
	const StringName millis_delta_name = "millis_delta";
	const StringName duration_name = "duration";
	const StringName ticks_duration_name = "ticks_duration";
	const StringName millis_duration_name = "millis_duration";
	const StringName realtime_duration_name = "realtime_duration";
	const StringName message_name = "message";
	const String message = "Hello World";

	RID countdown1 = server->graph_create_node(graph, CountDownNode::get_node_name());
	CHECK(countdown1.is_valid());

	PortAliases stage1_input_aliases = aliases({{value_name, one_name}});
	PortAliases stage2_input_aliases = aliases({{value_name, two_name}});
	PortAliases stage3_input_aliases = aliases({{value_name, three_name}});
	PortAliases stage4_input_aliases = aliases({{value_name, four_name}});
	PortAliases stage_change_outputs = aliases({{target_name, stage_name}});
	PortAliases complete_flag_inputs = aliases({{value_name, true_name}});
	PortAliases complete_flag_outputs = aliases({{target_name, complete_name}});
	PortAliases wait_ticks_inputs = aliases({{duration_name, ticks_duration_name}});
	PortAliases wait_millis_inputs = aliases({
		{duration_name, millis_duration_name},
		{delta_name, millis_delta_name}
	});
	PortAliases wait_realtime_inputs = aliases({{duration_name, realtime_duration_name}});

	RID set_stage1 = server->graph_create_node(graph, SetVariantNode::get_node_name(), stage1_input_aliases, stage_change_outputs);
	CHECK(set_stage1.is_valid());
	RID wait_seconds1 = server->graph_create_node(graph, WaitForSecondsNode::get_node_name());
	CHECK(wait_seconds1.is_valid());
	RID set_stage2 = server->graph_create_node(graph, SetVariantNode::get_node_name(), stage2_input_aliases, stage_change_outputs);
	CHECK(set_stage2.is_valid());
	RID subsequence = server->graph_create_node(graph, SequenceNode::get_node_name());
	CHECK(subsequence.is_valid());

	RID print_message = server->graph_create_node(graph, PrintMessageNode::get_node_name());
	CHECK(print_message.is_valid());
	RID wait_ticks = server->graph_create_node(graph, WaitForTicksNode::get_node_name(), wait_ticks_inputs);
	CHECK(wait_ticks.is_valid());
	RID set_stage3 = server->graph_create_node(graph, SetVariantNode::get_node_name(), stage3_input_aliases, stage_change_outputs);
	CHECK(set_stage3.is_valid());
	RID wait_millis = server->graph_create_node(graph, WaitForMillisecondsNode::get_node_name(), wait_millis_inputs);
	CHECK(wait_millis.is_valid());
	RID set_stage4 = server->graph_create_node(graph, SetVariantNode::get_node_name(), stage4_input_aliases, stage_change_outputs);
	CHECK(set_stage4.is_valid());

	TypedArray<RID> subsequence_nodes = {};
	subsequence_nodes.push_back(print_message);
	subsequence_nodes.push_back(wait_ticks);
	subsequence_nodes.push_back(set_stage3);
	subsequence_nodes.push_back(wait_millis);
	subsequence_nodes.push_back(set_stage4);

	RID wait_realtime = server->graph_create_node(graph, WaitForRealtimeSeconds::get_node_name(), wait_realtime_inputs);
	CHECK(wait_realtime.is_valid());
	RID set_complete = server->graph_create_node(graph, SetBoolNode::get_node_name(), complete_flag_inputs, complete_flag_outputs);
	CHECK(set_complete.is_valid());

	uint64_t nodes_count = server->graph_get_node_count(graph);

	CHECK_EQ(nodes_count, 13);

	CHECK(server->node_composite_add_child(graph, sequence, countdown1));
	CHECK(server->node_composite_add_child(graph, sequence, set_stage1));
	CHECK(server->node_composite_add_child(graph, sequence, wait_seconds1));
	CHECK(server->node_composite_add_child(graph, sequence, set_stage2));
	CHECK(server->node_composite_add_child(graph, sequence, subsequence));

	CHECK(server->node_has_children(graph, sequence));

	uint64_t sequence_count = server->node_composite_child_count(graph, sequence);
	CHECK_EQ(sequence_count, 5);

	server->node_composite_append_children(graph, subsequence, subsequence_nodes);

	uint64_t subsequence_count = server->node_composite_child_count(graph, subsequence);
	CHECK_EQ(subsequence_count, 5);

	server->node_composite_add_child(graph, sequence, set_complete);
	server->node_composite_add_child(graph, sequence, wait_realtime);

	RID unused_node = server->graph_create_node(graph, PrintMessageNode::get_node_name());

	Error insert_result = server->node_composite_insert_child(graph, sequence, 3, unused_node);
	CHECK_EQ(insert_result, OK);

	RID child_at_3 = server->node_composite_get_child(graph, sequence, 3);
	CHECK_EQ(child_at_3, unused_node);

	server->node_composite_remove_child_at(graph, sequence, 3);

	child_at_3 = server->node_composite_get_child(graph, sequence, 3);
	CHECK_NE(child_at_3, unused_node);

	server->node_composite_add_child(graph, subsequence, unused_node);
	subsequence_count = server->node_composite_child_count(graph, subsequence);
	CHECK_EQ(subsequence_count, 6);
	CHECK(server->node_composite_remove_child(graph, subsequence, unused_node));
	sequence_count = server->node_composite_child_count(graph, subsequence);
	CHECK_EQ(sequence_count, 5);

	TypedArray<Dictionary> rooted_statuses = server->graph_get_rooted_statuses(graph);
	const int64_t status_count = rooted_statuses.size();
	for (int64_t idx = 0; idx < status_count; ++idx) {
		Dictionary entry = rooted_statuses[idx];
		RID node = entry["node"];
		bool is_rooted = entry["is_rooted"];
		if (unlikely(node == unused_node)) {
			CHECK_FALSE(is_rooted);
		}
		else {
			CHECK(is_rooted);
		}
	}

	sequence_count = server->node_composite_child_count(graph, sequence);
	server->node_composite_swap_children(graph, sequence, sequence_count - 2, sequence_count - 1);

	CHECK_EQ(server->node_composite_get_child(graph, sequence, sequence_count - 2), wait_realtime);
	CHECK_EQ(server->node_composite_get_child(graph, sequence, sequence_count - 1), set_complete);

	RID child_at_2 = server->node_composite_get_child(graph, sequence, 2);
	CHECK_EQ(child_at_2, wait_seconds1);

	server->node_composite_set_child(graph, sequence, 2, unused_node);
	CHECK_EQ(server->node_composite_get_child(graph, sequence, 2), unused_node);

	server->node_composite_set_child(graph, sequence, 2, child_at_2);
	CHECK_EQ(server->node_composite_get_child(graph, sequence, 2), child_at_2);
	CHECK_EQ(server->node_composite_get_child(graph, sequence, 2), wait_seconds1);

	CHECK_FALSE(server->node_has_child(graph, sequence, unused_node));
	CHECK_FALSE(server->node_has_child(graph, subsequence, unused_node));
	
	const real_t seconds_delta = 1.0 / 60.0;
	const real_t seconds_duration = 5.0;
	const uint32_t countdown_count = 4;
	const uint64_t milliseconds_delta = 100;
	const uint64_t milliseconds_duration = 500;
	const uint64_t ticks_duration = 6;
	const real_t realtime_duration = 3.0;

	RID parent_blackboard = server->blackboard_create();

	server->blackboard_set_entry<uint32_t>(parent_blackboard, count_name, 4);
	server->blackboard_set_entry<int32_t>(parent_blackboard, one_name, 1);
	server->blackboard_set_entry<int32_t>(parent_blackboard, two_name, 2);
	server->blackboard_set_entry<int32_t>(parent_blackboard, three_name, 3);
	server->blackboard_set_entry<int32_t>(parent_blackboard, four_name, 4);
	server->blackboard_set_entry<bool>(parent_blackboard, true_name, true);
	server->blackboard_set_entry<real_t>(parent_blackboard, delta_name, seconds_delta);
	server->blackboard_set_entry<uint64_t>(parent_blackboard, millis_delta_name, milliseconds_delta);
	server->blackboard_set_entry<real_t>(parent_blackboard, duration_name, seconds_duration);
	server->blackboard_set_entry<uint64_t>(parent_blackboard, ticks_duration_name, ticks_duration);
	server->blackboard_set_entry<uint64_t>(parent_blackboard, millis_duration_name, milliseconds_duration);
	server->blackboard_set_entry<real_t>(parent_blackboard, realtime_duration_name, realtime_duration);
	server->blackboard_set_entry<String>(parent_blackboard, message_name, message);
	
	RID pipeline = server->pipeline_create(graph, parent_blackboard);
	RID blackboard = server->pipeline_get_execution_blackboard(pipeline);

	server->blackboard_set_entry<int32_t>(blackboard, stage_name, 0);
	server->blackboard_set_entry<bool>(blackboard, complete_name, false);

	const StringName &last_result_name = _last_result_name();

	auto check_stage = [=](int32_t p_stage_idx, BTNodeResult p_last_result = BTNodeResult::RUNNING, bool p_is_complete = false) -> void {
		const int32_t stage = server->blackboard_get_entry<int32_t>(blackboard, stage_name);
		const BTNodeResult last_result = server->blackboard_get_entry<BTNodeResult>(blackboard, last_result_name);
		const bool is_complete = server->blackboard_get_entry<bool>(blackboard, complete_name);

		CHECK_EQ(stage, p_stage_idx);
		CHECK_EQ(last_result, p_last_result);
		CHECK_EQ(is_complete, p_is_complete);
	};

	check_stage(0, BTNodeResult::SUCCESS);
	
	for (uint64_t count = 0; count < countdown_count; ++count) {
		server->pipeline_execute(pipeline);
	}
	check_stage(1);

	for (real_t time_remaining = seconds_duration; time_remaining > 0.0; time_remaining -= seconds_delta) {
		server->pipeline_execute(pipeline);
	}
	check_stage(2);

	for (uint64_t ticks = 0; ticks < ticks_duration; ++ticks) {
		server->pipeline_execute(pipeline);
	}
	check_stage(3);

	for (uint64_t millis = 0; millis < milliseconds_duration; millis += milliseconds_delta) {
		server->pipeline_execute(pipeline);
	}
	check_stage(4);

	std::this_thread::sleep_for(std::chrono::milliseconds(3100));

	server->pipeline_execute(pipeline);

	check_stage(4, BTNodeResult::SUCCESS, true);

	server->free_rid(pipeline);
	server->free_rid(graph);
	server->free_rid(parent_blackboard);
}

TEST_CASE("[Hydrogen][Game AI][Behavior Trees] Test Selector Node" * doctest::skip()) {
    BehaviorServer *server = BehaviorServer::get_singleton();
    REQUIRE(server);

    RID graph = server->behavior_tree_graph_create();
    CHECK(graph.is_valid());

    RID selector_node = server->graph_create_node(graph, SelectorNode::get_node_name());
    CHECK(selector_node.is_valid());
}

}