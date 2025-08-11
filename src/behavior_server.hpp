//
// Created by tkey on 4/2/25.
//

#ifndef BEHAVIOR_SERVER_HPP
#define BEHAVIOR_SERVER_HPP

#include <cstdint>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/rid_owner.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include "behavior_trees/behavior_tree_graph.hpp"
#include "godot_cpp/classes/packed_data_container.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"

#include "behavior_trees/behavior_trees.hpp"
#include "behavior_trees/behavior_tree_graph.hpp"
#include "blackboard.hpp"

namespace hydrogen {

using namespace godot;
using namespace behavior_trees;

class BehaviorServer final : public Object {
	GDCLASS(BehaviorServer, Object);

	static BehaviorServer *singleton;

	HashMap<RID, LocalVector<RID>> blackboard_parents_to_children = {};
	RID_PtrOwner<Blackboard> blackboard_owner = {};
	RID_PtrOwner<BehaviorTree> behavior_tree_owner = {};
	RID_PtrOwner<BehaviorTreeGraph> behavior_tree_graph_owner = {};
	Ref<Mutex> blackboard_mutex = {};
	Ref<Mutex> behavior_tree_mutex = {};
	Ref<Mutex> behavior_tree_graph_mutex = {};

	template <typename T>
	void free_ptr_resource(RID_PtrOwner<T> &p_owner, Ref<Mutex> &p_mutex, RID p_rid, std::function<void(T*)> p_cleanup = nullptr) {
		ERR_FAIL_COND(p_mutex.is_null());
		ERR_FAIL_COND(p_rid == RID());
		p_mutex->lock();

		T *resource = p_owner.get_or_null(p_rid);
		if (likely(p_cleanup != nullptr)) {
			p_cleanup(resource);
		}
		p_owner.free(p_rid);
		memdelete(resource);

		p_mutex->unlock();
	}

	// ---- Blackboard ----

	void blackboard_add_child(RID parent, RID child);
	void blackboard_remove_child(RID parent, RID child);
	void blackboard_erase(Blackboard *blackboard);

	void blackboard_emit_set_parent(RID p_child_rid, RID p_parent_rid);
	void blackboard_emit_created(RID blackboard);
	void blackboard_emit_destroyed(RID p_blackboard_rid);

	// ---- Blackboard END ----

	// ---- Behavior Tree ----

	void behavior_tree_graph_erase(BehaviorTreeGraph *p_graph);
	void behavior_tree_erase(BehaviorTree *p_behavior_tree);

	void behavior_tree_graph_emit_created(RID p_behavior_tree_graph);
	void behavior_tree_graph_emit_destroyed(RID p_behavior_tree_graph);

	void behavior_tree_emit_created(RID p_behavior_tree);
	void behavior_tree_emit_destroyed(RID p_behavior_tree);

	// ---- Behavior Tree END ----

	template <typename T>
	void cleanup_resources(RID_PtrOwner<T> &p_owner) {
		if (unlikely(p_owner.get_rid_count() == 0)) {
			return;
		}

		List<RID> owned_list = {};
		p_owner.get_owned_list(&owned_list);

	}

protected:
	static void _bind_methods();

public:
	static BehaviorServer *get_singleton();

	BehaviorServer();
	~BehaviorServer() override;

	void blackboards_lock() const;
	void blackboards_unlock() const;

	void behavior_tree_lock() const;
	void behavior_tree_unlock() const;

	void behavior_tree_graph_lock() const;
	void behavior_tree_graph_unlock() const;

	void free_rid(RID p_rid);

	RID blackboard_create();
	RID behavior_tree_graph_create();
	RID behavior_tree_create(RID p_blackboard, RID p_behavior_tree_graph);
	RID state_machine_create();
	RID agent_create();
	// RID pipeline_context_create(RID p_blackboard, RID p_pipeline);

	Error init();
	void finish();

	// ---- Blackboard ----

	bool blackboard_is_empty(RID p_blackboard_rid);
	uint32_t blackboard_get_size(RID p_blackboard_rid);

	bool blackboard_set_parent(RID p_rid, RID p_parent_rid);
	RID blackboard_get_parent(RID p_rid);
	bool blackboard_is_ancestor(RID p_rid, RID p_candidate);

	template<typename T>
	bool blackboard_try_get_entry(RID p_rid, const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	template<typename T>
	const T &blackboard_get_entry_fast(RID p_rid, const StringName &p_name, const T& p_default = {}, bool p_check_parents = true);

	template <typename T>
	T blackboard_get_entry(RID p_rid, const StringName &p_name, T p_default = {}, bool p_check_parents = true);

	template<typename T>
	void blackboard_set_entry_fast(RID p_rid, const StringName &p_name, const T &p_value);

	template <typename T>
	void blackboard_set_entry(RID p_rid, const StringName &p_name, T p_value);

	bool blackboard_erase_entry(RID p_rid, const StringName &p_name);

	bool blackboard_has_entry(RID p_rid, const StringName &p_name, bool p_check_parents = true);

	bool blackboard_import_entries(RID p_rid, const TypedDictionary<StringName, Variant> &p_data);
	Dictionary blackboard_export_entries(RID p_rid, bool p_include_parents = true);

	static Dictionary blackboard_export_type_infos();

	// ---- Blackboard END ----

	// ---- Behavior Tree ----

	RID behavior_tree_graph_create_node(RID p_graph_id, const StringName &p_node_type_name);
	bool behavior_tree_graph_destroy_node(RID p_graph_id, RID p_node_id);
	bool behavior_tree_graph_is_bound(RID p_graph_id);
	bool behavior_tree_graph_set_root(RID p_graph_id, RID p_node_id);
	RID behavior_tree_graph_get_root(RID p_graph_id);
	int32_t behavior_tree_graph_get_input_port_count(RID p_graph_id, RID p_node_id);
	int32_t behavior_tree_graph_get_output_port_count(RID p_graph_id, RID p_node_id);
	StringName behavior_tree_graph_get_input_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);
	StringName behavior_tree_graph_get_output_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);
	StringName behavior_tree_graph_get_input_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);
	StringName behavior_tree_graph_get_output_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);
	TypedArray<Dictionary> behavior_tree_graph_get_input_port_infos(RID p_graph_id, RID p_node_id);
	TypedArray<Dictionary> behavior_tree_graph_get_output_port_infos(RID p_graph_id, RID p_node_id);
	TypedArray<RID> behavior_tree_graph_get_unrooted_nodes(RID p_graph_id);
	TypedArray<RID> behavior_tree_graph_get_rooted_nodes(RID p_graph_id);

	// ---- Behavior Tree END ----
};

class HydrogenBehaviorServer final : public Object {
	GDCLASS(HydrogenBehaviorServer, Object);

	friend class BehaviorServer;
	static HydrogenBehaviorServer *singleton;

	void _blackboard_emit_set_parent(RID parent, RID child);
	void _blackboard_emit_created(RID p_blackboard_rid);
	void _blackboard_emit_destroyed(RID p_blackboard_rid);

	void _behavior_tree_graph_emit_created(RID p_behavior_tree_graph);
	void _behavior_tree_graph_emit_destroyed(RID p_behavior_tree_graph);

	void _behavior_tree_emit_created(RID p_behavior_tree);
	void _behavior_tree_emit_destroyed(RID p_behavior_tree);

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

	void free_rid(RID p_rid);

	RID blackboard_create();
	RID behavior_tree_graph_create();
	RID behavior_tree_create(RID p_blackboard, RID p_behavior_tree_graph);
	RID state_machine_create();
	RID agent_create();

	// ---- Blackboard ----

	void blackboards_lock() const;
	void blackboards_unlock() const;

	bool blackboard_is_empty(RID p_blackboard_rid) const;
	uint32_t blackboard_get_size(RID p_blackboard_rid) const;

	bool blackboard_set_parent(RID p_parent_rid, RID p_child_rid);
	RID blackboard_get_parent(RID p_blackboard_rid);
	bool blackboard_is_ancestor(RID p_blackboard_rid, RID p_candidate);

	template <typename T>
	bool blackboard_try_get_entry(RID p_blackboard_rid, const StringName &p_name, T &p_out_result, bool p_check_parents = true);

	Variant blackboard_try_get_as_variant(RID p_blackboard_rid, const StringName &p_name, bool p_check_parents = true);

	template <typename T>
	const T &blackboard_get_entry_fast(RID p_blackboard_rid, const StringName &p_name, const T &p_default = {}, bool p_check_parents = true);

	template <typename T>
	T blackboard_get_entry(RID p_blackboard_rid, const StringName &p_name, T p_default = {}, bool p_check_parents = true);

	template <typename T>
	void blackboard_set_entry_fast(RID p_blackboard_rid, const StringName &p_name, const T &p_value);

	template <typename T>
	void blackboard_set_entry(RID p_blackboard_rid, const StringName &p_name, T p_default = {});

	bool blackboard_erase_entry(RID p_blackboard_rid, const StringName &p_name);

	bool blackboard_has_entry(RID p_blackboard_rid, const StringName &p_name, bool p_check_parents = true);

	bool blackboard_import_entries(RID p_blackboard_rid, const TypedDictionary<StringName, Variant> &p_data);
	Dictionary blackboard_export_entries(RID p_blackboard_rid, bool p_include_parents = true);

	Dictionary blackboard_export_type_infos();

	// ---- Blackboard END ----

	// ---- Behavior Tree ----

	RID behavior_tree_graph_create_node(RID p_graph_id, const StringName &p_name);
	bool behavior_tree_graph_destroy_node(RID p_graph_id, RID p_node_id);

	bool behavior_tree_graph_is_bound(RID p_graph_id);

	bool behavior_tree_graph_set_root(RID p_graph_id, RID p_node_id);
	RID behavior_tree_graph_get_root(RID p_graph_id);

	int32_t behavior_tree_graph_get_input_port_count(RID p_graph_id, RID p_node_id);
	int32_t behavior_tree_graph_get_output_port_count(RID p_graph_id, RID p_node_id);

	StringName behavior_tree_graph_get_input_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);
	StringName behavior_tree_graph_get_output_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);

	StringName behavior_tree_graph_get_input_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);
	StringName behavior_tree_graph_get_output_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index);

	TypedArray<Dictionary> behavior_tree_graph_get_input_port_infos(RID p_graph_id, RID p_node_id);
	TypedArray<Dictionary> behavior_tree_graph_get_output_port_infos(RID p_graph_id, RID p_node_id);

	TypedArray<RID> behavior_tree_graph_get_unrooted_nodes(RID p_graph_id);
	TypedArray<RID> behavior_tree_graph_get_rooted_nodes(RID p_graph_id);



	// ---- Behavior Tree END ----

#if TESTS_ENABLED
	void run_tests();
#endif

};

template <typename T>
bool BehaviorServer::blackboard_try_get_entry(RID p_blackboard_rid, const StringName &p_name, T &p_out_result, bool p_check_parents) {
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	ERR_FAIL_NULL_V(blackboard, false);
	return blackboard->try_get_entry(p_name, p_out_result, p_check_parents);
}

template <typename T>
const T &BehaviorServer::blackboard_get_entry_fast(RID p_blackboard_rid, const StringName &p_name, const T &p_default, bool p_check_parents) {
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	ERR_FAIL_NULL_V(blackboard, p_default);
	return blackboard->get_entry_fast(p_name, p_default, p_check_parents);
}

template <typename T>
T BehaviorServer::blackboard_get_entry(RID p_blackboard_rid, const StringName &p_name, T p_default, bool p_check_parents) {
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	ERR_FAIL_NULL_V(blackboard, p_default);
	return blackboard->get_entry(p_name, p_default, p_check_parents);
}

template <typename T>
void BehaviorServer::blackboard_set_entry_fast(RID p_blackboard_rid, const StringName &p_name, const T &p_value) {
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	ERR_FAIL_NULL(blackboard);
	blackboard->set_entry_fast(p_name, p_value);
}

template <typename T>
void BehaviorServer::blackboard_set_entry(RID p_blackboard_rid, const StringName &p_name, T p_value) {
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	ERR_FAIL_NULL(blackboard);
	blackboard->set_entry(p_name, p_value);
}

template <typename T>
bool HydrogenBehaviorServer::blackboard_try_get_entry(RID p_blackboard_rid, const StringName &p_name, T &p_out_result, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_try_get_entry<T>(p_blackboard_rid, p_name, p_out_result, p_check_parents);
}

inline Variant HydrogenBehaviorServer::blackboard_try_get_as_variant(RID p_blackboard_rid, const StringName &p_name, bool p_check_parents) {
	Variant out_variant = Variant();
	blackboard_try_get_entry(p_blackboard_rid, p_name, out_variant, p_check_parents);
	return out_variant;
}

template <typename T>
const T &HydrogenBehaviorServer::blackboard_get_entry_fast(RID p_blackboard_rid, const StringName &p_name, const T &p_default, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_get_entry_fast<T>(p_blackboard_rid, p_name, p_default, p_check_parents);
}

template <typename T>
T HydrogenBehaviorServer::blackboard_get_entry(RID p_blackboard_rid, const StringName &p_name, T p_default, bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_get_entry<T>(p_blackboard_rid, p_name, p_default, p_check_parents);
}

template <typename T>
void HydrogenBehaviorServer::blackboard_set_entry_fast(RID p_blackboard_rid, const StringName &p_name, const T &p_value) {
	BehaviorServer::get_singleton()->blackboard_set_entry_fast<T>(p_blackboard_rid, p_name, p_value);
}

template <typename T>
void HydrogenBehaviorServer::blackboard_set_entry(RID p_blackboard_rid, const StringName &p_name, T p_default) {
	BehaviorServer::get_singleton()->blackboard_set_entry<T>(p_blackboard_rid, p_name, p_default);
}

}


#endif //BEHAVIOR_SERVER_HPP