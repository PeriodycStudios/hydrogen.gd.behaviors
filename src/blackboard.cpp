//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

namespace hydrogen {

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

template <typename T>
Blackboard::EntryBase *Blackboard::create_entry(const StringName &p_name,
		RID_PtrOwner<EntryBase> &p_owner,
		HashMap<StringName, EntryBase*> &p_entries) {
	EntryBase *entry = memnew(Entry<T>);

	const RID rid = p_owner.make_rid(entry);
	entry->set_self(rid);

	p_entries[p_name] = entry;
	return entry;
}


template <typename T>
void Blackboard::register_create() {
	const auto type_key = blackboard_storage_type<T>::get_type_key();
	factories[type_key] = std::function<entry_factory>(create_entry<T>);
}


void Blackboard::init_create_functions() {

	if (!factories.is_empty())
		return;

	register_create<bool>();
	register_create<uint8_t>();
	register_create<int16_t>();
	register_create<uint16_t>();
	register_create<int16_t>();
	register_create<uint32_t>();
	register_create<int32_t>();
	register_create<uint64_t>();
	register_create<int64_t>();

	register_create<char16_t>();
	register_create<char32_t>();

	register_create<float>();
	register_create<double>();

	register_create<String>();
	register_create<Vector2>();
	register_create<Vector2i>();
	register_create<Rect2>();
	register_create<Rect2i>();
	register_create<Vector3>();
	register_create<Vector3i>();
	register_create<Transform2D>();
	register_create<Vector4>();
	register_create<Vector4i>();
	register_create<Plane>();
	register_create<Quaternion>();
	register_create<AABB>();
	register_create<Basis>();
	register_create<Transform3D>();
	register_create<Projection>();
	register_create<Color>();
	register_create<StringName>();
	register_create<NodePath>();
	register_create<RID>();
	register_create<Callable>();
	register_create<Signal>();
	register_create<Array>();
	register_create<PackedByteArray>();
	register_create<PackedInt32Array>();
	register_create<PackedInt64Array>();
	register_create<PackedFloat32Array>();
	register_create<PackedFloat64Array>();
	register_create<PackedStringArray>();
	register_create<PackedVector2Array>();
	register_create<PackedVector3Array>();
	register_create<PackedVector4Array>();
	register_create<PackedColorArray>();

	register_create<Variant>();

	// TODO: Actually handle Object type correctly
}


Blackboard::Blackboard() :
		parent(nullptr) {}

Blackboard::~Blackboard() {}

template <typename T>
bool Blackboard::get_entry(const StringName &p_name, typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_out_result, const bool p_check_parents) const {

	auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		if (likely(p_check_parents)) {
			const Blackboard* current = parent;
			while (current != nullptr) {

				iter = current->entries.find(p_name);
				if (iter != current->entries.end()) {

					Vector2i type_key = iter->value->get_type_key();

					if (type_key != blackboard_storage_type<T>::get_type_key()) {
						return false;
					}

					goto has_result;
				}

				current = current->parent;
			}
		}

		return false;
	}

has_result:
	Entry<T>* result = iter->value;
	p_out_result = result->value;
	return true;
}

template<>
bool Blackboard::get_entry<Variant>(const StringName &p_name, Variant &p_out_result, const bool p_check_parents) const {

	auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		if (likely(p_check_parents)) {
			const Blackboard *current = parent;
			while (current != nullptr) {
				iter = current->entries.find(p_name);
				if (iter != current->entries.end()) {
					goto has_result;
				}

				current = current->parent;
			}
		}
		return false;
	}

has_result:
	p_out_result = iter->value->as_variant();
	return true;
}

template <typename T>
void Blackboard::set_entry(const StringName &p_name, const typename EnableIf<blackboard_storage_type<T>::value, T>::type &p_value) {


}

template <>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value) {

	// p_value.
}

void Blackboard::free_entry(const HashMap<StringName, EntryBase *>::Iterator &iter) {
	const EntryBase *entry = iter->value;
	const RID rid = entry->get_self();

	entries.remove(iter);
	entries_owner.free(rid);
	memdelete(entry);
}
bool Blackboard::erase_entry(const StringName &p_name) {

	const auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		return false;
	}

	free_entry(iter);

	return true;
}

bool Blackboard::has_entry(const StringName &p_name, bool p_check_parents) const {

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
