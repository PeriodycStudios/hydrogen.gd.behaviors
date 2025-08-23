//
// Created by tkey on 4/2/25.
//

#ifndef BEHAVIOR_SERVER_HPP
#define BEHAVIOR_SERVER_HPP

#include "blackboard.hpp"
#include "pipelines/node_interfaces.hpp"
#include "pipelines/pipeline.hpp"

#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

#include <cstdint>
#include <mutex>
#include "mutex_helpers.hpp"

namespace hydrogen {

using namespace godot;
using namespace pipelines;

#define BLACKBOARDS_LOCK LOCK_ONE(_blackboard_mutex)
#define BLACKBOARDS_LOCK_V(fail_result)	LOCK_ONE_V(_blackboard_mutex, fail_result)

#define TRY_GET_BLACKBOARD_V(fail_result)									\
	BLACKBOARDS_LOCK_V(fail_result);										\
	Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);	\
	ERR_FAIL_NULL_V(blackboard, fail_result)								\

#define TRY_GET_BLACKBOARD													\
	BLACKBOARDS_LOCK;														\
	Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);	\
	ERR_FAIL_NULL(blackboard)												\

class BehaviorServer final : public Object {
	GDCLASS(BehaviorServer, Object);

public:
	enum GraphType {
		BEHAVIOR_TREE,
		// FSM,
		GRAPH_TYPE_MAX
	};

private:

	static BehaviorServer *_singleton;

	HashMap<RID, LocalVector<RID>> _blackboard_parents_to_children = {};
	RID_PtrOwner<Blackboard> _blackboard_owner = {};
	RID_PtrOwner<IPipelineGraph> _graph_owner = {};
	RID_PtrOwner<Pipeline> _pipeline_owner = {};
	std::mutex *_blackboard_mutex = nullptr;
	std::mutex *_graph_mutex = nullptr;
	std::mutex *_pipeline_mutex = nullptr;

	template <typename T>
	void _free_ptr_resource(RID_PtrOwner<T> &p_owner, std::mutex *p_mutex, RID p, std::function<void(T*)> p_cleanup = nullptr) {
		ERR_FAIL_NULL(p_mutex);
		ERR_FAIL_COND(p == RID());
		std::scoped_lock lock(*p_mutex);

		T *resource = p_owner.get_or_null(p);
		if (likely(p_cleanup != nullptr)) {
			p_cleanup(resource);
		}
		p_owner.free(p);
		memdelete(resource);
	}

	RID _blackboard_create_helper() {
		BLACKBOARDS_LOCK_V(RID());

		Blackboard *blackboard = memnew(Blackboard);
		RID rid = _blackboard_owner.make_rid(blackboard);
		blackboard->set_self(rid);

		return rid;
	}

	template<typename TGRAPH>
	RID _graph_create_helper() {
		LOCK_ONE_V(_graph_mutex, RID());

		TGRAPH *graph = memnew(TGRAPH);
		RID rid = _graph_owner.make_rid(graph);
		graph->set_self(rid);

		_graph_emit_created(rid);

		return rid;
	}

	template<typename TPIPELINE, typename TGRAPH> 
	RID _pipeline_create_helper(RID p_blackboard, RID p_graph) {
		LOCK_THREE_V(_pipeline_mutex, _blackboard_mutex, _graph_mutex, RID());
		
		const Blackboard* blackboard = _blackboard_owner.get_or_null(p_blackboard);
		ERR_FAIL_NULL_V(blackboard, RID());

		TGRAPH *graph = dynamic_cast<TGRAPH *>(_graph_owner.get_or_null(p_graph));
		ERR_FAIL_NULL_V(graph, RID());

		TPIPELINE *pipeline = memnew(TPIPELINE(blackboard, graph));
		RID rid = _pipeline_owner.make_rid(pipeline);
		pipeline->set_self(rid);

		_pipeline_emit_created(rid);

		return rid;
	}

	// ---- Blackboard ----

	void _blackboard_add_child(RID parent, RID child);
	void _blackboard_remove_child(RID parent, RID child);
	void _blackboard_erase(Blackboard *blackboard);

	void _blackboard_emit_set_parent(RID p_child, RID p_parent);
	void _blackboard_emit_created(RID blackboard);
	void _blackboard_emit_destroyed(RID p_blackboard);

	// ---- Blackboard END ----

	// ---- Behavior Tree ----

	void _graph_erase(IPipelineGraph *p_graph);
	void _pipeline_erase(Pipeline *p_pipeline);

	void _graph_emit_created(RID p_behavior_tree_graph);
	void _graph_emit_destroyed(RID p_behavior_tree_graph);

	void _pipeline_emit_created(RID p_behavior_tree);
	void _pipeline_emit_destroyed(RID p_behavior_tree);

	// ---- Behavior Tree END ----

protected:
	static void _bind_methods();

public:
	static BehaviorServer *get_singleton();

	BehaviorServer();
	~BehaviorServer() override;

	void free_rid(RID p);

	RID blackboard_create();
	RID graph_create(GraphType p_graph_type);
	RID pipeline_create(GraphType p_graph_type, RID p_blackboard, RID p_graph);
	RID agent_create();

	Error init();
	void finish();

	// ---- Blackboard ----

	bool blackboard_is_empty(RID p_blackboard);
	uint32_t blackboard_get_size(RID p_blackboard);

	bool blackboard_set_parent(RID p, RID p_parent);
	RID blackboard_get_parent(RID p);
	bool blackboard_is_ancestor(RID p, RID p_candidate);

	template<typename T>
	bool blackboard_try_get_entry(RID p, const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	template<typename T>
	const T &blackboard_get_entry_fast(RID p, const StringName &p_name, const T& p_default = {}, bool p_check_parents = true);

	template <typename T>
	T blackboard_get_entry(RID p, const StringName &p_name, T p_default = {}, bool p_check_parents = true);

	template<typename T>
	void blackboard_set_entry_fast(RID p, const StringName &p_name, const T &p_value);

	template <typename T>
	void blackboard_set_entry(RID p, const StringName &p_name, T p_value);

	bool blackboard_erase_entry(RID p, const StringName &p_name);

	bool blackboard_has_entry(RID p, const StringName &p_name, bool p_check_parents = true);

	bool blackboard_import_entries(RID p, const TypedDictionary<StringName, Variant> &p_data);
	Dictionary blackboard_export_entries(RID p, bool p_include_parents = true);

	static Dictionary blackboard_export_type_infos();

	// ---- Blackboard END ----

	// ---- Graphs ----

	TypedArray<RID> graph_get_sub_graphs(RID p_graph);
	TypedArray<RID> graph_get_nodes(RID p_graph);

	RID graph_create_node(RID p_graph, const StringName &p_node_type_name);
	bool graph_destroy_node(RID p_graph, RID p_node);

	bool graph_is_bound(RID p_graph);
	bool graph_set_root(RID p_graph, RID p_node);
	RID graph_get_root(RID p_graph);
	TypedArray<Dictionary> graph_get_rooted_statuses(RID p_graph);

	// ---- Graphs END ----

	// ---- Nodes ----

	StringName node_get_type_name(RID p_graph, RID p_node);
	bool node_is_compatible(RID p_graph, RID p_node, RID p_other_node);

	TypedArray<Dictionary> node_get_port_infos(RID p_graph, RID p_node);

	bool node_is_composite(RID p_graph, RID p_node);
	bool node_is_decorator(RID p_graph, RID p_node);

	bool node_composite_add_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child_at(RID p_graph, RID p_node, int64_t p_child_index);
	void node_composite_clear(RID p_graph, RID p_node);

	RID node_composite_get_child(RID p_graph, RID p_node, int64_t p_child_index);
	void node_composite_set_child(RID p_graph, RID p_node, int64_t p_child_index, RID p_child);
	int64_t node_composite_child_count(RID p_graph, RID p_node);
	void node_composite_resize(RID p_graph, RID p_node, uint64_t p_size);
	void node_composite_resize_zeroed(RID p_graph, RID p_node, uint64_t p_size);
	void node_composite_swap_children(RID p_graph, RID p_node, uint64_t p_first_index, uint64_t p_second_index);
	Error node_composite_insert_child(RID p_graph, RID p_node, int64_t p_pos, RID p_child);
	void node_composite_append_children(RID p_graph, RID p_node, const Vector<RID> &p_children);
	
	RID node_decorator_get_node(RID p_graph, RID p_node);
	void node_decorator_set_node(RID p_graph, RID p_node, RID p_child);

	// ---- Nodes END ----

	// ---- Pipelines ----

	void pipeline_execute(RID p_pipeline);
	void pipeline_halt(RID p_pipeline);

	// ---- Pipelines END ----
};


template <typename T>
bool BehaviorServer::blackboard_try_get_entry(RID p_blackboard, const StringName &p_name, T &p_out_result, bool p_check_parents) {
	TRY_GET_BLACKBOARD_V(false);
	return blackboard->try_get_entry(p_name, p_out_result, p_check_parents);
}

template <typename T>
const T &BehaviorServer::blackboard_get_entry_fast(RID p_blackboard, const StringName &p_name, const T &p_default, bool p_check_parents) {
	TRY_GET_BLACKBOARD_V(p_default);
	return blackboard->get_entry_fast(p_name, p_default, p_check_parents);
}

template <typename T>
T BehaviorServer::blackboard_get_entry(RID p_blackboard, const StringName &p_name, T p_default, bool p_check_parents) {
	TRY_GET_BLACKBOARD_V(p_default);
	return blackboard->get_entry(p_name, p_default, p_check_parents);
}

template <typename T>
void BehaviorServer::blackboard_set_entry_fast(RID p_blackboard, const StringName &p_name, const T &p_value) {
	TRY_GET_BLACKBOARD;
	blackboard->set_entry_fast(p_name, p_value);
}

template <typename T>
void BehaviorServer::blackboard_set_entry(RID p_blackboard, const StringName &p_name, T p_value) {
	TRY_GET_BLACKBOARD;
	blackboard->set_entry(p_name, p_value);
}

#undef BLACKBOARDS_LOCK
#undef BLACKBOARDS_LOCK_V
#undef TRY_GET_BLACKBOARD
#undef TRY_GET_BLACKBOARD_V

class HydrogenBehaviorServer final : public Object {
	GDCLASS(HydrogenBehaviorServer, Object);

	friend class BehaviorServer;
	static HydrogenBehaviorServer *singleton;

	void _blackboard_emit_set_parent(RID parent, RID child);
	void _blackboard_emit_created(RID p_blackboard);
	void _blackboard_emit_destroyed(RID p_blackboard);

	void _graph_emit_created(RID p_behavior_tree_graph);
	void _graph_emit_destroyed(RID p_behavior_tree_graph);

	void _pipeline_emit_created(RID p_behavior_tree);
	void _pipeline_emit_destroyed(RID p_behavior_tree);

protected:
	static void _bind_methods();

public:
	static HydrogenBehaviorServer *get_singleton();

	HydrogenBehaviorServer() {
		singleton = this;
	};

	~HydrogenBehaviorServer() override {
		singleton = nullptr;
	};

	void free_rid(RID p);

	RID blackboard_create();
	RID graph_create(BehaviorServer::GraphType p_graph_type);
	RID pipeline_create(BehaviorServer::GraphType p_graph_type, RID p_blackboard, RID p_behavior_tree_graph);
	RID agent_create();

	// ---- Blackboard ----

	bool blackboard_is_empty(RID p_blackboard) const;
	uint32_t blackboard_get_size(RID p_blackboard) const;

	bool blackboard_set_parent(RID p_parent, RID p_child);
	RID blackboard_get_parent(RID p_blackboard);
	bool blackboard_is_ancestor(RID p_blackboard, RID p_candidate);

	template <typename T>
	bool blackboard_try_get_entry(RID p_blackboard, const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	Variant blackboard_try_get_as_variant(RID p_blackboard, const StringName &p_name, bool p_check_parents = true);

	template <typename T>
	const T &blackboard_get_entry_fast(RID p_blackboard, const StringName &p_name, const T &p_default = {}, bool p_check_parents = true);

	template <typename T>
	T blackboard_get_entry(RID p_blackboard, const StringName &p_name, T p_default = {}, bool p_check_parents = true);

	template <typename T>
	void blackboard_set_entry_fast(RID p_blackboard, const StringName &p_name, const T &p_value);

	template <typename T>
	void blackboard_set_entry(RID p_blackboard, const StringName &p_name, T p_default = {});

	bool blackboard_erase_entry(RID p_blackboard, const StringName &p_name);
	bool blackboard_has_entry(RID p_blackboard, const StringName &p_name, bool p_check_parents = true);

	bool blackboard_import_entries(RID p_blackboard, const TypedDictionary<StringName, Variant> &p_data);
	Dictionary blackboard_export_entries(RID p_blackboard, bool p_include_parents = true);

	Dictionary blackboard_export_type_infos();

	// ---- Blackboard END ----

	// ---- Graphs ----

	Vector<RID> graph_get_sub_graphs(RID p_graph);
	Vector<RID> graph_get_nodes(RID p_graph);

	RID graph_create_node(RID p_graph, const StringName &p_node_type_name);
	bool graph_destroy_node(RID p_graph, RID p_node);

	bool graph_is_bound(RID p_graph);
	bool graph_set_root(RID p_graph, RID p_node);
	RID graph_get_root(RID p_graph);
	TypedArray<Dictionary> graph_get_rooted_statuses(RID p_graph);

	// ---- Graphs END ----

	// ---- Nodes ----

	StringName node_get_type_name(RID p_graph, RID p_node);
	bool node_is_compatible(RID p_graph, RID p_node, RID p_other_node);

	TypedArray<Dictionary> node_get_port_infos(RID p_graph, RID p_node);

	// int32_t node_get_input_port_count(RID p_graph, RID p_node);
	// int32_t node_get_output_port_count(RID p_graph, RID p_node);
	// StringName node_get_input_port_type_name(RID p_graph, RID p_node, int32_t p_port_index);
	// StringName node_get_output_port_type_name(RID p_graph, RID p_node, int32_t p_port_index);
	// StringName node_get_input_port_name(RID p_graph, RID p_node, int32_t p_port_index);
	// StringName node_get_output_port_name(RID p_graph, RID p_node, int32_t p_port_index);
	// TypedArray<Dictionary> node_get_input_port_infos(RID p_graph, RID p_node);
	// TypedArray<Dictionary> node_get_output_port_infos(RID p_graph, RID p_node);

	void node_is_composite(RID p_graph, RID p_node);
	bool node_is_decorator(RID p_graph, RID p_node);

	bool node_composite_add_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child_at(RID p_graph, RID p_node, int32_t p_child_index);
	void node_composite_clear(RID p_graph, RID p_node);

	RID node_composite_get_child(RID p_graph, RID p_node, int64_t p_child_index);
	bool node_composite_set_child(RID p_graph, RID p_node, int64_t p_child_index, RID p_child);
	int64_t node_composite_child_count(RID p_graph, RID p_node);
	void node_composite_resize(RID p_graph, RID p_node, uint64_t p_size);
	void node_composite_resize_zeroed(RID p_graph, RID p_node, uint64_t p_size);
	void node_composite_swap_children(RID p_graph, RID p_node, uint64_t p_first_index, uint64_t p_second_index);
	Error node_composite_insert_child(RID p_graph, RID p_node, int64_t p_pos, RID p_child);
	void node_composite_append_children(RID p_graph, RID p_node, const TypedArray<RID> &p_childs);
	bool node_composite_is_descendent(RID p_graph, RID p_node, RID p_candidate);
	bool node_composite_is_child(RID p_graph, RID p_node, RID p_candidate);
	
	RID node_decorator_get_child(RID p_graph, RID p_node);
	void node_decorator_set_child(RID p_graph, RID p_node, RID p_child);

	// ---- Nodes END ----

	// ---- Pipelines ----

	// ---- Pipelines END ----

#if TESTS_ENABLED
	void run_tests();
#endif

};

template <typename T>
bool HydrogenBehaviorServer::blackboard_try_get_entry(RID p_blackboard, const StringName &p_name, T &p_out_result, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_try_get_entry<T>(p_blackboard, p_name, p_out_result, p_check_parents);
}

inline Variant HydrogenBehaviorServer::blackboard_try_get_as_variant(RID p_blackboard, const StringName &p_name, bool p_check_parents) {
	Variant out_variant = Variant();
	blackboard_try_get_entry(p_blackboard, p_name, out_variant, p_check_parents);
	return out_variant;
}

template <typename T>
const T &HydrogenBehaviorServer::blackboard_get_entry_fast(RID p_blackboard, const StringName &p_name, const T &p_default, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_get_entry_fast<T>(p_blackboard, p_name, p_default, p_check_parents);
}

template <typename T>
T HydrogenBehaviorServer::blackboard_get_entry(RID p_blackboard, const StringName &p_name, T p_default, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_get_entry<T>(p_blackboard, p_name, p_default, p_check_parents);
}

template <typename T>
void HydrogenBehaviorServer::blackboard_set_entry_fast(RID p_blackboard, const StringName &p_name, const T &p_value) {
	BehaviorServer::get_singleton()->blackboard_set_entry_fast<T>(p_blackboard, p_name, p_value);
}

template <typename T>
void HydrogenBehaviorServer::blackboard_set_entry(RID p_blackboard, const StringName &p_name, T p_default) {
	BehaviorServer::get_singleton()->blackboard_set_entry<T>(p_blackboard, p_name, p_default);
}

}

VARIANT_ENUM_CAST(hydrogen::BehaviorServer::GraphType)


#endif //BEHAVIOR_SERVER_HPP