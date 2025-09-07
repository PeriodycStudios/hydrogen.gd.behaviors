//
// Created by tkey on 4/2/25.
//

#ifndef BEHAVIOR_SERVER_HPP
#define BEHAVIOR_SERVER_HPP

#include "blackboard.hpp"
#include "godot_cpp/templates/hash_set.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "name_helpers.hpp"
#include "pipelines/node_interfaces.hpp"

#include <functional>
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

#define BLACKBOARDS_LOCK() LOCK_ONE(_blackboard_mutex)
#define BLACKBOARDS_LOCK_V(fail_result)	LOCK_ONE_V(_blackboard_mutex, fail_result)

#define TRY_GET_BLACKBOARD_V(fail_result)									\
	BLACKBOARDS_LOCK_V(fail_result);										\
	Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);	\
	ERR_FAIL_NULL_V(blackboard, fail_result)								\

#define TRY_GET_BLACKBOARD()												\
	BLACKBOARDS_LOCK();														\
	Blackboard *blackboard = _blackboard_owner.get_or_null(p_blackboard);	\
	ERR_FAIL_NULL(blackboard)												\

class BehaviorServer final : public Object {
	GDCLASS(BehaviorServer, Object);

	typedef std::function<RID(const StringName &)> create_graph_type;
	typedef std::function<RID(const StringName &, const Blackboard *, IPipelineGraph *, bool)> create_pipeline_type;

	struct PipelineRegistration {
		create_graph_type create_graph;
		create_pipeline_type create_pipeline;

		PipelineRegistration(create_graph_type p_create_graph, create_pipeline_type p_create_pipeline) {
			create_graph = p_create_graph;
			create_pipeline = p_create_pipeline;
		}
	};

	static BehaviorServer *_singleton;

	HashMap<StringName, PipelineRegistration> _pipeline_db = {};
	HashMap<RID, LocalVector<RID>> _blackboard_parents_to_children = {};
	RID_PtrOwner<Blackboard> _blackboard_owner = {};
	RID_PtrOwner<IPipelineGraph> _graph_owner = {};
	RID_PtrOwner<IPipeline> _pipeline_owner = {};
	std::recursive_mutex *_blackboard_mutex = nullptr;
	std::recursive_mutex *_graph_mutex = nullptr;
	std::recursive_mutex *_pipeline_mutex = nullptr;

	template <typename T>
	void _free_ptr_resource(RID_PtrOwner<T> &p_owner, std::recursive_mutex *p_mutex, RID p, std::function<void(T*)> p_cleanup = nullptr) {
		ERR_FAIL_NULL(p_mutex);
		ERR_FAIL_COND(!p.is_valid());
		std::scoped_lock lock(*p_mutex);

		T *resource = p_owner.get_or_null(p);
		if (likely(p_cleanup != nullptr)) {
			p_cleanup(resource);
		}
		p_owner.free(p);
		memdelete(resource);
	}

	RID _blackboard_create_helper() {
		Blackboard *blackboard = memnew(Blackboard);
		RID rid = _blackboard_owner.make_rid(blackboard);
		blackboard->set_self(rid);

		return rid;
	}

	template<typename TGRAPH>
	RID _graph_create_helper(const StringName &p_name_key) {
		TGRAPH *graph = memnew(TGRAPH(p_name_key));
		RID rid = _graph_owner.make_rid(graph);
		graph->set_self(rid);

		_graph_emit_created(rid);

		return rid;
	}

	template<typename TPIPELINE, typename TGRAPH> 
	RID _pipeline_create_helper(const StringName &p_name_key, const Blackboard * p_blackboard, IPipelineGraph *p_graph, bool p_owns_source_blackboard) {
		ERR_FAIL_NULL_V(p_blackboard, RID());
		ERR_FAIL_NULL_V(p_graph, RID());

		TGRAPH *graph = dynamic_cast<TGRAPH *>(p_graph);
		ERR_FAIL_NULL_V(graph, RID());

		TPIPELINE *pipeline = memnew(TPIPELINE(p_name_key, p_blackboard, graph, p_owns_source_blackboard));
		RID rid = _pipeline_owner.make_rid(pipeline);
		pipeline->set_self(rid);

		_blackboard_acquire(pipeline->get_execution_blackboard());

		_pipeline_emit_created(rid);

		return rid;
	}

	// ---- Blackboard ----

	void _blackboard_acquire(Blackboard *p_blackboard);
	void _blackboard_release(Blackboard *p_blackboard);

	void _blackboard_add_child(RID parent, RID child);
	void _blackboard_remove_child(RID parent, RID child);
	void _blackboard_erase(Blackboard *blackboard, bool p_is_final_shutdown = false);

	void _blackboard_emit_set_parent(RID p_child, RID p_parent);
	void _blackboard_emit_created(RID blackboard);
	void _blackboard_emit_destroyed(RID p_blackboard);

	// ---- Blackboard END ----

	// ---- Graphs ----
	
	void _graph_erase(IPipelineGraph *p_graph, bool p_is_final_shutdown = false);
	void _graph_emit_created(RID p_behavior_tree_graph);
	void _graph_emit_destroyed(RID p_behavior_tree_graph);

	// ---- Graphs END ----

	// ---- Pipelines ----
	
	void _pipeline_erase(IPipeline *p_pipeline, bool p_is_final_shutdown = false);
	void _pipeline_emit_created(RID p_behavior_tree);
	void _pipeline_emit_destroyed(RID p_behavior_tree);
	
	// ---- Pipelines END ----

protected:
	static void _bind_methods();

public:

	DEFINE_NAME_STATIC(BehaviorTree);

	static BehaviorServer *get_singleton();

	BehaviorServer();
	~BehaviorServer() override;

	template<typename TGRAPH, typename TPIPELINE>
	bool register_graph_group(const StringName &p_graph_group) {
		LOCK_TWO_V(_pipeline_mutex, _graph_mutex, false);

		if (unlikely(_pipeline_db.has(p_graph_group))) {
			return false;
		}

		create_graph_type create_graph_func = [this](const StringName &p_group_key) -> RID {
			return _graph_create_helper<TGRAPH>(p_group_key);
		};
		create_pipeline_type create_pipeline_func = [this](const StringName &p_group_key, const Blackboard *p_blackboard, IPipelineGraph *p_graph, bool p_owns_source_blackboard) -> RID {
			return _pipeline_create_helper<TPIPELINE, TGRAPH>(p_group_key, p_blackboard, p_graph, p_owns_source_blackboard);
		};

		_pipeline_db.insert(p_graph_group, PipelineRegistration(create_graph_func, create_pipeline_func));
		return true;
	}

	bool unregister_graph_group(const StringName &p_graph_group) {
		LOCK_TWO_V(_pipeline_mutex, _graph_mutex, false);

		if (unlikely(!_pipeline_db.has(p_graph_group))) {
			return false;
		}

		const uint32_t pipeline_rid_count = _pipeline_owner.get_rid_count();
		TightLocalVector<RID> pipeline_rids = TightLocalVector<RID>();
		pipeline_rids.resize(pipeline_rid_count);
		_pipeline_owner.fill_owned_buffer(pipeline_rids.ptr());

		TightLocalVector<RID> destroy_pipelines = {};
		HashSet<RID> destroy_graphs = {};
		destroy_pipelines.reserve(pipeline_rid_count);
		for (int32_t idx = 0; idx < pipeline_rid_count; ++idx) {
			const IPipeline *pipeline = _pipeline_owner.get_or_null(pipeline_rids[idx]);
			if (pipeline->group_key() == p_graph_group) {
				destroy_pipelines.push_back(pipeline->get_id());
				const IPipelineGraph *graph = pipeline->get_graph();
				RID graph_rid = graph->get_id();
				destroy_graphs.insert(graph_rid);
			}
		}

		for(RID rid : pipeline_rids) {
			free_rid(rid);
		}

		for (RID rid : destroy_graphs) {
			free_rid(rid);
		}

		_pipeline_db.erase(p_graph_group);
		return true;
	}

	void free_rid(RID p);

	RID blackboard_create();
	RID graph_create(const StringName &p_graph_group);
	RID behavior_tree_graph_create() { return graph_create(BehaviorTree_name()); }
	RID pipeline_create(RID p_graph, RID p_blackboard = {});
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

	const StringName &graph_get_type(RID p_graph);
	const StringName &graph_get_group_key(RID p_graph);
	bool graph_is_bound(RID p_graph);
	uint32_t graph_get_bind_count(RID p_graph);

	RID graph_create_node(RID p_graph, const StringName &p_node_type_name, const PortAliases &p_input_aliases, const PortAliases &p_output_aliases);
	bool graph_destroy_node(RID p_graph, RID p_node);
	
	uint64_t graph_get_node_count(RID p_graph);
	TypedArray<RID> graph_get_sub_graphs(RID p_graph);
	TypedArray<RID> graph_get_nodes(RID p_graph);
	bool graph_set_root(RID p_graph, RID p_node);
	RID graph_get_root(RID p_graph);
	TypedArray<RID> graph_query_node(RID p_graph, RID p_node, Callable p_predicate);
	TypedArray<RID> graph_query_nodes(RID p_graph, Callable p_predicate);
	TypedArray<Dictionary> graph_get_rooted_statuses(RID p_graph);
	bool graph_node_is_parented(RID p_graph, RID p_node);
	// ---- Graphs END ----

	// ---- Nodes ----

	const StringName &node_get_type_name(RID p_graph, RID p_node);

	TypedArray<Dictionary> node_get_ports(RID p_graph, RID p_node);
	TypedArray<Dictionary> node_get_connections(RID p_graph, RID p_node);

	bool node_set_connection(RID p_graph, RID p_node, const StringName &p_name, RID p_target_node);
	RID node_get_connection(RID p_graph, RID p_node, const StringName &p_name);

	bool node_is_parent(RID p_graph, RID p_node);
	bool node_is_composite(RID p_graph, RID p_node);
	bool node_is_decorator(RID p_graph, RID p_node);

	void node_set_name(RID p_graph, RID p_node, const String &p_name);
	const String &node_get_name(RID p_graph, RID p_node);
	
	void node_set_input_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases);
	PortAliases get_input_aliases(RID p_graph, RID p_node);

	void node_set_output_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases);
	PortAliases node_get_output_aliases(RID p_graph, RID p_node);

	bool node_supports_children(RID p_graph, RID p_node);
	bool node_has_children(RID p_graph, RID p_node);
	TypedArray<RID> node_get_children(RID p_graph, RID p_node);
	TypedArray<RID> node_get_descendants(RID p_graph, RID p_node);
	bool node_has_child(RID p_graph, RID p_node, RID p_candidate);
	bool node_has_descendant(RID p_graph, RID p_node, RID p_candidate);

	bool node_parent_has_child(RID p_graph, RID p_node, RID p_child);

	bool node_composite_add_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child_at(RID p_graph, RID p_node, int64_t p_child_index);
	void node_composite_clear(RID p_graph, RID p_node);
	RID node_composite_get_child(RID p_graph, RID p_node, int64_t p_child_index);
	void node_composite_set_child(RID p_graph, RID p_node, int64_t p_child_index, RID p_child);
	int64_t node_composite_child_count(RID p_graph, RID p_node);
	void node_composite_swap_children(RID p_graph, RID p_node, uint64_t p_first_index, uint64_t p_second_index);
	Error node_composite_insert_child(RID p_graph, RID p_node, int64_t p_pos, RID p_child);
	void node_composite_append_children(RID p_graph, RID p_node, const TypedArray<RID> &p_children);
	
	RID node_decorator_get_child(RID p_graph, RID p_node);
	void node_decorator_set_child(RID p_graph, RID p_node, RID p_child);

	// ---- Nodes END ----

	// ---- Pipelines ----

	const StringName &pipeline_get_group_key(RID p_pipeline);

	void pipeline_execute(RID p_pipeline);
	void pipeline_halt(RID p_pipeline);

	RID pipeline_get_execution_blackboard(RID p_pipeline);
	RID pipeline_get_source_blackboard(RID p_pipeline);
	RID pipeline_get_graph(RID p_pipeline);
	String pipeline_get_error(RID p_pipeline);

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
	TRY_GET_BLACKBOARD();
	blackboard->set_entry_fast(p_name, p_value);
}

template <typename T>
void BehaviorServer::blackboard_set_entry(RID p_blackboard, const StringName &p_name, T p_value) {
	TRY_GET_BLACKBOARD();
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
	RID graph_create(const StringName &p_registration_key);
	RID behavior_tree_graph_create();
	RID pipeline_create(RID p_graph, RID p_blackboard = {});
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

	const StringName &graph_get_type(RID p_graph);
	const StringName &graph_get_group_key(RID p_graph);
	bool graph_is_bound(RID p_graph);
	uint32_t graph_get_bind_count(RID p_graph);

	RID graph_create_node(RID p_graph, const StringName &p_node_type_name, const PortAliases &p_input_aliases = {}, const PortAliases &p_output_aliases = {});
	bool graph_destroy_node(RID p_graph, RID p_node);

	uint64_t graph_get_node_count(RID p_graph);
	TypedArray<RID> graph_get_sub_graphs(RID p_graph);
	TypedArray<RID> graph_get_nodes(RID p_graph);
	bool graph_set_root(RID p_graph, RID p_node);
	RID graph_get_root(RID p_graph);
	TypedArray<RID> graph_query_node(RID p_graph, RID p_node, Callable p_predicate);
	TypedArray<RID> graph_query_nodes(RID p_graph, Callable p_predicate);
	TypedArray<Dictionary> graph_get_rooted_statuses(RID p_graph);
	bool graph_node_is_parented(RID p_graph, RID p_node);

	// ---- Graphs END ----

	// ---- Nodes ----

	const StringName &node_get_type_name(RID p_graph, RID p_node);

	TypedArray<Dictionary> node_get_ports(RID p_graph, RID p_node);
	TypedArray<Dictionary> node_get_connections(RID p_graph, RID p_node);

	bool node_set_connection(RID p_graph, RID p_node, const StringName &p_name, RID p_target_node);
	RID node_get_connection(RID p_graph, RID p_node, const StringName &p_name);

	bool node_is_parent(RID p_graph, RID p_node);
	bool node_is_composite(RID p_graph, RID p_node);
	bool node_is_decorator(RID p_graph, RID p_node);

	void node_set_name(RID p_graph, RID p_node, const String &p_name);
	const String &node_get_name(RID p_graph, RID p_node);
	
	void node_set_input_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases);
	PortAliases node_get_input_aliases(RID p_graph, RID p_node);

	void node_set_output_aliases(RID p_graph, RID p_node, const PortAliases &p_aliases);
	PortAliases node_get_output_aliases(RID p_graph, RID p_node);

	bool node_supports_children(RID p_graph, RID p_node);
	bool node_has_children(RID p_graph, RID p_node);
	TypedArray<RID> node_get_children(RID p_graph, RID p_node);
	TypedArray<RID> node_get_descendants(RID p_graph, RID p_node);
	bool node_has_child(RID p_graph, RID p_node, RID p_candidate);
	bool node_has_descendant(RID p_graph, RID p_node, RID p_candidate);

	bool node_parent_has_child(RID p_graph, RID p_node, RID p_child);

	bool node_composite_add_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child(RID p_graph, RID p_node, RID p_child);
	bool node_composite_remove_child_at(RID p_graph, RID p_node, int64_t p_child_index);
	void node_composite_clear(RID p_graph, RID p_node);
	RID node_composite_get_child(RID p_graph, RID p_node, int64_t p_child_index);
	void node_composite_set_child(RID p_graph, RID p_node, int64_t p_child_index, RID p_child);
	int64_t node_composite_child_count(RID p_graph, RID p_node);
	void node_composite_swap_children(RID p_graph, RID p_node, uint64_t p_first_index, uint64_t p_second_index);
	Error node_composite_insert_child(RID p_graph, RID p_node, int64_t p_pos, RID p_child);
	void node_composite_append_children(RID p_graph, RID p_node, const TypedArray<RID> &p_childs);
	
	RID node_decorator_get_child(RID p_graph, RID p_node);
	void node_decorator_set_child(RID p_graph, RID p_node, RID p_child);

	// ---- Nodes END ----

	// ---- Pipelines ----

	const StringName &pipeline_get_group_key(RID p_pipeline);

	void pipeline_execute(RID p_pipeline);
	void pipeline_halt(RID p_pipeline);

	RID pipeline_get_execution_blackboard(RID p_pipeline);
	RID pipeline_get_source_blackboard(RID p_pipeline);
	RID pipeline_get_graph(RID p_pipeline);
	String pipeline_get_error(RID p_pipeline);

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


#endif //BEHAVIOR_SERVER_HPP