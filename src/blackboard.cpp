//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

namespace hydrogen {

Blackboard::TypeInfoMap Blackboard::type_infos = {};
Blackboard::FallbackTable Blackboard::variant_fallbacks = {nullptr};
const Blackboard::TypeInfo *Blackboard::ref_fallback_type_info = nullptr;
bool Blackboard::core_variants_registered = false;

void Blackboard::register_core_variant_types() {
	if (core_variants_registered) {
		return;
	}

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
	register_type<Object*>();
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
	SET_VARIANT_FALLBACK(uint64_t)
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
	SET_VARIANT_FALLBACK(Object*)
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

	core_variants_registered = true;
}

Blackboard::~Blackboard() {
	for (const auto& entry_kvp : entries) {
		const auto entry = entry_kvp.value;
		const RID rid = entry->get_self();
		entries_owner.free(rid);
		memfree(entry);
	}

	entries.clear();
}

bool Blackboard::validate_parent(const Blackboard *p_parent) const {
	if (!p_parent)
		return true;

	if (p_parent == this)
		return false;

	if (p_parent->parent == this)
		return false;

	if (p_parent->parent == nullptr)
		return true;

	auto a = p_parent;
	auto b = p_parent;

	while (a && b && b->parent) {
		a = a->parent;
		b = b->parent->parent;

		if (a == b)
			return false;
	}

	return true;
}

Blackboard *Blackboard::find_parent(const StringName &p_name) const {
	Blackboard *current = parent;
	while (current != nullptr) {
		if (current->get_name() == p_name) {
			return current;
		}
		current = current->parent;
	}
	return nullptr;
}

Blackboard *Blackboard::find_parent(const RID &p_rid) const {
	Blackboard *current = parent;
	while (current != nullptr) {
		if (current->get_self() == p_rid) {
			return current;
		}
		current = current->parent;
	}
	return nullptr;
}

void Blackboard::free_entry(EntryMap::Iterator &iter) {
	EntryBase *entry = iter->value;
	const RID rid = entry->get_self();

	entries.remove(iter);
	entries_owner.free(rid);
	memfree(entry);
}

bool Blackboard::erase_entry(const StringName &p_name) {

	auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		return false;
	}

	free_entry(iter);
	return true;
}

bool Blackboard::has_entry(const StringName &p_name, const bool p_check_parents) const {
	bool result = entries.has(p_name);

	if (!result && p_check_parents) {
		const Blackboard *current = parent;
		while (current != nullptr) {
			result = current->entries.has(p_name);
			if (result) {
				return true;
			}
			current = current->parent;
		}
	}

	return result;
}

} //namespace Hydrogen
