//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

#include <memory>
#include "type_name.hpp"

namespace hydrogen {

template <typename T>
const StringName &Blackboard::get_type_key() {

	const TypeInfo &type_info = RegisteredTypeInfo<T>::type_info;

	if (!type_info.is_registered) {
		static const auto default_type_key = StringName();
		return default_type_key;
	}

	return type_info.type_key;
}

template <typename T>
const Blackboard::entry_factory &Blackboard::get_entry_factory() {
	const TypeInfo &type_info = RegisteredTypeInfo<T>::type_info;

	if (!type_info.is_registered) {
		return nullptr;
	}

	return type_info.factory;
}

HashMap<StringName, Blackboard::entry_factory> Blackboard::factories = {};

template <typename T>
void Blackboard::register_type() {

	StringName type_key;
	entry_factory factory;
	GDExtensionVariantType variant_type = GDEXTENSION_VARIANT_TYPE_NIL;
	GDExtensionClassMethodArgumentMetadata metadata = GDEXTENSION_METHOD_ARGUMENT_METADATA_NONE;
	bool is_variant = false;
	bool is_gd_object = false;
	bool is_gd_reference = false;

	if constexpr (traits::has_class_name<T>::value) {
		type_key = T::get_class_name();
	}
	else {
		type_key = type_name<T>();
	}


	if constexpr (traits::has_get_type_info<T>::value) {
		variant_type = GetTypeInfo<T>::get_type();
		metadata = GetTypeInfo<T>::metadata;
		factory = create_variant_entry<T>;

		if constexpr (traits::is_object_ptr<>)
	}
	else {
		factory = create_entry<T>;
	}

	RegisteredTypeInfo<T>::type_info = TypeInfo(
		type_key,
		factory);

}

template <typename T>
Blackboard::EntryBase *Blackboard::create_entry(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase *> &p_entries) {
	Entry<T> *entry = memnew(Entry<T>);

	const RID rid = p_owner.make_rid(entry);
	entry->set_self(rid);

	p_entries[p_name] = entry;
	return entry;
}

template <typename T>
Blackboard::EntryBase* Blackboard::create_variant_entry(const StringName &p_name, RID_PtrOwner<EntryBase> &p_owner, HashMap<StringName, EntryBase *> &p_entries) {
	EntryVariant<T> *entry = memnew(EntryVariant<T>);

	const RID rid = p_owner.make_rid(entry);
	entry->set_self(rid);

	p_entries[p_name] = entry;
	return entry;
}
//
// template<typename T, typename EnableIf<BlackboardStorageType<T>::value, T>::type>
// void Blackboard::register_storage_type() {
// 	if (const StringName &type_key = BlackboardStorageType<T>::get_type_key(); factories.find(type_key) == factories.end()) {
// 		factories[type_key] = create_variant_entry<T>;
// 	}
// }


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
	static const StringName empty_string_name = "";
	const StringName &type_key = get_type_key<T>();
	if (type_key == empty_string_name) {
		return false;
	}

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

Blackboard *Blackboard::get_parent(const StringName &p_name) const {
	Blackboard *current = parent;
	while (current != nullptr) {
		if (current->get_name() == p_name) {
			return current;
		}
		current = current->parent;
	}
	return nullptr;
}

Blackboard *Blackboard::get_parent(const RID &p_rid) const {
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

	EntryVariant<T>* result = iter->value;
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

	// const StringName &type_key = get_type_key<T>();
	// if (type_key == k_empty_string_name) {
	// 	ERR_FAIL_MSG(vformat("Unknown entry type for {0}", p_name));
	// }
	//
	// auto iter = entries.find(p_name);
	// if (likely(iter != entries.end())) {
	// 	const auto existing_entry = iter->value;
	// 	if (likely(existing_entry->get_type_key() == )) {
	// 		iter->value = p_value;
	// 		return;
	// 	}
	// 	free_entry(iter);
	// }
	//
	// EntryVariant<T> *new_entry = create_entry<T>(p_name, entries_owner, entries);
	// new_entry->value = p_value;
}

// String resolve_type_key(const Variant &p_variant) {
// 	const Object * ptr = nullptr;
// 	switch (p_variant.get_type()) {
// 		case Variant::Type::NIL:
// 			return k_empty_string_name;
// 		case Variant::Type::BOOL:
// 			return BlackboardStorageType<bool>::get_type_key();
// 		case Variant::Type::INT:
// 			return BlackboardStorageType<int64_t>::get_type_key();
// 		case Variant::Type::FLOAT:
// 			return BlackboardStorageType<double>::get_type_key();
// 		case Variant::Type::STRING:
// 			return BlackboardStorageType<String>::get_type_key();
//
// 		case Variant::Type::VECTOR2:
// 			return BlackboardStorageType<Vector2>::get_type_key();
// 		case Variant::Type::VECTOR2I:
// 			return BlackboardStorageType<Vector2i>::get_type_key();
// 		case Variant::Type::RECT2:
// 			return BlackboardStorageType<Rect2>::get_type_key();
// 		case Variant::Type::RECT2I:
// 			return BlackboardStorageType<Rect2i>::get_type_key();
// 		case Variant::Type::VECTOR3:
// 			return BlackboardStorageType<Vector3>::get_type_key();
// 		case Variant::Type::VECTOR3I:
// 			return BlackboardStorageType<Vector3i>::get_type_key();
// 		case Variant::Type::TRANSFORM2D:
// 			return BlackboardStorageType<Transform2D>::get_type_key();
// 		case Variant::Type::VECTOR4:
// 			return BlackboardStorageType<Vector4>::get_type_key();
// 		case Variant::Type::VECTOR4I:
// 			return BlackboardStorageType<Vector4i>::get_type_key();
// 		case Variant::Type::PLANE:
// 			return BlackboardStorageType<Plane>::get_type_key();
// 		case Variant::Type::QUATERNION:
// 			return BlackboardStorageType<Quaternion>::get_type_key();
// 		case Variant::Type::AABB:
// 			return BlackboardStorageType<AABB>::get_type_key();
// 		case Variant::Type::BASIS:
// 			return BlackboardStorageType<Basis>::get_type_key();
// 		case Variant::Type::TRANSFORM3D:
// 			return BlackboardStorageType<Transform3D>::get_type_key();
// 		case Variant::Type::PROJECTION:
// 			return BlackboardStorageType<Projection>::get_type_key();
//
// 		case Variant::Type::COLOR:
// 			return BlackboardStorageType<Color>::get_type_key();
// 		case Variant::Type::STRING_NAME:
// 			return BlackboardStorageType<StringName>::get_type_key();
// 		case Variant::Type::NODE_PATH:
// 			return BlackboardStorageType<NodePath>::get_type_key();
// 		case Variant::Type::RID:
// 			return BlackboardStorageType<RID>::get_type_key();
// 		case Variant::Type::OBJECT:
// 			ptr = p_variant;
// 			if (ptr) {
// 				return ptr->get_class();
// 			}
// 			else {
// 				return k_empty_string_name;
// 			}
// 		case Variant::Type::CALLABLE:
// 			return BlackboardStorageType<Callable>::get_type_key();
// 		case Variant::Type::SIGNAL:
// 			return BlackboardStorageType<Signal>::get_type_key();
// 		case Variant::Type::DICTIONARY:
// 			return BlackboardStorageType<Dictionary>::get_type_key();
// 		case Variant::Type::ARRAY:
// 			return BlackboardStorageType<Array>::get_type_key();
//
// 		case Variant::Type::PACKED_BYTE_ARRAY:
// 			return BlackboardStorageType<PackedByteArray>::get_type_key();
// 		case Variant::Type::PACKED_INT32_ARRAY:
// 			return BlackboardStorageType<PackedInt32Array>::get_type_key();
// 		case Variant::Type::PACKED_INT64_ARRAY:
// 			return BlackboardStorageType<PackedInt64Array>::get_type_key();
// 		case Variant::Type::PACKED_FLOAT32_ARRAY:
// 			return BlackboardStorageType<PackedFloat32Array>::get_type_key();
// 		case Variant::Type::PACKED_FLOAT64_ARRAY:
// 			return BlackboardStorageType<PackedFloat64Array>::get_type_key();
// 		case Variant::Type::PACKED_STRING_ARRAY:
// 			return BlackboardStorageType<PackedStringArray>::get_type_key();
// 		case Variant::Type::PACKED_VECTOR2_ARRAY:
// 			return BlackboardStorageType<PackedVector2Array>::get_type_key();
// 		case Variant::Type::PACKED_VECTOR3_ARRAY:
// 			return BlackboardStorageType<PackedVector3Array>::get_type_key();
// 		case Variant::Type::PACKED_COLOR_ARRAY:
// 			return BlackboardStorageType<PackedColorArray>::get_type_key();
// 		case Variant::Type::PACKED_VECTOR4_ARRAY:
// 			return BlackboardStorageType<PackedVector4Array>::get_type_key();
//
// 		default:
// 			ERR_FAIL_V_MSG(k_empty_string_name, "Cannot resolve unknown variant type id.");
// 	}
// }

template <>
void Blackboard::set_entry<Variant>(const StringName &p_name, const Variant &p_value) {

	// const StringName &type_key = resolve_type_key(p_value);
	//
	// auto iter = entries.find(p_name);
	// if (likely(iter != entries.end())) {
	// 	EntryBase *existing_entry = iter->value;
	// 	const StringName &existing_type_key = existing_entry->get_type_key();
	// 	if (likely(type_key == existing_type_key)) {
	// 		existing_entry->set_from(p_value);
	// 		return;
	// 	}
	// 	free_entry(iter);
	// }
	//
	// // NIL will delete an existing entry or do nothing if one doesn't exist.
	// if (unlikely(type_key == k_empty_string_name)) {
	// 	return;
	// }
	//
	// auto factory_iter = factories.find(type_key);
	// if (likely(factory_iter)) {
	// 	entry_factory factory_func = factory_iter->value;
	//
	// 	EntryBase* entry = factory_func(p_name, entries_owner, entries);
	// 	entry->set_from(p_value);
	// }
	// else if (const Object * obj = p_value; likely(obj != nullptr)) {
	//
	// 	static const StringName &ref_counted_key = RefCounted::get_class_static();
	// 	static const StringName &object_key = Object::get_class_static();
	// 	const StringName &factory_key = obj->is_class(ref_counted_key) ? ref_counted_key : object_key;
	//
	// 	entry_factory factory_func = factories[factory_key];
	// 	// use the Object* or Ref<RefCounted> as fallback factory from now on
	// 	factories[type_key] = factory_func;
	//
	// 	EntryBase* entry = factory_func(p_name, entries_owner, entries);
	// 	entry->set_from(p_value);
	// }
	// else {
	// 	const String err_msg = vformat("No blackboard entry factory available for given type: {0}.", p_value.get_type_name(p_value.get_type()));
	// 	ERR_FAIL_MSG(err_msg);
	// }
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
