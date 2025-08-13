
#include "behavior_server.hpp"
#include "behavior_trees/behavior_tree_graph.hpp"
#include "behavior_trees/behavior_tree_node.hpp"
#include "behavior_trees/behavior_trees.hpp"
#include "blackboard.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/string_name.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "pipelines/node_interfaces.hpp"
#include <cstdint>

#ifdef TESTS_ENABLED
#include "test_runner.hpp"
#endif

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_dictionary.hpp>

namespace hydrogen {

BehaviorServer *BehaviorServer::singleton = nullptr;

void BehaviorServer::_bind_methods() {}

BehaviorServer *BehaviorServer::get_singleton() {
	return singleton;
}

BehaviorServer::BehaviorServer() {
	singleton = this;
}

BehaviorServer::~BehaviorServer() {
	singleton = nullptr;
}

void BehaviorServer::blackboards_lock() const {
	ERR_FAIL_COND(blackboard_mutex.is_null());
	blackboard_mutex->lock();
}

void BehaviorServer::blackboards_unlock() const {
	ERR_FAIL_COND(blackboard_mutex.is_null());
	blackboard_mutex->unlock();
}

void BehaviorServer::behavior_tree_lock() const {
	ERR_FAIL_COND(behavior_tree_mutex.is_null());
	behavior_tree_mutex->lock();
}

void BehaviorServer::behavior_tree_unlock() const {
	ERR_FAIL_COND(behavior_tree_mutex.is_null());
	behavior_tree_mutex->unlock();
}

#define HANDLE_CLEANUP(type, initials, name)											\
	const static auto initials##_cleanup = [this](type *item) { name##_erase(item); };	\
	free_ptr_resource<type>(name##_owner, name##_mutex, p_rid, initials##_cleanup);		\

void BehaviorServer::free_rid(RID p_rid) {
	if (blackboard_owner.owns(p_rid)) {
		HANDLE_CLEANUP(Blackboard, bb, blackboard)
	}
	else if (behavior_tree_owner.owns(p_rid)) {
		HANDLE_CLEANUP(BehaviorTree, bt, behavior_tree)
	}
	else if (behavior_tree_graph_owner.owns(p_rid)) {
		HANDLE_CLEANUP(BehaviorTreeGraph, btg, behavior_tree_graph)
	}
	else {
		ERR_FAIL_MSG("Invalid ID.");
	}
}

#undef HANDLE_CLEANUP

RID BehaviorServer::blackboard_create() {
	blackboards_lock();

	Blackboard *blackboard = memnew(Blackboard);
	RID rid = blackboard_owner.make_rid(blackboard);
	blackboard->set_self(rid);

	blackboards_unlock();

	blackboard_emit_created(rid);

	return rid;
}

RID BehaviorServer::behavior_tree_graph_create() {
	behavior_tree_lock();

	BehaviorTreeGraph* graph = memnew(BehaviorTreeGraph);
	RID rid = behavior_tree_graph_owner.make_rid(graph);
	graph->set_self(rid);

	behavior_tree_unlock();

	behavior_tree_graph_emit_created(rid);

	return rid;
}

RID BehaviorServer::behavior_tree_create(RID p_blackboard, RID p_behavior_tree_graph) {
	behavior_tree_lock();
	blackboards_lock();

	Blackboard* blackboard = blackboard_owner.get_or_null(p_blackboard);
	ERR_FAIL_NULL_V(blackboard, RID());

	BehaviorTreeGraph* graph = behavior_tree_graph_owner.get_or_null(p_behavior_tree_graph);
	ERR_FAIL_NULL_V(graph, RID());

	BehaviorTree* behavior_tree = memnew(BehaviorTree(blackboard, graph));
	RID rid = behavior_tree_owner.make_rid(behavior_tree);
	behavior_tree->set_self(rid);

	blackboards_unlock();
	behavior_tree_unlock();

	behavior_tree_emit_created(rid);

	return rid;
}

RID BehaviorServer::state_machine_create() {
	return {};
}

RID BehaviorServer::agent_create() {
	return {};
}

Error BehaviorServer::init() {
 	blackboard_mutex.instantiate();
	behavior_tree_mutex.instantiate();
	behavior_tree_graph_mutex.instantiate();
 	return OK;
}

void BehaviorServer::finish() {
	blackboard_mutex.unref();
	behavior_tree_mutex.unref();
	behavior_tree_graph_mutex.unref();
}

// ---- Blackboard ----

void BehaviorServer::blackboard_add_child(RID parent, RID child) {
	blackboards_lock();
	const auto iter = blackboard_parents_to_children.find(parent);
	if (iter == blackboard_parents_to_children.end()) {
		blackboard_parents_to_children[parent] = LocalVector{child};
	}
	else {
		LocalVector<RID> &children = iter->value;
		children.push_back(child);
	}
	blackboards_unlock();
}

void BehaviorServer::blackboard_remove_child(RID parent, RID child) {
	blackboards_lock();
	const auto iter = blackboard_parents_to_children.find(parent);
	if (iter == blackboard_parents_to_children.end()) {
		blackboards_unlock();
		return;
	}

	LocalVector<RID> &children = iter->value;
	children.erase(child);
	if (children.is_empty()) {
		blackboard_parents_to_children.remove(iter);
	}
	blackboards_unlock();
}

void BehaviorServer::blackboard_erase(Blackboard *blackboard) {

	blackboards_lock();
	const RID self = blackboard->get_self();

	if (const Blackboard *parent = blackboard->get_parent()) {
		const RID parent_rid = parent->get_self();
		const auto iter = blackboard_parents_to_children.find(parent_rid);
		if (iter != blackboard_parents_to_children.end()) {
			LocalVector<RID> &children = iter->value;
			children.erase(self);

			if (children.is_empty()) {

				blackboard_parents_to_children.remove(iter);
			}
		}
	}

	LocalVector<RID> children = {};
	const auto iter = blackboard_parents_to_children.find(self);
	if (iter != blackboard_parents_to_children.end()) {
		children = iter->value;
		for (const RID& child_rid : children) {
			Blackboard *child = blackboard_owner.get_or_null(child_rid);
			ERR_CONTINUE(child == nullptr);
			child->set_parent(nullptr);

		}
		children.clear();
		blackboard_parents_to_children.remove(iter);
	}
	blackboards_unlock();

	for (const auto& child : children) {
		blackboard_emit_set_parent(child, RID());
	}

	blackboard_emit_destroyed(self);
}

_FORCE_INLINE_ void BehaviorServer::blackboard_emit_set_parent(RID p_child_rid, RID p_parent_rid) {
	HydrogenBehaviorServer::get_singleton()->_blackboard_emit_set_parent(p_child_rid, p_parent_rid);
}

_FORCE_INLINE_ void BehaviorServer::blackboard_emit_created(RID p_blackboard_rid) {
	HydrogenBehaviorServer::get_singleton()->_blackboard_emit_created(p_blackboard_rid);
}

_FORCE_INLINE_ void BehaviorServer::blackboard_emit_destroyed(RID p_blackboard_rid) {
	HydrogenBehaviorServer::get_singleton()->_blackboard_emit_destroyed(p_blackboard_rid);
}

#define TRY_GET_CONST_BLACKBOARD(fail_result)											\
	blackboards_lock();																	\
	const Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);		\
	blackboards_unlock();																\
	ERR_FAIL_NULL_V(blackboard, fail_result);											\

#define TRY_GET_BLACKBOARD(fail_result)													\
	blackboards_lock();																	\
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);			\
	blackboards_unlock();																\
	ERR_FAIL_NULL_V(blackboard, fail_result);											\

bool BehaviorServer::blackboard_is_empty(RID p_blackboard_rid) {
	TRY_GET_CONST_BLACKBOARD(false)
	return blackboard->is_empty();
}

uint32_t BehaviorServer::blackboard_get_size(RID p_blackboard_rid) {
	TRY_GET_CONST_BLACKBOARD(0)
	return blackboard->size();
}

bool BehaviorServer::blackboard_set_parent(RID p_blackboard_rid, RID p_parent_rid) {
	blackboards_lock();
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	const Blackboard *parent = blackboard_owner.get_or_null(p_parent_rid);
	blackboards_unlock();

	ERR_FAIL_NULL_V(blackboard, false);

	const Blackboard *previous = blackboard->get_parent();
	const bool success = blackboard->set_parent(parent);

	if (previous) {
		blackboard_remove_child(previous->get_self(), p_blackboard_rid);
	}

	if (likely(success)) {
		if (likely(parent)) {
			blackboard_add_child(p_parent_rid, p_blackboard_rid);
		}

		blackboard_emit_set_parent(p_blackboard_rid, p_parent_rid);
	}

	return success;
}

RID BehaviorServer::blackboard_get_parent(RID p_blackboard_rid) {
	TRY_GET_CONST_BLACKBOARD(RID())
	const Blackboard *parent = blackboard->get_parent();
	return parent ? parent->get_self() : RID();
}

bool BehaviorServer::blackboard_is_ancestor(RID p_blackboard_rid, RID p_candidate) {
	blackboards_lock();
	const Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	const Blackboard *candidate = blackboard_owner.get_or_null(p_candidate);
	blackboards_unlock();
	ERR_FAIL_NULL_V(blackboard, false);
	ERR_FAIL_NULL_V(candidate, false);
	return  blackboard->is_ancestor(candidate);
}

bool BehaviorServer::blackboard_erase_entry(RID p_blackboard_rid, const StringName &p_name) {
	TRY_GET_BLACKBOARD(false)
	return blackboard->erase_entry(p_name);
}

bool BehaviorServer::blackboard_has_entry(RID p_blackboard_rid, const StringName &p_name, bool p_check_parents) {
	TRY_GET_CONST_BLACKBOARD(false)
	return blackboard->has_entry(p_name, p_check_parents);
}

bool BehaviorServer::blackboard_import_entries(RID p_blackboard_rid, const TypedDictionary<StringName, Variant> &p_values) {
	TRY_GET_BLACKBOARD(false)
	return blackboard->import_entries(p_values);
}

Dictionary BehaviorServer::blackboard_export_entries(RID p_blackboard_rid, const bool p_include_parents) {
	TRY_GET_CONST_BLACKBOARD({})
	return blackboard->export_entries(p_include_parents);
}

Dictionary BehaviorServer::blackboard_export_type_infos() {
	return Blackboard::export_type_infos();
}

#undef TRY_GET_CONST_BLACKBOARD
#undef TRY_GET_BLACKBOARD

// ---- Blackboard END ----

// ---- Behavior Tree ----

void BehaviorServer::behavior_tree_graph_erase(BehaviorTreeGraph *p_graph) {
	behavior_tree_graph_emit_destroyed(p_graph->get_self());
}

void BehaviorServer::behavior_tree_erase(BehaviorTree *p_behavior_tree) {
	behavior_tree_emit_destroyed(p_behavior_tree->get_self());
}

void BehaviorServer::behavior_tree_graph_emit_created(RID p_behavior_tree_graph) {
	HydrogenBehaviorServer::get_singleton()->_behavior_tree_graph_emit_created(p_behavior_tree_graph);
}

void BehaviorServer::behavior_tree_graph_emit_destroyed(RID p_behavior_tree_graph) {
	HydrogenBehaviorServer::get_singleton()->_behavior_tree_graph_emit_destroyed(p_behavior_tree_graph);
}

void BehaviorServer::behavior_tree_emit_created(RID p_behavior_tree) {
	HydrogenBehaviorServer::get_singleton()->_behavior_tree_emit_created(p_behavior_tree);
}

void BehaviorServer::behavior_tree_emit_destroyed(RID p_behavior_tree) {
	return HydrogenBehaviorServer::get_singleton()->_behavior_tree_emit_destroyed(p_behavior_tree);
}

#define TRY_GET_CONST_GRAPH(graph_type, fail_result, own_id)				\
	own_id##_graph_lock();													\
	const graph_type *graph = own_id##_graph_owner.get_or_null(p_graph_id);	\
	own_id##_graph_lock();													\
	ERR_FAIL_NULL_V(graph, fail_result);									\

#define TRY_GET_GRAPH(fail_result)										\
	own_id##_graph_lock();												\
	graph_type *graph = own_id##_graph_owner.get_or_null(p_graph_id);	\
	own_id##_graph_lock();												\
	ERR_FAIL_NULL_V(graph, fail_result);								\

#define TRY_GET_CONST_NODE(fail_result)							\
	const BehaviorTreeNode *node = graph->get_node(p_node_id);	\
	ERR_FAIL_NULL_V(node, fail_result);							\

#define TRY_GET_NODE(fail_result)						\
	BehaviorTreeNode *node = graph->get_node(p_node_id);\
	ERR_FAIL_NULL_V(node, fail_result);					\

RID BehaviorServer::pipeline_graph_create_node(RID p_graph_id, const StringName &p_node_type_name) {
	TRY_GET_GRAPH(RID());
	if (unlikely(graph->is_bound())) {
		WARN_PRINT("Cannot edit behavior tree graph while it's bound!");
		return RID();
	}
	return graph->create_node(p_node_type_name);
}

bool BehaviorServer::pipeline_graph_destroy_node(RID p_graph_id, RID p_node_id) {
	TRY_GET_GRAPH(false);
	if (unlikely(graph->is_bound())) {
		WARN_PRINT("Cannot edit behavior tree graph while it's bound!");
		return false;
	}
	return graph->destroy_node(p_node_id);
}

bool BehaviorServer::pipeline_graph_is_bound(RID p_graph_id) {
	TRY_GET_CONST_GRAPH(false);
	return graph->is_bound();
}

bool BehaviorServer::pipeline_graph_set_root(RID p_graph_id, RID p_node_id) {
	TRY_GET_GRAPH(false);
	if (unlikely(graph->is_bound())) {
		WARN_PRINT("Cannot edit behavior tree graph while it's bound!");
		return false;
	}
	return graph->set_root_id(p_node_id);
}

RID BehaviorServer::pipeline_graph_get_root(RID p_graph_id) {
	TRY_GET_CONST_GRAPH(RID());
	return graph->get_root_id();
}

int32_t BehaviorServer::node_get_input_port_count(RID p_graph_id, RID p_node_id) {
	TRY_GET_CONST_GRAPH(0);
	TRY_GET_CONST_NODE(0)

	return node->get_input_port_count(p_node_id);
}

int32_t BehaviorServer::node_get_output_port_count(RID p_graph_id, RID p_node_id) {
	TRY_GET_CONST_GRAPH(0);
	return graph->get_output_port_count(p_node_id);
}

StringName BehaviorServer::node_get_input_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH(StringName());
	return graph->get_input_port_type_name(p_node_id, p_port_index);
}

StringName BehaviorServer::node_get_output_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH(StringName());
	return graph->get_output_port_type_name(p_node_id, p_port_index);
}

StringName BehaviorServer::node_get_input_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH(StringName());
	return graph->get_input_port_name(p_node_id, p_port_index);
}

StringName BehaviorServer::node_get_output_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	TRY_GET_CONST_GRAPH(StringName());
	return graph->get_output_port_name(p_node_id, p_port_index);
}

TypedArray<Dictionary> prepare_port_infos(const Vector<NodePortInfo> &port_infos) {
	TypedArray<Dictionary> result = {};
	if(port_infos.is_empty()) {
		return result;
	}
	const int32_t port_infos_count = port_infos.size();
	result.resize(port_infos_count);
	for (int32_t i = 0; i < port_infos_count; ++i) {
		const NodePortInfo &port_info = port_infos[i];
		Dictionary out_port_info = Dictionary();
		out_port_info["name"] = port_info.name;
		out_port_info["type_name"] = port_info.type_name;
	}

	return result;
}

TypedArray<Dictionary> BehaviorServer::node_get_input_port_infos(RID p_graph_id, RID p_node_id) {
	TRY_GET_CONST_GRAPH({});
	Vector<NodePortInfo> port_infos = {};
	graph->get_input_port_infos(p_node_id, port_infos);
	return prepare_port_infos(port_infos);
}

TypedArray<Dictionary> BehaviorServer::node_get_output_port_infos(RID p_graph_id, RID p_node_id) {
	TRY_GET_CONST_GRAPH({});
	Vector<NodePortInfo> port_infos;
	graph->get_output_port_infos(p_node_id, port_infos);
	return prepare_port_infos(port_infos);
}

TypedArray<RID> prepare_node_ids(const Vector<RID> &p_node_ids) {
	TypedArray<RID>	result = {};
	if (p_node_ids.is_empty()) {
		return result;
	}

	const int32_t nodes_count = p_node_ids.size();
	result.resize(nodes_count);
	for (int32_t i = 0; i < nodes_count; ++i) {
		result[i] = p_node_ids[i];
	}

	return result;
}

TypedArray<RID> BehaviorServer::pipeline_graph_get_unrooted_nodes(RID p_graph_id) {
	TRY_GET_CONST_GRAPH({});
	return prepare_node_ids(graph->get_unrooted_nodes());
}

TypedArray<RID> BehaviorServer::pipeline_graph_get_rooted_nodes(RID p_graph_id) {
	TRY_GET_CONST_GRAPH({});
	return prepare_node_ids(graph->get_rooted_nodes());
}

#undef TRY_GET_CONST_GRAPH
#undef TRY_GET_GRAPH

// ---- Behavior Tree END ----

HydrogenBehaviorServer *HydrogenBehaviorServer::singleton = nullptr;
HydrogenBehaviorServer *HydrogenBehaviorServer::get_singleton() {
	return singleton;
}

void HydrogenBehaviorServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("free_rid", "rid"), &HydrogenBehaviorServer::free_rid);
	ClassDB::bind_method(D_METHOD("blackboard_create"), &HydrogenBehaviorServer::blackboard_create);
	ClassDB::bind_method(D_METHOD("behavior_tree_create"), &HydrogenBehaviorServer::behavior_tree_create);
	ClassDB::bind_method(D_METHOD("state_machine_create"), &HydrogenBehaviorServer::state_machine_create);
	ClassDB::bind_method(D_METHOD("agent_create"), &HydrogenBehaviorServer::agent_create);

	// ---- Blackboard ----

	ClassDB::bind_method(D_METHOD("blackboards_lock"), &HydrogenBehaviorServer::blackboards_lock);
	ClassDB::bind_method(D_METHOD("blackboards_unlock"), &HydrogenBehaviorServer::blackboards_unlock);

	ClassDB::bind_method(D_METHOD("blackboard_is_empty", "blackboard_rid"), &HydrogenBehaviorServer::blackboard_is_empty);
	ClassDB::bind_method(D_METHOD("blackboard_get_size", "blackboard_rid"), &HydrogenBehaviorServer::blackboard_get_size);

	ClassDB::bind_method(D_METHOD("blackboard_set_parent", "blackboard_rid", "parent"), &HydrogenBehaviorServer::blackboard_set_parent);
	ClassDB::bind_method(D_METHOD("blackboard_get_parent", "blackboard_rid"), &HydrogenBehaviorServer::blackboard_get_parent);
	ClassDB::bind_method(D_METHOD("blackboard_is_ancestor", "blackboard_rid", "candidate"), &HydrogenBehaviorServer::blackboard_is_ancestor);

	ClassDB::bind_method(D_METHOD("blackboard_try_get_entry", "blackboard_rid", "name", "check_parents"), &HydrogenBehaviorServer::blackboard_try_get_as_variant, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("blackboard_get_entry", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Variant>, DEFVAL(Variant()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_bool", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<bool>, DEFVAL(false), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_int", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<int64_t>, DEFVAL(0), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_float", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<real_t>, DEFVAL(0.0), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_string", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<String>, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector2", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector2>, DEFVAL(Vector2()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector2i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector2i>, DEFVAL(Vector2i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_rect2", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Rect2>, DEFVAL(Rect2()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_rect2i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Rect2i>, DEFVAL(Rect2i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector3", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector3>, DEFVAL(Vector3()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector3i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector3i>, DEFVAL(Vector3i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_transform2d", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Transform2D>, DEFVAL(Transform2D()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector4", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector4>, DEFVAL(Vector4()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_vector4i", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Vector4i>, DEFVAL(Vector4i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_plane", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Plane>, DEFVAL(Plane()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_quaternion", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Quaternion>, DEFVAL(Quaternion()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_aabb", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<AABB>, DEFVAL(AABB()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_basis", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Basis>, DEFVAL(Basis()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_transform3d", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Transform3D>, DEFVAL(Transform3D()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_projection", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Projection>, DEFVAL(Projection()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_color", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Color>, DEFVAL(Color()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_string_name", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<StringName>, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_node_path", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<NodePath>, DEFVAL(NodePath()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_rid", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<RID>, DEFVAL(RID()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_object", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Object*>, DEFVAL(nullptr), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_ref_counted", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Ref<RefCounted>>, DEFVAL(Ref<RefCounted>()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_callable", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Callable>, DEFVAL(Callable()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_signal", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Signal>, DEFVAL(Signal()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_dictionary", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Dictionary>, DEFVAL(Dictionary()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<Array>, DEFVAL(Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_byte_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedByteArray>, DEFVAL(PackedByteArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_int32_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedInt32Array>, DEFVAL(PackedInt32Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_int64_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedInt64Array>, DEFVAL(PackedInt64Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_float32_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedFloat32Array>, DEFVAL(PackedFloat32Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_float64_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedFloat64Array>, DEFVAL(PackedFloat64Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_string_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedStringArray>, DEFVAL(PackedStringArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_vector2_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedVector2Array>, DEFVAL(PackedVector2Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_vector3_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedVector3Array>, DEFVAL(PackedVector3Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_color_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedColorArray>, DEFVAL(PackedColorArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("blackboard_get_packed_vector4_array", "blackboard_rid", "name", "default", "check_parents"), &HydrogenBehaviorServer::blackboard_get_entry<PackedVector4Array>, DEFVAL(PackedVector4Array()), DEFVAL(true));

	ClassDB::bind_method(D_METHOD("blackboard_set_entry", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Variant>);
	ClassDB::bind_method(D_METHOD("blackboard_set_bool", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<bool>);
	ClassDB::bind_method(D_METHOD("blackboard_set_int", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<int64_t>);
	ClassDB::bind_method(D_METHOD("blackboard_set_float", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<real_t>);
	ClassDB::bind_method(D_METHOD("blackboard_set_string", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<String>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector2", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector2>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector2i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector2i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_rect2", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Rect2>);
	ClassDB::bind_method(D_METHOD("blackboard_set_rect2i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Rect2i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector3", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector3>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector3i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector3i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_transform2d", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Transform2D>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector4", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector4>);
	ClassDB::bind_method(D_METHOD("blackboard_set_vector4i", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Vector4i>);
	ClassDB::bind_method(D_METHOD("blackboard_set_plane", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Plane>);
	ClassDB::bind_method(D_METHOD("blackboard_set_quaternion", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Quaternion>);
	ClassDB::bind_method(D_METHOD("blackboard_set_aabb", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<AABB>);
	ClassDB::bind_method(D_METHOD("blackboard_set_basis", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Basis>);
	ClassDB::bind_method(D_METHOD("blackboard_set_transform3d", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Transform3D>);
	ClassDB::bind_method(D_METHOD("blackboard_set_projection", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Projection>);
	ClassDB::bind_method(D_METHOD("blackboard_set_color", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Color>);
	ClassDB::bind_method(D_METHOD("blackboard_set_string_name", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<StringName>);
	ClassDB::bind_method(D_METHOD("blackboard_set_node_path", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<NodePath>);
	ClassDB::bind_method(D_METHOD("blackboard_set_rid", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<RID>);
	ClassDB::bind_method(D_METHOD("blackboard_set_object", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Object*>);
	ClassDB::bind_method(D_METHOD("blackboard_set_ref_counted", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Ref<RefCounted>>);
	ClassDB::bind_method(D_METHOD("blackboard_set_callable", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Callable>);
	ClassDB::bind_method(D_METHOD("blackboard_set_signal", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Signal>);
	ClassDB::bind_method(D_METHOD("blackboard_set_dictionary", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Dictionary>);
	ClassDB::bind_method(D_METHOD("blackboard_set_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_byte_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedByteArray>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_int32_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedInt32Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_int64_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedInt64Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_float32_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedFloat32Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_float64_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedFloat64Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_string_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedStringArray>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_vector2_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedVector2Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_vector3_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedVector3Array>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_color_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedColorArray>);
	ClassDB::bind_method(D_METHOD("blackboard_set_packed_vector4_array", "blackboard_rid", "name", "value"), &HydrogenBehaviorServer::blackboard_set_entry<PackedVector4Array>);

	ClassDB::bind_method(D_METHOD("blackboard_erase_entry", "blackboard_rid", "name"), &HydrogenBehaviorServer::blackboard_erase_entry);
	ClassDB::bind_method(D_METHOD("blackboard_has_entry", "blackboard_rid", "name"), &HydrogenBehaviorServer::blackboard_has_entry);

	ClassDB::bind_method(D_METHOD("blackboard_import_entries", "blackboard_rid", "data"), &HydrogenBehaviorServer::blackboard_import_entries);
	ClassDB::bind_method(D_METHOD("blackboard_export_entries", "blackboard_rid", "p_include_parents"), &HydrogenBehaviorServer::blackboard_export_entries, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("blackboard_export_type_infos"), &HydrogenBehaviorServer::blackboard_export_type_infos);

	ADD_SIGNAL(MethodInfo("blackboard_created", PropertyInfo(Variant::RID, "blackboard_rid")));
	ADD_SIGNAL(MethodInfo("blackboard_destroyed", PropertyInfo(Variant::RID, "blackboard_rid")));
	ADD_SIGNAL(MethodInfo("blackboard_parent_set", PropertyInfo(Variant::RID, "child_rid"), PropertyInfo(Variant::RID, "parent_rid")));
	// ---- Blackboard End ----

	// ---- Behavior Tree ----

	ClassDB::bind_method(D_METHOD("behavior_tree_graph_create"), &HydrogenBehaviorServer::behavior_tree_graph_create);
	ClassDB::bind_method(D_METHOD("behavior_tree_create", "blackboard", "behavior_tree_graph"), &HydrogenBehaviorServer::behavior_tree_create);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_create_node", "node_type_name"), &HydrogenBehaviorServer::behavior_tree_graph_create_node);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_destroy_node", "node_id"), &HydrogenBehaviorServer::behavior_tree_graph_destroy_node);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_is_bound"), &HydrogenBehaviorServer::behavior_tree_graph_is_bound);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_set_root", "graph_id", "node_id"), &HydrogenBehaviorServer::behavior_tree_graph_set_root);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_root", "graph_id"), &HydrogenBehaviorServer::behavior_tree_graph_get_root);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_input_port_count", "graph_id", "node_id"), &HydrogenBehaviorServer::behavior_tree_graph_get_input_port_count);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_output_port_count", "graph_id", "node_id"), &HydrogenBehaviorServer::behavior_tree_graph_get_output_port_count);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_input_port_type_name", "graph_id", "node_id", "port_index"), &HydrogenBehaviorServer::behavior_tree_graph_get_input_port_type_name);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_output_port_type_name", "graph_id", "node_id", "port_index"), &HydrogenBehaviorServer::behavior_tree_graph_get_output_port_type_name);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_input_port_name", "graph_id", "node_id", "port_index"), &HydrogenBehaviorServer::behavior_tree_graph_get_input_port_name);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_output_port_name", "graph_id", "node_id", "port_index"), &HydrogenBehaviorServer::behavior_tree_graph_get_output_port_name);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_input_port_infos", "graph_id", "node_id"), &HydrogenBehaviorServer::behavior_tree_graph_get_input_port_infos);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_output_port_infos", "graph_id", "node_id"), &HydrogenBehaviorServer::behavior_tree_graph_get_output_port_infos);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_unrooted_nodes", "graph_id"), &HydrogenBehaviorServer::behavior_tree_graph_get_unrooted_nodes);
	ClassDB::bind_method(D_METHOD("behavior_tree_graph_get_rooted_nodes"), &HydrogenBehaviorServer::behavior_tree_graph_get_rooted_nodes);

	ADD_SIGNAL(MethodInfo("behavior_tree_graph_created", PropertyInfo(Variant::RID, "graph_id")));
	ADD_SIGNAL(MethodInfo("behavior_tree_graph_destroyed", PropertyInfo(Variant::RID, "graph_id")));
	
	ADD_SIGNAL(MethodInfo("behavior_tree_created", PropertyInfo(Variant::RID, "tree_id")));
	ADD_SIGNAL(MethodInfo("behavior_tree_destroyed", PropertyInfo(Variant::RID, "tree_id")));
	// ---- Behavior Tree End ----

#if TESTS_ENABLED
	ClassDB::bind_method(D_METHOD("run_tests"), &HydrogenBehaviorServer::run_tests);
#endif
}

// ReSharper disable CppMemberFunctionMayBeStatic
void HydrogenBehaviorServer::free_rid(RID p_rid) {
	BehaviorServer::get_singleton()->free_rid(p_rid);
}

RID HydrogenBehaviorServer::blackboard_create() {
	return BehaviorServer::get_singleton()->blackboard_create();
}

RID HydrogenBehaviorServer::behavior_tree_graph_create() {
	return BehaviorServer::get_singleton()->behavior_tree_graph_create();
}

RID HydrogenBehaviorServer::behavior_tree_create(RID p_blackboard, RID p_behavior_tree_graph) {
	return BehaviorServer::get_singleton()->behavior_tree_create(p_blackboard, p_behavior_tree_graph);
}

RID HydrogenBehaviorServer::state_machine_create() {
	return BehaviorServer::get_singleton()->state_machine_create();
}

RID HydrogenBehaviorServer::agent_create() {
	return BehaviorServer::get_singleton()->agent_create();
}

// ---- Blackboard ----

void HydrogenBehaviorServer::_blackboard_emit_created(RID p_blackboard_rid) {
	emit_signal("blackboard_created", p_blackboard_rid);
}

void HydrogenBehaviorServer::_blackboard_emit_destroyed(RID p_blackboard_rid) {
	emit_signal("blackboard_destroyed", p_blackboard_rid);
}

void HydrogenBehaviorServer::_blackboard_emit_set_parent(RID p_child_rid, RID p_parent_rid) {
	emit_signal("blackboard_emit_set_parent", p_child_rid, p_parent_rid);
}

void HydrogenBehaviorServer::blackboards_lock() const {
	BehaviorServer::get_singleton()->blackboards_lock();
}

void HydrogenBehaviorServer::blackboards_unlock() const {
	BehaviorServer::get_singleton()->blackboards_unlock();
}

bool HydrogenBehaviorServer::blackboard_is_empty(RID p_blackboard_rid) const {
	return BehaviorServer::get_singleton()->blackboard_is_empty(p_blackboard_rid);
}

uint32_t HydrogenBehaviorServer::blackboard_get_size(RID p_blackboard_rid) const {
	return BehaviorServer::get_singleton()->blackboard_get_size(p_blackboard_rid);
}

bool HydrogenBehaviorServer::blackboard_set_parent(RID p_child_rid, RID p_parent_rid) {
	return BehaviorServer::get_singleton()->blackboard_set_parent(p_child_rid, p_parent_rid);
}

RID HydrogenBehaviorServer::blackboard_get_parent(RID p_blackboard_rid) {
	return BehaviorServer::get_singleton()->blackboard_get_parent(p_blackboard_rid);
}

bool HydrogenBehaviorServer::blackboard_is_ancestor(RID p_blackboard_rid, RID p_candidate) {
	return BehaviorServer::get_singleton()->blackboard_is_ancestor(p_blackboard_rid, p_candidate);
}

bool HydrogenBehaviorServer::blackboard_erase_entry(RID p_blackboard_rid, const StringName &p_name) {
	return BehaviorServer::get_singleton()->blackboard_erase_entry(p_blackboard_rid, p_name);
}

bool HydrogenBehaviorServer::blackboard_has_entry(RID p_blackboard_rid, const StringName &p_name, const bool p_check_parents) {
	return BehaviorServer::get_singleton()->blackboard_has_entry(p_blackboard_rid, p_name, p_check_parents);
}

bool HydrogenBehaviorServer::blackboard_import_entries(RID p_blackboard_rid, const TypedDictionary<StringName, Variant> &p_data) {
	return BehaviorServer::get_singleton()->blackboard_import_entries(p_blackboard_rid, p_data);
}

Dictionary HydrogenBehaviorServer::blackboard_export_entries(RID p_blackboard_rid, const bool p_include_parents) {
	return BehaviorServer::get_singleton()->blackboard_export_entries(p_blackboard_rid, p_include_parents);
}

Dictionary HydrogenBehaviorServer::blackboard_export_type_infos() {
	return BehaviorServer::blackboard_export_type_infos();
}
// ---- Blackboard END ----

// ---- Behavior Tree ----

void HydrogenBehaviorServer::_behavior_tree_graph_emit_created(RID p_behavior_tree_graph) {
	emit_signal("behavior_tree_graph_created", p_behavior_tree_graph);
}

void HydrogenBehaviorServer::_behavior_tree_graph_emit_destroyed(RID p_behavior_tree_graph) {
	emit_signal("behavior_tree_graph_destroyed", p_behavior_tree_graph);
}

void HydrogenBehaviorServer::_behavior_tree_emit_created(RID p_behavior_tree) {
	emit_signal("behavior_tree_created", p_behavior_tree);
}

void HydrogenBehaviorServer::_behavior_tree_emit_destroyed(RID p_behavior_tree) {
	emit_signal("behavior_tree_destroyed", p_behavior_tree);
}

RID HydrogenBehaviorServer::behavior_tree_graph_create_node(RID p_graph_id, const StringName &p_node_type_name) {
	return BehaviorServer::get_singleton()->pipeline_graph_create_node(p_graph_id, p_node_type_name);
}

bool HydrogenBehaviorServer::behavior_tree_graph_destroy_node(RID p_graph_id, RID p_node_id) {
	return BehaviorServer::get_singleton()->pipeline_graph_destroy_node(p_graph_id, p_node_id);
}

bool HydrogenBehaviorServer::behavior_tree_graph_is_bound(RID p_graph_id) {
	return BehaviorServer::get_singleton()->pipeline_graph_is_bound(p_graph_id);
}

bool HydrogenBehaviorServer::behavior_tree_graph_set_root(RID p_graph_id, RID p_node_id) {
	return BehaviorServer::get_singleton()->pipeline_graph_set_root(p_graph_id, p_node_id);
}

RID HydrogenBehaviorServer::behavior_tree_graph_get_root(RID p_graph_id) {
	return BehaviorServer::get_singleton()->pipeline_graph_get_root(p_graph_id);
}

int32_t HydrogenBehaviorServer::behavior_tree_graph_get_input_port_count(RID p_graph_id, RID p_node_id) {
	return BehaviorServer::get_singleton()->node_get_input_port_count(p_graph_id, p_node_id);
}

int32_t HydrogenBehaviorServer::behavior_tree_graph_get_output_port_count(RID p_graph_id, RID p_node_id) {
	return BehaviorServer::get_singleton()->node_get_output_port_count(p_graph_id, p_node_id);
}

StringName HydrogenBehaviorServer::behavior_tree_graph_get_input_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_input_port_type_name(p_graph_id, p_node_id, p_port_index);
}

StringName HydrogenBehaviorServer::behavior_tree_graph_get_output_port_type_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_output_port_type_name(p_graph_id, p_node_id, p_port_index);
}

StringName HydrogenBehaviorServer::behavior_tree_graph_get_input_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_input_port_name(p_graph_id, p_node_id, p_port_index);
}

StringName HydrogenBehaviorServer::behavior_tree_graph_get_output_port_name(RID p_graph_id, RID p_node_id, int32_t p_port_index) {
	return BehaviorServer::get_singleton()->node_get_output_port_name(p_graph_id, p_node_id, p_port_index);
}

TypedArray<Dictionary> HydrogenBehaviorServer::behavior_tree_graph_get_input_port_infos(RID p_graph_id, RID p_node_id) {
	return BehaviorServer::get_singleton()->node_get_input_port_infos(p_graph_id, p_node_id);
}

TypedArray<Dictionary> HydrogenBehaviorServer::behavior_tree_graph_get_output_port_infos(RID p_graph_id, RID p_node_id) {
	return BehaviorServer::get_singleton()->node_get_output_port_infos(p_graph_id, p_node_id);
}

TypedArray<RID> HydrogenBehaviorServer::behavior_tree_graph_get_unrooted_nodes(RID p_graph_id) {
	return BehaviorServer::get_singleton()->pipeline_graph_get_unrooted_nodes(p_graph_id);
}

TypedArray<RID> HydrogenBehaviorServer::behavior_tree_graph_get_rooted_nodes(RID p_graph_id) {
	return BehaviorServer::get_singleton()->pipeline_graph_get_rooted_nodes(p_graph_id);
}

// ---- Behavior Tree END ----

#if TESTS_ENABLED
void HydrogenBehaviorServer::run_tests() {
	tests::behavior_test_runner();
}
#endif
// ReSharper restore CppMemberFunctionMayBeStatic

}






