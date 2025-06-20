//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

#include <memory>
#include "type_name.hpp"

namespace hydrogen {

HashMap<StringName, const Blackboard::TypeInfo*> Blackboard::type_infos = {};
std::array<const Blackboard::TypeInfo*, GDEXTENSION_VARIANT_TYPE_VARIANT_MAX> Blackboard::core_variant_type_infos = {nullptr};
const Blackboard::TypeInfo *Blackboard::ref_fallback_type_info = nullptr;

template <typename T>
Variant Blackboard::EntryVariant<T>::as_variant() const {
	return Variant(this->value);
}

template <typename T>
bool Blackboard::EntryVariant<T>::set_from(const Variant &p_value) {
	this->value = p_value;
	return true;
}

template <>
Variant Blackboard::EntryVariant<char16_t>::as_variant() const {
	const int64_t converted = this->value;
	return Variant(converted);
}

template <>
bool Blackboard::EntryVariant<char16_t>::set_from(const Variant &p_value) {
	const int64_t converted = p_value;
	this->value = static_cast<char16_t>(converted);
	return true;
}

template <>
Variant Blackboard::EntryVariant<char32_t>::as_variant() const {
	const int64_t converted = this->value;
	return Variant(converted);
}

template <>
bool Blackboard::EntryVariant<char32_t>::set_from(const Variant &p_value) {
	const int64_t converted = p_value;
	this->value = static_cast<char32_t>(converted);
	return true;
}

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
	register_type<ObjectID>();
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

	register_type<Object*>();
	register_type<Ref<RefCounted>>();

#define SET_VARIANT_FALLBACK(type) core_variant_type_infos[GetTypeInfo<type>::VARIANT_TYPE] = RegisteredTypeInfo<type>::get_info_ptr();
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


template <typename T>
void Blackboard::register_type() {

	typedef typename traits::unadorned_type<T>::type type;
	if (RegisteredTypeInfo<type>::is_registered()) {
		return;
	}

	TypeInfo type_info = {};
	if constexpr (traits::is_variant_type<type>::value) {

		type_info.factory = create_variant_entry<type>;
		type_info.variant_type = GetTypeInfo<type>::VARIANT_TYPE;
		type_info.variant_argument_metadata = GetTypeInfo<type>::METADATA;
		type_info.variant_prop_info = GetTypeInfo<type>::get_class_info();
		type_info.enable_flag(TypeInfo::Flags::IS_VARIANT_TYPE);

		if (type_info.variant_type == GDEXTENSION_VARIANT_TYPE_OBJECT) {
			type_info.type_key = type_info.variant_prop_info.class_name;
			type_info.enable_flag(TypeInfo::Flags::IS_GD_OBJECT);
		}

		if constexpr (traits::is_gd_ref_helper<type>::value) {
			type_info.enable_flag(TypeInfo::Flags::IS_GD_REF);
		}
	}
	else {
		type_info.factory = create_data_entry<type>;
	}

	if (type_info.type_key == StringName()) {
		const std::string type_name_str(type_name<type>());
		type_info.type_key = StringName(type_name_str.c_str());
	}

	type_info.enable_flag(TypeInfo::Flags::IS_REGISTERED);

	RegisteredTypeInfo<type>::set_info(type_info);
	type_infos[type_info.type_key] = RegisteredTypeInfo<type>::get_info_ptr();
}

template <typename T>
Blackboard::EntryBase *Blackboard::create_data_entry(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase *> &p_entries) {
	typedef typename traits::unadorned_type<T>::type type;
	EntryData<type> *entry = memnew(EntryData<type>());

	const RID rid = p_owner.make_rid(entry);
	entry->set_self(rid);

	p_entries[p_name] = entry;
	return entry;
}

template <typename T>
Blackboard::EntryBase* Blackboard::create_variant_entry(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase *> &p_entries) {
	typedef typename traits::unadorned_type<T>::type type;
	EntryVariant<type> *entry = memnew(EntryVariant<type>());

	const RID rid = p_owner.make_rid(entry);
	entry->set_self(rid);

	p_entries[p_name] = entry;
	return entry;
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

template <typename T>
bool Blackboard::find_entry(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, const bool p_check_parents) const {
	typedef typename traits::unadorned_type<T>::type type;
	const TypeInfo &type_info = RegisteredTypeInfo<type>::get_info();
	if (unlikely(!type_info.is_registered())) {
		return false;
	}

	const StringName &type_key = type_info.type_key;

	auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		if (likely(p_check_parents)) {
			const Blackboard *current = parent;
			while (current != nullptr) {
				iter = current->entries.find(p_name);
				if (iter != current->entries.end()) {
					const StringName &entry_type_key = iter->value->get_type_key();

					if (entry_type_key != type_key) {
						return false;
					}

					p_out_result = iter;
					return true;
				}

				current = current->parent;
			}
		}
	}

	return false;
}

template <>
bool Blackboard::find_entry<Variant>(const StringName &p_name, HashMap<StringName, EntryBase *>::ConstIterator &p_out_result, bool p_check_parents) const {
	auto iter = entries.find(p_name);
	if (unlikely(iter == entries.end())) {
		if (likely(p_check_parents)) {
			const Blackboard *current = parent;
			while (current != nullptr) {
				iter = current->entries.find(p_name);
				if (iter != current->entries.end()) {
					p_out_result = iter;
					return true;
				}

				current = current->parent;
			}
		}
	}

	return false;
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

template <typename T>
bool Blackboard::try_get_entry(const StringName &p_name, T &p_out_result, const bool p_check_parents) const {

	HashMap<StringName, EntryBase *>::ConstIterator iter;
	if (unlikely(!find_entry<T>(p_name, &iter, p_check_parents))) {
		return false;
	}

	typedef typename traits::unadorned_type<T>::type type;
	EntryData<type> *result = iter->value;
	p_out_result = result->value;
	return true;
}

template<>
bool Blackboard::try_get_entry<Variant>(const StringName &p_name, Variant &p_out_result, const bool p_check_parents) const {

	HashMap<StringName, EntryBase *>::ConstIterator iter;
	if (unlikely(!find_entry<Variant>(p_name, iter, p_check_parents))) {
		return false;
	}

	const EntryBase *result = iter->value;
	p_out_result = result->as_variant();
	return true;
}

template <typename T>
T Blackboard::get_entry(const StringName &p_name, const T &p_default, bool p_check_parents) const {
	T result;
	if (likely(try_get_entry<T>(p_name, result, p_check_parents)))
		return result;

	return p_default;
}

template <>
Variant Blackboard::get_entry<Variant>(const StringName &p_name, const Variant &p_default, bool p_check_parents) const {
	Variant result;
	if (likely(try_get_entry<Variant>(p_name, result, p_check_parents)))
		return result;
	return p_default;
}

template <typename T>
void Blackboard::set_entry(const StringName &p_name, const T &p_value) {

	typedef typename traits::unadorned_type<T>::type type;
	if (unlikely(!RegisteredTypeInfo<type>::is_registered())) {
		register_type<type>();
	}

	const TypeInfo &type_info = RegisteredTypeInfo<type>::get_info();
	const StringName &type_key = type_info.type_key;

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		const auto existing_entry = iter->value;
		if (likely(existing_entry->get_type_key() == type_key)) {
			iter->value = p_value;
			return;
		}
		free_entry(iter);
	}

	EntryData<type> *new_entry = type_info.factory(p_name, entries_owner, entries);
	new_entry->value = p_value;
}

template <>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value) {

	const auto variant_type = static_cast<GDExtensionVariantType>(p_value.get_type());

	auto iter = entries.find(p_name);
	if (likely(iter != entries.end())) {
		EntryBase *existing_entry = iter->value;
		if (likely(existing_entry->get_variant_type() == variant_type)) {
			existing_entry->set_from(p_value);
			return;
		}

		free_entry(iter);
	}


	// NIL will delete an existing entry or do nothing if one doesn't exist.
	if (unlikely(variant_type == GDEXTENSION_VARIANT_TYPE_NIL)) {
		return;
	}

	const TypeInfo *type_info;
	if (unlikely(variant_type == GDEXTENSION_VARIANT_TYPE_OBJECT)) {
		const Ref<RefCounted> ref = p_value;
		if (ref.is_null()) {
			type_info = core_variant_type_infos[GDEXTENSION_VARIANT_TYPE_OBJECT];
		}
		else {
			type_info = ref_fallback_type_info;
		}
	}
	else {
		type_info = core_variant_type_infos[variant_type];
	}

	EntryBase *entry = type_info->factory(p_name, entries_owner, entries);
	entry->set_from(p_value);
}

void Blackboard::free_entry(HashMap<StringName, EntryBase *>::Iterator &iter) {
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
