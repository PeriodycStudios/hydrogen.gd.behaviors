//
// Created by tkey on 5/6/25.
//

#include "blackboard.hpp"

namespace hydrogen {

// template<typename V, EnableIf<is_variant_type<V>::value, V> = true>
// Vector2i static get_variant_type_key() {
// 	static Vector2i key = Vector2i(
// 		GetTypeInfo<V>::VARIANT_TYPE,
// 		GDEXTENSION_VARIANT_TYPE_VARIANT_MAX + GetTypeInfo<V>::METADATA);
//
// 	return key;
// }

template <typename V>
Variant BlackboardEntryTable<V>::get_variant(const StringName &p_name) const {
	return Variant();
}



bool Blackboard::validate_parent(Blackboard *p_parent) {
	return false;
}

Blackboard::Blackboard() :
		parent(nullptr) {}

Blackboard::Blackboard(Blackboard *p_parent) :
		parent(p_parent) {}

Blackboard::~Blackboard() {
}

template <typename T>
typename EnableIf<detail::is_variant_type<T>::value, T>::type Blackboard::get_entry(const StringName &p_name) const {
	return {};
}
template <typename T, EnableIf<detail::is_variant_type<T>::value, T>>
void Blackboard::set_entry(const StringName &p_name, const T &p_value) {
}

template<>
Variant Blackboard::get_entry<Variant>(const StringName &p_name) const {
	return {};
}

void Blackboard::set_entry(const StringName &p_name, const Variant &p_value) {

}

bool Blackboard::erase_entry(const StringName &p_name) {
	return false;
}

bool Blackboard::has_entry(const StringName &p_name, bool check_parents) const {
	return false;
}

} //namespace Hydrogen
