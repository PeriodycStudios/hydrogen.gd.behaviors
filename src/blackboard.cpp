//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/memory.hpp"
#include "mutex_helpers.hpp"
#include <mutex>

namespace hydrogen {

// TODO: tkey250628 - need a custom allocator so the Entries aren't fragmented.

std::recursive_mutex *Blackboard::register_mutex = nullptr;
HashMap<int64_t, StringName> Blackboard::TypeInfo::type_names = {};
Blackboard::TypeInfoMap Blackboard::type_infos = {};
Blackboard::TypeInfoMap Blackboard::object_class_infos = {};
Blackboard::FallbackTable Blackboard::variant_fallbacks = { nullptr };
const Blackboard::TypeInfo *Blackboard::ref_fallback_type_info = nullptr;
bool Blackboard::is_registration_ready = false;

void Blackboard::registration_init() {
	if (is_registration_ready) {
		return;
	}

	register_mutex = memnew(std::recursive_mutex);

	register_core_variant_types();
	is_registration_ready = true;
}

void Blackboard::registration_finish() {
	if (!is_registration_ready) {
		return;
	}

	registration_clear();

	memdelete(register_mutex);
	is_registration_ready = false;
}

void Blackboard::registration_clear() {
	LOCK_ONE(register_mutex);

	for (auto &kvp : type_infos) {
		TypeInfo *type_info = const_cast<TypeInfo *>(kvp.value);
		*type_info = TypeInfo();
	}

	TypeInfo::type_names.clear();
	type_infos.clear();
	object_class_infos.clear();
	for (int i = 0; i < Variant::VARIANT_MAX; ++i) {
		variant_fallbacks[i] = nullptr;
	}
	ref_fallback_type_info = nullptr;
}

void Blackboard::register_core_variant_types() {
	LOCK_ONE(register_mutex);

	register_type<bool>();
	register_type<uint8_t>();
	register_type<int8_t>();
	register_type<uint16_t>();
	register_type<int16_t>();
	register_type<uint32_t>();
	register_type<int32_t>();
	register_type<uint64_t>();
	register_type<int64_t>();

	register_type<char16_t>();
	register_type<char32_t>();

	register_type<float>();
	register_type<double>();

	register_type<String>();
	register_type<Vector2>();
	register_type<Vector2i>();
	register_type<Rect2>();
	register_type<Rect2i>();
	register_type<Vector3>();
	register_type<Vector3i>();
	register_type<Transform2D>();
	register_type<Vector4>();
	register_type<Vector4i>();
	register_type<Plane>();
	register_type<Quaternion>();
	register_type<AABB>();
	register_type<Basis>();
	register_type<Transform3D>();
	register_type<Projection>();
	register_type<Color>();
	register_type<StringName>();
	register_type<NodePath>();
	register_type<RID>();
	register_type<Object *>();
	register_type<Callable>();
	register_type<Signal>();
	register_type<Dictionary>();
	register_type<Array>();

	register_type<PackedByteArray>();
	register_type<PackedInt32Array>();
	register_type<PackedInt64Array>();
	register_type<PackedFloat32Array>();
	register_type<PackedFloat64Array>();
	register_type<PackedStringArray>();
	register_type<PackedVector2Array>();
	register_type<PackedVector3Array>();
	register_type<PackedVector4Array>();
	register_type<PackedColorArray>();

	register_type<ObjectID>();
	register_type<Ref<RefCounted>>();

#define SET_VARIANT_FALLBACK(type) variant_fallbacks[GetTypeInfo<type>::VARIANT_TYPE] = RegisteredTypeInfo<type>::get_info_ptr();
	SET_VARIANT_FALLBACK(bool)
	SET_VARIANT_FALLBACK(int64_t)
	SET_VARIANT_FALLBACK(double)
	SET_VARIANT_FALLBACK(String)
	SET_VARIANT_FALLBACK(Vector2)
	SET_VARIANT_FALLBACK(Vector2i)
	SET_VARIANT_FALLBACK(Rect2)
	SET_VARIANT_FALLBACK(Rect2i)
	SET_VARIANT_FALLBACK(Vector3)
	SET_VARIANT_FALLBACK(Vector3i)
	SET_VARIANT_FALLBACK(Transform2D)
	SET_VARIANT_FALLBACK(Vector4)
	SET_VARIANT_FALLBACK(Vector4i)
	SET_VARIANT_FALLBACK(Plane)
	SET_VARIANT_FALLBACK(Quaternion)
	SET_VARIANT_FALLBACK(AABB)
	SET_VARIANT_FALLBACK(Basis)
	SET_VARIANT_FALLBACK(Transform3D)
	SET_VARIANT_FALLBACK(Projection)

	SET_VARIANT_FALLBACK(Color)
	SET_VARIANT_FALLBACK(StringName)
	SET_VARIANT_FALLBACK(NodePath)
	SET_VARIANT_FALLBACK(RID)
	SET_VARIANT_FALLBACK(Object *)
	SET_VARIANT_FALLBACK(Callable)
	SET_VARIANT_FALLBACK(Signal)
	SET_VARIANT_FALLBACK(Dictionary)
	SET_VARIANT_FALLBACK(Array)

	SET_VARIANT_FALLBACK(PackedByteArray)
	SET_VARIANT_FALLBACK(PackedInt32Array)
	SET_VARIANT_FALLBACK(PackedInt64Array)
	SET_VARIANT_FALLBACK(PackedFloat32Array)
	SET_VARIANT_FALLBACK(PackedFloat64Array)
	SET_VARIANT_FALLBACK(PackedStringArray)
	SET_VARIANT_FALLBACK(PackedVector2Array)
	SET_VARIANT_FALLBACK(PackedVector3Array)
	SET_VARIANT_FALLBACK(PackedColorArray)
	SET_VARIANT_FALLBACK(PackedVector4Array)
#undef SET_VARIANT_FALLBACK

	ref_fallback_type_info = RegisteredTypeInfo<Ref<RefCounted>>::get_info_ptr();
}

Blackboard::Blackboard() {
	mutex = memnew(std::recursive_mutex);
}

Blackboard::~Blackboard() {
	{
		LOCK_ONE(mutex);
		for (const auto &entry_kvp : entries) {
			const auto entry = entry_kvp.value;
			const RID rid = entry->get_self();
			entries_owner.free(rid);
			memdelete(entry);
		}
		entries.clear();
		parent = nullptr;
	}

	memdelete(mutex);
}

bool Blackboard::validate_candidate_parent(const Blackboard *p_candidate) const {
	if (p_candidate == nullptr) {
		return true;
	}

	if (p_candidate == this) {
		return false;
	}

	const Blackboard *current = p_candidate->parent;

	while (current) {
		LOCK_ONE_V(current->mutex, false);

		if (unlikely(current == this)) {
			return false;
		}

		const Blackboard *next = current->parent;

		current = next;
	}

	return true;
}

bool Blackboard::set_parent(const Blackboard *p_parent) {
	LOCK_ONE_V(mutex, false);

	if (likely(p_parent != parent && validate_candidate_parent(p_parent))) {
		parent = p_parent;

		return true;
	}

	return false;
}

const Blackboard *Blackboard::get_parent() const {
	LOCK_ONE_V(mutex, nullptr);
	const Blackboard *p = parent;
	return p;
}

bool Blackboard::is_ancestor(const Blackboard *p_candidate) const {
	LOCK_ONE_V(mutex, false);
	const Blackboard *current = parent;

	while (current != nullptr) {
		LOCK_ONE_V(current->mutex, false);

		if (current == p_candidate) {
			return true;
		}

		const Blackboard *next = current->parent;

		current = next;
	}

	return false;
}

void Blackboard::free_entry(const EntryMap::Iterator &iter) {
	LOCK_ONE(mutex);
	EntryBase *entry = iter->value;

	const RID rid = entry->get_self();

	entries.remove(iter);
	entries_owner.free(rid);

	memdelete(entry);
}

bool Blackboard::import_entries(const TypedDictionary<StringName, Variant> &p_data) {
	if (unlikely(p_data.is_empty())) {
		return false;
	}

	LOCK_ONE_V(mutex, false);

	const TypedArray<StringName> keys(p_data.keys());
	const Array values = p_data.values();

	for (int i = 0; i < p_data.size(); ++i) {
		const StringName key = keys[i];
		const Variant &value = values[i];
		set_entry_fast<Variant>(key, value);
	}

	return true;
}

bool Blackboard::erase_entry(const StringName &p_name) {
	LOCK_ONE_V(mutex, false);

	auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		return false;
	}

	free_entry(iter);

	return true;
}

bool Blackboard::has_entry(const StringName &p_name, const bool p_check_parents) const {
	LOCK_ONE_V(mutex, false);
	bool result = entries.has(p_name);

	if (!result && p_check_parents) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			LOCK_ONE_V(current->mutex, false);

			if (current->entries.has(p_name)) {
				return true;
			}

			const Blackboard *next = current->parent;

			current = next;
		}
	}

	return result;
}

Dictionary Blackboard::export_entries(const bool p_include_parents) const {
	Dictionary dict = {};

	LOCK_ONE_V(mutex, dict);

	if (likely(p_include_parents)) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			LOCK_ONE_V(current->mutex, dict);
			for (const auto &kvp : current->entries) {
				dict[kvp.key] = kvp.value->as_variant();
			}
			current = current->parent;
		}
	}

	for (const auto &kvp : entries) {
		dict[kvp.key] = kvp.value->as_variant();
	}

	return dict;
}

Dictionary Blackboard::export_type_infos() {
	Dictionary results = {};

	LOCK_ONE_V(register_mutex, results);
	for (const auto &kvp : type_infos) {
		const TypeInfo *info = kvp.value;
		Dictionary dict = {};
		const StringName &type_name = info->get_name();
		dict["type_key"] = kvp.key;
		dict["type_name"] = type_name;
		dict["variant_type"] = info->variant_type;
		dict["object_class_key"] = info->object_class_key;
		dict["is_registered"] = info->is_registered();
		dict["is_variant_type"] = info->is_variant_type();
		dict["is_object_ptr_type"] = info->is_object_ptr_type();
		dict["is_ref_counted"] = info->is_ref_counted();
		dict["is_gd_object"] = info->is_gd_object();
		dict["is_convertable"] = info->is_convertible();

		results[type_name] = dict;
	}

	return results;
}

} //namespace hydrogen
