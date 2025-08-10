
#include "behavior_server.hpp"
#include "behavior_trees/behavior_tree_graph.hpp"
#include "behavior_trees/behavior_trees.hpp"
#include "blackboard.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"

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

void BehaviorServer::free_rid(RID p_rid) {
	if (blackboard_owner.owns(p_rid)) {
		const static auto bb_cleanup = [this](Blackboard* bb){ blackboard_erase(bb);};
		free_ptr_resource<Blackboard>(blackboard_owner, blackboard_mutex, p_rid, bb_cleanup);
	}
	else if (behavior_tree_owner.owns(p_rid)) {
		const static auto bt_cleanup = [this](BehaviorTree* bt){ behavior_tree_erase(bt);};
		free_ptr_resource<BehaviorTree>(behavior_tree_owner, behavior_tree_mutex, p_rid, bt_cleanup);
	}
	else if (behavior_tree_graph_owner.owns(p_rid))
	{
		const static auto btg_cleanup = [this](BehaviorTreeGraph * btg) { behavior_tree_graph_erase(btg); };
		free_ptr_resource<BehaviorTreeGraph>(behavior_tree_graph_owner, behavior_tree_mutex, p_rid, btg_cleanup);
	}
	else {
		ERR_FAIL_MSG("Invalid ID.");
	}
}

RID BehaviorServer::blackboard_create() {
	blackboards_lock();

	Blackboard *blackboard = memnew(Blackboard);
	RID rid = blackboard_owner.make_rid(blackboard);
	blackboard->set_self(rid);

	blackboards_unlock();

	return rid;
}

RID BehaviorServer::behavior_tree_graph_create() {
	behavior_tree_lock();

	BehaviorTreeGraph* graph = memnew(BehaviorTreeGraph);
	RID rid = behavior_tree_graph_owner.make_rid(graph);
	graph->set_self(rid);

	behavior_tree_unlock();

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
 	return OK;
}

void BehaviorServer::finish() {
	blackboard_mutex.unref();
	behavior_tree_mutex.unref();
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

bool BehaviorServer::blackboard_is_empty(RID p_blackboard_rid) {
	blackboards_lock();
	const Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();

	ERR_FAIL_NULL_V(blackboard, false);

	return blackboard->is_empty();
}

uint32_t BehaviorServer::blackboard_get_size(RID p_blackboard_rid) {
	blackboards_lock();
	const Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();

	ERR_FAIL_NULL_V(blackboard, 0);

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
	blackboards_lock();
	const Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();
	ERR_FAIL_NULL_V(blackboard, RID());
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
	blackboards_lock();
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();
	ERR_FAIL_NULL_V(blackboard, false);
	return blackboard->erase_entry(p_name);
}

bool BehaviorServer::blackboard_has_entry(RID p_blackboard_rid, const StringName &p_name, bool p_check_parents) {
	blackboards_lock();
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();
	ERR_FAIL_NULL_V(blackboard, false);
	return blackboard->has_entry(p_name, p_check_parents);
}

bool BehaviorServer::blackboard_import_entries(RID p_blackboard_rid, const TypedDictionary<StringName, Variant> &p_values) {
	blackboards_lock();
	Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();
	ERR_FAIL_NULL_V(blackboard, false);
	return blackboard->import_entries(p_values);
}

Dictionary BehaviorServer::blackboard_export_entries(RID p_blackboard_rid, const bool p_include_parents) {
	blackboards_lock();
	const Blackboard *blackboard = blackboard_owner.get_or_null(p_blackboard_rid);
	blackboards_unlock();
	ERR_FAIL_NULL_V(blackboard, {});
	return blackboard->export_entries(p_include_parents);
}

Dictionary BehaviorServer::blackboard_export_type_infos() {
	return Blackboard::export_type_infos();
}

// ---- Blackboard END ----

// ---- Behavior Tree ----

void BehaviorServer::behavior_tree_graph_erase(BehaviorTreeGraph *p_graph) {

}

void BehaviorServer::behavior_tree_erase(BehaviorTree *behavior_tree) {

}




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

#if TESTS_ENABLED
void HydrogenBehaviorServer::run_tests() {
	tests::behavior_test_runner();
}
#endif
// ReSharper restore CppMemberFunctionMayBeStatic

}






