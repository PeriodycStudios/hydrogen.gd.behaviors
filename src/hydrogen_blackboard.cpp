
#include "hydrogen_blackboard.hpp"

#include "behavior_server.hpp"

using namespace hydrogen;

HashMap<RID, HydrogenBlackboard*> HydrogenBlackboard::lookups = {};

HydrogenBlackboard *HydrogenBlackboard::get_by_rid(RID rid) {
	const auto iter = lookups.find(rid);
	if (likely(iter != lookups.end())) {
		return iter->value;
	}
	return nullptr;
}

void HydrogenBlackboard::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_parent", "parent"), &HydrogenBlackboard::set_parent);
	ClassDB::bind_method(D_METHOD("get_parent"), &HydrogenBlackboard::get_parent);

	ClassDB::bind_method(D_METHOD("get_entry", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Variant>, DEFVAL(Variant()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_bool", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<bool>, DEFVAL(false), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_int", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<int64_t>, DEFVAL(0), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_float", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<real_t>, DEFVAL(0.0), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_string", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<String>, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_vector2", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Vector2>, DEFVAL(Vector2()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_vector2i", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Vector2i>, DEFVAL(Vector2i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_rect2", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Rect2>, DEFVAL(Rect2()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_rect2i", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Rect2i>, DEFVAL(Rect2i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_vector3", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Vector3>, DEFVAL(Vector3()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_vector3i", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Vector3i>, DEFVAL(Vector3i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_transform2d", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Transform2D>, DEFVAL(Transform2D()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_vector4", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Vector4>, DEFVAL(Vector4()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_vector4i", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Vector4i>, DEFVAL(Vector4i()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_plane", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Plane>, DEFVAL(Plane()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_quaternion", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Quaternion>, DEFVAL(Quaternion()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_aabb", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<AABB>, DEFVAL(AABB()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_basis", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Basis>, DEFVAL(Basis()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_transform3d", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Transform3D>, DEFVAL(Transform3D()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_projection", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Projection>, DEFVAL(Projection()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_color", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Color>, DEFVAL(Color()));
	ClassDB::bind_method(D_METHOD("get_string_name", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<StringName>, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_node_path", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<NodePath>, DEFVAL(NodePath()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_rid", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<RID>, DEFVAL(RID()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_object", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Object*>, DEFVAL(nullptr), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_ref_counted", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Ref<RefCounted>>, DEFVAL(Ref<RefCounted>()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_callable", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Callable>, DEFVAL(Callable()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_signal", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Signal>, DEFVAL(Signal()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_dictionary", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Dictionary>, DEFVAL(Dictionary()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<Array>, DEFVAL(Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_byte_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedByteArray>, DEFVAL(PackedByteArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_int32_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedInt32Array>, DEFVAL(PackedInt32Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_int64_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedInt64Array>, DEFVAL(PackedInt64Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_float32_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedFloat32Array>, DEFVAL(PackedFloat32Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_float64_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedFloat64Array>, DEFVAL(PackedFloat64Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_string_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedStringArray>, DEFVAL(PackedStringArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_vector2_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedVector2Array>, DEFVAL(PackedVector2Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_vector3_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedVector3Array>, DEFVAL(PackedVector3Array()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_color_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedColorArray>, DEFVAL(PackedColorArray()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_packed_vector4_array", "name", "default", "check_parents"), &HydrogenBlackboard::get_entry<PackedVector4Array>, DEFVAL(PackedVector4Array()), DEFVAL(true));
	
	ClassDB::bind_method(D_METHOD("set_entry", "name", "value"), &HydrogenBlackboard::set_entry<Variant>);
	ClassDB::bind_method(D_METHOD("set_bool", "name", "value"), &HydrogenBlackboard::set_entry<bool>);
	ClassDB::bind_method(D_METHOD("set_int", "name", "value"), &HydrogenBlackboard::set_entry<int64_t>);
	ClassDB::bind_method(D_METHOD("set_float", "name", "value"), &HydrogenBlackboard::set_entry<real_t>);
	ClassDB::bind_method(D_METHOD("set_string", "name", "value"), &HydrogenBlackboard::set_entry<String>);
	ClassDB::bind_method(D_METHOD("set_vector2", "name", "value"), &HydrogenBlackboard::set_entry<Vector2>);
	ClassDB::bind_method(D_METHOD("set_vector2i", "name", "value"), &HydrogenBlackboard::set_entry<Vector2i>);
	ClassDB::bind_method(D_METHOD("set_rect2", "name", "value"), &HydrogenBlackboard::set_entry<Rect2>);
	ClassDB::bind_method(D_METHOD("set_rect2i", "name", "value"), &HydrogenBlackboard::set_entry<Rect2i>);
	ClassDB::bind_method(D_METHOD("set_vector3", "name", "value"), &HydrogenBlackboard::set_entry<Vector3>);
	ClassDB::bind_method(D_METHOD("set_vector3i", "name", "value"), &HydrogenBlackboard::set_entry<Vector3i>);
	ClassDB::bind_method(D_METHOD("set_transform2d", "name", "value"), &HydrogenBlackboard::set_entry<Transform2D>);
	ClassDB::bind_method(D_METHOD("set_vector4", "name", "value"), &HydrogenBlackboard::set_entry<Vector4>);
	ClassDB::bind_method(D_METHOD("set_vector4i", "name", "value"), &HydrogenBlackboard::set_entry<Vector4i>);
	ClassDB::bind_method(D_METHOD("set_plane", "name", "value"), &HydrogenBlackboard::set_entry<Plane>);
	ClassDB::bind_method(D_METHOD("set_quaternion", "name", "value"), &HydrogenBlackboard::set_entry<Quaternion>);
	ClassDB::bind_method(D_METHOD("set_aabb", "name", "value"), &HydrogenBlackboard::set_entry<AABB>);
	ClassDB::bind_method(D_METHOD("set_basis", "name", "value"), &HydrogenBlackboard::set_entry<Basis>);
	ClassDB::bind_method(D_METHOD("set_transform3d", "name", "value"), &HydrogenBlackboard::set_entry<Transform3D>);
	ClassDB::bind_method(D_METHOD("set_projection", "name", "value"), &HydrogenBlackboard::set_entry<Projection>);
	ClassDB::bind_method(D_METHOD("set_color", "name", "value"), &HydrogenBlackboard::set_entry<Color>);
	ClassDB::bind_method(D_METHOD("set_string_name", "name", "value"), &HydrogenBlackboard::set_entry<StringName>);
	ClassDB::bind_method(D_METHOD("set_node_path", "name", "value"), &HydrogenBlackboard::set_entry<NodePath>);
	ClassDB::bind_method(D_METHOD("set_rid", "name", "value"), &HydrogenBlackboard::set_entry<RID>);
	ClassDB::bind_method(D_METHOD("set_object", "name", "value"), &HydrogenBlackboard::set_entry<Object*>);
	ClassDB::bind_method(D_METHOD("set_ref_counted", "name", "value"), &HydrogenBlackboard::set_entry<Ref<RefCounted>>);
	ClassDB::bind_method(D_METHOD("set_callable", "name", "value"), &HydrogenBlackboard::set_entry<Callable>);
	ClassDB::bind_method(D_METHOD("set_signal", "name", "value"), &HydrogenBlackboard::set_entry<Signal>);
	ClassDB::bind_method(D_METHOD("set_dictionary", "name", "value"), &HydrogenBlackboard::set_entry<Dictionary>);
	ClassDB::bind_method(D_METHOD("set_array", "name", "value"), &HydrogenBlackboard::set_entry<Array>);
	ClassDB::bind_method(D_METHOD("set_packed_byte_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedByteArray>);
	ClassDB::bind_method(D_METHOD("set_packed_int32_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedInt32Array>);
	ClassDB::bind_method(D_METHOD("set_packed_int64_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedInt64Array>);
	ClassDB::bind_method(D_METHOD("set_packed_float32_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedFloat32Array>);
	ClassDB::bind_method(D_METHOD("set_packed_float64_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedFloat64Array>);
	ClassDB::bind_method(D_METHOD("set_packed_string_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedStringArray>);
	ClassDB::bind_method(D_METHOD("set_packed_vector2_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedVector2Array>);
	ClassDB::bind_method(D_METHOD("set_packed_vector3_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedVector3Array>);
	ClassDB::bind_method(D_METHOD("set_packed_color_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedColorArray>);
	ClassDB::bind_method(D_METHOD("set_packed_vector4_array", "name", "value"), &HydrogenBlackboard::set_entry<PackedVector4Array>);

	ClassDB::bind_method(D_METHOD("erase_entry", "name"), &HydrogenBlackboard::erase_entry);
	ClassDB::bind_method(D_METHOD("has_entry", "name"), &HydrogenBlackboard::has_entry);

	ClassDB::bind_method(D_METHOD("export_entries"), &HydrogenBlackboard::export_entries);

	ClassDB::bind_static_method("HydrogenBlackboard", D_METHOD("export_type_infos"), &HydrogenBlackboard::export_type_infos);
}

HydrogenBlackboard::HydrogenBlackboard() {
	blackboard = BehaviorServer::get_singleton()->blackboard_create();
	lookups[blackboard] = this;
}

HydrogenBlackboard::~HydrogenBlackboard() {
	ERR_FAIL_NULL(BehaviorServer::get_singleton());
	lookups.erase(blackboard);
	BehaviorServer::get_singleton()->free_rid(blackboard);
}

_FORCE_INLINE_ bool HydrogenBlackboard::set_parent(Ref<HydrogenBlackboard> parent) const {
	RID parent_rid = parent.is_valid() ? parent->_get_rid() : RID();
	return BehaviorServer::get_singleton()->blackboard_set_parent(blackboard, parent_rid);
}

Ref<HydrogenBlackboard> HydrogenBlackboard::get_parent() const {
	RID parent_rid = BehaviorServer::get_singleton()->blackboard_get_parent(blackboard);
	if (parent_rid == RID()) {
		return {};
	}

	Ref parent = get_by_rid(parent_rid);
	ERR_FAIL_COND_V(parent.is_null(), parent);
	return parent;
}

_FORCE_INLINE_ bool HydrogenBlackboard::erase_entry(const StringName &p_name) const {
	return BehaviorServer::get_singleton()->blackboard_erase_entry(blackboard, p_name);
}

_FORCE_INLINE_ bool HydrogenBlackboard::has_entry(const StringName &p_name) const {
	return BehaviorServer::get_singleton()->blackboard_has_entry(blackboard, p_name);
}

_FORCE_INLINE_ Dictionary HydrogenBlackboard::export_entries() const {
	return BehaviorServer::get_singleton()->blackboard_export_entries(blackboard);
}

_FORCE_INLINE_ Dictionary HydrogenBlackboard::export_type_infos() {
	return BehaviorServer::blackboard_export_type_infos();
}

